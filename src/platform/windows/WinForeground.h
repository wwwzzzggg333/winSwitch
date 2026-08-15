#pragma once

#if defined(Q_OS_WIN)

#include <windows.h>

// Forces a window to the foreground even when the calling process is in the
// background. Windows refuses SetForegroundWindow from a background process
// (the foreground lock), so the foreground thread's input queue is attached
// first to make the call succeed. Reused by the window source (activating
// target windows) and by the main window (activating the panel itself after a
// global hotkey).
namespace winSwitch {

inline void forceForegroundWindow(HWND hwnd) {
    if (!hwnd || !IsWindow(hwnd)) {
        return;
    }
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

} // namespace winSwitch

#endif // Q_OS_WIN
