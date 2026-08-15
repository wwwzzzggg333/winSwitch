#include "platform/IWindowSource.h"
#include "platform/PlatformCapabilities.h"

#if defined(Q_OS_WIN)
std::unique_ptr<IWindowSource> createWindowSourceImpl();
std::unique_ptr<IIconCapture> createIconCaptureImpl();
std::unique_ptr<IThumbnailCapture> createThumbnailCaptureImpl();
std::unique_ptr<IWindowEventSource> createWindowEventSourceImpl();
PlatformCapabilities queryPlatformCapabilitiesImpl();
#elif defined(Q_OS_MACOS)
std::unique_ptr<IWindowSource> createWindowSourceImpl();
std::unique_ptr<IIconCapture> createIconCaptureImpl();
std::unique_ptr<IThumbnailCapture> createThumbnailCaptureImpl();
PlatformCapabilities queryPlatformCapabilitiesImpl();
#elif defined(Q_OS_LINUX)
std::unique_ptr<IWindowSource> createWindowSourceImpl();
std::unique_ptr<IIconCapture> createIconCaptureImpl();
std::unique_ptr<IThumbnailCapture> createThumbnailCaptureImpl();
PlatformCapabilities queryPlatformCapabilitiesImpl();
#endif

std::unique_ptr<IWindowSource> createWindowSource() {
    return createWindowSourceImpl();
}

std::unique_ptr<IIconCapture> createIconCapture() {
    return createIconCaptureImpl();
}

std::unique_ptr<IThumbnailCapture> createThumbnailCapture() {
    return createThumbnailCaptureImpl();
}

PlatformCapabilities queryPlatformCapabilities() {
    return queryPlatformCapabilitiesImpl();
}

std::unique_ptr<IWindowEventSource> createWindowEventSource() {
#if defined(Q_OS_WIN)
    return createWindowEventSourceImpl();
#else
    return nullptr;
#endif
}
