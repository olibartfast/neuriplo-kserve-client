# Changelog

All notable changes to this project are documented here.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

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

[0.2.0]: https://github.com/olibartfast/neuriplo-kserve-client/compare/v0.1.0...v0.2.0
[0.1.0]: https://github.com/olibartfast/neuriplo-kserve-client/releases/tag/v0.1.0
