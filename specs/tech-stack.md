# Neuriplo KServe Client Technical Stack

> Status: working brownfield constitution, reconstructed on 2026-10-04. The
> implementation and executable checks take precedence if this file drifts;
> update both in the same branch when a durable technical decision changes.

## Core Stack

| Area | Current choice | Boundary |
| --- | --- | --- |
| Library | C++20 static library `neuriplo-kserve-client`, alias `neuriplo::kserve-client` | `CMAKE_CXX_STANDARD 20` is required. Do not change the standard without a compatibility decision; consumers embed this by FetchContent. |
| Build | CMake 3.20 or newer | `CMakeLists.txt` and `cmake/CompileSpeed.cmake` (ccache when available). No `CMakePresets.json`; configure with plain `-D` options. |
| Transports | Hand-rolled socket HTTP client; optional gRPC | gRPC needs Protobuf and gRPC found by `find_package`; absent, the build degrades to HTTP only with a warning. |
| TLS | Optional OpenSSL | HTTPS and `grpcs://`; compiled out when OpenSSL is absent, with a runtime error for `https://` endpoints. |
| JSON | nlohmann/json 3.12.0 | Reuses the consumer's `nlohmann_json::nlohmann_json` target when present, then `find_package`, then FetchContent from the pinned release URL. |
| Tests | GoogleTest 1.15.2 (FetchContent) and CTest | Enabled by `KSERVE_CLIENT_BUILD_TESTS` (default ON at top level). |
| CI | GitHub Actions `.github/workflows/ci.yml` | Matrix: gRPC ON/OFF x proto profile `OIP_REPOSITORY`/`OIP`, plus a `conformance` label dry run. |

Public headers live in `include/`: `KserveTypes.hpp` (neutral types and the
`kserve::IClient` contract), `KserveProtocol.hpp`, `KserveHttpClient.hpp`,
`KserveGrpcClient.hpp`, `KserveRetry.hpp`. Implementations are in `src/`.

## Dependencies and Pinning

- There is no `versions.env` and no `docs/` tree here. Dependency versions are
  the `URL` pins in `CMakeLists.txt` (nlohmann/json) and `test/CMakeLists.txt`
  (GoogleTest); gRPC, Protobuf, and OpenSSL come from the system (CI uses
  Ubuntu packages).
- The library version is the `VERSION` file, read by `project()`; history is in
  `CHANGELOG.md` (Keep a Changelog, SemVer).
- Consumers pin this repo by git tag through FetchContent. Updating that pin in
  neuriplo-infer is human-owned, and the ecosystem version matrix is kept in
  [neuriplo-platform](https://github.com/olibartfast/neuriplo-platform).
- No new runtime or inference dependency without explicit approval.

## Architectural Boundaries

- `kserve::IClient` is the single transport-neutral contract. HTTP and gRPC
  implement it with the same logical behavior.
- Tensor payloads are raw little-endian bytes in `InferInput` / `InferOutput`.
  gRPC raw contents are the default; `KSERVE_BINARY=0` selects typed `contents`
  (legacy compatibility). The HTTP binary extension is opt-in via
  `KSERVE_BINARY=1`.
- `proto/` selects the gRPC profile: `OIP` (strict KServe/OVMS) or
  `OIP_REPOSITORY` (default; Triton-compatible Model Repository RPCs), with
  `KSERVE_CLIENT_PROTO_FILE` as a path override. See
  [proto/README.md](../proto/README.md). Review all proto edits.
- The target exports PUBLIC `KSERVE_CLIENT_WITH_GRPC` and, when the repository
  RPCs are present, `KSERVE_CLIENT_PROTO_REPOSITORY`, so consumers can `#ifdef`.
- The library never depends on neuriplo, neuriplo-tasks, or any backend. Result
  interpretation (including ensemble envelopes) belongs to the consumer.
- Wire-contract changes follow cross-repo order: runtime first (or additive and
  optional on the client), then this repo, then the neuriplo-infer adapter.

## Configuration Surface

CMake options: `KSERVE_CLIENT_ENABLE_GRPC`, `KSERVE_CLIENT_ENABLE_TLS`,
`KSERVE_CLIENT_BUILD_TESTS`, `KSERVE_CLIENT_PROTO_PROFILE`,
`KSERVE_CLIENT_PROTO_FILE`. Runtime environment: `KSERVE_BEARER_TOKEN[_FILE]`,
`KSERVE_BINARY`, `KSERVE_CA_CERT`, `KSERVE_CLIENT_CERT`, `KSERVE_CLIENT_KEY`,
and the `KSERVE_MAX_RETRIES` / `KSERVE_RETRY_*` retry knobs. `README.md` is the
user-facing reference; keep it and this file consistent.

## Explicit Non-Choices

- No dependency on neuriplo, neuriplo-tasks, or any inference backend.
- No breaking `IClient` or wire-format change without human review.
- No third-party gRPC test tooling: live gRPC is exercised through the
  `kserve-client-conformance` binary (the real client code path), not `grpcurl`.
- No ensemble-specific API; an ensemble is an ordinary model on the wire.
- No release or version bump without human coordination.
- Live end-to-end conformance is not part of CI (see Open Technical Decisions).

## Validation Entrypoints

Default loop (matches the CI matrix axes; also in `AGENTS.md`):

```bash
cmake -B build -DKSERVE_CLIENT_BUILD_TESTS=ON
cmake --build build -j
ctest --test-dir build --output-on-failure
```

CI profile variants:

```bash
cmake -B build-oip -DKSERVE_CLIENT_BUILD_TESTS=ON -DKSERVE_CLIENT_PROTO_PROFILE=OIP
cmake --build build-oip -j && ctest --test-dir build-oip --output-on-failure

cmake -B build-http -DKSERVE_CLIENT_BUILD_TESTS=ON -DKSERVE_CLIENT_ENABLE_GRPC=OFF
cmake --build build-http -j && ctest --test-dir build-http --output-on-failure
```

Conformance:

```bash
ctest --test-dir build --output-on-failure -L conformance   # CI dry run
bash scripts/runtime_conformance.sh --dry-run
bash scripts/runtime_conformance.sh --live                  # needs sibling runtime build
```

Wire-contract PRs run `ctest` plus `runtime_conformance.sh` (live when the
sibling `neuriplo-kserve-runtime` checkout is available). The live ensemble leg
is `test/kserve-client-conformance --grpc-endpoint <ep> --ensemble-model <name>`.

## Open Technical Decisions

- Whether to add a scheduled or `workflow_dispatch` live-conformance CI job
  (`plan/NEXT_STEPS.md` Step 3, needs a runtime artifact and human sign-off).
- Whether to add FP16/BF16 raw-contents coverage once the runtime exposes them.
- Error-mapping policy for HTTP status and gRPC codes to stable messages.
- Whether a consumer-embedding smoke test replaces reliance on infer's CI.

## Assumptions to Confirm

- A-6: AGENTS.md names `feat/*` branches while the other C++ siblings use
  `feature/*`; the ecosystem convention is assumed to be `feature/*` and the
  AGENTS.md wording stale.
- A-7: The README FetchContent snippet pins `v0.2.0`; it is assumed stale
  rather than a deliberate minimum version.
- A-8: GoogleTest and nlohmann/json pins are assumed deliberate and not
  coordinated through a platform-level version matrix.
- A-9: The `.cursor/rules/*.mdc` files are assumed to mirror the MANDATORY
  sections of `AGENTS.md`; `AGENTS.md` wins on conflict.

_Revision: 2026-10-04 - initial brownfield adoption._
