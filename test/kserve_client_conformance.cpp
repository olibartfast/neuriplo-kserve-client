#include "KserveGrpcClient.hpp"
#include "KserveProtocol.hpp"

#include <cstdint>
#include <exception>
#include <iostream>
#include <numeric>
#include <string>
#include <vector>

namespace {

std::int64_t elementCount(const std::vector<std::int64_t> &shape) {
  if (shape.empty()) {
    return 0;
  }
  return std::accumulate(shape.begin(), shape.end(), std::int64_t{1},
                         [](std::int64_t acc, std::int64_t dim) {
                           return dim > 0 ? acc * dim : acc;
                         });
}

std::string argValue(int argc, char **argv, const std::string &name,
                     const std::string &fallback) {
  for (int i = 1; i + 1 < argc; ++i) {
    if (argv[i] == name) {
      return argv[i + 1];
    }
  }
  return fallback;
}

int usage(const char *argv0) {
  std::cerr << "usage: " << argv0
            << " --grpc-endpoint grpc://127.0.0.1:19091"
               " [--model-name demo] [--model-version 1]"
               " [--ensemble-model yolo_ensemble]\n";
  return 2;
}

// --- Ensemble leg ------------------------------------------------------------
//
// An ensemble is an ordinary KServe model on the wire, so this exercises the
// same client against one: the encoded-image input and the decoded envelope
// from the platform ensemble contract. The client stays protocol-only -- it
// checks datatypes and byte counts, never what the numbers mean.

struct EnvelopeTensor {
  const char *name;
  const char *datatype;
};

const EnvelopeTensor kDetectionEnvelope[] = {
    {"NUM_DETECTIONS", "INT32"},
    {"BOXES", "INT32"},
    {"SCORES", "FP32"},
    {"CLASSES", "INT32"},
};

const kserve::TensorSpec *findSpec(const std::vector<kserve::TensorSpec> &specs,
                                   const std::string &name) {
  for (const auto &spec : specs) {
    if (spec.name == name) {
      return &spec;
    }
  }
  return nullptr;
}

const kserve::InferOutput *
findOutput(const std::vector<kserve::InferOutput> &outputs,
           const std::string &name) {
  for (const auto &output : outputs) {
    if (output.name == name) {
      return &output;
    }
  }
  return nullptr;
}

// A real 16x16 JPEG, not a hand-written header. The ensemble decodes it on the
// server, so a stub carrying only SOI/EOI markers would be rejected before any
// output existed to check -- the leg would then be asserting nothing.
std::vector<std::uint8_t> tinyJpeg() {
  return {
      0xff, 0xd8, 0xff, 0xe0, 0x00, 0x10, 0x4a, 0x46, 0x49, 0x46, 0x00, 0x01,
      0x02, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0xff, 0xfe, 0x00, 0x10,
      0x4c, 0x61, 0x76, 0x63, 0x36, 0x30, 0x2e, 0x33, 0x31, 0x2e, 0x31, 0x30,
      0x32, 0x00, 0xff, 0xdb, 0x00, 0x43, 0x00, 0x08, 0x18, 0x18, 0x1c, 0x18,
      0x1c, 0x21, 0x21, 0x21, 0x21, 0x21, 0x21, 0x27, 0x24, 0x27, 0x28, 0x28,
      0x28, 0x27, 0x27, 0x27, 0x27, 0x28, 0x28, 0x28, 0x2b, 0x2b, 0x2b, 0x33,
      0x33, 0x33, 0x2b, 0x2b, 0x2b, 0x28, 0x28, 0x2b, 0x2b, 0x30, 0x30, 0x33,
      0x33, 0x37, 0x39, 0x37, 0x34, 0x34, 0x33, 0x34, 0x39, 0x39, 0x3c, 0x3c,
      0x3c, 0x48, 0x48, 0x45, 0x45, 0x54, 0x54, 0x57, 0x67, 0x67, 0x7c, 0xff,
      0xc4, 0x00, 0x4c, 0x00, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x01, 0x01,
      0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x06, 0x07, 0x10, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x11,
      0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xc0, 0x00, 0x11, 0x08, 0x00, 0x10,
      0x00, 0x10, 0x03, 0x01, 0x22, 0x00, 0x02, 0x11, 0x00, 0x03, 0x11, 0x00,
      0xff, 0xda, 0x00, 0x0c, 0x03, 0x01, 0x00, 0x02, 0x11, 0x03, 0x11, 0x00,
      0x3f, 0x00, 0x8b, 0x00, 0x51, 0x7f, 0x7f, 0xff, 0xd9,
  };
}

int runEnsembleLeg(const std::string &endpoint, const std::string &model_name,
                   const std::string &model_version) {
  kserve::GrpcClient client(endpoint, model_name, model_version, 5000);
  const auto metadata = client.modelMetadata();

  if (metadata.platform != "ensemble") {
    std::cerr << "ensemble metadata platform is '" << metadata.platform
              << "', expected 'ensemble'\n";
    return 1;
  }
  if (metadata.inputs.size() != 1 || metadata.inputs.front().name != "IMAGE" ||
      metadata.inputs.front().datatype != "UINT8") {
    std::cerr << "ensemble must expose exactly one UINT8 input named IMAGE\n";
    return 1;
  }

  const bool decoded = findSpec(metadata.outputs, "NUM_DETECTIONS") != nullptr;
  if (decoded) {
    for (const auto &expected : kDetectionEnvelope) {
      const auto *spec = findSpec(metadata.outputs, expected.name);
      if (spec == nullptr) {
        std::cerr << "ensemble is missing envelope output " << expected.name
                  << "\n";
        return 1;
      }
      if (spec->datatype != expected.datatype) {
        std::cerr << "envelope output " << expected.name << " is "
                  << spec->datatype << ", expected " << expected.datatype
                  << "\n";
        return 1;
      }
    }
  }

  const auto image = tinyJpeg();
  const std::vector<kserve::InferInput> inputs = {
      {"IMAGE", "UINT8", {1, static_cast<std::int64_t>(image.size())}, &image},
  };
  const auto outputs = client.infer(inputs);
  if (outputs.empty()) {
    std::cerr << "ensemble inference returned no outputs\n";
    return 1;
  }

  if (decoded) {
    const auto *count = findOutput(outputs, "NUM_DETECTIONS");
    if (count == nullptr || count->datatype != "INT32" ||
        count->data.size() != sizeof(std::int32_t)) {
      std::cerr << "NUM_DETECTIONS is missing or not a single INT32\n";
      return 1;
    }
    // The envelope's padded arrays arrive at full length whatever the
    // detection count; a short one here is the empty-frame defect.
    const auto *scores = findOutput(outputs, "SCORES");
    if (scores == nullptr || scores->data.size() % sizeof(float) != 0) {
      std::cerr << "SCORES is missing or not a whole number of FP32 values\n";
      return 1;
    }
  }

  std::cout << "ensemble conformance OK: " << model_name
            << " outputs=" << outputs.size()
            << (decoded ? " (decoded envelope)" : " (passthrough)") << "\n";
  return 0;
}

} // namespace

int main(int argc, char **argv) {
  const std::string endpoint = argValue(argc, argv, "--grpc-endpoint", "");
  if (endpoint.empty()) {
    return usage(argv[0]);
  }

  const std::string model_name = argValue(argc, argv, "--model-name", "demo");
  const std::string model_version =
      argValue(argc, argv, "--model-version", "1");
  const std::string ensemble_model =
      argValue(argc, argv, "--ensemble-model", "");

  if (!ensemble_model.empty()) {
    try {
      return runEnsembleLeg(endpoint, ensemble_model, model_version);
    } catch (const std::exception &error) {
      std::cerr << "ensemble conformance failed: " << error.what() << "\n";
      return 1;
    }
  }

  try {
    kserve::GrpcClient client(endpoint, model_name, model_version, 5000);
    const auto metadata = client.modelMetadata();
    if (metadata.inputs.empty() || metadata.outputs.empty()) {
      std::cerr << "metadata missing inputs or outputs\n";
      return 1;
    }

    const auto &input = metadata.inputs.front();
    const auto input_width = kserve::datatypeByteWidth(input.datatype);
    if (input_width == 0) {
      std::cerr << "unsupported live input datatype: " << input.datatype
                << "\n";
      return 1;
    }
    const auto input_elements = elementCount(input.shape);
    std::vector<std::uint8_t> input_bytes(
        static_cast<std::size_t>(input_elements) * input_width, 0);
    const std::vector<kserve::InferInput> inputs = {
        {input.name, input.datatype, input.shape, &input_bytes},
    };

    const auto outputs = client.infer(inputs);
    if (outputs.empty()) {
      std::cerr << "inference returned no outputs\n";
      return 1;
    }

    const auto &output = outputs.front();
    const auto output_width = kserve::datatypeByteWidth(output.datatype);
    if (output_width == 0) {
      std::cerr << "unsupported live output datatype: " << output.datatype
                << "\n";
      return 1;
    }
    const auto expected_bytes =
        static_cast<std::size_t>(elementCount(output.shape)) * output_width;
    if (output.data.size() != expected_bytes) {
      std::cerr << "output byte count mismatch: got " << output.data.size()
                << ", expected " << expected_bytes << "\n";
      return 1;
    }

    std::cout << "gRPC conformance OK: " << output.name << " "
              << output.datatype << " bytes=" << output.data.size() << "\n";
    return 0;
  } catch (const std::exception &error) {
    std::cerr << "gRPC conformance failed: " << error.what() << "\n";
    return 1;
  }
}
