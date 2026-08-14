param(
    [Parameter(Mandatory = $true)]
    [string]$InstallerPath
)

$ErrorActionPreference = 'Stop'

. "$PSScriptRoot/installer_verification_helpers.ps1"

$resolvedInstaller = (Resolve-Path -LiteralPath $InstallerPath).Path
$installDir = Join-Path $env:TEMP "winSwitch installer test $([Guid]::NewGuid().ToString('N'))"
$runKey = 'HKCU:\Software\Microsoft\Windows\CurrentVersion\Run'
$valueName = 'winSwitch'
$previousValue = $null
$hadPreviousValue = $false
$appProcess = $null
$uninstallLog = Join-Path $env:TEMP "winSwitch-uninstall-$([Guid]::NewGuid().ToString('N')).log"

try {
    $preexisting = Get-CimInstance Win32_Process -Filter "Name='winSwitch.exe'" -ErrorAction SilentlyContinue
    if ($preexisting) {
        throw 'Installer verification requires no pre-existing winSwitch.exe process'
    }

    $previousStartupValue = Get-OptionalRegistryValue -LiteralPath $runKey -Name $valueName
    if ($previousStartupValue.Exists) {
        $previousValue = $previousStartupValue.Value
        $hadPreviousValue = $true
    }

    $install = Start-Process -FilePath $resolvedInstaller -ArgumentList @(
        '/VERYSILENT',
        '/SUPPRESSMSGBOXES',
        '/NORESTART',
        ('/DIR="{0}"' -f $installDir)
    ) -Wait -PassThru
    if ($install.ExitCode -ne 0) {
        throw "Installer exited with code $($install.ExitCode)"
    }

    $installedExe = Join-Path $installDir 'bin\winSwitch.exe'
    $uninstaller = Get-ChildItem -LiteralPath $installDir -Filter 'unins*.exe' -File |
        Select-Object -First 1 -ExpandProperty FullName
    if (-not $uninstaller) {
        throw "Installed package does not contain an uninstaller in $installDir"
    }
    $requiredFiles = @(
        $installedExe,
        (Join-Path $installDir 'bin\qt.conf'),
        (Join-Path $installDir 'bin\Qt6Core.dll'),
        (Join-Path $installDir 'bin\Qt6Gui.dll'),
        (Join-Path $installDir 'bin\Qt6Widgets.dll'),
        (Join-Path $installDir 'bin\msvcp140.dll'),
        (Join-Path $installDir 'bin\vcruntime140.dll'),
        (Join-Path $installDir 'bin\vcruntime140_1.dll'),
        (Join-Path $installDir 'plugins\platforms\qwindows.dll'),
        $uninstaller
    )
    $missing = @($requiredFiles | Where-Object { -not (Test-Path -LiteralPath $_) })
    if ($missing.Count -gt 0) {
        throw "Installed package is missing: $($missing -join ', ')"
    }
    $deploymentVerification = @{
        PackageRoot = $installDir
        BinDirectory = 'bin'
        PluginDirectory = 'plugins'
        MaximumSizeMiB = 35
        RequireCompilerRuntime = $true
    }
    & "$PSScriptRoot/verify_deployment_contents.ps1" @deploymentVerification

    $expectedStartupValue = '"{0}"' -f $installedExe
    $actualStartupValue = Get-OptionalRegistryValue -LiteralPath $runKey -Name $valueName
    if (-not $actualStartupValue.Exists -or $actualStartupValue.Value -ne $expectedStartupValue) {
        throw "Unexpected startup registry value. Expected '$expectedStartupValue', got '$($actualStartupValue.Value)'"
    }

    $appProcess = Start-Process -FilePath $installedExe -PassThru
    Start-Sleep -Seconds 2
    if ($appProcess.HasExited) {
        throw "Installed application exited during startup with code $($appProcess.ExitCode)"
    }
    $unexpectedMutableFiles = @(
        (Join-Path $installDir 'bin\app.log'),
        (Join-Path $installDir 'bin\config.json'),
        (Join-Path $installDir 'bin\.welcome_shown')
    ) | Where-Object { Test-Path -LiteralPath $_ }
    if ($unexpectedMutableFiles) {
        throw "Installed application wrote mutable state inside the install directory: $($unexpectedMutableFiles -join ', ')"
    }

    $uninstall = Start-Process -FilePath $uninstaller -ArgumentList @(
        '/VERYSILENT',
        '/SUPPRESSMSGBOXES',
        '/NORESTART',
        ('/LOG="{0}"' -f $uninstallLog)
    ) -Wait -PassThru
    if ($uninstall.ExitCode -ne 0) {
        throw "Uninstaller exited with code $($uninstall.ExitCode)"
    }

    if (Test-Path -LiteralPath $installedExe) {
        $logTail = if (Test-Path -LiteralPath $uninstallLog) {
            (Get-Content -LiteralPath $uninstallLog -Tail 40) -join [Environment]::NewLine
        } else {
            'Uninstall log was not created'
        }
        throw "Uninstaller left the application executable behind: $installedExe`n$logTail"
    }
    if ((Get-OptionalRegistryValue -LiteralPath $runKey -Name $valueName).Exists) {
        throw 'Uninstaller did not remove the winSwitch startup registry value'
    }
    if ($appProcess -and -not $appProcess.HasExited) {
        throw 'Uninstaller did not close the running winSwitch process'
    }
    for ($attempt = 0; $attempt -lt 50 -and (Test-Path -LiteralPath $installDir); $attempt++) {
        Start-Sleep -Milliseconds 100
    }
    if (Test-Path -LiteralPath $installDir) {
        throw "Uninstaller left the installation directory behind: $installDir"
    }

    Write-Output "Installer install/startup/uninstall behavior verified: $resolvedInstaller"
} finally {
    if ($appProcess -and -not $appProcess.HasExited) {
        Stop-Process -Id $appProcess.Id -Force -ErrorAction SilentlyContinue
    }
    $remainingUninstaller = Get-ChildItem -LiteralPath $installDir -Filter 'unins*.exe' -File -ErrorAction SilentlyContinue |
        Select-Object -First 1 -ExpandProperty FullName
    if ($remainingUninstaller) {
        Start-Process -FilePath $remainingUninstaller -ArgumentList @(
            '/VERYSILENT',
            '/SUPPRESSMSGBOXES',
            '/NORESTART'
        ) -Wait | Out-Null
    }
    if (Test-Path -LiteralPath $installDir) {
        Remove-Item -LiteralPath $installDir -Recurse -Force -ErrorAction SilentlyContinue
    }
    Remove-Item -LiteralPath $uninstallLog -Force -ErrorAction SilentlyContinue
    if ($hadPreviousValue) {
        Set-ItemProperty -LiteralPath $runKey -Name $valueName -Value $previousValue
    } else {
        Remove-ItemProperty -LiteralPath $runKey -Name $valueName -ErrorAction SilentlyContinue
    }
}
