#include "platform/IWindowSource.h"

#include <windows.h>

#include <utility>

namespace {

IWindowEventSource::MinimizeHandler g_minimizeHandler;

void CALLBACK minimizeEventProc(
    HWINEVENTHOOK,
    DWORD event,
    HWND hwnd,
    LONG idObject,
    LONG idChild,
    DWORD,
    DWORD) {
    if (event != EVENT_SYSTEM_MINIMIZESTART || idObject != OBJID_WINDOW || idChild != 0) {
        return;
    }
    if (!hwnd || !g_minimizeHandler) {
        return;
    }
    if (GetAncestor(hwnd, GA_ROOT) != hwnd) {
        return;
    }
    DWORD pid = 0;
    GetWindowThreadProcessId(hwnd, &pid);
    if (pid == GetCurrentProcessId()) {
        return;
    }
    const LONG_PTR exStyle = GetWindowLongPtrW(hwnd, GWL_EXSTYLE);
    if (exStyle & WS_EX_TOOLWINDOW) {
        return;
    }
    if (GetWindowTextLengthW(hwnd) <= 0) {
        return;
    }
    g_minimizeHandler(reinterpret_cast<qint64>(hwnd));
}

class WinWindowEventSource final : public IWindowEventSource {
public:
    ~WinWindowEventSource() override {
        if (m_hook) {
            UnhookWinEvent(m_hook);
            m_hook = nullptr;
        }
        g_minimizeHandler = nullptr;
    }

    void setMinimizeHandler(MinimizeHandler handler) override {
        g_minimizeHandler = std::move(handler);
    }

    void start() override {
        if (m_hook) {
            return;
        }
        m_hook = SetWinEventHook(
            EVENT_SYSTEM_MINIMIZESTART,
            EVENT_SYSTEM_MINIMIZESTART,
            nullptr,
            minimizeEventProc,
            0,
            0,
            WINEVENT_OUTOFCONTEXT);
    }

private:
    HWINEVENTHOOK m_hook = nullptr;
};

} // namespace

std::unique_ptr<IWindowEventSource> createWindowEventSourceImpl() {
    return std::make_unique<WinWindowEventSource>();
}
