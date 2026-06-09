# neuriplo-kserve-client

A standalone C++ client for the **KServe V2 / Open Inference Protocol** (OIP),
the wire protocol exposed by [KServe](https://kserve.github.io/website/),
[Triton Inference Server](https://github.com/triton-inference-server/server),
[OpenVINO Model Server](https://github.com/openvinotoolkit/model_server) and
TorchServe.

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
| `proto/kserve_grpc.proto` | KServe V2 gRPC service (+ Model Repository extension) |

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

When Protobuf/gRPC or OpenSSL are absent the build still succeeds with the
corresponding capability compiled out (`https://` / `grpcs://` endpoints then
fail fast with a clear error).

## Use as a dependency (CMake FetchContent)

```cmake
include(FetchContent)
FetchContent_Declare(neuriplo-kserve-client
    GIT_REPOSITORY https://github.com/olibartfast/neuriplo-kserve-client.git
    GIT_TAG        v0.1.0)
FetchContent_MakeAvailable(neuriplo-kserve-client)
target_link_libraries(your_target PRIVATE neuriplo::kserve-client)
```

The target exports a PUBLIC `KSERVE_CLIENT_WITH_GRPC` define when the gRPC
transport is compiled in, so consumers can `#ifdef` on its availability.

## Runtime configuration (environment)

| Variable | Purpose |
|----------|---------|
| `KSERVE_BEARER_TOKEN` / `KSERVE_BEARER_TOKEN_FILE` | Bearer auth (value or file) |
| `KSERVE_BINARY` | Opt into the HTTP binary tensor extension (gRPC raw contents are default) |
| `KSERVE_CA_CERT` / `KSERVE_CLIENT_CERT` / `KSERVE_CLIENT_KEY` | TLS / mTLS material |
| `KSERVE_MAX_RETRIES` / `KSERVE_RETRY_BASE_MS` / `KSERVE_RETRY_MAX_MS` / `KSERVE_RETRY_JITTER` | Retry policy |

## License

MIT — see [LICENSE](LICENSE).
