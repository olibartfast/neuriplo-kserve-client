# neuriplo-kserve-client

A standalone C++ client for the **KServe V2 / Open Inference Protocol** (OIP),
the wire protocol exposed by [KServe](https://kserve.github.io/website/),
[Triton Inference Server](https://github.com/triton-inference-server/server),
[OpenVINO Model Server](https://github.com/openvinotoolkit/model_server).

It is a **pure protocol client**: tensor payloads are carried as raw
little-endian bytes (like Triton's client library), so the library depends only
on the wire protocol and standard transport libraries — never on any inference
backend. Converting bytes to an application's typed tensors is the job of an
adapter layer in the consumer (e.g. `KserveEngine` in
[neuriplo-infer](https://github.com/olibartfast/neuriplo-infer)).

## Features

- **HTTP** transport (hand-rolled socket client) and optional **gRPC** transport
  over the standard `inference.GRPCInferenceService`.
- Model **metadata**, **inference**, and health probes (`serverLive` /
  `serverReady` / `modelReady`).
- **Model Repository extension**: `repositoryIndex()` / `loadModel(name)` /
  `unloadModel(name)` (HTTP `POST /v2/repository/...`, gRPC `RepositoryIndex` /
  `RepositoryModelLoad` / `RepositoryModelUnload`).
- KServe **Binary Tensor Data Extension** (HTTP) and `raw_input/output_contents`
  (gRPC) for efficient large-tensor transfer.
- **Server-side ensembles**: talks to an ensemble as an ordinary model. No new
  API -- see below.
- **Retry with exponential backoff + jitter** on transient HTTP/gRPC failures.
- HTTP **keep-alive** connection reuse.
- **TLS**: HTTPS for the HTTP client (OpenSSL) and `grpcs://` for gRPC, with
  optional **mTLS**; secrets sourced from env vars or the files they name.

## Layout

| Path | Contents |
|------|----------|
| `include/KserveTypes.hpp` | Neutral types + the `kserve::IClient` contract |
| `include/KserveProtocol.hpp` | Pure helpers: URL/HTTP parsing, tensor encode/decode, path builders |
| `include/KserveHttpClient.hpp` / `KserveGrpcClient.hpp` | Transport clients |
| `include/KserveRetry.hpp` | Retry policy + backoff schedule |
| `proto/` | gRPC proto profiles — see [proto/README.md](proto/README.md) |
| `proto/kserve_grpc.proto` | Default profile: OIP + Model Repository (Triton-compatible) |
| `proto/oip/grpc_predict_v2.proto` | Strict KServe OIP (OVMS, base KServe) |

## Build

```bash
cmake -B build -DKSERVE_CLIENT_BUILD_TESTS=ON
cmake --build build -j
ctest --test-dir build --output-on-failure
```

Options:

| Option | Default | Effect |
|--------|---------|--------|
| `KSERVE_CLIENT_ENABLE_GRPC` | `ON` | Build the gRPC transport (needs Protobuf + gRPC) |
| `KSERVE_CLIENT_ENABLE_TLS` | `ON` | HTTPS for the HTTP client (needs OpenSSL) |
| `KSERVE_CLIENT_BUILD_TESTS` | top-level only | Build the GoogleTest suite |
| `KSERVE_CLIENT_PROTO_PROFILE` | `OIP_REPOSITORY` | `OIP` (strict KServe/OVMS) or `OIP_REPOSITORY` (Triton + repo) |
| `KSERVE_CLIENT_PROTO_FILE` | *(unset)* | Override: path to a custom `.proto` |

```bash
# Strict OIP for KServe / OVMS (no repository gRPC stubs)
cmake -B build -DKSERVE_CLIENT_PROTO_PROFILE=OIP
```

When Protobuf/gRPC or OpenSSL are absent the build still succeeds with the
corresponding capability compiled out (`https://` / `grpcs://` endpoints then
fail fast with a clear error).

## Use as a dependency (CMake FetchContent)

```cmake
include(FetchContent)
FetchContent_Declare(neuriplo-kserve-client
    GIT_REPOSITORY https://github.com/olibartfast/neuriplo-kserve-client.git
    GIT_TAG        v0.2.0)
FetchContent_MakeAvailable(neuriplo-kserve-client)
target_link_libraries(your_target PRIVATE neuriplo::kserve-client)
```

The target exports a PUBLIC `KSERVE_CLIENT_WITH_GRPC` define when the gRPC
transport is compiled in, and `KSERVE_CLIENT_PROTO_REPOSITORY` when the proto
includes the Model Repository gRPC extension, so consumers can `#ifdef` on both.

## Runtime configuration (environment)

| Variable | Purpose |
|----------|---------|
| `KSERVE_BEARER_TOKEN` / `KSERVE_BEARER_TOKEN_FILE` | Bearer auth (value or file) |
| `KSERVE_BINARY` | Opt into the HTTP binary tensor extension (gRPC raw contents are default) |
| `KSERVE_CA_CERT` / `KSERVE_CLIENT_CERT` / `KSERVE_CLIENT_KEY` | TLS / mTLS material |
| `KSERVE_MAX_RETRIES` / `KSERVE_RETRY_BASE_MS` / `KSERVE_RETRY_MAX_MS` / `KSERVE_RETRY_JITTER` | Retry policy |

## Server-side ensembles

An ensemble moves preprocessing (and optionally postprocessing) onto the server:
the client sends an encoded image instead of a dense float tensor, and may get
decoded results back instead of raw model outputs. Both
`neuriplo-kserve-runtime` and NVIDIA Triton serve them behind the same contract.

This needs no new client API, and deliberately so. An ensemble is an ordinary
KServe V2 model on the wire:

- `ModelMetadata::platform` reports `"ensemble"`.
- The input is one `UINT8` tensor named `IMAGE`, of shape `[1, N]` where `N` is
  the encoded byte length. Pass the file bytes as an `InferInput` like any other
  raw payload.
- Outputs are either the inner model's tensors unchanged (a passthrough
  ensemble) or a fixed decoded envelope in `INT32` / `FP32` / `INT64` / `UINT8`.

A passthrough ensemble's own metadata only describes an encoded image, which
tells a task layer nothing about tensor layout. Callers that need the inner
model's shapes construct a second client for it -- clients are per-model, so
that is just another `HttpClient` / `GrpcClient`.

Interpreting the envelope is **not** this library's job. It is a pure protocol
peer with no dependency on neuriplo or task types; decoding results belongs in
the application adapter. The envelope's shapes and datatypes are specified in
the platform ensemble contract.

## Validation

Unit tests cover protocol helpers, retry, and TLS config (no live server):

```bash
cmake -B build -DKSERVE_CLIENT_BUILD_TESTS=ON
cmake --build build -j
ctest --test-dir build --output-on-failure
```

**Runtime conformance** — dry-run (CI) and live against a sibling
`neuriplo-kserve-runtime` checkout:

```bash
# Dry-run — validates command construction; registered as CTest
bash scripts/runtime_conformance.sh --dry-run

# Live — needs a built runtime (stub backend), curl, and gRPC build for --transports grpc
bash scripts/runtime_conformance.sh --live
bash scripts/runtime_conformance.sh --live --transports http
# gRPC live uses test/kserve-client-conformance (KserveGrpcClient oracle)
```

**Ensemble leg** — against a server with a loaded ensemble model:

```bash
test/kserve-client-conformance --grpc-endpoint grpc://127.0.0.1:19091 \
  --ensemble-model yolo_ensemble
```

It checks `platform: ensemble`, the single `UINT8` `IMAGE` input, the envelope
datatypes when the ensemble decodes results, and an encoded-image round trip.

See `plan/NEXT_STEPS.md` for the Codex work track and
`AGENTS.md` for cross-repo sequencing rules.

## License

MIT — see [LICENSE](LICENSE).
