# Next Steps — Codex work track

**Agent:** Codex · **Integration branch:** `develop` · **Status:** onboarding complete; conformance track active

This file is the living task queue for Codex-owned work in
`neuriplo-kserve-client`. Cursor owns runtime (`neuriplo-kserve-runtime`) and
infer adapter/integration; the human owns `neuriplo` and release merges.

## Completed baseline (v0.2.0)

- Standalone HTTP + optional gRPC client extracted from neuriplo-infer
- `raw_input_contents` / `raw_output_contents` on gRPC (default; typed fallback via `KSERVE_BINARY=0`)
- HTTP binary tensor extension (opt-in via `KSERVE_BINARY=1`)
- Proto profiles: `OIP` and `OIP_REPOSITORY` (CI matrix)
- Model Repository extension (index / load / unload)
- Unit tests: protocol, retry, security

## Active track: runtime conformance

**Goal:** Own client ↔ `neuriplo-kserve-runtime` validation in this repo so Codex
can prove wire correctness without editing infer or runtime.

### Step 1 — Conformance harness (in progress)

- [x] `scripts/runtime_conformance.sh` — dry-run + live driver against sibling runtime
- [x] Register with CTest as `runtime_conformance_dry_run` (always runs in CI)
- [x] Document live invocation in `README.md` § Validation

**Live prerequisites** (sibling checkouts next to this repo):

```bash
../neuriplo-kserve-runtime/build/real-onnx/neuriplo-kserve-runtime   # or debug preset
cmake -B build -DKSERVE_CLIENT_BUILD_TESTS=ON && cmake --build build
```

```bash
# Dry-run (no runtime binary needed):
bash scripts/runtime_conformance.sh --dry-run

# Live HTTP + gRPC against local runtime:
bash scripts/runtime_conformance.sh --live
```

### Step 2 — gRPC live parity (library oracle, not grpcurl)

`neuriplo-infer/docs/KserveCompatibility.md` marks runtime gRPC as dry-run only.
HTTP live conformance is green against `neuriplo-kserve-runtime@develop`.

**Decision:** do not depend on system `grpcurl`. Build a tiny
`kserve-client-conformance` binary (linked to `KserveGrpcClient`) that performs one
raw-contents infer against a live runtime. That exercises the real client code path
and removes an external harness dependency.

- [ ] Add `test/conformance_grpc_main.cpp` (or similar) — one FP32 raw-contents round-trip
- [ ] Wire `scripts/runtime_conformance.sh --live --transports grpc` to invoke the binary
- [ ] Add FP16/BF16 raw-contents case if runtime exposes those outputs (spec § tensor contents)

Until Step 2 lands, `scripts/runtime_conformance.sh` dry-run still prints a grpcurl
command for documentation; live gRPC skips gracefully when the binary is absent.

### Step 3 — CI wiring

- [ ] CTest dry-run job in `.github/workflows/ci.yml` (no runtime binary in GitHub runners)
- [ ] Optional: scheduled or manual `workflow_dispatch` live job with runtime artifact (coordinate with human)

## Backlog (prioritized)

1. **Repository extension conformance** — `repositoryIndex` / load / unload against runtime when model-control is enabled
2. **Strict OIP profile live test** — `KSERVE_CLIENT_PROTO_PROFILE=OIP` against OVMS or minimal OIP server
3. **Error mapping audit** — HTTP status and gRPC codes → stable `std::runtime_error` messages per spec
4. **Consumer contract test** — compile-time/header smoke that mimics `neuriplo-infer` FetchContent embedding

## Out of scope for Codex (escalate)

- `neuriplo` backend ABI, plugin loading, or CI matrix flakes
- `neuriplo-infer` CLI, visualization, or `KserveEngine` adapter (Cursor)
- Runtime scheduling, batching, or admission logic (Cursor)
- Release tags and `versions.env` pin bumps (human)

## References

- KServe V2 spec: https://kserve.github.io/website/
- Cluster map: `neuriplo-platform/ops/CLUSTER_MAP.yaml`
- Infer compatibility matrix: `neuriplo-infer/docs/KserveCompatibility.md`
- Platform e2e: `neuriplo-platform/integration-tests/kserve-runtime-e2e/`
