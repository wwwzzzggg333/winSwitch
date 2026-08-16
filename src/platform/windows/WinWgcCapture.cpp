// C++/WinRT's base header includes <experimental/coroutine>, which MSVC 14.51+
// (VS 2026) marks deprecated with a hard static_assert. Define the official
// suppression macro before any header so the WGC source builds on newer toolchains.
#define _SILENCE_EXPERIMENTAL_COROUTINE_DEPRECATION_WARNINGS

#include "platform/windows/WinWgcCapture.h"

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <d3d11.h>
#include <dxgi1_2.h>

#include <windows.graphics.capture.interop.h>
#include <windows.graphics.directx.direct3d11.interop.h>

#include <winrt/base.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Graphics.h>
#include <winrt/Windows.Graphics.DirectX.h>
#include <winrt/Windows.Graphics.DirectX.Direct3D11.h>
#include <winrt/Windows.Graphics.Capture.h>

#include <chrono>
#include <thread>

namespace {

using namespace winrt;
using namespace winrt::Windows::Graphics;
using namespace winrt::Windows::Graphics::Capture;
using namespace winrt::Windows::Graphics::DirectX;
using namespace winrt::Windows::Graphics::DirectX::Direct3D11;

constexpr int kMaxCaptureDim = 4096;
constexpr int kFrameTimeoutMs = 700;

// Grabs exactly one frame into a CPU-readable buffer. Must run on an MTA thread.
ImageRgba grabOneFrame(HWND hwnd) {
    try {
        com_ptr<ID3D11Device> device;
        HRESULT hr = D3D11CreateDevice(
            nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, D3D11_CREATE_DEVICE_BGRA_SUPPORT,
            nullptr, 0, D3D11_SDK_VERSION, device.put(), nullptr, nullptr);
        if (FAILED(hr)) {
            hr = D3D11CreateDevice(
                nullptr, D3D_DRIVER_TYPE_WARP, nullptr, D3D11_CREATE_DEVICE_BGRA_SUPPORT,
                nullptr, 0, D3D11_SDK_VERSION, device.put(), nullptr, nullptr);
        }
        if (FAILED(hr) || !device) {
            return {};
        }

        auto factory = get_activation_factory<GraphicsCaptureItem, IGraphicsCaptureItemInterop>();
        GraphicsCaptureItem item{ nullptr };
        hr = factory->CreateForWindow(hwnd, guid_of<GraphicsCaptureItem>(), put_abi(item));
        if (FAILED(hr) || !item) {
            return {};
        }
        const SizeInt32 size = item.Size();
        if (size.Width <= 0 || size.Height <= 0
            || size.Width > kMaxCaptureDim || size.Height > kMaxCaptureDim) {
            return {};
        }

        auto direct3DDevice = device.as<IDXGIDevice>().as<IDirect3DDevice>();
        Direct3D11CaptureFramePool pool = Direct3D11CaptureFramePool::Create(
            direct3DDevice, DirectXPixelFormat::B8G8R8A8UIntNormalized, 2, size);
        GraphicsCaptureSession session = pool.CreateCaptureSession(item);
        session.StartCapture();

        com_ptr<ID3D11DeviceContext> context;
        device->GetImmediateContext(context.put());

        ImageRgba result;
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(kFrameTimeoutMs);
        while (std::chrono::steady_clock::now() < deadline) {
            Direct3D11CaptureFrame frame = pool.TryGetNextFrame();
            if (!frame) {
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
                continue;
            }
            auto surface = frame.Surface();
            com_ptr<::Windows::Graphics::DirectX::Direct3D11::IDirect3DDxgiInterfaceAccess> access;
            if (FAILED(winrt::get_unknown(surface)->QueryInterface(
                    __uuidof(::Windows::Graphics::DirectX::Direct3D11::IDirect3DDxgiInterfaceAccess),
                    access.put_void()))) {
                continue;
            }
            com_ptr<ID3D11Texture2D> texture;
            if (FAILED(access->GetInterface(__uuidof(ID3D11Texture2D), texture.put_void()))) {
                continue;
            }
            D3D11_TEXTURE2D_DESC desc{};
            texture->GetDesc(&desc);
            D3D11_TEXTURE2D_DESC stagingDesc = desc;
            stagingDesc.Usage = D3D11_USAGE_STAGING;
            stagingDesc.BindFlags = 0;
            stagingDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
            stagingDesc.MiscFlags = 0;
            com_ptr<ID3D11Texture2D> staging;
            if (FAILED(device->CreateTexture2D(&stagingDesc, nullptr, staging.put()))) {
                continue;
            }
            context->CopyResource(staging.get(), texture.get());
            D3D11_MAPPED_SUBRESOURCE mapped{};
            if (FAILED(context->Map(staging.get(), 0, D3D11_MAP_READ, 0, &mapped))) {
                continue;
            }
            const int w = static_cast<int>(desc.Width);
            const int h = static_cast<int>(desc.Height);
            result.width = w;
            result.height = h;
            result.pixels.resize(w * h * 4);
            const uchar *src = static_cast<const uchar *>(mapped.pData);
            const int stride = static_cast<int>(mapped.RowPitch);
            uchar *dst = reinterpret_cast<uchar *>(result.pixels.data());
            for (int y = 0; y < h; ++y) {
                const uchar *s = src + static_cast<size_t>(y) * stride;
                uchar *d = dst + static_cast<size_t>(y) * w * 4;
                for (int x = 0; x < w; ++x) {
                    d[x * 4 + 0] = s[x * 4 + 2];
                    d[x * 4 + 1] = s[x * 4 + 1];
                    d[x * 4 + 2] = s[x * 4 + 0];
                    d[x * 4 + 3] = 255;
                }
            }
            context->Unmap(staging.get(), 0);
            break;
        }

        session.Close();
        pool.Close();
        return result;
    } catch (...) {
        return {};
    }
}

} // namespace

ImageRgba captureWindowViaWgc(qint64 windowId) {
    HWND hwnd = reinterpret_cast<HWND>(windowId);
    if (!hwnd || !IsWindow(hwnd)) {
        return {};
    }
    ImageRgba result;
    std::thread worker([&result, hwnd]() {
        init_apartment(apartment_type::multi_threaded);
        try {
            result = grabOneFrame(hwnd);
        } catch (...) {
            result = {};
        }
        uninit_apartment();
    });
    worker.join();
    return result;
}
