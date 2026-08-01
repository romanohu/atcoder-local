# shellcheck shell=bash

_atcoder_local_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

acc() {
  ATCODER_LOCAL_WRAPPER=1 uv run python "${_atcoder_local_root}/tools/acc_wrapper.py" "$@"
}
