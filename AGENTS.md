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

Out of scope (owned elsewhere — open a follow-up in the owner repo instead):

| Concern | Owner repo |
|---------|------------|
| `TensorElement` / task tensors | `neuriplo-infer` (`KserveEngine`) |
| Backend execution, plugins | `neuriplo` |
| KServe HTTP server, scheduling | `neuriplo-kserve-runtime` |
| Task preprocessing/postprocessing | `neuriplo-tasks` |
| Release pins in consumers | Human (`neuriplo-infer/versions.env`) |

Rules:

1. **No neuriplo dependency** — do not link or include neuriplo headers.
2. **Raw bytes on the wire** — `InferInput` / `InferOutput` carry little-endian
   payloads; typed conversion belongs in consumer adapters.
3. **`IClient` changes are breaking** — treat signature or semantic changes as
   breaking; note downstream impact for `neuriplo-infer`.
4. **Proto edits need review** — document which profile (`OIP` /
   `OIP_REPOSITORY`) is affected; they change gRPC stubs and server compatibility.
5. **Do not edit sibling repos from this task** unless the user explicitly asks
   for a coordinated cross-repo change.

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

For release-gating changes, also run the platform e2e
(`neuriplo-platform/integration-tests/`).

## MANDATORY: GitFlow workflow

Follow [Atlassian GitFlow](https://www.atlassian.com/git/tutorials/comparing-workflows/gitflow-workflow).
In this repo GitFlow **`main`** is **`master`**; **`develop`** is the integration branch.

Branch roles:

| Branch | Parent | Purpose |
|--------|--------|---------|
| `master` | — | Official release history; tag releases here |
| `develop` | — | Integration branch for upcoming release |
| `feat/*` | `develop` | New work; PR back to `develop`. Never target `master` for feature work |
| `release/*` | `develop` | Release prep (bug fixes, docs only) |
| `hotfix/*` | `master` only | Production patches; merge to `master` and `develop` |

Agent workflow:

- **Feature:** `git checkout develop && git pull && git checkout -b feat/<short-name>`;
  commit on the branch; PR targeting `develop`; delete the branch after merge.
- **Release:** branch `release/<version>` from `develop`; update `CHANGELOG.md`
  (fixes/docs only, no new features); merge into `master`, tag `vX.Y.Z`; merge
  back into `develop`; delete the branch locally and on `origin` (checklist below).
- **Hotfix:** branch `hotfix/<short-name>` from `master`; fix; merge into both
  `master` and `develop`; tag on `master`; delete the branch locally and on `origin`.

Rules:

- Before starting work, confirm the current branch matches the task type
  (feature → `develop`, hotfix → `master`).
- Do not commit feature work on `develop` or `master`; use a `feat/*` branch.
- Prefer PRs for merges into `develop`, `master`, and `release/*`.
- Do not push directly to `develop` or `master` unless the user explicitly asks.
- After every `master` release, `develop` must not lag behind `master` (`git rev-list
  --left-right --count origin/develop...origin/master` → `0 0`).

Release/hotfix branch cleanup (after merge to `master`, tag, and back-merge to
`develop`): the **`master` tag** (`vX.Y.Z`) is the immutable release ref — do not
leave finished branches on the remote.

```bash
git checkout develop
git branch -d release/0.7.0
git push origin --delete release/0.7.0
```

Agent checklist when a release is complete:

1. Merged `release/X.Y.Z` → `master`; tagged `vX.Y.Z`; pushed `master` and tag.
2. Merged `release/X.Y.Z` → `develop` (bump dev `VERSION` if the repo does that);
   pushed `develop`.
3. Deleted the branch locally and on `origin`.
4. Confirmed `git branch -a | grep release` shows no finished release branch.

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
`specs/roadmap.md` (Phase 1 and 4) for the conformance track.

External oracles when validating wire behavior:

| Check | When required |
|-------|----------------|
| `ctest --test-dir build` | Every C++ change |
| `scripts/runtime_conformance.sh` | Wire or transport changes |
| `neuriplo-infer/.../kserve_integration.sh --dry-run` | If infer harness commands may break |

| Harness | Location | What it checks |
|---------|----------|----------------|
| Runtime conformance | `scripts/runtime_conformance.sh` (this repo) | Client ↔ `neuriplo-kserve-runtime` HTTP/gRPC |
| Infer integration dry-run | `neuriplo-infer/app/test/kserve_integration.sh --dry-run` | Command construction for Triton/OVMS/runtime |

For wire-contract PRs, run at minimum: `ctest` here + `runtime_conformance.sh`
when the sibling runtime checkout is available.

## MANDATORY: Agent guide maintenance

**Keep this file current.** When your task changes build commands, CI, module
boundaries, cross-repo rules, or `specs/roadmap.md`, update the matching
`AGENTS.md` section in the same PR — do not wait for the user to ask.

Triggers — update `AGENTS.md` when you:

1. Materially edit `specs/roadmap.md` or add a new spec packet
2. Change build/test commands, CMake options, or CI validation steps
3. Add new mandatory agent workflow rules
4. Change repo layout, module boundaries, or the `IClient` public surface
5. Add or move conformance/integration harnesses (`scripts/`, `test/`)

What to sync:

- Commands and conventions in the sections your change affects
- Pointers to new scripts, tests, or cross-repo validation paths
- One-line status in "Current work track" if `specs/roadmap.md` moved forward

Do not:

- Duplicate full `specs/roadmap.md` content inside `AGENTS.md`
- Skip the update because the user did not mention `AGENTS.md`
- Add neuriplo-infer or runtime implementation detail — link instead

Quick check before finishing: if you touched `specs/`, `CMakeLists.txt`,
`scripts/`, or `.github/workflows/`, re-read `AGENTS.md` and fix stale references.

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

When editing documentation (`README.md`, `specs/**/*.md`) with hyperlinks:
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

See `specs/roadmap.md` for status and the active task queue. New work gets a dated packet under `specs/`.

## Specs (constitution and planning)

`specs/` holds the project constitution (`mission.md`, `tech-stack.md`,
`roadmap.md`) and is the planning entry point; git history and `CHANGELOG.md`
are the historical record.
Read `specs/roadmap.md` first. Per its Specification Rule, multi-phase, public
behavior or architecture, or low-reversibility work needs a dated packet in
`specs/YYYY-MM-DD-feature-name/` before implementation. Cross-repo work is
specified in neuriplo-platform. Conventions: `specs/README.md`.
