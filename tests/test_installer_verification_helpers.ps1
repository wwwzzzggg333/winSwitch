$ErrorActionPreference = 'Stop'

. "$PSScriptRoot/installer_verification_helpers.ps1"

$missingPath = "HKCU:\Software\winSwitch\InstallerVerificationTests\$([Guid]::NewGuid().ToString('N'))"
$result = Get-OptionalRegistryValue -LiteralPath $missingPath -Name 'winSwitch'

if ($result.Exists) {
    throw 'A registry value under a missing key must not be reported as existing'
}
if ($null -ne $result.Value) {
    throw 'A registry value under a missing key must return a null value'
}

$missingValueName = "winSwitchMissing$([Guid]::NewGuid().ToString('N'))"
$result = Get-OptionalRegistryValue -LiteralPath 'HKCU:\Software' -Name $missingValueName

if ($result.Exists) {
    throw 'A missing registry value under an existing key must not be reported as existing'
}
if ($null -ne $result.Value) {
    throw 'A missing registry value under an existing key must return a null value'
}

Write-Output 'Missing registry keys and values are handled as absent optional values'
