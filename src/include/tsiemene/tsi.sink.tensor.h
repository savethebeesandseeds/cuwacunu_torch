// ./include/tsiemene/tsi.sink.tensor.h
// SPDX-License-Identifier: MIT
#pragma once

#include <cstddef>
#include <deque>
#include <string>
#include <string_view>

#include "tsiemene/utils/tsi.h"

namespace tsiemene {

class TsiSinkTensor final : public TsiSink {
 public:
  static constexpr PortId IN = 1;

  struct Item {
    Wave wave{};
    torch::Tensor tensor{};
  };

  explicit TsiSinkTensor(TsiId id,
                         std::string instance_name = "tsi.sink.tensor",
                         std::size_t capacity = 1024)
      : id_(id),
        instance_name_(std::move(instance_name)),
        capacity_(capacity) {}

  [[nodiscard]] std::string_view type_name() const noexcept override { return "tsi.sink.tensor"; }
  [[nodiscard]] std::string_view instance_name() const noexcept override { return instance_name_; }
  [[nodiscard]] TsiId id() const noexcept override { return id_; }

  [[nodiscard]] std::span<const Port> ports() const noexcept override {
    static constexpr Port kPorts[] = {
      port(IN, PortDir::In, Schema::Tensor(), /*tag=*/{}, "sink input tensor"),
    };
    return std::span<const Port>(kPorts, 1);
  }

  void reset(TsiContext&) override { items_.clear(); }

  void step(const Wave& wave, Ingress in, TsiContext&, Emitter&) override {
    if (in.port != IN) return;
    if (in.signal.kind != PayloadKind::Tensor) return;
    if (!in.signal.tensor.defined()) return;

    if (capacity_ > 0 && items_.size() >= capacity_) {
      items_.pop_front(); // bounded retention
    }
    items_.push_back(Item{.wave = wave, .tensor = std::move(in.signal.tensor)});
  }

  // For tests / inspection
  [[nodiscard]] std::size_t size() const noexcept { return items_.size(); }
  [[nodiscard]] const Item& at(std::size_t i) const { return items_.at(i); }

 private:
  TsiId id_{};
  std::string instance_name_;
  std::size_t capacity_{1024};
  std::deque<Item> items_;
};

} // namespace tsiemene
