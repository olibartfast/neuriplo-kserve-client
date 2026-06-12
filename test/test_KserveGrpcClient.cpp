#include "KserveGrpcClient.hpp"

#include "kserve_grpc.grpc.pb.h"

#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>
#include <grpcpp/server_context.h>

#include <cstdlib>
#include <cstring>
#include <gtest/gtest.h>
#include <memory>
#include <string>
#include <vector>

namespace {

template <typename T> std::vector<std::uint8_t> bytesOf(const T *values,
                                                        std::size_t count) {
  std::vector<std::uint8_t> out(sizeof(T) * count);
  std::memcpy(out.data(), values, out.size());
  return out;
}

class EnvVar {
public:
  explicit EnvVar(const char *name) : name_(name) {
    const char *value = std::getenv(name_);
    if (value != nullptr) {
      had_value_ = true;
      old_value_ = value;
    }
  }

  ~EnvVar() {
    if (had_value_) {
      ::setenv(name_, old_value_.c_str(), 1);
    } else {
      ::unsetenv(name_);
    }
  }

  EnvVar(const EnvVar &) = delete;
  EnvVar &operator=(const EnvVar &) = delete;

private:
  const char *name_;
  bool had_value_ = false;
  std::string old_value_;
};

class CapturingInferenceService final
    : public inference::GRPCInferenceService::Service {
public:
  grpc::Status ModelInfer(grpc::ServerContext *,
                          const inference::ModelInferRequest *request,
                          inference::ModelInferResponse *response) override {
    last_request = *request;

    response->set_model_name(request->model_name());
    response->set_model_version(request->model_version());
    auto *output = response->add_outputs();
    output->set_name("scores");
    output->set_datatype("FP32");
    output->add_shape(2);

    const float values[] = {3.5F, -1.25F};
    if (raw_output) {
      const auto bytes = bytesOf(values, 2);
      response->add_raw_output_contents(
          std::string(bytes.begin(), bytes.end()));
    } else {
      for (const float value : values) {
        output->mutable_contents()->add_fp64_contents(value);
      }
    }
    return grpc::Status::OK;
  }

  bool raw_output = true;
  inference::ModelInferRequest last_request;
};

class GrpcServerFixture {
public:
  GrpcServerFixture() {
    grpc::ServerBuilder builder;
    builder.AddListeningPort("127.0.0.1:0", grpc::InsecureServerCredentials(),
                             &port_);
    builder.RegisterService(&service_);
    server_ = builder.BuildAndStart();
  }

  ~GrpcServerFixture() { server_->Shutdown(); }

  std::string endpoint() const {
    return "grpc://127.0.0.1:" + std::to_string(port_);
  }

  CapturingInferenceService service_;

private:
  int port_ = 0;
  std::unique_ptr<grpc::Server> server_;
};

} // namespace

TEST(KserveGrpcClient, SendsRawInputContentsAndReadsRawOutputContents) {
  EnvVar kserve_binary("KSERVE_BINARY");
  ::unsetenv("KSERVE_BINARY");

  GrpcServerFixture fixture;
  kserve::GrpcClient client(fixture.endpoint(), "echo", "1", 5000);

  const float input_values[] = {1.0F, 2.0F};
  const auto input_bytes = bytesOf(input_values, 2);
  const std::vector<kserve::InferInput> inputs = {
      {"input", "FP32", {2}, &input_bytes},
  };

  const auto outputs = client.infer(inputs);

  ASSERT_EQ(fixture.service_.last_request.inputs_size(), 1);
  EXPECT_EQ(fixture.service_.last_request.raw_input_contents_size(), 1);
  EXPECT_EQ(fixture.service_.last_request.raw_input_contents(0),
            std::string(input_bytes.begin(), input_bytes.end()));
  EXPECT_EQ(fixture.service_.last_request.inputs(0).contents()
                .fp32_contents_size(),
            0);

  ASSERT_EQ(outputs.size(), 1U);
  EXPECT_EQ(outputs[0].name, "scores");
  EXPECT_EQ(outputs[0].datatype, "FP32");
  EXPECT_EQ(outputs[0].shape, std::vector<int64_t>({2}));

  const float output_values[] = {3.5F, -1.25F};
  EXPECT_EQ(outputs[0].data, bytesOf(output_values, 2));
}

TEST(KserveGrpcClient, ReadsFp64TypedContentsForFp32Output) {
  EnvVar kserve_binary("KSERVE_BINARY");
  ::unsetenv("KSERVE_BINARY");

  GrpcServerFixture fixture;
  fixture.service_.raw_output = false;
  kserve::GrpcClient client(fixture.endpoint(), "echo", "1", 5000);

  const float input_values[] = {1.0F, 2.0F};
  const auto input_bytes = bytesOf(input_values, 2);
  const std::vector<kserve::InferInput> inputs = {
      {"input", "FP32", {2}, &input_bytes},
  };

  const auto outputs = client.infer(inputs);

  ASSERT_EQ(outputs.size(), 1U);
  const float output_values[] = {3.5F, -1.25F};
  EXPECT_EQ(outputs[0].data, bytesOf(output_values, 2));
}
