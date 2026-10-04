# Runtime Conformance Track - requirements

> Retrospective packet. This track was delivered before this repository adopted
> spec-driven packets; this packet was ported on 2026-10-05 from the former
> planning status file (`plan/NEXT_STEPS.md`, later `specs/history/next-steps.md`,
> removed in db6b480). The text is kept as written where possible.

Roadmap phase: [Phase 1 - Runtime Conformance Track](../roadmap.md#phase-1---runtime-conformance-track)
Status: **Complete through live gRPC** (v0.3.0); open items carried to Phase 4
Specified: 2026-06-12

## Goal

Own client to `neuriplo-kserve-runtime` validation in this repo, so wire
correctness can be proven without editing neuriplo-infer or the runtime.

## Baseline (v0.2.0, already delivered)

- Standalone HTTP + optional gRPC client extracted from neuriplo-infer
- `raw_input_contents` / `raw_output_contents` on gRPC (default; typed fallback via `KSERVE_BINARY=0`)
- HTTP binary tensor extension (opt-in via `KSERVE_BINARY=1`)
- Proto profiles: `OIP` and `OIP_REPOSITORY` (CI matrix)
- Model Repository extension (index / load / unload)
- Unit tests: protocol, retry, security

## In Scope

- A conformance harness with a dry-run mode and a live mode against a sibling
  runtime checkout.
- A gRPC live parity check that runs the real client code path.
- CI wiring for the dry run.

## Requirements

- [R-1] `scripts/runtime_conformance.sh` runs a dry run with no runtime binary
  and a live HTTP + gRPC run against a local runtime.
- [R-2] The dry run is registered with CTest as `runtime_conformance_dry_run`
  and always runs in CI.
- [R-3] The live invocation is documented in `README.md` § Validation.
- [R-4] Live gRPC parity uses a client-linked oracle binary
  (`kserve-client-conformance`) doing one raw-contents infer, not `grpcurl`.
- [R-5] gRPC unit tests cover raw contents and the `fp64_contents` typed
  fallback.
- [R-6] FP16/BF16 raw-contents case, if the runtime exposes those outputs.
- [R-7] Optional scheduled or `workflow_dispatch` live job with a runtime
  artifact.

## Decisions

- [D-1] Do not depend on system `grpcurl`. A tiny `kserve-client-conformance`
  binary linked to `KserveGrpcClient` exercises the real client code path and
  removes an external harness dependency.

## Out of Scope

Escalated rather than owned by this track:

- `neuriplo` backend ABI, plugin loading, or CI matrix flakes
- `neuriplo-infer` CLI, visualization, or `KserveEngine` adapter
- Runtime scheduling, batching, or admission logic
- Release tags and `versions.env` pin bumps (human)

Backlog recorded at the end of the track, now roadmap Phases 4-6:

1. **Repository extension conformance** - `repositoryIndex` / load / unload against runtime when model-control is enabled
2. **Strict OIP profile live test** - `KSERVE_CLIENT_PROTO_PROFILE=OIP` against OVMS or minimal OIP server
3. **Error mapping audit** - HTTP status and gRPC codes to stable `std::runtime_error` messages per spec
4. **Consumer contract test** - compile-time/header smoke that mimics `neuriplo-infer` FetchContent embedding

## References

- KServe V2 spec: https://kserve.github.io/website/
- Infer compatibility matrix: `neuriplo-infer/docs/KserveCompatibility.md`
