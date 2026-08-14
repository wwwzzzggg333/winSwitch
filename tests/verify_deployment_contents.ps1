param(
    [Parameter(Mandatory = $true)]
    [string]$PackageRoot,

    [string]$BinDirectory = '.',

    [string]$PluginDirectory = '.',

    [Parameter(Mandatory = $true)]
    [double]$MaximumSizeMiB,

    [switch]$RequireCompilerRuntime
)

$ErrorActionPreference = 'Stop'

$root = (Resolve-Path -LiteralPath $PackageRoot).Path
$binRoot = Join-Path $root $BinDirectory
$pluginRoot = Join-Path $root $PluginDirectory

$requiredFiles = @(
    (Join-Path $binRoot 'winSwitch.exe'),
    (Join-Path $binRoot 'Qt6Core.dll'),
    (Join-Path $binRoot 'Qt6Gui.dll'),
    (Join-Path $binRoot 'Qt6Widgets.dll'),
    (Join-Path $pluginRoot 'platforms\qwindows.dll')
)
if ($RequireCompilerRuntime) {
    $requiredFiles += @(
        (Join-Path $binRoot 'msvcp140.dll'),
        (Join-Path $binRoot 'vcruntime140.dll'),
        (Join-Path $binRoot 'vcruntime140_1.dll')
    )
}

$missingFiles = @($requiredFiles | Where-Object { -not (Test-Path -LiteralPath $_ -PathType Leaf) })
if ($missingFiles.Count -gt 0) {
    throw "Deployment is missing required files: $($missingFiles -join ', ')"
}

$forbiddenFiles = @(
    (Join-Path $binRoot 'opengl32sw.dll'),
    (Join-Path $binRoot 'D3Dcompiler_47.dll'),
    (Join-Path $binRoot 'dxcompiler.dll'),
    (Join-Path $binRoot 'dxil.dll'),
    (Join-Path $binRoot 'Qt6Network.dll'),
    (Join-Path $binRoot 'Qt6Svg.dll'),
    (Join-Path $pluginRoot 'generic\qtuiotouchplugin.dll'),
    (Join-Path $pluginRoot 'networkinformation\qnetworklistmanager.dll'),
    (Join-Path $pluginRoot 'tls\qcertonlybackend.dll'),
    (Join-Path $pluginRoot 'tls\qopensslbackend.dll'),
    (Join-Path $pluginRoot 'tls\qschannelbackend.dll'),
    (Join-Path $pluginRoot 'iconengines\qsvgicon.dll'),
    (Join-Path $pluginRoot 'imageformats\qsvg.dll')
)
$unexpectedFiles = @($forbiddenFiles | Where-Object { Test-Path -LiteralPath $_ -PathType Leaf })
if ($unexpectedFiles.Count -gt 0) {
    throw "Deployment contains unused runtime files: $($unexpectedFiles -join ', ')"
}

$totalBytes = (Get-ChildItem -LiteralPath $root -Recurse -File | Measure-Object -Property Length -Sum).Sum
$totalMiB = $totalBytes / 1MB
if ($totalMiB -gt $MaximumSizeMiB) {
    throw ('Deployment size is {0:N2} MiB, exceeding the {1:N2} MiB limit' -f $totalMiB, $MaximumSizeMiB)
}

Write-Output ('Deployment contents verified: {0:N2} MiB at {1}' -f $totalMiB, $root)
