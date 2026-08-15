#include "platform/IWindowSource.h"
#include "platform/PlatformCapabilities.h"
#include "platform/windows/WinExplorerPaths.h"
#include "platform/windows/WinForeground.h"

#include <windows.h>
#include <dwmapi.h>
#include <psapi.h>
#include <shellapi.h>

#include <QHash>
#include <QImage>
#include <QSet>
#include <algorithm>
#include <cstring>

#ifndef PROCESS_NAME_WIN32
#define PROCESS_NAME_WIN32 0x00000001
#endif

namespace {

DWORD ownPid() {
    static DWORD pid = GetCurrentProcessId();
    return pid;
}

QString windowClass(HWND hwnd) {
    wchar_t buf[256] = {};
    const int n = GetClassNameW(hwnd, buf, 256);
    return n > 0 ? QString::fromWCharArray(buf, n) : QString();
}

bool isSpuriousTitle(const QString &title) {
    const QString lc = title.trimmed().toLower();
    return lc.contains(QStringLiteral("唤出面板"))
        || lc.contains(QStringLiteral("open panel"))
        || lc.contains(QStringLiteral("open the panel"));
}

QString exePathOf(DWORD pid) {
    if (pid == 0) {
        return {};
    }
    HANDLE handle = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (!handle) {
        return {};
    }
    wchar_t buf[512] = {};
    DWORD size = 512;
    const bool ok = QueryFullProcessImageNameW(handle, PROCESS_NAME_WIN32, buf, &size);
    CloseHandle(handle);
    return ok && size > 0 ? QString::fromWCharArray(buf, size) : QString();
}

struct EnumContext {
    QList<RawWindow> *out = nullptr;
    QSet<qint64> *seen = nullptr;
};

bool appendWindowFromHwnd(HWND hwnd, EnumContext *ctx) {
    if (!ctx || !ctx->out || !ctx->seen) {
        return false;
    }
    const qint64 id = reinterpret_cast<qint64>(hwnd);
    if (ctx->seen->contains(id)) {
        return false;
    }
    if (!IsWindow(hwnd) || !IsWindowVisible(hwnd)) {
        return false;
    }
    const int len = GetWindowTextLengthW(hwnd);
    if (len <= 0) {
        return false;
    }
    QVector<wchar_t> buf(len + 1);
    const int n = GetWindowTextW(hwnd, buf.data(), len + 1);
    if (n <= 0) {
        return false;
    }
    const QString title = QString::fromWCharArray(buf.data(), n);
    const LONG_PTR exStyle = GetWindowLongPtrW(hwnd, GWL_EXSTYLE);
    if (exStyle & WS_EX_TOOLWINDOW) {
        return false;
    }
    DWORD cloaked = 0;
    DwmGetWindowAttribute(hwnd, DWMWA_CLOAKED, &cloaked, sizeof(cloaked));
    if (cloaked != 0) {
        return false;
    }
    DWORD pid = 0;
    GetWindowThreadProcessId(hwnd, &pid);
    if (pid == ownPid()) {
        return false;
    }
    const QString cls = windowClass(hwnd);
    if (cls.compare(QStringLiteral("tooltips_class32"), Qt::CaseInsensitive) == 0
        || cls.compare(QStringLiteral("SysShadow"), Qt::CaseInsensitive) == 0) {
        return false;
    }
    if (isSpuriousTitle(title)) {
        return false;
    }
    const QString exePath = exePathOf(pid);
    if (exePath.isEmpty()) {
        return false;
    }
    const int slash = qMax(exePath.lastIndexOf('\\'), exePath.lastIndexOf('/'));
    QString appName = exePath.mid(slash + 1);
    if (appName.endsWith(QStringLiteral(".exe"), Qt::CaseInsensitive)) {
        appName.chop(4);
    }
    RawWindow w;
    w.windowId = id;
    w.title = title;
    w.exePath = exePath;
    w.appName = appName;
    ctx->out->append(w);
    ctx->seen->insert(id);
    return true;
}

BOOL CALLBACK enumProc(HWND hwnd, LPARAM lparam) {
    auto *ctx = reinterpret_cast<EnumContext *>(lparam);
    appendWindowFromHwnd(hwnd, ctx);
    return TRUE;
}

// SEH-protected GetDIBits — must be in a function without C++ destructors
// to avoid MSVC C2712 (cannot use __try in functions requiring object unwinding).
static int safeGetDIBits(
    HDC hdc, HBITMAP hbm, UINT start, UINT cLines, void *bits, BITMAPINFO *bi, UINT usage) {
    __try {
        return GetDIBits(hdc, hbm, start, cLines, bits, bi, usage);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return 0;
    }
}

static int dibStride32(int width) {
    return ((width * 32 + 31) / 32) * 4;
}

static bool readDdbToImageRgba(HDC hdc, HBITMAP bmp, int width, int height, ImageRgba &out) {
    if (width <= 0 || height <= 0) {
        return false;
    }
    const int stride = dibStride32(width);
    QByteArray dib(stride * height, Qt::Uninitialized);
    BITMAPINFO bi{};
    bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bi.bmiHeader.biWidth = width;
    bi.bmiHeader.biHeight = -height;
    bi.bmiHeader.biPlanes = 1;
    bi.bmiHeader.biBitCount = 32;
    bi.bmiHeader.biCompression = BI_RGB;
    const int got = safeGetDIBits(
        hdc, bmp, 0, static_cast<UINT>(height), dib.data(), &bi, DIB_RGB_COLORS);
    if (got == 0) {
        return false;
    }
    out.width = width;
    out.height = height;
    out.pixels.resize(width * height * 4);
    for (int y = 0; y < height; ++y) {
        const uchar *src = reinterpret_cast<const uchar *>(dib.constData() + y * stride);
        uchar *dst = reinterpret_cast<uchar *>(out.pixels.data() + y * width * 4);
        for (int x = 0; x < width; ++x) {
            dst[x * 4 + 0] = src[x * 4 + 2];
            dst[x * 4 + 1] = src[x * 4 + 1];
            dst[x * 4 + 2] = src[x * 4 + 0];
            dst[x * 4 + 3] = 255;
        }
    }
    return true;
}

ImageRgba imageFromQImage(QImage image) {
    if (image.isNull()) {
        return {};
    }
    image = image.convertToFormat(QImage::Format_RGBA8888);
    if (image.isNull()) {
        return {};
    }
    ImageRgba out;
    out.width = image.width();
    out.height = image.height();
    const int tightBytes = out.width * 4;
    const int bytesPerLine = image.bytesPerLine();
    out.pixels.resize(out.width * out.height * 4);
    if (bytesPerLine == tightBytes) {
        memcpy(out.pixels.data(), image.constBits(), static_cast<size_t>(out.pixels.size()));
    } else {
        for (int y = 0; y < out.height; ++y) {
            memcpy(out.pixels.data() + y * tightBytes, image.constScanLine(y), static_cast<size_t>(tightBytes));
        }
    }
    return out;
}

ImageRgba captureHicon(HICON hicon, bool destroyIcon) {
    if (!hicon) {
        return {};
    }
    constexpr int kIconDrawSize = 256;
    HDC screenDc = GetDC(nullptr);
    if (!screenDc) {
        if (destroyIcon) {
            DestroyIcon(hicon);
        }
        return {};
    }
    HDC memDc = CreateCompatibleDC(screenDc);
    if (!memDc) {
        ReleaseDC(nullptr, screenDc);
        if (destroyIcon) {
            DestroyIcon(hicon);
        }
        return {};
    }
    HBITMAP bmp = CreateCompatibleBitmap(screenDc, kIconDrawSize, kIconDrawSize);
    if (!bmp) {
        DeleteDC(memDc);
        ReleaseDC(nullptr, screenDc);
        if (destroyIcon) {
            DestroyIcon(hicon);
        }
        return {};
    }
    HGDIOBJ old = SelectObject(memDc, bmp);
    if (!old) {
        DeleteObject(bmp);
        DeleteDC(memDc);
        ReleaseDC(nullptr, screenDc);
        if (destroyIcon) {
            DestroyIcon(hicon);
        }
        return {};
    }

    const HBRUSH brush = CreateSolidBrush(RGB(0, 0, 0));
    RECT fillRect{0, 0, kIconDrawSize, kIconDrawSize};
    FillRect(memDc, &fillRect, brush);
    DeleteObject(brush);

    const BOOL drawn = DrawIconEx(
        memDc, 0, 0, hicon, kIconDrawSize, kIconDrawSize, 0, nullptr, DI_NORMAL);

    ImageRgba image;
    if (drawn) {
        if (!readDdbToImageRgba(memDc, bmp, kIconDrawSize, kIconDrawSize, image)) {
            image = {};
        }
    }

    SelectObject(memDc, old);
    DeleteObject(bmp);
    DeleteDC(memDc);
    ReleaseDC(nullptr, screenDc);
    if (destroyIcon) {
        DestroyIcon(hicon);
    }
    return image;
}

class WinWindowSource final : public IWindowSource {
public:
    QList<RawWindow> listWindows() override {
        QList<RawWindow> out;
        QSet<qint64> seen;
        EnumContext ctx{&out, &seen};
        EnumWindows(enumProc, reinterpret_cast<LPARAM>(&ctx));

        const QHash<qint64, QString> paths = explorerPaths();
        for (auto it = paths.constBegin(); it != paths.constEnd(); ++it) {
            appendWindowFromHwnd(reinterpret_cast<HWND>(it.key()), &ctx);
        }

        QList<RawWindow> filtered;
        for (RawWindow &w : out) {
            if (w.appName.compare(QStringLiteral("explorer"), Qt::CaseInsensitive) == 0) {
                const QString path = paths.value(w.windowId);
                if (!path.isEmpty()) {
                    w.folderPath = path;
                }
            }
            if (w.isListable()) {
                filtered.append(w);
            }
        }
        return filtered;
    }

    void activate(qint64 windowId) override {
        winSwitch::forceForegroundWindow(reinterpret_cast<HWND>(windowId));
    }

    void closeWindow(qint64 windowId) override {
        PostMessageW(reinterpret_cast<HWND>(windowId), WM_CLOSE, 0, 0);
    }
};

class WinIconCapture final : public IIconCapture {
public:
    ImageRgba windowIcon(qint64 windowId) override {
        HWND hwnd = reinterpret_cast<HWND>(windowId);
        if (!IsWindow(hwnd)) {
            return {};
        }
        DWORD pid = 0;
        GetWindowThreadProcessId(hwnd, &pid);
        const QString exePath = exePathOf(pid);
        if (!exePath.isEmpty()) {
            SHFILEINFOW sfi{};
            if (SHGetFileInfoW(
                    reinterpret_cast<LPCWSTR>(exePath.utf16()),
                    0,
                    &sfi,
                    sizeof(sfi),
                    SHGFI_ICON | SHGFI_LARGEICON) != 0
                && sfi.hIcon != nullptr) {
                return captureHicon(sfi.hIcon, true);
            }
        }
        HICON hicon = reinterpret_cast<HICON>(GetClassLongPtrW(hwnd, GCLP_HICON));
        if (!hicon) {
            hicon = reinterpret_cast<HICON>(GetClassLongPtrW(hwnd, GCLP_HICONSM));
        }
        if (!hicon) {
            return {};
        }
        return captureHicon(hicon, false);
    }
};

class WinThumbnailCapture final : public IThumbnailCapture {
public:
    ImageRgba capture(qint64 windowId) override {
        HWND hwnd = reinterpret_cast<HWND>(windowId);
        if (!IsWindow(hwnd) || !IsWindowVisible(hwnd)) {
            return {};
        }
        if (IsIconic(hwnd)) {
            return {};
        }
        RECT windowRect{};
        if (!GetWindowRect(hwnd, &windowRect)) {
            return {};
        }
        RECT frameRect = windowRect;
        DwmGetWindowAttribute(hwnd, DWMWA_EXTENDED_FRAME_BOUNDS, &frameRect, sizeof(frameRect));

        const int fullW = qMax(1, static_cast<int>(windowRect.right - windowRect.left));
        const int fullH = qMax(1, static_cast<int>(windowRect.bottom - windowRect.top));
        if (fullW < 8 || fullH < 8) {
            return {};
        }
        constexpr int kMaxCaptureDim = 4096;
        if (fullW > kMaxCaptureDim || fullH > kMaxCaptureDim) {
            return {};
        }

        HDC screenDc = GetDC(nullptr);
        if (!screenDc) {
            return {};
        }
        HDC memDc = CreateCompatibleDC(screenDc);
        if (!memDc) {
            ReleaseDC(nullptr, screenDc);
            return {};
        }
        HBITMAP bmp = CreateCompatibleBitmap(screenDc, fullW, fullH);
        if (!bmp) {
            DeleteDC(memDc);
            ReleaseDC(nullptr, screenDc);
            return {};
        }
        HGDIOBJ old = SelectObject(memDc, bmp);
        if (!old) {
            DeleteObject(bmp);
            DeleteDC(memDc);
            ReleaseDC(nullptr, screenDc);
            return {};
        }
        const BOOL ok = PrintWindow(hwnd, memDc, PW_RENDERFULLCONTENT);
        ImageRgba image;
        if (!ok || !readDdbToImageRgba(memDc, bmp, fullW, fullH, image)) {
            SelectObject(memDc, old);
            DeleteObject(bmp);
            DeleteDC(memDc);
            ReleaseDC(nullptr, screenDc);
            return {};
        }
        SelectObject(memDc, old);
        DeleteObject(bmp);
        DeleteDC(memDc);
        ReleaseDC(nullptr, screenDc);

        const int cropX = qBound(0, static_cast<int>(frameRect.left - windowRect.left), fullW - 1);
        const int cropY = qBound(0, static_cast<int>(frameRect.top - windowRect.top), fullH - 1);
        const int cropW = qBound(1, static_cast<int>(frameRect.right - frameRect.left), fullW - cropX);
        const int cropH = qBound(1, static_cast<int>(frameRect.bottom - frameRect.top), fullH - cropY);
        QImage src(
            reinterpret_cast<const uchar *>(image.pixels.constData()),
            fullW,
            fullH,
            fullW * 4,
            QImage::Format_RGBA8888);
        QImage cropped = src.copy(cropX, cropY, cropW, cropH);
        return downscale(imageFromQImage(cropped));
    }

private:
    static bool rowIsFlat(const ImageRgba &img, int row) {
        if (row < 0 || row >= img.height || img.width <= 0) {
            return true;
        }
        int minR = 255;
        int maxR = 0;
        int minG = 255;
        int maxG = 0;
        int minB = 255;
        int maxB = 0;
        const int offset = row * img.width * 4;
        for (int x = 0; x < img.width; ++x) {
            const int i = offset + x * 4;
            const int r = static_cast<uchar>(img.pixels.at(i));
            const int g = static_cast<uchar>(img.pixels.at(i + 1));
            const int b = static_cast<uchar>(img.pixels.at(i + 2));
            minR = qMin(minR, r);
            maxR = qMax(maxR, r);
            minG = qMin(minG, g);
            maxG = qMax(maxG, g);
            minB = qMin(minB, b);
            maxB = qMax(maxB, b);
        }
        return (maxR - minR) < 8 && (maxG - minG) < 8 && (maxB - minB) < 8;
    }

    static bool looksTruncated(const ImageRgba &img) {
        if (img.height <= 0 || img.width <= 0) {
            return true;
        }
        constexpr int kSampleRows = 16;
        int nonFlatTop = 0;
        int flatBottom = 0;
        for (int i = 0; i < kSampleRows; ++i) {
            const int row = (i * img.height) / kSampleRows;
            if (rowIsFlat(img, row)) {
                if (row >= img.height * 0.4) {
                    ++flatBottom;
                }
            } else if (row < img.height * 0.4) {
                ++nonFlatTop;
            }
        }
        if (nonFlatTop == 0 && flatBottom == kSampleRows) {
            return true;
        }
        if (nonFlatTop > 0 && flatBottom >= kSampleRows * 0.6) {
            return true;
        }
        return false;
    }

    static bool isBlank(const QByteArray &buf) {
        bool allWhite = true;
        bool allBlack = true;
        for (int i = 0; i < buf.size(); i += 4) {
            const uchar r = static_cast<uchar>(buf.at(i));
            const uchar g = static_cast<uchar>(buf.at(i + 1));
            const uchar b = static_cast<uchar>(buf.at(i + 2));
            if (r != 255 || g != 255 || b != 255) {
                allWhite = false;
            }
            if (r != 0 || g != 0 || b != 0) {
                allBlack = false;
            }
            if (!allWhite && !allBlack) {
                return false;
            }
        }
        return allWhite || allBlack;
    }

    static ImageRgba downscale(ImageRgba thumb) {
        if (looksTruncated(thumb)) {
            return {};
        }
        constexpr int maxW = 420;
        constexpr int maxH = 236;
        if (thumb.width <= maxW && thumb.height <= maxH) {
            if (isBlank(thumb.pixels)) {
                return {};
            }
            return thumb;
        }
        const double scale = qMin(maxW / static_cast<double>(thumb.width), maxH / static_cast<double>(thumb.height));
        const int newW = qMax(1, static_cast<int>(thumb.width * scale));
        const int newH = qMax(1, static_cast<int>(thumb.height * scale));
        QImage src(
            reinterpret_cast<const uchar *>(thumb.pixels.constData()),
            thumb.width,
            thumb.height,
            thumb.width * 4,
            QImage::Format_RGBA8888);
        QImage scaled = src.copy().scaled(newW, newH, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
        ImageRgba out = imageFromQImage(scaled);
        if (out.pixels.isEmpty() || isBlank(out.pixels) || looksTruncated(out)) {
            return {};
        }
        return out;
    }
};

} // namespace

std::unique_ptr<IWindowSource> createWindowSourceImpl() {
    return std::make_unique<WinWindowSource>();
}

std::unique_ptr<IIconCapture> createIconCaptureImpl() {
    return std::make_unique<WinIconCapture>();
}

std::unique_ptr<IThumbnailCapture> createThumbnailCaptureImpl() {
    return std::make_unique<WinThumbnailCapture>();
}

PlatformCapabilities queryPlatformCapabilitiesImpl() {
    PlatformCapabilities caps;
    caps.platformName = QStringLiteral("Windows");
    caps.sessionType = QStringLiteral("Desktop");
    caps.hotkey = CapabilityLevel::Full;
    caps.activate = CapabilityLevel::Full;
    caps.closeWindow = CapabilityLevel::Full;
    caps.icon = CapabilityLevel::Full;
    caps.thumbnail = CapabilityLevel::Full;
    caps.folderPath = CapabilityLevel::Full;
    return caps;
}
