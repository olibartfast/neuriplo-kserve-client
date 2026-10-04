# Runtime Conformance Track - plan

> Retrospective packet. This track was delivered before this repository adopted
> spec-driven packets; this packet was ported on 2026-10-05 from the former
> planning status file (`plan/NEXT_STEPS.md`, later `specs/history/next-steps.md`,
> removed in db6b480). The text is kept as written where possible.

The track ran as three steps. At the time it was a Codex-owned work queue on
`develop`; Cursor owned the runtime and the infer adapter, and the human owned
`neuriplo` and release merges.

## Group 1 - Conformance harness (done)

- [x] [T-1] `scripts/runtime_conformance.sh` - dry-run + live driver against sibling runtime
- [x] [T-2] Register with CTest as `runtime_conformance_dry_run` (always runs in CI)
- [x] [T-3] Document live invocation in `README.md` § Validation

## Group 2 - gRPC live parity, library oracle (done)

Runtime PR #9 (`raw_output_contents` on responses) was the dependency.

- [x] [T-4] Add `test/kserve_client_conformance.cpp` - live gRPC oracle via `KserveGrpcClient`
- [x] [T-5] Wire `scripts/runtime_conformance.sh --live --transports grpc` to invoke the binary
- [x] [T-6] gRPC unit tests for raw contents + `fp64_contents` typed fallback
- [ ] [T-7] Add FP16/BF16 raw-contents case if runtime exposes those outputs (carried to roadmap Phase 4)

## Group 3 - CI wiring

- [x] [T-8] CTest dry-run job in `.github/workflows/ci.yml` (`ctest -L conformance`)
- [ ] [T-9] Optional: scheduled or manual `workflow_dispatch` live job with runtime artifact (carried to roadmap Phase 4; needs human sign-off)
