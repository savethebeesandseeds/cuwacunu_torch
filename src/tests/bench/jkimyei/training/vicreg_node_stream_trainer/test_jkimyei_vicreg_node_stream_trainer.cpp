#include "jkimyei/training/representation/vicreg_node_stream_trainer.h"

#include <cmath>
#include <exception>
#include <iostream>
#include <string>
#include <vector>

namespace trainer = cuwacunu::jkimyei::training::representation;
namespace nodelift = cuwacunu::wikimyei::expression::nodelift::srl::stream;

namespace {

void check(bool condition, const std::string &message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

void close(double actual, double expected, double tol,
           const std::string &message) {
  if (std::abs(actual - expected) > tol) {
    throw std::runtime_error(message + " expected=" + std::to_string(expected) +
                             " actual=" + std::to_string(actual));
  }
}

nodelift::node_lifted_batch_t<int64_t>
make_batch(int64_t B = 1, int64_t C = 2, int64_t H = 3, int64_t N = 2) {
  nodelift::node_lifted_batch_t<int64_t> out{};
  out.node_features = torch::ones({B, C, H, N, 9}, torch::kFloat32);
  out.node_mask = torch::ones({B, C, H, N, 9}, torch::kBool);
  out.anchor_keys = torch::arange(B, torch::kInt64);
  out.node_ids = {"n0", "n1"};
  return out;
}

struct fake_stream_t {
  std::vector<nodelift::node_lifted_batch_t<int64_t>> batches{};
  std::size_t cursor{0};

  bool has_next() const { return cursor < batches.size(); }

  nodelift::node_lifted_batch_t<int64_t> next() { return batches.at(cursor++); }
};

struct fake_train_result_t {
  torch::Tensor loss{};
  bool optimizer_step_applied{false};
  bool skipped{false};
};

struct fake_model_t {
  std::vector<int64_t> rows_seen{};
  std::vector<int64_t> valid_counts_seen{};
  std::vector<int> swa_seen{};
  std::vector<bool> verbose_seen{};

  fake_train_result_t train_one_batch(const torch::Tensor &data,
                                      const torch::Tensor &mask,
                                      int swa_start_iter, bool verbose) {
    rows_seen.push_back(data.size(0));
    valid_counts_seen.push_back(mask.sum().item<int64_t>());
    swa_seen.push_back(swa_start_iter);
    verbose_seen.push_back(verbose);
    return fake_train_result_t{
        .loss = torch::tensor(static_cast<double>(data.size(0)) +
                              static_cast<double>(mask.sum().item<int64_t>()) *
                                  0.01),
        .optimizer_step_applied = true,
        .skipped = false,
    };
  }
};

void test_trainer_compacts_and_summarizes() {
  auto first = make_batch();
  first.node_mask.index_put_({0, torch::indexing::Slice(),
                              torch::indexing::Slice(), 1,
                              torch::indexing::Slice()},
                             false);
  auto second = make_batch(/*B=*/1, /*C=*/2, /*H=*/3, /*N=*/1);
  second.node_ids = {"n0"};

  fake_stream_t stream{{first, second}};
  fake_model_t model{};

  trainer::vicreg_node_stream_trainer_options_t options{};
  options.swa_start_iter = 17;
  options.verbose = true;
  auto summary = trainer::train_vicreg_node_stream(stream, model, options);

  check(summary.stream_batches_seen == 2, "stream batches seen");
  check(summary.train_batches_applied == 2, "train batches applied");
  check(summary.empty_batches_skipped == 0, "no empty batches skipped");
  check(summary.optimizer_steps_applied == 2, "optimizer step count");
  check(summary.original_rows == 3, "summary original rows");
  check(summary.kept_rows == 2, "summary kept rows");
  check(summary.dropped_rows == 1, "summary dropped rows");
  check(summary.valid_channel_time_count == 12,
        "summary original valid channel-time count");
  check(summary.kept_valid_channel_time_count == 12,
        "summary kept valid channel-time count");
  check(summary.batches.size() == 2, "batch reports kept");
  check(model.rows_seen == std::vector<int64_t>({1, 1}),
        "model saw compacted rows");
  check(model.valid_counts_seen == std::vector<int64_t>({6, 6}),
        "model saw valid counts");
  check(model.swa_seen == std::vector<int>({17, 17}),
        "swa_start_iter forwarded");
  check(model.verbose_seen == std::vector<bool>({true, true}),
        "verbose forwarded");
  close(summary.mean_loss(), 1.06, 1e-6, "mean loss");
}

void test_trainer_skips_empty_batches_and_honors_max_batches() {
  auto empty = make_batch();
  empty.node_mask.fill_(false);
  auto valid = make_batch();

  fake_stream_t stream{{empty, valid}};
  fake_model_t model{};

  trainer::vicreg_node_stream_trainer_options_t options{};
  options.max_batches = 1;
  auto summary = trainer::train_vicreg_node_stream(stream, model, options);

  check(summary.stream_batches_seen == 1, "max batches honored");
  check(summary.train_batches_applied == 0, "empty batch not trained");
  check(summary.empty_batches_skipped == 1, "empty batch skipped");
  check(summary.batches.size() == 1, "empty batch report kept");
  check(summary.batches.front().skipped_empty, "empty report marked");
  check(model.rows_seen.empty(), "model not called for skipped empty batch");
}

} // namespace

int main() {
  try {
    test_trainer_compacts_and_summarizes();
    test_trainer_skips_empty_batches_and_honors_max_batches();
    std::cout << "[Jkimyei VICRegNodeStreamTrainer test] all checks passed\n";
    return 0;
  } catch (const std::exception &ex) {
    std::cerr << "[Jkimyei VICRegNodeStreamTrainer test] failure: " << ex.what()
              << "\n";
    return 1;
  }
}
