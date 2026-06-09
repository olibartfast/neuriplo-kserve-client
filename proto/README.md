# gRPC proto profiles

The client generates stubs from one proto at configure time. Pick the profile
that matches your inference server; all profiles use `package inference` and
produce the same generated C++ API names.

| Profile | CMake value | Proto source | Typical servers |
|---------|-------------|--------------|-----------------|
| **OIP** | `OIP` | `oip/grpc_predict_v2.proto` | KServe, OpenVINO Model Server (OVMS) |
| **OIP + repository** (default) | `OIP_REPOSITORY` | `kserve_grpc.proto` | Triton (with model control), KServe/OVMS when repo gRPC is enabled |

## CMake

```bash
# Default — Triton-compatible OIP + Model Repository extension
cmake -B build -DKSERVE_CLIENT_PROTO_PROFILE=OIP_REPOSITORY

# Strict KServe OIP only (no RepositoryIndex / Load / Unload stubs)
cmake -B build -DKSERVE_CLIENT_PROTO_PROFILE=OIP
```

Override the proto file entirely:

```bash
cmake -B build -DKSERVE_CLIENT_PROTO_FILE=/path/to/custom.proto
```

When `KSERVE_CLIENT_PROTO_FILE` is set it takes precedence over the profile.
Repository gRPC methods are compiled in only when the selected proto defines
them (`KSERVE_CLIENT_PROTO_REPOSITORY`).

## grpcurl / integration tests

Point `-proto` at the profile your server implements:

```bash
grpcurl -plaintext -proto proto/kserve_grpc.proto ...
grpcurl -plaintext -proto proto/oip/grpc_predict_v2.proto ...
```
