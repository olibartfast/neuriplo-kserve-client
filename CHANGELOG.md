# Changelog

All notable changes to this project are documented here.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added

- Server-side ensemble support, with no API change: an ensemble is an ordinary
  KServe V2 model on the wire, so the existing `UINT8` inputs, `INT32` /
  `INT64` / `FP32` raw outputs, and `ModelMetadata::platform` already carry it.
  Coverage is what was missing:
  - unit tests for variable-length `[1, N]` `UINT8` encoded-image inputs over
    both the binary extension and the inline-JSON path, and for decoding the
    decoded-result envelope out of a framed binary body;
  - a `--ensemble-model` leg in `kserve-client-conformance` that checks
    `platform: ensemble`, the single `IMAGE` input, and the envelope datatypes
    against a live server.
- README section on ensembles, including why envelope decoding stays out of
  this library: it is a pure protocol peer, so interpreting results belongs to
  the application adapter.

## [0.4.0] - 2026-06-14

### Added

- `ModelMetadata.platform`: the server's KServe V2 metadata `platform` field
  (e.g. `tensorrt_plan`, `onnxruntime_onnx`, `openvino`, `neuriplo_litert`),
  parsed by both the HTTP and gRPC clients. Empty when the server omits it.
  Lets callers attribute results to the backend that actually ran the model.

## [0.3.0] - 2026-06-12

### Added

- gRPC raw-contents unit tests and `kserve-client-conformance` live oracle binary.
- Runtime conformance harness gRPC leg via `KserveGrpcClient` (no grpcurl).

### Changed

- `KserveGrpcClient` `fp64_contents` typed fallback documented as legacy-compat
  for pre-0.3 runtimes; `neuriplo-kserve-runtime@develop` emits
  `raw_output_contents` (runtime PR #9).
- Conformance plan: Step 3 CI dry-run checked off; live gRPC revalidated against
  merged runtime.

## [0.2.0] - 2026-06-11

### Added

- Selectable gRPC proto profiles via `KSERVE_CLIENT_PROTO_PROFILE`:
  `OIP` (strict KServe/OVMS) and `OIP_REPOSITORY` (default; Triton-compatible
  model repository RPCs).
- `proto/oip/grpc_predict_v2.proto` and `proto/README.md` documenting profile
  selection and `grpcurl` usage.
- Optional local compile-speed helpers in `cmake/CompileSpeed.cmake` (ccache when
  available).
- CI ccache setup for faster repeated matrix builds.
- `CONTRIBUTING.md` with Gitflow branch workflow for contributors.

### Changed

- CI matrix now exercises both proto profiles (`OIP`, `OIP_REPOSITORY`).
- README updated for proto profile CMake options; TorchServe references removed
  from server compatibility docs.

## [0.1.0] - 2026-06-09

### Added

- Initial standalone KServe V2 / Open Inference Protocol client extracted from
  neuriplo-infer.
- HTTP transport (hand-rolled socket client) and optional gRPC transport over
  `inference.GRPCInferenceService`.
- Model metadata, inference, and health probes (`serverLive`, `serverReady`,
  `modelReady`).
- Model Repository extension: `repositoryIndex`, `loadModel`, `unloadModel`.
- KServe Binary Tensor Data Extension (HTTP) and gRPC raw input/output contents.
- Retry with exponential backoff and jitter; HTTP keep-alive; TLS/mTLS via env or
  file-backed secrets.
- GoogleTest suite for protocol helpers, retry policy, and TLS/secret resolution.

[0.4.0]: https://github.com/olibartfast/neuriplo-kserve-client/compare/v0.3.0...v0.4.0
[0.3.0]: https://github.com/olibartfast/neuriplo-kserve-client/compare/v0.2.0...v0.3.0
[0.2.0]: https://github.com/olibartfast/neuriplo-kserve-client/compare/v0.1.0...v0.2.0
[0.1.0]: https://github.com/olibartfast/neuriplo-kserve-client/releases/tag/v0.1.0
