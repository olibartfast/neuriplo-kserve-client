#pragma once

// Neutral data types and the pure protocol-client contract for the KServe V2 /
// Open Inference Protocol. Nothing here depends on the neuriplo backend types
// (no TensorElement, no InferenceInterface), so a protocol client built on this
// header is a standalone peer of e.g. Triton's client library: the only shared
// contract with the server is the wire protocol, not a C++ class.
//
// Tensor payloads are carried as raw little-endian bytes (like Triton's
// InferResult raw data); converting bytes to the application's typed tensors is
// the job of the adapter layer (KserveEngine), not the client.

#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

namespace kserve {

// Name + datatype tag + shape of one model input or output.
struct TensorSpec {
  std::string name;
  std::string datatype; // KServe tag, e.g. "FP32", "INT64"
  std::vector<int64_t> shape;
};

// Model input/output description as reported by the server.
struct ModelMetadata {
  std::vector<TensorSpec> inputs;
  std::vector<TensorSpec> outputs;
  // Serving backend reported by the server (KServe V2 metadata "platform"
  // field), e.g. "tensorrt_plan", "onnxruntime_onnx", "openvino". Empty when
  // the server omits it. Callers use it to attribute results to the backend
  // that actually ran the model.
  std::string platform;
};

// One input tensor for an inference request. `data` points at preprocessed raw
// little-endian bytes owned by the caller for the duration of the infer() call.
struct InferInput {
  std::string name;
  std::string datatype;
  std::vector<int64_t> shape;
  const std::vector<uint8_t> *data = nullptr;
};

// One entry of the model repository index: a model the server knows about and
// its load state, as reported by the KServe V2 Model Repository extension.
// `version` / `reason` may be empty depending on the server.
struct RepositoryModel {
  std::string name;
  std::string version; // may be empty
  std::string state;   // e.g. "READY", "UNAVAILABLE", "LOADING"
  std::string reason;  // failure reason when not READY; may be empty
};

// One output tensor returned by the server, as raw little-endian bytes.
struct InferOutput {
  std::string name;
  std::string datatype;
  std::vector<int64_t> shape;
  std::vector<uint8_t> data;
};

// Pure protocol client: speaks KServe V2 over HTTP or gRPC. Implementations
// depend only on the wire protocol and standard transport libraries, never on
// neuriplo. State such as metadata caching belongs to the caller.
class IClient {
public:
  virtual ~IClient() = default;

  // Fetches the model's input/output description from the server.
  virtual ModelMetadata modelMetadata() = 0;

  // Runs inference and returns every output the server produced.
  virtual std::vector<InferOutput>
  infer(const std::vector<InferInput> &inputs) = 0;

  // KServe V2 health probes. Return true when the server/model reports healthy.
  // A reachable server that reports "not ready" returns false; a transport
  // failure (unreachable endpoint) propagates as an exception so callers can
  // distinguish "down" from "up but not ready".
  //   serverLive  — GET /v2/health/live     / gRPC ServerLive
  //   serverReady — GET /v2/health/ready    / gRPC ServerReady
  //   modelReady  — GET /v2/models/{m}/ready / gRPC ModelReady (this model)
  virtual bool serverLive() = 0;
  virtual bool serverReady() = 0;
  virtual bool modelReady() = 0;

  // KServe V2 Model Repository extension (optional capability). The default
  // implementations throw so a client that does not support model management
  // (or a test double that does not care) need not override them; the HTTP and
  // gRPC clients do. HTTP POSTs /v2/repository/index and
  // /v2/repository/models/{m}/{load,unload}; gRPC calls RepositoryIndex /
  // RepositoryModelLoad / RepositoryModelUnload. load/unload take an explicit
  // model name (independent of the client's bound inference model) and throw on
  // failure; index returns every model the server knows about.
  virtual std::vector<RepositoryModel> repositoryIndex() {
    throw std::runtime_error(
        "model repository index not supported by this client");
  }
  virtual void loadModel(const std::string &model_name) {
    (void)model_name;
    throw std::runtime_error("model load not supported by this client");
  }
  virtual void unloadModel(const std::string &model_name) {
    (void)model_name;
    throw std::runtime_error("model unload not supported by this client");
  }
};

} // namespace kserve
