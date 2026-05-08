#!/usr/bin/env bash

# Root entry for AclassTool build helpers.
# Usage:
#   source ./aclassenv.sh
#   Ahelp
# or:
#   ./aclassenv.sh build debug

set -euo pipefail

_aclass_entry_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# shellcheck source=scripts/cmake-tools.sh
source "${_aclass_entry_dir}/scripts/cmake-tools.sh"

if [[ "${BASH_SOURCE[0]}" == "${0}" ]]; then
  aclass_cli "$@"
fi

