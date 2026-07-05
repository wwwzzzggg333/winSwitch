#pragma once

#include <QString>

enum class CapabilityLevel {
    Full,
    Partial,
    Unavailable
};

struct PlatformCapabilities {
    QString platformName;
    QString sessionType;
    CapabilityLevel hotkey = CapabilityLevel::Unavailable;
    CapabilityLevel activate = CapabilityLevel::Unavailable;
    CapabilityLevel closeWindow = CapabilityLevel::Unavailable;
    CapabilityLevel icon = CapabilityLevel::Unavailable;
    CapabilityLevel thumbnail = CapabilityLevel::Unavailable;
    CapabilityLevel folderPath = CapabilityLevel::Unavailable;
};

PlatformCapabilities queryPlatformCapabilities();
