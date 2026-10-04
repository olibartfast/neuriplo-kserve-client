# Neuriplo KServe Client Mission

> Status: working brownfield constitution, reconstructed on 2026-10-04 from
> the current code, documentation, repository metadata, and recent history.
> Product assumptions that still need maintainer confirmation are listed below
> as `A-n`.

## Why This Library Exists

`neuriplo-kserve-client` is a standalone C++ client for the KServe V2 / Open
Inference Protocol (OIP), the wire protocol exposed by KServe, Triton Inference
Server, and OpenVINO Model Server. It lets an application talk to a remote
model server over HTTP or gRPC without linking any inference backend, and
without that application re-implementing the protocol, retry, and TLS handling.

It is a **pure protocol client**: tensor payloads are raw little-endian bytes,
and the library depends only on the wire spec and transport libraries. It was
extracted from [neuriplo-infer](https://github.com/olibartfast/neuriplo-infer)
(v0.1.0) so the protocol layer can be versioned, tested, and reused on its own.

## Who It Serves

- The `KserveEngine` adapter in neuriplo-infer, which consumes this library via
  FetchContent and converts bytes to and from typed tensors.
- Other C++ applications that need an OIP client with the same wire behavior
  as Triton's client library.
- Maintainers of
  [neuriplo-kserve-runtime](https://github.com/olibartfast/neuriplo-kserve-runtime),
  the serving runtime, which is the conformance oracle for this client.

## Position in the Ecosystem

| Repository | Relationship |
| --- | --- |
| [neuriplo](https://github.com/olibartfast/neuriplo) | No dependency. Backend abstraction; never linked here. |
| [neuriplo-tasks](https://github.com/olibartfast/neuriplo-tasks) | No dependency. Task pre/postprocessing belongs there. |
| neuriplo-kserve-runtime | Server counterpart; wire changes land there first. |
| neuriplo-infer | Consumer; owns the `KserveEngine` adapter and CLI. |
| [neuriplo-platform](https://github.com/olibartfast/neuriplo-platform) | Owns cross-repo contracts, the version matrix, and cross-repo spec packets. |

## Product Promise

The library provides:

- one `kserve::IClient` contract with HTTP and optional gRPC implementations;
- model metadata (including `platform`), inference, health probes, and the
  Model Repository extension (index, load, unload);
- the HTTP Binary Tensor Data Extension and gRPC `raw_input/output_contents`
  for efficient large tensors;
- retry with exponential backoff and jitter, HTTP keep-alive, and TLS/mTLS with
  secrets sourced from the environment or the files they name;
- gRPC proto profiles (`OIP`, `OIP_REPOSITORY`) so one client works against
  strict OIP servers and Triton-compatible ones;
- transparent use of server-side ensembles as ordinary models, with no
  ensemble-specific API.

## Success Criteria

- A consumer upgrades within a compatible release without source changes to
  `IClient` call sites.
- HTTP and gRPC return the same logical result for the same operation.
- Wire behavior is proven against `neuriplo-kserve-runtime@develop`, not only
  against the library's own unit tests.
- Transient failures are retried within bounded policy; permanent failures
  surface as actionable errors.
- The library builds with gRPC, TLS, or both compiled out, with a clear
  runtime error for any compiled-out capability that is requested.
- A new maintainer can find the boundaries and run the required checks from
  version-controlled files.

## Boundaries and Non-Goals

- No neuriplo types, no backend dependency, no task pre/postprocessing, no CLI
  flags, no visualization. Interpreting an ensemble's decoded envelope is the
  adapter's job, not this library's.
- Does not edit neuriplo-infer's `KserveEngine`; an `IClient` change that needs
  an adapter update is a separate PR there.
- Does not own server behavior, scheduling, batching, or admission logic.
- Does not claim support for protocol extensions it has not been validated
  against a live server.
- New runtime dependencies, breaking `IClient` or wire-format changes, and large
  cross-module refactors require explicit human review.
- Release tags and sibling pin bumps are human-owned.

## Engineering Values and Tone

KServe V2 / OIP spec correctness is the authority. Backward compatibility of
`IClient` and the raw-byte payload contract, HTTP/gRPC parity, and retry/TLS/auth
edge cases take priority over surface-level uniformity. Documentation should be
direct and operational: exact commands, known exclusions, and failure
behavior, without implying coverage that has not been exercised live.

## Assumptions to Confirm

- A-1: The library remains a static, protocol-only library embedded by
  FetchContent; no installed package or shared library is planned. (Basis: only
  `add_library(... STATIC)` and no install rules exist.)
- A-2: The ABI is not a promise; only source compatibility of `IClient` is.
  (Basis: static library, no export or versioning machinery.)
- A-3: The "Primary agent: Codex" ownership split in `AGENTS.md` is current,
  though [2026-06-12-runtime-conformance](2026-06-12-runtime-conformance/plan.md) marks the Codex track as on standby.
- A-4: Servers in scope are the runtime, Triton, OVMS, and KServe; no other
  servers need compatibility claims. (Basis: README and CHANGELOG wording.)
- A-5: Quantitative latency, throughput, or footprint targets do not exist and
  none are wanted yet; feature packets state their own.

_Revision: 2026-10-04 - initial brownfield adoption._
