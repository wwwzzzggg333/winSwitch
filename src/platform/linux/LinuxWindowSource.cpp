#include "platform/IWindowSource.h"
#include "platform/PlatformCapabilities.h"

#include <QByteArray>
#include <QCoreApplication>
#include <QFile>
#include <QImage>
#include <QProcessEnvironment>

#include <optional>
#include <unistd.h>

#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#ifdef Bool
#undef Bool
#endif
#ifdef Status
#undef Status
#endif
#ifdef KeyPress
#undef KeyPress
#endif
#ifdef KeyRelease
#undef KeyRelease
#endif

namespace {

Display *display() {
    static Display *dpy = XOpenDisplay(nullptr);
    return dpy;
}

pid_t ownPid() {
    static pid_t pid = getpid();
    return pid;
}

QString readProcessExe(pid_t pid) {
    QFile file(QStringLiteral("/proc/%1/comm").arg(pid));
    if (!file.open(QIODevice::ReadOnly)) {
        return {};
    }
    return QString::fromUtf8(file.readAll()).trimmed();
}

QString readProcessCmdline(pid_t pid) {
    QFile file(QStringLiteral("/proc/%1/cmdline").arg(pid));
    if (!file.open(QIODevice::ReadOnly)) {
        return readProcessExe(pid);
    }
    const QByteArray data = file.readAll();
    const int zero = data.indexOf('\0');
    const QByteArray first = zero >= 0 ? data.left(zero) : data;
    return QString::fromUtf8(first);
}

bool isWaylandSession() {
    return !qEnvironmentVariableIsEmpty("WAYLAND_DISPLAY")
        && qEnvironmentVariableIsEmpty("DISPLAY");
}

Atom atom(Display *dpy, const char *name) {
    return XInternAtom(dpy, name, False);
}

class LinuxWindowSource final : public IWindowSource {
public:
    QList<RawWindow> listWindows() override {
        QList<RawWindow> out;
        if (isWaylandSession()) {
            return out;
        }
        Display *dpy = display();
        if (!dpy) {
            return out;
        }
        Window root = DefaultRootWindow(dpy);
        Atom netClientList = atom(dpy, "_NET_CLIENT_LIST");
        Atom actualType = 0;
        int actualFormat = 0;
        unsigned long itemCount = 0;
        unsigned long bytesAfter = 0;
        unsigned char *data = nullptr;
        if (XGetWindowProperty(
                dpy, root, netClientList, 0, (~0L), False, AnyPropertyType,
                &actualType, &actualFormat, &itemCount, &bytesAfter, &data)
                != Success
            || !data) {
            return out;
        }
        Window *windows = reinterpret_cast<Window *>(data);
        const unsigned long count = itemCount;
        for (unsigned long i = 0; i < count; ++i) {
            if (auto w = inspectWindow(dpy, windows[i])) {
                out.append(*w);
            }
        }
        XFree(data);
        return out;
    }

    void activate(qint64 windowId) override {
        if (isWaylandSession()) {
            return;
        }
        Display *dpy = display();
        if (!dpy) {
            return;
        }
        Window root = DefaultRootWindow(dpy);
        XEvent ev{};
        ev.type = ClientMessage;
        ev.xclient.window = static_cast<Window>(windowId);
        ev.xclient.message_type = atom(dpy, "_NET_ACTIVE_WINDOW");
        ev.xclient.format = 32;
        ev.xclient.data.l[0] = 1;
        ev.xclient.data.l[1] = CurrentTime;
        XSendEvent(dpy, root, False, SubstructureRedirectMask | SubstructureNotifyMask, &ev);
        XMapRaised(dpy, static_cast<Window>(windowId));
        XFlush(dpy);
    }

    void closeWindow(qint64 windowId) override {
        if (isWaylandSession()) {
            return;
        }
        Display *dpy = display();
        if (!dpy) {
            return;
        }
        XEvent ev{};
        ev.type = ClientMessage;
        ev.xclient.window = static_cast<Window>(windowId);
        ev.xclient.message_type = atom(dpy, "WM_PROTOCOLS");
        ev.xclient.format = 32;
        ev.xclient.data.l[0] = atom(dpy, "WM_DELETE_WINDOW");
        ev.xclient.data.l[1] = CurrentTime;
        XSendEvent(dpy, static_cast<Window>(windowId), False, NoEventMask, &ev);
        XFlush(dpy);
    }

private:
    std::optional<RawWindow> inspectWindow(Display *dpy, Window win) {
        XWindowAttributes attrs{};
        if (!XGetWindowAttributes(dpy, win, &attrs) || attrs.map_state != IsViewable) {
            return std::nullopt;
        }
        Atom actualType = 0;
        int actualFormat = 0;
        unsigned long itemCount = 0;
        unsigned long bytesAfter = 0;
        unsigned char *data = nullptr;
        pid_t pid = 0;
        if (XGetWindowProperty(
                dpy, win, atom(dpy, "_NET_WM_PID"), 0, 1, False, XA_CARDINAL,
                &actualType, &actualFormat, &itemCount, &bytesAfter, &data)
                == Success
            && data) {
            pid = *reinterpret_cast<pid_t *>(data);
            XFree(data);
        }
        if (pid == 0 || pid == ownPid()) {
            return std::nullopt;
        }
        char *name = nullptr;
        if (!XFetchName(dpy, win, &name) || !name || name[0] == '\0') {
            if (name) {
                XFree(name);
            }
            return std::nullopt;
        }
        const QString title = QString::fromUtf8(name);
        XFree(name);
        const QString exePath = readProcessCmdline(pid);
        if (exePath.isEmpty()) {
            return std::nullopt;
        }
        RawWindow w;
        w.windowId = static_cast<qint64>(win);
        w.title = title;
        w.exePath = exePath;
        const int slash = qMax(exePath.lastIndexOf('/'), exePath.lastIndexOf('\\'));
        w.appName = exePath.mid(slash + 1);
        if (w.isListable()) {
            return w;
        }
        return std::nullopt;
    }
};

class LinuxIconCapture final : public IIconCapture {
public:
    ImageRgba windowIcon(qint64 windowId) override {
        Q_UNUSED(windowId)
        return {};
    }
};

class LinuxThumbnailCapture final : public IThumbnailCapture {
public:
    ImageRgba capture(qint64 windowId) override {
        if (isWaylandSession()) {
            return {};
        }
        Display *dpy = display();
        if (!dpy) {
            return {};
        }
        XWindowAttributes attrs{};
        if (!XGetWindowAttributes(dpy, static_cast<Window>(windowId), &attrs)) {
            return {};
        }
        const int w = attrs.width;
        const int h = attrs.height;
        if (w < 8 || h < 8) {
            return {};
        }
        XImage *image = XGetImage(dpy, static_cast<Window>(windowId), 0, 0, w, h, AllPlanes, ZPixmap);
        if (!image) {
            return {};
        }
        QImage qimg(w, h, QImage::Format_RGBA8888);
        for (int y = 0; y < h; ++y) {
            for (int x = 0; x < w; ++x) {
                const unsigned long pixel = XGetPixel(image, x, y);
                const QRgb rgb = qRgb(
                    (pixel >> 16) & 0xFF,
                    (pixel >> 8) & 0xFF,
                    pixel & 0xFF);
                qimg.setPixel(x, y, rgb);
            }
        }
        XDestroyImage(image);
        ImageRgba out;
        out.width = w;
        out.height = h;
        out.pixels = QByteArray(reinterpret_cast<const char *>(qimg.constBits()), qimg.sizeInBytes());
        return out;
    }
};

} // namespace

std::unique_ptr<IWindowSource> createWindowSourceImpl() {
    return std::make_unique<LinuxWindowSource>();
}

std::unique_ptr<IIconCapture> createIconCaptureImpl() {
    return std::make_unique<LinuxIconCapture>();
}

std::unique_ptr<IThumbnailCapture> createThumbnailCaptureImpl() {
    return std::make_unique<LinuxThumbnailCapture>();
}

PlatformCapabilities queryPlatformCapabilitiesImpl() {
    PlatformCapabilities caps;
    caps.platformName = QStringLiteral("Linux");
    if (isWaylandSession()) {
        caps.sessionType = QStringLiteral("Wayland");
        caps.hotkey = CapabilityLevel::Unavailable;
        caps.activate = CapabilityLevel::Unavailable;
        caps.closeWindow = CapabilityLevel::Unavailable;
        caps.icon = CapabilityLevel::Unavailable;
        caps.thumbnail = CapabilityLevel::Unavailable;
        caps.folderPath = CapabilityLevel::Unavailable;
    } else {
        caps.sessionType = QStringLiteral("X11");
        caps.hotkey = CapabilityLevel::Unavailable;
        caps.activate = CapabilityLevel::Full;
        caps.closeWindow = CapabilityLevel::Full;
        caps.icon = CapabilityLevel::Unavailable;
        caps.thumbnail = CapabilityLevel::Partial;
        caps.folderPath = CapabilityLevel::Unavailable;
    }
    return caps;
}
