# Agent Instructions — neuriplo-kserve-client

**Primary agent:** Codex (OpenAI). This repo is the Codex-owned spoke in the
neuriplo ensemble. Cursor owns `neuriplo-kserve-runtime` and `neuriplo-infer`;
Claude Code owns `neuriplo` (hub — registry, plugin ABI, CI matrix). Merges,
release tags, and sibling pin bumps stay with the human across all repos.

## System overview

Standalone C++ library for the **KServe V2 / Open Inference Protocol** (OIP). It
is a **pure protocol client**: tensor payloads are raw little-endian bytes; the
library depends only on the wire spec and transport libraries — never on
`neuriplo`, `neuriplo-tasks`, or any inference backend.

| Layer | Repo | Role |
|-------|------|------|
| Protocol client | **this repo** | HTTP/gRPC encode-decode, retry, TLS, `IClient` |
| Server | `neuriplo-kserve-runtime` | KServe V2 serving runtime (test oracle) |
| Adapter | `neuriplo-infer` | `KserveEngine` — bytes ↔ `TensorElement` |

## MANDATORY: Protocol layer boundary

**Stay inside this repo's contract.** Do not add neuriplo types, task
preprocessing/postprocessing, CLI flags, or visualization. Do not edit
`neuriplo-infer/app/src/KserveEngine.cpp` from here — open a separate PR in
infer when an `IClient` API change requires adapter updates.

Owned surfaces (see `repo-meta`):

- `include/` — `kserve::IClient`, transports, protocol helpers
- `src/` — HTTP/gRPC clients, codec, retry
- `proto/` — gRPC service definitions (review all proto edits)
- `test/` — unit tests (GoogleTest)

Forbidden without explicit human review:

- New runtime/inference dependencies
- Breaking `IClient` or wire-format changes
- Large cross-module refactors

## MANDATORY: Cross-repo sequencing

Anything touching the **wire contract** must land in
`neuriplo-kserve-runtime` first (or be purely additive/optional on the client).
The runtime is the conformance oracle; client work validates against
`neuriplo-kserve-runtime@develop`.

Dependency order for contract changes:

1. Runtime server behavior (if the spec gap is server-side)
2. Client encode/decode + unit tests (this repo)
3. `neuriplo-infer` adapter + integration harness (Cursor repo)

When in doubt: runtime merge first, then client PR → `develop`.

## MANDATORY: GitFlow workflow

Follow [Atlassian GitFlow](https://www.atlassian.com/git/tutorials/comparing-workflows/gitflow-workflow).
See `.cursor/rules/gitflow-workflow.mdc`. In this repo GitFlow **`main`** is
**`master`**; **`develop`** is the integration branch.

- **`feat/*`** — branch from `develop`; PR back to `develop`. Never target
  `master` for feature work.
- **`release/*`** — release prep on `develop`; merge to `master`, tag, back-merge
  to `develop`, then delete locally and on `origin`.
- **`hotfix/*`** — branch from `master`; merge to `master` and `develop`, then
  delete locally and on `origin`.
- Do not push directly to `develop` or `master` unless the user explicitly asks.
- After every `master` release, `develop` must not lag behind `master` (`git rev-list
  --left-right --count origin/develop...origin/master` → `0 0`).

Release tags and `versions.env` pin updates in `neuriplo-infer` are human-owned.
Codex opens PRs; the human cuts releases.

## Build, test, and development commands

Default local loop (matches CI matrix axes):

```bash
cmake -B build -DKSERVE_CLIENT_BUILD_TESTS=ON
cmake --build build -j
ctest --test-dir build --output-on-failure
```

Proto profile variants (CI exercises both):

```bash
cmake -B build-oip -DKSERVE_CLIENT_BUILD_TESTS=ON -DKSERVE_CLIENT_PROTO_PROFILE=OIP
cmake --build build-oip -j && ctest --test-dir build-oip --output-on-failure

cmake -B build-http -DKSERVE_CLIENT_BUILD_TESTS=ON -DKSERVE_CLIENT_ENABLE_GRPC=OFF
cmake --build build-http -j && ctest --test-dir build-http --output-on-failure
```

CMake options: see `README.md` (`KSERVE_CLIENT_ENABLE_GRPC`, `KSERVE_CLIENT_ENABLE_TLS`,
`KSERVE_CLIENT_PROTO_PROFILE`).

## Validation beyond unit tests

Repo-local CI (`.github/workflows/ci.yml`) runs the unit suite only. End-to-end
round-trips against a live server are **not** in this repo's CI yet — see
`plan/NEXT_STEPS.md` for the conformance track.

External oracles when validating wire behavior:

| Harness | Location | What it checks |
|---------|----------|----------------|
| Runtime conformance | `scripts/runtime_conformance.sh` (this repo) | Client ↔ `neuriplo-kserve-runtime` HTTP/gRPC |
| Infer integration dry-run | `neuriplo-infer/app/test/kserve_integration.sh --dry-run` | Command construction for Triton/OVMS/runtime |

For wire-contract PRs, run at minimum: `ctest` here + `runtime_conformance.sh`
when the sibling runtime checkout is available.

## MANDATORY: Agent guide maintenance

**Keep this file current.** When your task changes build commands, CI, module
boundaries, cross-repo rules, or `plan/NEXT_STEPS.md`, update the matching
`AGENTS.md` section in the same PR. See `.cursor/rules/agents-md-maintenance.mdc`.

## Review focus

- KServe V2 / OIP spec correctness (public spec is the authority)
- Backward compatibility of `IClient` and raw-byte payload contract
- gRPC vs HTTP parity for the same logical operation
- Retry, TLS, and auth edge cases
- Proto profile selection (`OIP` vs `OIP_REPOSITORY`)
- Missing unit or conformance coverage

Avoid:

- Pulling in neuriplo or task-layer abstractions
- Breaking consumers (`neuriplo-infer` links this via FetchContent)
- Release/version bumps without human coordination

## Hyperlink verification

When editing documentation (`README.md`, `plan/*.md`) with hyperlinks:
- Verify all relative links resolve to existing files in the repo.
- Verify absolute GitHub URLs are reachable.
- Prefer absolute GitHub blob/tree URLs over fragile cross-repo relative paths.

## Coding conventions

- C++20, 4-space indent
- Headers in `include/`, implementation in `src/`
- `PascalCase` types, `camelCase` functions (match existing files)
- Tensor payloads: raw little-endian bytes in `InferInput` / `InferOutput`
- gRPC raw contents are the default; `KSERVE_BINARY=0` selects typed `contents`

## Commit and PR guidelines

Short imperative subjects (e.g. `Add runtime gRPC conformance dry-run`). PRs
target `develop` and should list:

- `ctest` matrix configs run
- Whether runtime conformance was exercised (live or dry-run)
- Cross-repo impact (`none` / `needs infer adapter` / `needs runtime first`)
- Link to KServe spec section when changing wire behavior

## Current work track

See `plan/NEXT_STEPS.md` for status and the active task queue.
