#pragma once

#include <array>
#include <string>
#include <vector>
#include <utility>

namespace cuwacunu {
namespace iinuji {

/* Fixed-slot typed data model (safe / bounded)
   - bounded indices only
   - no path traversal
*/
struct IInstructionsData {
  virtual ~IInstructionsData() = default;

  virtual int max_vec() const { return 16; }
  virtual int max_str() const { return 16; }
  virtual int max_num() const { return 16; }

  virtual bool supports_vec(int i) const { return i >= 0 && i < max_vec(); }
  virtual bool supports_str(int i) const { return i >= 0 && i < max_str(); }
  virtual bool supports_num(int i) const { return i >= 0 && i < max_num(); }

  // READ (used by initial render + _update)
  virtual bool get_vec(int i, std::vector<std::pair<double,double>>& out) const {
    (void)i; (void)out; return false;
  }
  virtual bool get_str(int i, std::string& out) const {
    (void)i; (void)out; return false;
  }
  virtual bool get_num(int i, double& out) const {
    (void)i; (void)out; return false;
  }

  // WRITE (used by _action)
  virtual bool set_vec(int i, const std::vector<std::pair<double,double>>& in) {
    (void)i; (void)in; return false;
  }
  virtual bool set_str(int i, const std::string& in) {
    (void)i; (void)in; return false;
  }
  virtual bool set_num(int i, double in) {
    (void)i; (void)in; return false;
  }
};

struct NullInstructionsData : IInstructionsData {};

/* Concrete fixed storage for tests / simple apps */
struct FixedInstructionsData : IInstructionsData {
  static constexpr int V = 16, S = 16, N = 16;

  std::array<std::vector<std::pair<double,double>>, V> vec{};
  std::array<std::string, S> str{};
  std::array<double, N> num{};

  std::array<bool, V> vec_set{};
  std::array<bool, S> str_set{};
  std::array<bool, N> num_set{};

  int max_vec() const override { return V; }
  int max_str() const override { return S; }
  int max_num() const override { return N; }

  bool get_vec(int i, std::vector<std::pair<double,double>>& out) const override {
    if (!supports_vec(i) || !vec_set[(size_t)i]) return false;
    out = vec[(size_t)i];
    return true;
  }
  bool get_str(int i, std::string& out) const override {
    if (!supports_str(i) || !str_set[(size_t)i]) return false;
    out = str[(size_t)i];
    return true;
  }
  bool get_num(int i, double& out) const override {
    if (!supports_num(i) || !num_set[(size_t)i]) return false;
    out = num[(size_t)i];
    return true;
  }

  bool set_vec(int i, const std::vector<std::pair<double,double>>& in) override {
    if (!supports_vec(i)) return false;
    vec[(size_t)i] = in;
    vec_set[(size_t)i] = true;
    return true;
  }
  bool set_str(int i, const std::string& in) override {
    if (!supports_str(i)) return false;
    str[(size_t)i] = in;
    str_set[(size_t)i] = true;
    return true;
  }
  bool set_num(int i, double in) override {
    if (!supports_num(i)) return false;
    num[(size_t)i] = in;
    num_set[(size_t)i] = true;
    return true;
  }
};

} // namespace iinuji
} // namespace cuwacunu
