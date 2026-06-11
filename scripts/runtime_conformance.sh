#!/usr/bin/env bash
# Client ↔ neuriplo-kserve-runtime conformance harness.
#
# Validates that the KServe V2 wire surface the client implements matches the
# reference runtime (stub backend). Uses curl and grpcurl as portable oracles;
# unit tests in test/ cover client encode/decode directly.
#
# Modes:
#   --dry-run   Print/validate commands without executing (CI default).
#   --live      Start a local runtime binary and run HTTP (+ gRPC when available).
#
# Usage:
#   bash scripts/runtime_conformance.sh --dry-run
#   bash scripts/runtime_conformance.sh --live
#   bash scripts/runtime_conformance.sh --live --transports http
#
# Environment overrides:
#   RUNTIME_BIN   Path to neuriplo-kserve-runtime executable
#   HTTP_PORT     HTTP listen port (default 19090)
#   GRPC_PORT     gRPC listen port (default 19091; 0 disables gRPC checks)
#   MODEL_NAME    Model name (default demo — stub backend)
#
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"

DRY_RUN=false
REQUIRE_LIVE=false
TRANSPORTS="http,grpc"
MODEL_NAME="${MODEL_NAME:-demo}"
HTTP_PORT="${HTTP_PORT:-19090}"
GRPC_PORT="${GRPC_PORT:-19091}"
PROTO_FILE="${PROTO_FILE:-${REPO_ROOT}/proto/kserve_grpc.proto}"

RUNTIME_PID=""

usage() {
  sed -n '2,22p' "${BASH_SOURCE[0]}" | sed 's/^# \{0,1\}//'
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --dry-run) DRY_RUN=true ;;
    --live) DRY_RUN=false ;;
    --require-live) REQUIRE_LIVE=true; DRY_RUN=false ;;
    --transports) TRANSPORTS="${2:?}"; shift ;;
    --runtime-bin) RUNTIME_BIN="${2:?}"; shift ;;
    --http-port) HTTP_PORT="${2:?}"; shift ;;
    --grpc-port) GRPC_PORT="${2:?}"; shift ;;
    --model-name) MODEL_NAME="${2:?}"; shift ;;
    -h | --help) usage; exit 0 ;;
    *) echo "Unknown argument: $1" >&2; usage; exit 2 ;;
  esac
  shift
done

has() { [[ ",$1," == *",$2,"* ]]; }

log() { printf '%s\n' "$*"; }
section() { printf '\n=== %s ===\n' "$*"; }

run_cmd() {
  printf '+ %s\n' "$*"
  if [[ "${DRY_RUN}" == false ]]; then
    "$@"
  fi
}

skip_or_fail() {
  local reason="$1"
  if [[ "${REQUIRE_LIVE}" == true ]]; then
    echo "ERROR: ${reason} (and --require-live was set)" >&2
    exit 1
  fi
  log "SKIP (live): ${reason}"
  exit 0
}

cleanup() {
  if [[ -n "${RUNTIME_PID}" ]]; then
    kill "${RUNTIME_PID}" 2>/dev/null || true
    wait "${RUNTIME_PID}" 2>/dev/null || true
  fi
}
trap cleanup EXIT

resolve_runtime_bin() {
  if [[ -n "${RUNTIME_BIN:-}" ]]; then
    printf '%s' "${RUNTIME_BIN}"
    return
  fi
  local candidate
  for candidate in \
    "${REPO_ROOT}/../neuriplo-kserve-runtime/build/real-onnx-grpc/neuriplo-kserve-runtime" \
    "${REPO_ROOT}/../neuriplo-kserve-runtime/build/real-onnx/neuriplo-kserve-runtime" \
    "${REPO_ROOT}/../neuriplo-kserve-runtime/build/debug/neuriplo-kserve-runtime"; do
    if [[ -x "${candidate}" ]]; then
      printf '%s' "${candidate}"
      return
    fi
  done
  printf '%s' "${REPO_ROOT}/../neuriplo-kserve-runtime/build/debug/neuriplo-kserve-runtime"
}

wait_http_ready() {
  local base="$1"
  local tries="${2:-30}"
  if [[ "${DRY_RUN}" == true ]]; then
    run_cmd curl -fsS "${base}/v2/health/live"
    return 0
  fi
  local i
  for ((i = 1; i <= tries; i++)); do
    if curl -fsS "${base}/v2/health/live" >/dev/null 2>&1; then
      return 0
    fi
    sleep 1
  done
  return 1
}

check_http_health() {
  local base="http://127.0.0.1:${HTTP_PORT}"
  section "HTTP health"
  run_cmd curl -fsS "${base}/v2/health/live"
  run_cmd curl -fsS "${base}/v2/health/ready"
  if [[ "${DRY_RUN}" == false ]]; then
    curl -fsS "${base}/v2/health/live" | grep -q '"live":true' ||
      { echo "ERROR: live probe failed" >&2; return 1; }
    curl -fsS "${base}/v2/health/ready" | grep -q '"ready":true' ||
      { echo "ERROR: ready probe failed" >&2; return 1; }
  fi
  log "HTTP health OK"
}

check_http_metadata() {
  local base="http://127.0.0.1:${HTTP_PORT}"
  section "HTTP model metadata"
  run_cmd curl -fsS "${base}/v2/models/${MODEL_NAME}"
  if [[ "${DRY_RUN}" == false ]]; then
    local meta
    meta="$(curl -fsS "${base}/v2/models/${MODEL_NAME}")"
    echo "${meta}" | grep -q '"input"' ||
      { echo "ERROR: metadata missing input tensor" >&2; return 1; }
  fi
  log "HTTP metadata OK"
}

check_http_infer() {
  local base="http://127.0.0.1:${HTTP_PORT}"
  local url="${base}/v2/models/${MODEL_NAME}/infer"
  local body='{"id":"client-conformance","inputs":[{"name":"input","shape":[1,3,224,224],"datatype":"FP32","data":[]}]}'
  section "HTTP inference"
  run_cmd curl -fsS -H 'Content-Type: application/json' -X POST "${url}" -d "${body}"
  if [[ "${DRY_RUN}" == false ]]; then
    local resp
    resp="$(curl -fsS -H 'Content-Type: application/json' -X POST "${url}" -d "${body}")"
    echo "${resp}" | grep -q '"id":"client-conformance"' ||
      { echo "ERROR: infer response missing request id" >&2; return 1; }
    echo "${resp}" | grep -q '"name":"output"' ||
      { echo "ERROR: infer response missing output tensor" >&2; return 1; }
  fi
  log "HTTP infer OK"
}

check_grpc_infer() {
  section "gRPC inference"
  local hostport="127.0.0.1:${GRPC_PORT}"
  local req='{"model_name":"'"${MODEL_NAME}"'","inputs":[{"name":"input","shape":[1,3,224,224],"datatype":"FP32","contents":{"fp32_contents":[]}}]}'
  local cmd=(grpcurl -plaintext -proto "${PROTO_FILE}"
    -d "${req}" "${hostport}" inference.GRPCInferenceService/ModelInfer)
  run_cmd "${cmd[@]}"
  if [[ "${DRY_RUN}" == true ]]; then
    return 0
  fi
  if ! command -v grpcurl >/dev/null 2>&1; then
    skip_or_fail "grpcurl not installed"
  fi
  local resp
  resp="$("${cmd[@]}")" || {
    echo "ERROR: gRPC infer failed (runtime may lack gRPC; use a real-onnx-grpc build)" >&2
    return 1
  }
  echo "${resp}" | grep -q 'output' ||
    { echo "ERROR: gRPC infer response missing output" >&2; return 1; }
  log "gRPC infer OK"
}

start_runtime() {
  local runtime_bin="$1"
  section "Start runtime (stub)"
  local args=(
    "${runtime_bin}"
    --model-name "${MODEL_NAME}"
    --backend stub
    --port "${HTTP_PORT}"
  )
  if has "${TRANSPORTS}" grpc && [[ "${GRPC_PORT}" != "0" ]]; then
    args+=(--grpc-port "${GRPC_PORT}")
  fi
  if [[ "${DRY_RUN}" == true ]]; then
    run_cmd "${args[@]}"
    return 0
  fi
  "${args[@]}" &
  RUNTIME_PID=$!
  wait_http_ready "http://127.0.0.1:${HTTP_PORT}" ||
    { echo "ERROR: runtime did not become ready" >&2; return 1; }
}

section "Runtime conformance ($([[ ${DRY_RUN} == true ]] && echo dry-run || echo live))"
log "transports=${TRANSPORTS} model=${MODEL_NAME} http_port=${HTTP_PORT} grpc_port=${GRPC_PORT}"

RUNTIME_BIN="$(resolve_runtime_bin)"
log "runtime_bin=${RUNTIME_BIN}"
log "proto_file=${PROTO_FILE}"

if [[ "${DRY_RUN}" == false ]]; then
  command -v curl >/dev/null 2>&1 || skip_or_fail "curl not available"
  [[ -x "${RUNTIME_BIN}" ]] || skip_or_fail "runtime binary not executable at ${RUNTIME_BIN}"
fi

if [[ ! -f "${PROTO_FILE}" && "${DRY_RUN}" == false ]] && has "${TRANSPORTS}" grpc; then
  skip_or_fail "gRPC proto not found at ${PROTO_FILE}"
fi

start_runtime "${RUNTIME_BIN}"

rc=0
if has "${TRANSPORTS}" http; then
  check_http_health || rc=1
  check_http_metadata || rc=1
  check_http_infer || rc=1
fi
if has "${TRANSPORTS}" grpc && [[ "${GRPC_PORT}" != "0" ]]; then
  check_grpc_infer || rc=1
fi

if [[ "${rc}" -eq 0 ]]; then
  section "RESULT: PASS ($([[ ${DRY_RUN} == true ]] && echo 'dry-run' || echo 'live'))"
else
  section "RESULT: FAIL"
fi
exit "${rc}"
