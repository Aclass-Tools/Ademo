$script:ACLASS_ROOT = Split-Path -Parent (Split-Path -Parent $MyInvocation.MyCommand.Path)
$script:ACLASS_CMAKE_EXE = if ($env:ACLASS_CMAKE_EXE) { $env:ACLASS_CMAKE_EXE } else { "cmake" }
$script:ACLASS_GENERATOR = if ($env:ACLASS_GENERATOR) { $env:ACLASS_GENERATOR } else { "Ninja" }
$script:ACLASS_BUILD_BASE = if ($env:ACLASS_BUILD_BASE) { $env:ACLASS_BUILD_BASE } else { Join-Path $script:ACLASS_ROOT "build" }

$script:ACLASS_QT_PREFIX_PATH = if ($env:ACLASS_QT_PREFIX_PATH) { $env:ACLASS_QT_PREFIX_PATH } else { "C:/Qt/6.8.3/mingw_64" }
$script:ACLASS_CC = if ($env:ACLASS_CC) { $env:ACLASS_CC } else { "C:/Qt/Tools/mingw1310_64/bin/gcc.exe" }
$script:ACLASS_CXX = if ($env:ACLASS_CXX) { $env:ACLASS_CXX } else { "C:/Qt/Tools/mingw1310_64/bin/g++.exe" }
$script:ACLASS_MAKE_PROGRAM = if ($env:ACLASS_MAKE_PROGRAM) { $env:ACLASS_MAKE_PROGRAM } else { "C:/Qt/Tools/Ninja/ninja.exe" }
$script:ACLASS_WINDEPLOYQT = if ($env:ACLASS_WINDEPLOYQT) { $env:ACLASS_WINDEPLOYQT } else { "C:/Qt/6.8.3/mingw_64/bin/windeployqt.exe" }
$script:ACLASS_NSIS_MAKENSIS = if ($env:ACLASS_NSIS_MAKENSIS) { $env:ACLASS_NSIS_MAKENSIS } else { "C:/Program Files (x86)/NSIS/makensis.exe" }

function _AbuildDir {
    param([string]$Type = "Debug")
    return (Join-Path $script:ACLASS_BUILD_BASE $Type)
}

function Aconfigure {
    param([string]$Type = "Debug")
    $buildDir = _AbuildDir -Type $Type

    & $script:ACLASS_CMAKE_EXE -S $script:ACLASS_ROOT -B $buildDir `
        -G $script:ACLASS_GENERATOR `
        -DCMAKE_BUILD_TYPE=$Type `
        -DCMAKE_PREFIX_PATH=$script:ACLASS_QT_PREFIX_PATH `
        -DCMAKE_C_COMPILER=$script:ACLASS_CC `
        -DCMAKE_CXX_COMPILER=$script:ACLASS_CXX `
        -DCMAKE_MAKE_PROGRAM=$script:ACLASS_MAKE_PROGRAM `
        -DWINDEPLOYQT_EXECUTABLE=$script:ACLASS_WINDEPLOYQT `
        -DNSIS_MAKENSIS_EXECUTABLE=$script:ACLASS_NSIS_MAKENSIS
    if ($LASTEXITCODE -ne 0) { throw "Aconfigure failed." }
}

function Abuild {
    param([string]$Type = "Debug")
    $buildDir = _AbuildDir -Type $Type
    Aconfigure -Type $Type
    & $script:ACLASS_CMAKE_EXE --build $buildDir
    if ($LASTEXITCODE -ne 0) { throw "Abuild failed." }
}

function AbuildDebug { Abuild -Type "Debug" }
function AbuildRelease { Abuild -Type "Release" }

function Ainstall {
    param([string]$Type = "Release")
    $buildDir = _AbuildDir -Type $Type
    Aconfigure -Type $Type
    & $script:ACLASS_CMAKE_EXE --build $buildDir --target install
    if ($LASTEXITCODE -ne 0) { throw "Ainstall failed." }
}

function Adist {
    param([string]$Type = "Release")
    $buildDir = _AbuildDir -Type $Type
    Aconfigure -Type $Type
    & $script:ACLASS_CMAKE_EXE --build $buildDir --target dist_win
    if ($LASTEXITCODE -ne 0) { throw "Adist failed." }
}

function Apack {
    param([string]$Type = "Release")
    $buildDir = _AbuildDir -Type $Type
    Aconfigure -Type $Type
    & $script:ACLASS_CMAKE_EXE --build $buildDir --target dist_installer
    if ($LASTEXITCODE -ne 0) { throw "Apack failed." }
}

function Aclean {
    param([string]$Type = "all")
    if ($Type -eq "all") {
        if (Test-Path $script:ACLASS_BUILD_BASE) {
            Remove-Item -Recurse -Force -LiteralPath $script:ACLASS_BUILD_BASE
        }
        return
    }

    $target = _AbuildDir -Type $Type
    if (Test-Path $target) {
        Remove-Item -Recurse -Force -LiteralPath $target
    }
}

function Arebuild {
    param([string]$Type = "Debug")
    Aclean -Type $Type
    Abuild -Type $Type
}

function Ahelp {
    $help = @(
        "AclassTool PowerShell helpers",
        "",
        "Load functions:",
        "  . .\\aclassenv.ps1",
        "",
        "Preferred functions:",
        "  Aconfigure [Debug|Release] (default: Debug)",
        "  Abuild [Debug|Release] (default: Debug)",
        "  AbuildDebug",
        "  AbuildRelease",
        "  Ainstall [Debug|Release] (default: Release)",
        "  Adist [Debug|Release] (default: Release)",
        "  Apack [Debug|Release] (default: Release)",
        "  Aclean [Debug|Release|all] (default: all)",
        "  Arebuild [Debug|Release]",
        "  Ahelp",
        "",
        "Direct CLI:",
        "  .\\aclassenv.ps1 build",
        "  .\\aclassenv.ps1 build release",
        "  .\\aclassenv.ps1 install",
        "  .\\aclassenv.ps1 dist",
        "  .\\aclassenv.ps1 pack",
        "  .\\aclassenv.ps1 clean"
    )
    $help -join [Environment]::NewLine | Write-Output
}

function Acli {
    param(
        [string]$Command = "help",
        [string]$Mode = ""
    )

    $cmd = $Command.ToLowerInvariant()
    $modeLower = $Mode.ToLowerInvariant()

    switch ($cmd) {
        "configure" {
            if ($modeLower -eq "release") { Aconfigure -Type "Release" }
            else { Aconfigure -Type "Debug" }
        }
        "build" {
            if ($modeLower -eq "release") { Abuild -Type "Release" }
            else { Abuild -Type "Debug" }
        }
        "install" {
            if ($modeLower -eq "debug") { Ainstall -Type "Debug" }
            else { Ainstall -Type "Release" }
        }
        "dist" {
            if ($modeLower -eq "debug") { Adist -Type "Debug" }
            else { Adist -Type "Release" }
        }
        "pack" {
            if ($modeLower -eq "debug") { Apack -Type "Debug" }
            else { Apack -Type "Release" }
        }
        "clean" {
            if ([string]::IsNullOrEmpty($modeLower) -or $modeLower -eq "all") {
                Aclean -Type "all"
            }
            elseif ($modeLower -eq "debug") {
                Aclean -Type "Debug"
            }
            elseif ($modeLower -eq "release") {
                Aclean -Type "Release"
            }
            else {
                throw "Invalid clean mode: $Mode. Use debug|release|all."
            }
        }
        "rebuild" {
            if ($modeLower -eq "release") { Arebuild -Type "Release" }
            else { Arebuild -Type "Debug" }
        }
        "help" { Ahelp }
        default { throw "Unknown command: $Command" }
    }
}

function aclass_configure { param([string]$Type = "Debug") Aconfigure -Type $Type }
function aclass_build { param([string]$Type = "Debug") Abuild -Type $Type }
function aclass_build_debug { AbuildDebug }
function aclass_build_release { AbuildRelease }
function aclass_install { param([string]$Type = "Release") Ainstall -Type $Type }
function aclass_dist { param([string]$Type = "Release") Adist -Type $Type }
function aclass_pack { param([string]$Type = "Release") Apack -Type $Type }
function aclass_clean { param([string]$Type = "all") Aclean -Type $Type }
function aclass_rebuild { param([string]$Type = "Debug") Arebuild -Type $Type }
function aclass_help { Ahelp }
function aclass_cli { param([string]$Command = "help", [string]$Mode = "") Acli -Command $Command -Mode $Mode }

