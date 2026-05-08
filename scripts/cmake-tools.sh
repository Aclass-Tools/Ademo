#!/usr/bin/env bash

set -euo pipefail

ACLASS_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
ACLASS_CMAKE_EXE="${ACLASS_CMAKE_EXE:-cmake}"
ACLASS_GENERATOR="${ACLASS_GENERATOR:-Ninja}"
ACLASS_BUILD_BASE="${ACLASS_BUILD_BASE:-${ACLASS_ROOT}/build}"

ACLASS_QT_PREFIX_PATH="${ACLASS_QT_PREFIX_PATH:-C:/Qt/6.8.3/mingw_64}"
ACLASS_CC="${ACLASS_CC:-C:/Qt/Tools/mingw1310_64/bin/gcc.exe}"
ACLASS_CXX="${ACLASS_CXX:-C:/Qt/Tools/mingw1310_64/bin/g++.exe}"
ACLASS_MAKE_PROGRAM="${ACLASS_MAKE_PROGRAM:-C:/Qt/Tools/Ninja/ninja.exe}"
ACLASS_WINDEPLOYQT="${ACLASS_WINDEPLOYQT:-C:/Qt/6.8.3/mingw_64/bin/windeployqt.exe}"
ACLASS_NSIS_MAKENSIS="${ACLASS_NSIS_MAKENSIS:-C:/Program Files (x86)/NSIS/makensis.exe}"

_Abuild_dir() {
  local type="${1:-Debug}"
  printf "%s/%s" "${ACLASS_BUILD_BASE}" "${type}"
}

Aconfigure() {
  local type="${1:-Debug}"
  local build_dir
  build_dir="$(_Abuild_dir "${type}")"

  "${ACLASS_CMAKE_EXE}" -S "${ACLASS_ROOT}" -B "${build_dir}" \
    -G "${ACLASS_GENERATOR}" \
    -DCMAKE_BUILD_TYPE="${type}" \
    -DCMAKE_PREFIX_PATH="${ACLASS_QT_PREFIX_PATH}" \
    -DCMAKE_C_COMPILER="${ACLASS_CC}" \
    -DCMAKE_CXX_COMPILER="${ACLASS_CXX}" \
    -DCMAKE_MAKE_PROGRAM="${ACLASS_MAKE_PROGRAM}" \
    -DWINDEPLOYQT_EXECUTABLE="${ACLASS_WINDEPLOYQT}" \
    -DNSIS_MAKENSIS_EXECUTABLE="${ACLASS_NSIS_MAKENSIS}"
}

Abuild() {
  local type="${1:-Debug}"
  local build_dir
  build_dir="$(_Abuild_dir "${type}")"
  Aconfigure "${type}"
  "${ACLASS_CMAKE_EXE}" --build "${build_dir}"
}

AbuildDebug() {
  Abuild "Debug"
}

AbuildRelease() {
  Abuild "Release"
}

Ainstall() {
  local type="${1:-Release}"
  local build_dir
  build_dir="$(_Abuild_dir "${type}")"
  Aconfigure "${type}"
  "${ACLASS_CMAKE_EXE}" --build "${build_dir}" --target install
}

Adist() {
  local type="${1:-Release}"
  local build_dir
  build_dir="$(_Abuild_dir "${type}")"
  Aconfigure "${type}"
  "${ACLASS_CMAKE_EXE}" --build "${build_dir}" --target dist_win
}

Apack() {
  local type="${1:-Release}"
  local build_dir
  build_dir="$(_Abuild_dir "${type}")"
  Aconfigure "${type}"
  "${ACLASS_CMAKE_EXE}" --build "${build_dir}" --target dist_installer
}

Aclean() {
  local type="${1:-all}"
  if [[ "${type}" == "all" ]]; then
    rm -rf "${ACLASS_BUILD_BASE}"
    return
  fi
  rm -rf "$(_Abuild_dir "${type}")"
}

Arebuild() {
  local type="${1:-Debug}"
  Aclean "${type}"
  Abuild "${type}"
}

Ahelp() {
  cat <<'EOF'
AclassTool build helpers

After sourcing:
  source ./aclassenv.sh

Preferred functions:
  Aconfigure [Debug|Release] (default: Debug)
  Abuild [Debug|Release] (default: Debug)
  AbuildDebug
  AbuildRelease
  Ainstall [Debug|Release] (default: Release)
  Adist [Debug|Release] (default: Release)
  Apack [Debug|Release] (default: Release)
  Aclean [Debug|Release|all] (default: all)
  Arebuild [Debug|Release]
  Ahelp

Direct CLI:
  ./aclassenv.sh configure     # default: debug
  ./aclassenv.sh build [debug|release]  # default: debug
  ./aclassenv.sh install [debug|release] # default: release
  ./aclassenv.sh dist [debug|release]    # default: release
  ./aclassenv.sh pack [debug|release]    # default: release
  ./aclassenv.sh clean [debug|release|all] # default: all

Legacy aliases still work:
  aclass_configure, aclass_build, aclass_build_debug, aclass_build_release
  aclass_install, aclass_dist, aclass_pack, aclass_clean, aclass_rebuild, aclass_help
EOF
}

_Acli() {
  local cmd="${1:-help}"
  local mode="${2:-}"

  local mode_lc="${mode,,}"

  case "${cmd,,}" in
    configure)
      if [[ "${mode_lc}" == "release" ]]; then Aconfigure "Release"; else Aconfigure "Debug"; fi
      ;;
    build)
      if [[ "${mode_lc}" == "release" ]]; then Abuild "Release"; else Abuild "Debug"; fi
      ;;
    install)
      if [[ "${mode_lc}" == "debug" ]]; then Ainstall "Debug"; else Ainstall "Release"; fi
      ;;
    dist)
      if [[ "${mode_lc}" == "debug" ]]; then Adist "Debug"; else Adist "Release"; fi
      ;;
    pack)
      if [[ "${mode_lc}" == "debug" ]]; then Apack "Debug"; else Apack "Release"; fi
      ;;
    clean)
      if [[ -z "${mode}" || "${mode_lc}" == "all" ]]; then
        Aclean "all"
      elif [[ "${mode_lc}" == "debug" ]]; then
        Aclean "Debug"
      elif [[ "${mode_lc}" == "release" ]]; then
        Aclean "Release"
      else
        echo "Invalid clean mode: ${mode}. Use debug|release|all." >&2
        return 1
      fi
      ;;
    rebuild)
      if [[ "${mode_lc}" == "release" ]]; then Arebuild "Release"; else Arebuild "Debug"; fi
      ;;
    help|--help|-h) Ahelp ;;
    *)
      echo "Unknown command: ${cmd}" >&2
      Ahelp
      return 1
      ;;
  esac
}

aclass_configure() { Aconfigure "$@"; }
aclass_build() { Abuild "$@"; }
aclass_build_debug() { AbuildDebug "$@"; }
aclass_build_release() { AbuildRelease "$@"; }
aclass_install() { Ainstall "$@"; }
aclass_dist() { Adist "$@"; }
aclass_pack() { Apack "$@"; }
aclass_clean() { Aclean "$@"; }
aclass_rebuild() { Arebuild "$@"; }
aclass_help() { Ahelp "$@"; }
aclass_cli() { _Acli "$@"; }

