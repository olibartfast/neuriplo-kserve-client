# Runtime Conformance Track - validation

> Retrospective packet. This track was delivered before this repository adopted
> spec-driven packets; this packet was ported on 2026-10-05 from the former
> planning status file (`plan/NEXT_STEPS.md`, later `specs/history/next-steps.md`,
> removed in db6b480). The text is kept as written where possible.

## Checks

- [x] [V-1] -> [R-1], [R-2]: `ctest -L conformance` runs `runtime_conformance_dry_run` in CI.
- [x] [V-2] -> [R-1], [R-4]: `bash scripts/runtime_conformance.sh --live` passes HTTP and gRPC against a live runtime.
- [x] [V-3] -> [R-5]: gRPC unit tests for raw contents and `fp64_contents` pass.
- [x] [V-4] -> [R-3]: `README.md` § Validation documents the live invocation.
- [ ] [V-5] -> [R-6]: FP16/BF16 raw-contents case. Open, roadmap Phase 4.
- [ ] [V-6] -> [R-7]: live CI job. Open, roadmap Phase 4.

## Live prerequisites

Sibling checkouts next to this repo:

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

## Evidence

| Date | Check | Result |
| --- | --- | --- |
| 2026-06-12 | V-2 live conformance against `neuriplo-kserve-runtime@develop` after runtime PR #9 | HTTP + gRPC PASS |
| 2026-06-12 | V-1, V-3 in CI | Pass (track closed in "docs(client): close conformance track after runtime PR #9") |
