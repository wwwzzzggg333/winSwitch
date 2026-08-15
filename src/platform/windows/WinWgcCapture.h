#pragma once

#include "platform/WindowInfo.h"

// Captures a single frame of the given window using Windows Graphics Capture.
// Unlike PrintWindow this reaches GPU-composited content (Chromium/Electron,
// DirectX, OpenGL) even when the window is occluded by others, at the cost of
// spinning up a D3D11 capture session on a short-lived worker thread. Minimized
// windows are not captured here and the caller skips them before reaching this
// fallback. Returns an empty ImageRgba on any failure.
ImageRgba captureWindowViaWgc(qint64 windowId);
