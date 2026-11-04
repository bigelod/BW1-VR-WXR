# Builds the mod and stages the files that are ready to test on the device.
#
#   .\bwvr\tools\wxr\deploy.ps1              build and stage
#   .\bwvr\tools\wxr\deploy.ps1 -Game        also put the files in the game
#   .\bwvr\tools\wxr\deploy.ps1 -SkipBuild   stage what is built now
#
# The bundle is Release\BnW-VR-WXR. Copy that folder beside runblack.exe on
# the device. The script writes nothing else under Release.
#
# The guard is the point of this script: it refuses to stage a ddraw.dll that
# is older than the newest source file. A test of a stale build looks like a
# change that does nothing, and it costs a whole session.

[CmdletBinding()]
param(
    # Also put the files in the game folder of local\game-path.txt, for a test
    # on the PC.
    [switch]$Game,
    # Stage the build that is there now. The guard still runs.
    [switch]$SkipBuild,
    # Build each file again. Needed after a change of a header in the build
    # folder, because MSBuild does not track those.
    [switch]$Clean
)

Set-StrictMode -Version 2.0
$ErrorActionPreference = 'Stop'

$Repo     = (Resolve-Path (Join-Path $PSScriptRoot '..\..\..')).Path
$Bwvr     = Join-Path $Repo 'bwvr'
$BuildDir = Join-Path $Bwvr 'build'
$ModDll   = Join-Path $BuildDir 'src\mod\Release\ddraw.dll'
$StampCpp = Join-Path $BuildDir 'generated\build_stamp.cpp'
$Bundle   = Join-Path $Repo 'Release\BnW-VR-WXR'
$IniSrc    = Join-Path $Bwvr 'config\bwvr.wxr.ini'
$ReadmeSrc = Join-Path $Bwvr 'config\wxr-readme.txt'

function Say([string]$Text)  { Write-Host ('== ' + $Text) -ForegroundColor Cyan }
function Note([string]$Text) { Write-Host ('   ' + $Text) }
function Stop-Deploy([string]$Text) {
    Write-Host ''
    Write-Host $Text -ForegroundColor Red
    Write-Host ''
    exit 1
}

# The newest write time of the files that the mod is built from.
function Get-NewestSource {
    $roots = @((Join-Path $Bwvr 'src'), (Join-Path $Bwvr 'cmake'))
    $newest = $null
    foreach ($root in $roots) {
        if (-not (Test-Path -LiteralPath $root)) { continue }
        foreach ($file in (Get-ChildItem -LiteralPath $root -Recurse -File -Include *.cpp, *.h, *.c, *.in, *.cmake, *.rc, CMakeLists.txt)) {
            if ($null -eq $newest -or $file.LastWriteTime -gt $newest.LastWriteTime) { $newest = $file }
        }
    }
    foreach ($one in @((Join-Path $Bwvr 'CMakeLists.txt'))) {
        if (Test-Path -LiteralPath $one) {
            $file = Get-Item -LiteralPath $one
            if ($null -eq $newest -or $file.LastWriteTime -gt $newest.LastWriteTime) { $newest = $file }
        }
    }
    return $newest
}

# ------------------------------------------------------------------- build

# dev.ps1 configures, builds and runs the math tests, and it finds a CMake of
# 3.20 or later by itself.
if (-not $SkipBuild) {
    $dev = Join-Path $Repo 'dev.ps1'
    if ($Clean) { & $dev build -Clean } else { & $dev build }
    if ($LASTEXITCODE -ne 0) { Stop-Deploy 'The build failed. See the lines above.' }
}

if (-not (Test-Path -LiteralPath $ModDll)) {
    Stop-Deploy "The mod is not built. Expected:`n$ModDll"
}

# ------------------------------------------------------------------- guard

$dll    = Get-Item -LiteralPath $ModDll
$newest = Get-NewestSource
if ($null -ne $newest -and $dll.LastWriteTime -lt $newest.LastWriteTime) {
    $rel = $newest.FullName.Substring($Repo.Length).TrimStart('\')
    Stop-Deploy ("STOPPED: the build is older than the source.`n`n" +
        "  ddraw.dll  $($dll.LastWriteTime.ToString('s'))`n" +
        "  $rel  $($newest.LastWriteTime.ToString('s'))`n`n" +
        "Build again. Do not work around this: a test of a stale build looks`n" +
        "like a change that does nothing.")
}

# What the first line of the log on the device must say.
$stamp = ''
if (Test-Path -LiteralPath $StampCpp) {
    $text = Get-Content -LiteralPath $StampCpp -Raw
    if ($text -match 'kBuildStamp\[\]\s*=\s*"([^"]+)"') { $stamp = $Matches[1] }
}

# ------------------------------------------------------------------- stage

Say 'Stage the bundle'
[void](New-Item -ItemType Directory -Force -Path $Bundle)

foreach ($need in @($IniSrc, $ReadmeSrc)) {
    if (-not (Test-Path -LiteralPath $need)) { Stop-Deploy "A file that ships is missing:`n$need" }
}

$staged = @(
    @{ Dest = 'ddraw.dll';  Src = $ModDll;   What = 'the mod' }
    @{ Dest = 'bwvr.ini';   Src = $IniSrc;   What = 'the settings for the container' }
    @{ Dest = 'README.txt'; Src = $ReadmeSrc; What = 'what to test and what to send back' }
)

foreach ($item in $staged) {
    $dest = Join-Path $Bundle $item.Dest
    Copy-Item -LiteralPath $item.Src -Destination $dest -Force
    $a = (Get-FileHash -LiteralPath $item.Src  -Algorithm SHA256).Hash
    $b = (Get-FileHash -LiteralPath $dest -Algorithm SHA256).Hash
    Note ("$($item.Dest)  $([int](Get-Item -LiteralPath $dest).Length) bytes  match=$($a -eq $b)  ($($item.What))")
    if ($a -ne $b) { Stop-Deploy "The copy of $($item.Dest) does not match its source." }
}

# ------------------------------------------------------------------- game

if ($Game) {
    $gameFile = Join-Path $Repo 'local\game-path.txt'
    if (-not (Test-Path -LiteralPath $gameFile)) {
        Stop-Deploy 'No game folder. Run: .\dev.ps1 setup -Game "<the folder that holds runblack.exe>"'
    }
    $dir = (Get-Content -LiteralPath $gameFile -TotalCount 1).Trim()
    if (-not (Test-Path -LiteralPath (Join-Path $dir 'runblack.exe'))) {
        Stop-Deploy "The game folder holds no runblack.exe:`n$dir"
    }
    if (Get-Process runblack -ErrorAction SilentlyContinue) { Stop-Deploy 'The game runs. Close it first.' }

    Say "Put the files in $dir"
    $kept = Join-Path $dir 'ddraw.dll.before-bwvr'
    $live = Join-Path $dir 'ddraw.dll'
    $ours = $false
    if (Test-Path -LiteralPath $live) {
        $ours = ([System.Diagnostics.FileVersionInfo]::GetVersionInfo($live).ProductName -eq 'Black & White VR Mod')
    }
    if ((Test-Path -LiteralPath $live) -and -not $ours -and -not (Test-Path -LiteralPath $kept)) {
        Copy-Item -LiteralPath $live -Destination $kept
        Note 'kept the ddraw.dll that was there as ddraw.dll.before-bwvr'
    }
    foreach ($file in (Get-ChildItem -LiteralPath $Bundle -File)) {
        Copy-Item -LiteralPath $file.FullName -Destination (Join-Path $dir $file.Name) -Force
        Note $file.Name
    }
}

# ------------------------------------------------------------------- report

Write-Host ''
Say 'Ready'
Note ('the bundle: ' + $Bundle)
if ($stamp) {
    Note 'the first line of bwvr.log on the device must hold this stamp:'
    Note ('  built ' + $stamp)
    Note 'a different stamp means that another build ran.'
}
