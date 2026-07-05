#include "platform/IWindowSource.h"
#include "platform/PlatformCapabilities.h"

#import <AppKit/AppKit.h>
#import <ApplicationServices/ApplicationServices.h>
#import <CoreGraphics/CoreGraphics.h>

#include <QCoreApplication>
#include <QFileInfo>
#include <QImage>
#include <algorithm>

namespace {

pid_t ownPid() {
    static pid_t pid = getpid();
    return pid;
}

QString bundlePathForPid(pid_t pid) {
    NSRunningApplication *app = [NSRunningApplication runningApplicationWithProcessIdentifier:pid];
    if (!app || !app.bundleURL) {
        return {};
    }
    return QString::fromUtf8(app.bundleURL.path.UTF8String);
}

QString appNameFromPath(const QString &path) {
    if (path.isEmpty()) {
        return {};
    }
    QFileInfo info(path);
    QString name = info.completeBaseName();
    if (name.isEmpty()) {
        name = info.fileName();
    }
    return name;
}

bool isSpuriousTitle(const QString &title) {
    const QString lc = title.trimmed().toLower();
    return lc.contains(QStringLiteral("唤出面板"))
        || lc.contains(QStringLiteral("open panel"))
        || lc.contains(QStringLiteral("window switcher"));
}

class MacWindowSource final : public IWindowSource {
public:
    QList<RawWindow> listWindows() override {
        QList<RawWindow> out;
        CFArrayRef windowList = CGWindowListCopyWindowInfo(
            kCGWindowListOptionOnScreenOnly | kCGWindowListExcludeDesktopElements,
            kCGNullWindowID);
        if (!windowList) {
            return out;
        }
        const CFIndex count = CFArrayGetCount(windowList);
        for (CFIndex i = 0; i < count; ++i) {
            CFDictionaryRef info = static_cast<CFDictionaryRef>(CFArrayGetValueAtIndex(windowList, i));
            CFNumberRef layerRef = static_cast<CFNumberRef>(CFDictionaryGetValue(info, kCGWindowLayer));
            int layer = 0;
            if (layerRef) {
                CFNumberGetValue(layerRef, kCFNumberIntType, &layer);
            }
            if (layer != 0) {
                continue;
            }
            CFNumberRef pidRef = static_cast<CFNumberRef>(CFDictionaryGetValue(info, kCGWindowOwnerPID));
            pid_t pid = 0;
            if (!pidRef || !CFNumberGetValue(pidRef, kCFNumberIntType, &pid) || pid == ownPid()) {
                continue;
            }
            CFStringRef titleRef = static_cast<CFStringRef>(CFDictionaryGetValue(info, kCGWindowName));
            if (!titleRef || CFStringGetLength(titleRef) == 0) {
                continue;
            }
            const QString title = QString::fromUtf8([(__bridge NSString *)titleRef UTF8String]);
            if (isSpuriousTitle(title)) {
                continue;
            }
            CFNumberRef widRef = static_cast<CFNumberRef>(CFDictionaryGetValue(info, kCGWindowNumber));
            qint64 windowId = 0;
            if (!widRef || !CFNumberGetValue(widRef, kCFNumberLongLongType, &windowId)) {
                continue;
            }
            const QString exePath = bundlePathForPid(pid);
            if (exePath.isEmpty()) {
                continue;
            }
            RawWindow w;
            w.windowId = windowId;
            w.title = title;
            w.exePath = exePath;
            w.appName = appNameFromPath(exePath);
            if (w.appName.compare(QStringLiteral("Finder"), Qt::CaseInsensitive) == 0) {
                w.folderPath = title;
            }
            if (w.isListable()) {
                out.append(w);
            }
        }
        CFRelease(windowList);
        return out;
    }

    void activate(qint64 windowId) override {
        CFArrayRef windowList = CGWindowListCopyWindowInfo(kCGWindowListOptionIncludingWindow, static_cast<CGWindowID>(windowId));
        if (!windowList || CFArrayGetCount(windowList) == 0) {
            if (windowList) {
                CFRelease(windowList);
            }
            return;
        }
        CFDictionaryRef info = static_cast<CFDictionaryRef>(CFArrayGetValueAtIndex(windowList, 0));
        CFNumberRef pidRef = static_cast<CFNumberRef>(CFDictionaryGetValue(info, kCGWindowOwnerPID));
        pid_t pid = 0;
        if (pidRef) {
            CFNumberGetValue(pidRef, kCFNumberIntType, &pid);
        }
        CFRelease(windowList);
        if (pid == 0) {
            return;
        }
        NSRunningApplication *app = [NSRunningApplication runningApplicationWithProcessIdentifier:pid];
        [app activateWithOptions:(NSApplicationActivateIgnoringOtherApps | NSApplicationActivateAllWindows)];
    }

    void closeWindow(qint64 windowId) override {
        Q_UNUSED(windowId)
        // macOS 没有统一的跨应用 WM_CLOSE；后续可通过 AppleScript / Accessibility API 增强。
    }
};

class MacIconCapture final : public IIconCapture {
public:
    ImageRgba windowIcon(qint64 windowId) override {
        Q_UNUSED(windowId)
        return {};
    }
};

class MacThumbnailCapture final : public IThumbnailCapture {
public:
    ImageRgba capture(qint64 windowId) override {
#if defined(__MAC_OS_X_VERSION_MAX_ALLOWED) && __MAC_OS_X_VERSION_MAX_ALLOWED >= 150000
        Q_UNUSED(windowId)
        return {};
#else
        CGImageRef image = CGWindowListCreateImage(
            CGRectNull,
            kCGWindowListOptionIncludingWindow,
            static_cast<CGWindowID>(windowId),
            kCGWindowImageBoundsIgnoreFraming | kCGWindowImageBestResolution);
        if (!image) {
            return {};
        }
        const size_t w = CGImageGetWidth(image);
        const size_t h = CGImageGetHeight(image);
        if (w < 8 || h < 8) {
            CGImageRelease(image);
            return {};
        }
        QImage qimg(static_cast<int>(w), static_cast<int>(h), QImage::Format_RGBA8888);
        CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
        CGContextRef ctx = CGBitmapContextCreate(
            qimg.bits(), w, h, 8, qimg.bytesPerLine(), colorSpace,
            kCGImageAlphaPremultipliedLast | kCGBitmapByteOrder32Big);
        CGContextDrawImage(ctx, CGRectMake(0, 0, w, h), image);
        CGContextRelease(ctx);
        CGColorSpaceRelease(colorSpace);
        CGImageRelease(image);
        ImageRgba out;
        out.width = static_cast<int>(w);
        out.height = static_cast<int>(h);
        out.pixels = QByteArray(reinterpret_cast<const char *>(qimg.constBits()), qimg.sizeInBytes());
        return out;
#endif
    }
};

} // namespace

std::unique_ptr<IWindowSource> createWindowSourceImpl() {
    return std::make_unique<MacWindowSource>();
}

std::unique_ptr<IIconCapture> createIconCaptureImpl() {
    return std::make_unique<MacIconCapture>();
}

std::unique_ptr<IThumbnailCapture> createThumbnailCaptureImpl() {
    return std::make_unique<MacThumbnailCapture>();
}

PlatformCapabilities queryPlatformCapabilitiesImpl() {
    PlatformCapabilities caps;
    caps.platformName = QStringLiteral("macOS");
    caps.sessionType = QStringLiteral("Desktop");
    caps.hotkey = CapabilityLevel::None;
    caps.activate = CapabilityLevel::Partial;
    caps.closeWindow = CapabilityLevel::None;
    caps.icon = CapabilityLevel::None;
#if defined(__MAC_OS_X_VERSION_MAX_ALLOWED) && __MAC_OS_X_VERSION_MAX_ALLOWED >= 150000
    caps.thumbnail = CapabilityLevel::None;
#else
    caps.thumbnail = CapabilityLevel::Partial;
#endif
    caps.folderPath = CapabilityLevel::Partial;
    return caps;
}
