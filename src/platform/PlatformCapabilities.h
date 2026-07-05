#pragma once

#include <QString>

enum class CapabilityLevel {
    Full,
    Partial,
    None
};

struct PlatformCapabilities {
    QString platformName;
    QString sessionType;
    CapabilityLevel hotkey = CapabilityLevel::None;
    CapabilityLevel activate = CapabilityLevel::None;
    CapabilityLevel closeWindow = CapabilityLevel::None;
    CapabilityLevel icon = CapabilityLevel::None;
    CapabilityLevel thumbnail = CapabilityLevel::None;
    CapabilityLevel folderPath = CapabilityLevel::None;
};

PlatformCapabilities queryPlatformCapabilities();
