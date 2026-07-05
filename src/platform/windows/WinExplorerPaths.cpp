#include "platform/windows/WinExplorerPaths.h"

#include <windows.h>
#include <knownfolders.h>
#include <shlobj.h>
#include <exdisp.h>
#include <shobjidl.h>

#include <QHash>
#include <QString>

namespace {

QString fileUrlToPath(const QString &url) {
    QString s = url;
    if (s.startsWith(QStringLiteral("file:///"), Qt::CaseInsensitive)) {
        s = s.mid(8);
    } else if (s.startsWith(QStringLiteral("file://"), Qt::CaseInsensitive)) {
        s = s.mid(7);
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

QString knownFolderPath(REFKNOWNFOLDERID folderId) {
    PWSTR path = nullptr;
    if (SUCCEEDED(SHGetKnownFolderPath(folderId, KF_FLAG_DEFAULT, nullptr, &path)) && path) {
        const QString result = QString::fromWCharArray(path);
        CoTaskMemFree(path);
        return result;
    }
    return {};
}

QString shellNameToPath(const QString &shellName) {
    const QString lower = shellName.toLower();
    if (lower == QStringLiteral("downloads")) {
        return knownFolderPath(FOLDERID_Downloads);
    }
    if (lower == QStringLiteral("desktop")) {
        return knownFolderPath(FOLDERID_Desktop);
    }
    if (lower == QStringLiteral("documents")) {
        return knownFolderPath(FOLDERID_Documents);
    }
    if (lower == QStringLiteral("pictures")) {
        return knownFolderPath(FOLDERID_Pictures);
    }
    if (lower == QStringLiteral("music")) {
        return knownFolderPath(FOLDERID_Music);
    }
    if (lower == QStringLiteral("videos")) {
        return knownFolderPath(FOLDERID_Videos);
    }
    if (lower == QStringLiteral("profile") || lower == QStringLiteral("personal")) {
        return knownFolderPath(FOLDERID_Profile);
    }
    if (lower == QStringLiteral("recent")) {
        return knownFolderPath(FOLDERID_Recent);
    }
    return shellName;
}

QString locationUrlToFolderPath(const QString &url) {
    if (url.isEmpty()) {
        return {};
    }
    if (url.startsWith(QStringLiteral("file:"), Qt::CaseInsensitive)) {
        return fileUrlToPath(url);
    }
    if (url.startsWith(QStringLiteral("shell:"), Qt::CaseInsensitive)) {
        return shellNameToPath(url.mid(6));
    }
    if (url.contains(QStringLiteral("20D04FE0-3AEA-1069-A2D8-08002B30309D"), Qt::CaseInsensitive)) {
        return {};
    }
    if (url.startsWith(QStringLiteral("::{"), Qt::CaseInsensitive)) {
        return {};
    }
    return url;
}

} // namespace

QHash<qint64, QString> explorerPaths() {
    QHash<qint64, QString> map;
    const HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    const bool comInitializedHere = (hr == S_OK);
    IShellWindows *shellWindows = nullptr;
    if (FAILED(CoCreateInstance(CLSID_ShellWindows, nullptr, CLSCTX_ALL, IID_PPV_ARGS(&shellWindows)))) {
        if (comInitializedHere) {
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
                if (SUCCEEDED(browser->get_HWND(&hwnd)) && hwnd != 0 && SUCCEEDED(browser->get_LocationURL(&url))
                    && url) {
                    const QString urlStr = QString::fromWCharArray(url);
                    map.insert(static_cast<qint64>(hwnd), locationUrlToFolderPath(urlStr));
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
    if (comInitializedHere) {
        CoUninitialize();
    }
    return map;
}
