#include "platform/IWindowSource.h"
#include "platform/PlatformCapabilities.h"
#include "platform/windows/WinExplorerPaths.h"

#include <windows.h>
#include <dwmapi.h>
#include <psapi.h>

#include <QHash>
#include <QImage>
#include <QSet>
#include <algorithm>

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
        HWND hwnd = reinterpret_cast<HWND>(windowId);
        if (IsIconic(hwnd)) {
            ShowWindow(hwnd, SW_RESTORE);
        }
        HWND fg = GetForegroundWindow();
        const DWORD targetTid = GetWindowThreadProcessId(hwnd, nullptr);
        const DWORD fgTid = GetWindowThreadProcessId(fg, nullptr);
        const DWORD curTid = GetCurrentThreadId();
        AttachThreadInput(curTid, targetTid, TRUE);
        if (fgTid != 0 && fgTid != curTid) {
            AttachThreadInput(fgTid, targetTid, TRUE);
        }
        BringWindowToTop(hwnd);
        SetForegroundWindow(hwnd);
        if (fgTid != 0 && fgTid != curTid) {
            AttachThreadInput(fgTid, targetTid, FALSE);
        }
        AttachThreadInput(curTid, targetTid, FALSE);
    }

    void closeWindow(qint64 windowId) override {
        PostMessageW(reinterpret_cast<HWND>(windowId), WM_CLOSE, 0, 0);
    }
};

class WinIconCapture final : public IIconCapture {
public:
    ImageRgba windowIcon(qint64 windowId) override {
        HWND hwnd = reinterpret_cast<HWND>(windowId);
        HICON hicon = reinterpret_cast<HICON>(SendMessageW(hwnd, WM_GETICON, ICON_BIG, 0));
        if (!hicon) {
            hicon = reinterpret_cast<HICON>(GetClassLongPtrW(hwnd, GCLP_HICON));
        }
        if (!hicon) {
            return {};
        }
        ICONINFO info{};
        if (!GetIconInfo(hicon, &info)) {
            return {};
        }
        BITMAP bmp{};
        if (GetObjectW(info.hbmColor, sizeof(bmp), &bmp) == 0) {
            DeleteObject(info.hbmColor);
            DeleteObject(info.hbmMask);
            return {};
        }
        const int w = bmp.bmWidth;
        const int h = bmp.bmHeight;
        if (w <= 0 || h <= 0) {
            DeleteObject(info.hbmColor);
            DeleteObject(info.hbmMask);
            return {};
        }
        BITMAPINFO bi{};
        bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bi.bmiHeader.biWidth = w;
        bi.bmiHeader.biHeight = -h;
        bi.bmiHeader.biPlanes = 1;
        bi.bmiHeader.biBitCount = 32;
        bi.bmiHeader.biCompression = BI_RGB;
        ImageRgba image;
        image.width = w;
        image.height = h;
        image.pixels.resize(w * h * 4);
        HDC dc = CreateCompatibleDC(nullptr);
        GetDIBits(dc, info.hbmColor, 0, h, image.pixels.data(), &bi, DIB_RGB_COLORS);
        DeleteDC(dc);
        DeleteObject(info.hbmColor);
        DeleteObject(info.hbmMask);
        for (int i = 0; i < image.pixels.size(); i += 4) {
            std::swap(image.pixels[i], image.pixels[i + 2]);
        }
        return image;
    }
};

class WinThumbnailCapture final : public IThumbnailCapture {
public:
    ImageRgba capture(qint64 windowId) override {
        HWND hwnd = reinterpret_cast<HWND>(windowId);
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

        HDC screenDc = GetDC(nullptr);
        HDC memDc = CreateCompatibleDC(screenDc);
        HBITMAP bmp = CreateCompatibleBitmap(screenDc, fullW, fullH);
        HGDIOBJ old = SelectObject(memDc, bmp);
        const BOOL ok = PrintWindow(hwnd, memDc, PW_RENDERFULLCONTENT);
        BITMAPINFO bi{};
        bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bi.bmiHeader.biWidth = fullW;
        bi.bmiHeader.biHeight = -fullH;
        bi.bmiHeader.biPlanes = 1;
        bi.bmiHeader.biBitCount = 32;
        bi.bmiHeader.biCompression = BI_RGB;
        ImageRgba image;
        image.width = fullW;
        image.height = fullH;
        image.pixels.resize(fullW * fullH * 4);
        const int got = GetDIBits(memDc, bmp, 0, fullH, image.pixels.data(), &bi, DIB_RGB_COLORS);
        SelectObject(memDc, old);
        DeleteObject(bmp);
        DeleteDC(memDc);
        ReleaseDC(nullptr, screenDc);
        if (!ok || got == 0) {
            return {};
        }
        for (int i = 0; i < image.pixels.size(); i += 4) {
            std::swap(image.pixels[i], image.pixels[i + 2]);
            image.pixels[i + 3] = static_cast<char>(255);
        }

        const int cropX = qBound(0, static_cast<int>(frameRect.left - windowRect.left), fullW - 1);
        const int cropY = qBound(0, static_cast<int>(frameRect.top - windowRect.top), fullH - 1);
        const int cropW = qBound(1, static_cast<int>(frameRect.right - frameRect.left), fullW - cropX);
        const int cropH = qBound(1, static_cast<int>(frameRect.bottom - frameRect.top), fullH - cropY);
        QImage src(
            reinterpret_cast<const uchar *>(image.pixels.constData()),
            fullW,
            fullH,
            QImage::Format_RGBA8888);
        QImage cropped = src.copy(cropX, cropY, cropW, cropH);
        ImageRgba croppedImage;
        croppedImage.width = cropped.width();
        croppedImage.height = cropped.height();
        croppedImage.pixels = QByteArray(
            reinterpret_cast<const char *>(cropped.constBits()),
            cropped.sizeInBytes());
        return downscale(croppedImage);
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
            QImage::Format_RGBA8888);
        QImage scaled = src.scaled(newW, newH, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
        ImageRgba out;
        out.width = newW;
        out.height = newH;
        out.pixels = QByteArray(reinterpret_cast<const char *>(scaled.constBits()), scaled.sizeInBytes());
        if (isBlank(out.pixels) || looksTruncated(out)) {
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
