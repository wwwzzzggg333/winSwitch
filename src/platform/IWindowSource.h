#pragma once

#include "platform/WindowInfo.h"

#include <QList>
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
};

std::unique_ptr<IWindowSource> createWindowSource();
std::unique_ptr<IIconCapture> createIconCapture();
std::unique_ptr<IThumbnailCapture> createThumbnailCapture();
