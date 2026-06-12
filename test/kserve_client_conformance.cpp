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
               " [--model-name demo] [--model-version 1]\n";
  return 2;
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
