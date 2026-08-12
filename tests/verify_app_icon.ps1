$ErrorActionPreference = 'Stop'

$projectRoot = Split-Path -Parent $PSScriptRoot

function Assert-FileContains {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Path,
        [Parameter(Mandatory = $true)]
        [string]$Pattern,
        [Parameter(Mandatory = $true)]
        [string]$Description
    )

    $content = Get-Content -Raw -LiteralPath (Join-Path $projectRoot $Path)
    if ($content -notmatch $Pattern) {
        throw "Missing app icon integration: $Description ($Path)"
    }
}

$requiredAssets = @(
    'resources/icons/winswitch-1024.png',
    'resources/icons/winswitch-256.png',
    'resources/icons/winswitch.ico'
)
foreach ($asset in $requiredAssets) {
    if (-not (Test-Path -LiteralPath (Join-Path $projectRoot $asset) -PathType Leaf)) {
        throw "Missing app icon asset: $asset"
    }
}

Assert-FileContains 'resources/resources.qrc' '<file>icons/winswitch-256\.png</file>' 'PNG registered in Qt resources'
Assert-FileContains 'src/main.cpp' 'QIcon\(QStringLiteral\(":/icons/winswitch-256\.png"\)\)' 'Qt application uses the branded icon'
Assert-FileContains 'src/app/Application.cpp' 'QApplication::windowIcon\(\)' 'system tray uses the Qt application icon'
Assert-FileContains 'resources/winswitch.rc' 'winswitch\.ico' 'Windows resource script references the ICO'
Assert-FileContains 'CMakeLists.txt' 'resources/winswitch\.rc' 'Windows resource script is compiled into the executable'

Write-Output 'Application icon integration verified.'
