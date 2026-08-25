param(
    [string]$BuildDirectory = "build-windows",
    [string]$OutputDirectory = "dist/windows",
    [string]$Configuration = "Release"
)

$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot
$buildRoot = Join-Path $root $BuildDirectory
$binaryDirectory = Join-Path $buildRoot $Configuration
if (-not (Test-Path $binaryDirectory)) {
    $binaryDirectory = $buildRoot
}

$executableName = "Parallels X - Clash of Souls.exe"
$executable = Join-Path $binaryDirectory $executableName
if (-not (Test-Path $executable)) {
    throw "Windows executable was not generated: $executable"
}

$outputRoot = Join-Path $root $OutputDirectory
$packageDirectory = Join-Path $outputRoot "package"
$assetDirectory = Join-Path $packageDirectory "assets/characters/rrvvfo"
$zipName = "Parallels-X-Clash-of-Souls-3.0R-U16-Final-Prototype-Windows.zip"
$zipPath = Join-Path $outputRoot $zipName
$checksumPath = "$zipPath.sha256"

if (Test-Path $packageDirectory) {
    Remove-Item $packageDirectory -Recurse -Force
}
New-Item $assetDirectory -ItemType Directory -Force | Out-Null
Copy-Item $executable (Join-Path $packageDirectory $executableName)
Copy-Item (Join-Path $root "assets/characters/rrvvfo/rrvvfo-dev.pxskel") `
    (Join-Path $assetDirectory "rrvvfo-dev.pxskel")
Copy-Item (Join-Path $root "packaging/windows/README.txt") `
    (Join-Path $packageDirectory "README.txt")

# CI uses the static SDL/vcpkg triplet. Refuse to silently ship an unpackaged
# SDL runtime if that configuration ever changes.
$unexpectedDll = Get-ChildItem $binaryDirectory -Filter "SDL2.dll" -ErrorAction SilentlyContinue
if ($unexpectedDll) {
    Copy-Item $unexpectedDll.FullName (Join-Path $packageDirectory "SDL2.dll")
}

if (Test-Path $zipPath) {
    Remove-Item $zipPath -Force
}
Compress-Archive -Path (Join-Path $packageDirectory "*") -DestinationPath $zipPath -CompressionLevel Optimal
$hash = (Get-FileHash $zipPath -Algorithm SHA256).Hash.ToLowerInvariant()
Set-Content -Path $checksumPath -Value "$hash  $zipName" -NoNewline -Encoding ascii

Write-Host "Packaged $zipPath"
Write-Host "SHA-256 $hash"
