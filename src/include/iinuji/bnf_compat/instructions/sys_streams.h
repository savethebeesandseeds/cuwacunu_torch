#pragma once

#include <iostream>
#include <streambuf>
#include <string>
#include <vector>
#include <mutex>
#include <utility>
#include <memory>
#include <functional>
#include <unordered_set>

#include "iinuji/bnf_compat/instructions/build.h"
#include "iinuji/bnf_compat/instructions/dispatch.h"
#include "iinuji/bnf_compat/instructions/form.h"

RUNTIME_WARNING("(iinuji/sys_streams.h)[] sys_stream_router_t replaces std::cout/std::cerr rdbuf globally; prevent multiple attachments and document global-side-effect.\n");
RUNTIME_WARNING("(iinuji/sys_streams.h)[] stdout/stderr capture drops empty lines; consider preserving them or making it an option.\n");
RUNTIME_WARNING("(iinuji/sys_streams.h)[] tap_streambuf_t::overflow should return traits_type::not_eof(ch) for correctness; current behavior likely ok but non-idiomatic.\n");

namespace cuwacunu {
namespace iinuji {

enum class sys_stream_e { Stdout, Stderr };

struct sys_line_t {
  sys_stream_e stream;
  std::string line;
};

class sys_line_queue_t {
public:
  void push(sys_stream_e s, std::string line) {
    std::lock_guard<std::mutex> lk(m_);
    q_.push_back(sys_line_t{s, std::move(line)});
  }

  std::vector<sys_line_t> drain() {
    std::lock_guard<std::mutex> lk(m_);
    std::vector<sys_line_t> out;
    out.swap(q_);
    return out;
  }

private:
  std::mutex m_;
  std::vector<sys_line_t> q_;
};

class tap_streambuf_t : public std::streambuf {
public:
  using on_line_fn = std::function<void(std::string)>;

  tap_streambuf_t(std::streambuf* original, on_line_fn cb, bool passthrough=false)
  : orig_(original), cb_(std::move(cb)), passthrough_(passthrough) {}

protected:
  using traits_type = std::streambuf::traits_type;
  using int_type    = traits_type::int_type;

  int_type overflow(int_type ch) override {
    if (traits_type::eq_int_type(ch, traits_type::eof()))
      return traits_type::eof();

    char c = traits_type::to_char_type(ch);
    handle_char(c);

    if (passthrough_ && orig_) {
      // propagate success/failure from original buffer
      return orig_->sputc(c);
    }

    // indicate success
    return traits_type::not_eof(ch);
  }

  std::streamsize xsputn(const char* s, std::streamsize n) override {
    for (std::streamsize i=0;i<n;++i) handle_char(s[i]);
    if (passthrough_ && orig_) return orig_->sputn(s, n);
    return n;
  }

  int sync() override {
    flush_partial();
    if (passthrough_ && orig_) return orig_->pubsync();
    return 0;
  }

private:
  void handle_char(char c) {
    if (line_.size() > max_line_bytes_) flush_partial();

    if (c == '\n') {
      emit_line();
    } else if (c != '\r') {
      line_.push_back(c);
    }
  }

  void emit_line() {
    if (!cb_) { line_.clear(); return; }
    std::string out;
    out.swap(line_);
    cb_(std::move(out));
  }

  void flush_partial() {
    if (!line_.empty()) emit_line();
  }

private:
  std::streambuf* orig_{nullptr};
  on_line_fn cb_;
  bool passthrough_{false};
  std::string line_;
  std::size_t max_line_bytes_{4096};
};

class ostream_redirect_t {
public:
  ostream_redirect_t() = default;

  ostream_redirect_t(std::ostream& os,
                     std::function<void(std::string)> on_line,
                     bool passthrough=false)
  : os_(&os)
  {
    old_ = os_->rdbuf();
    tap_ = std::make_unique<tap_streambuf_t>(old_, std::move(on_line), passthrough);
    os_->rdbuf(tap_.get());
  }

  ~ostream_redirect_t() { reset(); }

  ostream_redirect_t(const ostream_redirect_t&) = delete;
  ostream_redirect_t& operator=(const ostream_redirect_t&) = delete;

  ostream_redirect_t(ostream_redirect_t&& o) noexcept { *this = std::move(o); }
  ostream_redirect_t& operator=(ostream_redirect_t&& o) noexcept {
    if (this == &o) return *this;
    reset();
    os_  = o.os_;  o.os_ = nullptr;
    old_ = o.old_; o.old_ = nullptr;
    tap_ = std::move(o.tap_);
    return *this;
  }

  void reset() {
    if (os_ && old_) {
      os_->rdbuf(old_);
    }
    tap_.reset();
    os_ = nullptr;
    old_ = nullptr;
  }

private:
  std::ostream* os_{nullptr};
  std::streambuf* old_{nullptr};
  std::unique_ptr<tap_streambuf_t> tap_;
};

/* Router:
   - attaches to std::cout/std::cerr
   - pushes lines to a queue (shared_ptr so lambdas remain valid)
   - pump() dispatches them to matching .sys.* events
*/
class sys_stream_router_t {
public:
  sys_stream_router_t() : queue_(std::make_shared<sys_line_queue_t>()) {}

  sys_stream_router_t(const sys_stream_router_t&) = delete;
  sys_stream_router_t& operator=(const sys_stream_router_t&) = delete;

  sys_stream_router_t(sys_stream_router_t&&) noexcept = default;
  sys_stream_router_t& operator=(sys_stream_router_t&&) noexcept = default;

  static std::unique_ptr<sys_stream_router_t>
  attach_for(instructions_build_result_t& built, bool passthrough=false) {
    auto R = std::make_unique<sys_stream_router_t>();

    // discover events...
    for (const auto& kv : built.events_by_name) {
      const auto& E = kv.second;
      for (const auto& b : E.bindings) {
        if (b.ref.kind != data_kind_e::System) continue;
        if (b.bind_kind != bind_kind_e::Str) continue;

        if (b.ref.sys == sys_ref_e::Stdout) R->stdout_events_.push_back(E.name);
        if (b.ref.sys == sys_ref_e::Stderr) R->stderr_events_.push_back(E.name);
      }
    }

    if (!R->stdout_events_.empty()) {
      R->out_ = std::make_unique<ostream_redirect_t>(
        std::cout,
        [Rptr=R.get()](std::string line){
          if (!line.empty()) Rptr->queue_->push(sys_stream_e::Stdout, std::move(line));
        },
        passthrough
      );
    }
    if (!R->stderr_events_.empty()) {
      R->err_ = std::make_unique<ostream_redirect_t>(
        std::cerr,
        [Rptr=R.get()](std::string line){
          if (!line.empty()) Rptr->queue_->push(sys_stream_e::Stderr, std::move(line));
        },
        passthrough
      );
    }
    return R;
  }

  static std::unique_ptr<sys_stream_router_t>
  attach_for_many(const std::vector<instructions_build_result_t*>& builts,
                  bool passthrough=false)
  {
    auto R = std::make_unique<sys_stream_router_t>();

    std::unordered_set<std::string> seen_out;
    std::unordered_set<std::string> seen_err;

    auto add_unique = [](std::vector<std::string>& v,
                         std::unordered_set<std::string>& seen,
                         const std::string& s)
    {
      if (seen.insert(s).second) v.push_back(s);
    };

    for (auto* B : builts) {
      if (!B) continue;
      for (const auto& kv : B->events_by_name) {
        const auto& E = kv.second;
        for (const auto& b : E.bindings) {
          if (b.ref.kind != data_kind_e::System) continue;
          if (b.bind_kind != bind_kind_e::Str) continue;

          if (b.ref.sys == sys_ref_e::Stdout) add_unique(R->stdout_events_, seen_out, E.name);
          if (b.ref.sys == sys_ref_e::Stderr) add_unique(R->stderr_events_, seen_err, E.name);
        }
      }
    }

    if (!R->stdout_events_.empty()) {
      R->out_ = std::make_unique<ostream_redirect_t>(
        std::cout,
        [Rptr=R.get()](std::string line){
          if (!line.empty()) Rptr->queue_->push(sys_stream_e::Stdout, std::move(line));
        },
        passthrough
      );
    }
    if (!R->stderr_events_.empty()) {
      R->err_ = std::make_unique<ostream_redirect_t>(
        std::cerr,
        [Rptr=R.get()](std::string line){
          if (!line.empty()) Rptr->queue_->push(sys_stream_e::Stderr, std::move(line));
        },
        passthrough
      );
    }

    return R;
  }

  // returns true if anything was dispatched (so you should re-render)
  bool pump(instructions_build_result_t& built, IInstructionsData& data) {
    bool changed = false;

    auto items = queue_->drain();
    for (auto& it : items) {
      dispatch_payload_t p;
      p.has_str = true;
      p.str = std::move(it.line);

      const auto& targets = (it.stream == sys_stream_e::Stdout) ? stdout_events_ : stderr_events_;
      for (const auto& ev : targets) {
        (void)dispatch_event(built, ev, data, &p);
        changed = true;
      }
    }
    return changed;
  }

  bool pump_all(const std::vector<instructions_build_result_t*>& builts,
                IInstructionsData& data)
  {
    bool changed = false;
    auto items = queue_->drain();

    for (auto& it : items) {
      dispatch_payload_t p;
      p.has_str = true;
      p.str = std::move(it.line);

      const auto& targets =
        (it.stream == sys_stream_e::Stdout) ? stdout_events_ : stderr_events_;

      for (const auto& ev : targets) {
        for (auto* B : builts) {
          if (!B || !B->root) continue;
          if (B->events_by_name.find(ev) == B->events_by_name.end()) continue; // skip screens without it
          (void)dispatch_event(*B, ev, data, &p);
          changed = true;
        }
      }
    }

    return changed;
  }

private:
  std::shared_ptr<sys_line_queue_t> queue_;

  std::vector<std::string> stdout_events_;
  std::vector<std::string> stderr_events_;

  std::unique_ptr<ostream_redirect_t> out_;
  std::unique_ptr<ostream_redirect_t> err_;
};

} // namespace iinuji
} // namespace cuwacunu
