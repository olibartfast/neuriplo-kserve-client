# Feature Validation — <feature name>

> Copy to `specs/YYYY-MM-DD-<feature-name>/validation.md`. Write this *before*
> implementation, so the criteria cannot be fitted to whatever gets built.
> Execute it from this file — do not summarize it from memory.

## Automated

- [ ] Default matrix configures, builds, and passes:
      `cmake -B build -DKSERVE_CLIENT_BUILD_TESTS=ON && cmake --build build -j && ctest --test-dir build --output-on-failure`
- [ ] Focused tests cover the success path *and* the failure path this feature adds.
- [ ] Affected proto-profile variants still pass — list the ones that apply
      (`OIP`, `OIP_REPOSITORY`, `-DKSERVE_CLIENT_ENABLE_GRPC=OFF`).
- [ ] Conformance dry-run passes: `bash scripts/runtime_conformance.sh --dry-run`.
- [ ] Conformance live passes if the change touches wire or transport behavior
      and a sibling `neuriplo-kserve-runtime` checkout is available
      (`bash scripts/runtime_conformance.sh --live`).

## Manual

- [ ] The behavior matches `requirements.md` — record the exact invocation and
      the observed output.
- [ ] HTTP and gRPC agree on the same logical operation where both apply.
- [ ] Invalid and edge-case inputs fail fast with a stable, actionable error.
- [ ] Every hyperlink added to docs resolves (`ls` for relative, `curl -sI` for absolute).

## Definition of Done

- [ ] Every requirement is implemented or explicitly deferred in `requirements.md`.
- [ ] Nothing in *Out of Scope* was implemented anyway.
- [ ] Deviations from this file are recorded here, honestly, with what was run instead.
- [ ] `CHANGELOG.md` describes the change; `../roadmap.md` phase status is updated.
- [ ] PR states cross-repo impact (`none` / `needs runtime first` /
      `needs infer adapter`) and which oracles ran; links the KServe spec
      section when wire behavior changed.
- [ ] Spec, code, changelog, and roadmap tell the same story in one branch.
