# Neuriplo KServe Client Roadmap

> Status: living brownfield roadmap, reviewed on 2026-10-04 against the current
> tree and history. `develop` is 4 commits ahead of `master` (released as
> v0.4.0); that work is unreleased and is marked as such below. Phases after the
> completed ones are a working sequence to confirm with the maintainer.

This roadmap is scoped to the [neuriplo-kserve-client](https://github.com/olibartfast/neuriplo-kserve-client)
library. Cross-repo work lives in
[neuriplo-platform](https://github.com/olibartfast/neuriplo-platform) specs;
this file links to those packets when a phase is part of one. The sibling
[neuriplo-kserve-runtime](https://github.com/olibartfast/neuriplo-kserve-runtime)
is the conformance oracle.

The former planning status file is kept as the historical implementation record
at [history/next-steps.md](history/next-steps.md), with [CHANGELOG.md](../CHANGELOG.md).
This file is the planning entry point; its open items were merged here and
completed history is summarized below, not copied.

## Status Key

- **Complete** - the intended capability has landed; maintenance may remain.
- **Complete (unreleased)** - landed on `develop`, not yet in a tagged release.
- **Next** - the next phase to specify and implement.
- **Specified** - a dated feature packet exists and validation is defined;
  implementation has not started.
- **Planned** - ordered but not yet specified as a feature packet.
- **Blocked** - cannot proceed without an identified decision or dependency.

## Roadmap Principles

- Preserve `kserve::IClient` and the raw-byte payload contract unless a packet
  approves and validates a migration.
- Wire-contract work lands in the runtime first, or is additive and optional here.
- Keep HTTP and gRPC behavior in parity for the same logical operation.
- Never add a neuriplo, task, or backend dependency.
- Validate one observable slice against the live runtime before expanding scope.

## Phase 0 - Standalone Client Foundation

**Status: Complete** (v0.1.0 to v0.2.0)

Extracted from neuriplo-infer: HTTP and gRPC transports, metadata, inference,
health probes, Model Repository extension, binary tensor data and raw contents,
retry with backoff and jitter, keep-alive, TLS/mTLS, selectable proto profiles
(`OIP`, `OIP_REPOSITORY`), the CI matrix, and the GoogleTest suite.
See [CHANGELOG.md](../CHANGELOG.md) 0.1.0 and 0.2.0.

## Phase 1 - Runtime Conformance Track

**Status: Complete through live gRPC** (v0.3.0); open items carried to Phase 4

`scripts/runtime_conformance.sh` (dry-run and live), the `runtime_conformance_dry_run`
CTest in CI, and the `kserve-client-conformance` oracle binary exercising the real
`KserveGrpcClient`. Live HTTP and gRPC revalidated 2026-06-12 against
`neuriplo-kserve-runtime@develop`. Detail: [history/next-steps.md](history/next-steps.md).

## Phase 2 - Backend Attribution and Ensemble Coverage

**Status: Complete (unreleased)** - `ModelMetadata.platform` shipped in v0.4.0;
ensemble coverage is on `develop` only.

- v0.4.0: `ModelMetadata.platform` parsed by HTTP and gRPC clients.
- Unreleased on `develop` (no API change): unit tests for variable-length
  `UINT8` encoded-image inputs and framed binary decoded-envelope bodies, a
  `--ensemble-model` leg in `kserve-client-conformance`, and a README ensemble
  section. Envelope decoding deliberately stays out of this library.

## Phase 3 - v0.5.0 Release Cut (name provisional)

**Status: Next** - waiting on the human to decide the version and cut it.

Goal: ship the unreleased Phase 2 work from `develop` to `master` per GitFlow,
with the changelog moved out of `[Unreleased]`, tag on the merge commit, and
`develop` not lagging `master`. The runtime-side counterpart is the platform
packet [2026-10-03-kserve-dynamic-dim-encoded-image](https://github.com/olibartfast/neuriplo-platform/tree/main/specs/2026-10-03-kserve-dynamic-dim-encoded-image),
which currently drives the runtime's v0.4.0 release. This client is not part
of that packet: the packet established on 2026-10-03 that the client already
sends a concrete extent, so it needs no change (resolved, was A-10). This
release is independent of it.

Exit criteria: tag exists, CHANGELOG sections dated, README FetchContent tag
refreshed (A-7), infer pin bump handed to the human.

## Phase 4 - Conformance Backlog

**Status: Planned**

Scope from [history/next-steps.md](history/next-steps.md) (Step 2, Step 3, Backlog 1-2):

- Repository extension conformance (index, load, unload) when model control is on.
- Strict `OIP` profile live test against OVMS or a minimal OIP server.
- FP16/BF16 raw-contents case if the runtime exposes those outputs.
- Optional scheduled or `workflow_dispatch` live-conformance job (human sign-off).

Exit criteria: each item has a recorded live run or a stated reason it is deferred.

## Phase 5 - Error Mapping Audit

**Status: Planned**

HTTP status and gRPC codes to stable `std::runtime_error` messages per spec,
with tests, without changing `IClient`. Likely needs a packet because error
text is observable behavior.

## Phase 6 - Consumer Contract Test

**Status: Planned**

A compile-time or header smoke test that mimics neuriplo-infer's FetchContent
embedding, so `IClient` breakage is caught here and not in the consumer.

## Specification Rule

Create a dated `specs/YYYY-MM-DD-feature-name/` packet for active work that is
multi-phase, changes public behavior or architecture (`IClient`, wire format,
proto profiles, error semantics), or has low reversibility. The packet contains
`requirements.md`, `plan.md`, and `validation.md`, with validation defined
before implementation and evidence recorded after execution. Small contained
fixes may use a concise PR-level specification; trivial fixes need no packet.
Do not create speculative packets for inactive roadmap items or backfill
packets for completed work. Work spanning repositories is specified in
neuriplo-platform, not here.

## Assumptions to Confirm

- A-11: Phases 4 to 6 order is a reconstruction of the backlog ordering in
  [history/next-steps.md](history/next-steps.md), and the Codex "standby" status there may be outdated.

_Revision: 2026-10-04 - initial brownfield adoption._
