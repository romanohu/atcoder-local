# shellcheck shell=bash

_atcoder_local_root="$(cd "$(dirname "${(%):-%x}")/.." && pwd)"
_atcoder_local_raw_acc="$(command -v acc 2>/dev/null || true)"

acc() {
  ATCODER_LOCAL_WRAPPER=1 \
  ATCODER_LOCAL_RAW_ACC="${_atcoder_local_raw_acc}" \
  uv run python "${_atcoder_local_root}/tools/acc_wrapper.py" "$@"
}
