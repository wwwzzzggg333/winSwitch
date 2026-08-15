#pragma once

#include "platform/WindowInfo.h"

#include <QList>
#include <functional>
#include <memory>

class IWindowSource {
public:
    virtual ~IWindowSource() = default;

    virtual QList<RawWindow> listWindows() = 0;
    virtual void activate(qint64 windowId) = 0;
    virtual void closeWindow(qint64 windowId) = 0;
};

class IIconCapture {
public:
    virtual ~IIconCapture() = default;
    virtual ImageRgba windowIcon(qint64 windowId) = 0;
};

class IThumbnailCapture {
public:
    virtual ~IThumbnailCapture() = default;
    virtual ImageRgba capture(qint64 windowId) = 0;

    // Fast capture used the moment a window starts to minimize, while it is still on
    // screen. Defaults to capture(); the Windows implementation overrides it to skip
    // the minimized-window guard and the expensive WGC fallback.
    virtual ImageRgba captureForMinimize(qint64 windowId) { return capture(windowId); }
};

// Watches for global window events (currently only "about to minimize") so the app
// can cache a thumbnail before a window leaves the screen. Windows-only for now;
// other platforms return nullptr and the caller simply skips watching.
class IWindowEventSource {
public:
    virtual ~IWindowEventSource() = default;
    using MinimizeHandler = std::function<void(qint64 windowId)>;
    virtual void setMinimizeHandler(MinimizeHandler handler) = 0;
    virtual void start() = 0;
};

std::unique_ptr<IWindowSource> createWindowSource();
std::unique_ptr<IIconCapture> createIconCapture();
std::unique_ptr<IThumbnailCapture> createThumbnailCapture();
std::unique_ptr<IWindowEventSource> createWindowEventSource();
