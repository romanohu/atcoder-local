# shellcheck shell=bash

_atcoder_local_root="$(cd "$(dirname "${(%):-%x}")/.." && pwd)"

acc() {
  uv run python "${_atcoder_local_root}/tools/acc_wrapper.py" "$@"
}

