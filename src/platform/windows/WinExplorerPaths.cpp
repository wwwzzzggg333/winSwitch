#include "platform/windows/WinExplorerPaths.h"

#include <windows.h>
#include <shlobj.h>
#include <exdisp.h>
#include <shobjidl.h>

#include <QHash>
#include <QString>

namespace {

QString fileUrlToPath(const QString &url) {
    QString s = url;
    if (s.startsWith(QStringLiteral("file:///"))) {
        s = s.mid(8);
    }
    QByteArray bytes = s.toUtf8();
    QByteArray out;
    out.reserve(bytes.size());
    for (int i = 0; i < bytes.size(); ++i) {
        if (bytes.at(i) == '%' && i + 2 < bytes.size()) {
            bool ok1 = false;
            bool ok2 = false;
            const int hi = QByteArray(1, bytes.at(i + 1)).toInt(&ok1, 16);
            const int lo = QByteArray(1, bytes.at(i + 2)).toInt(&ok2, 16);
            if (ok1 && ok2) {
                out.append(static_cast<char>(hi * 16 + lo));
                i += 2;
                continue;
            }
        }
        out.append(bytes.at(i));
    }
    return QString::fromUtf8(out).replace('/', '\\');
}

} // namespace

QHash<qint64, QString> explorerPaths() {
    QHash<qint64, QString> map;
    HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    IShellWindows *shellWindows = nullptr;
    if (FAILED(CoCreateInstance(CLSID_ShellWindows, nullptr, CLSCTX_ALL, IID_PPV_ARGS(&shellWindows)))) {
        if (SUCCEEDED(hr)) {
            CoUninitialize();
        }
        return map;
    }

    long count = 0;
    if (SUCCEEDED(shellWindows->get_Count(&count))) {
        for (long i = 0; i < count; ++i) {
            VARIANT idx;
            VariantInit(&idx);
            idx.vt = VT_I4;
            idx.lVal = i;
            IDispatch *disp = nullptr;
            if (FAILED(shellWindows->Item(idx, &disp)) || !disp) {
                VariantClear(&idx);
                continue;
            }
            IWebBrowserApp *browser = nullptr;
            if (SUCCEEDED(disp->QueryInterface(IID_PPV_ARGS(&browser))) && browser) {
                SHANDLE_PTR hwnd = 0;
                BSTR url = nullptr;
                if (SUCCEEDED(browser->get_HWND(&hwnd)) && SUCCEEDED(browser->get_LocationURL(&url)) && url) {
                    const QString urlStr = QString::fromWCharArray(url);
                    if (hwnd != 0 && urlStr.startsWith(QStringLiteral("file:///"))) {
                        map.insert(static_cast<qint64>(hwnd), fileUrlToPath(urlStr));
                    }
                }
                if (url) {
                    SysFreeString(url);
                }
                browser->Release();
            }
            disp->Release();
            VariantClear(&idx);
        }
    }
    shellWindows->Release();
    if (SUCCEEDED(hr)) {
        CoUninitialize();
    }
    return map;
}
