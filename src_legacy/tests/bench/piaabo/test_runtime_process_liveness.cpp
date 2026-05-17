#include <cassert>
#include <csignal>
#include <cstdint>
#include <optional>
#include <string>
#include <sys/wait.h>
#include <unistd.h>

#include "hero/runtime_hero/runtime_job.h"

namespace {

bool wait_for_process_state(pid_t pid, char expected_state,
                            std::uint64_t *out_start_ticks) {
  constexpr int kMaxAttempts = 200;
  constexpr useconds_t kSleepUs = 10 * 1000;
  for (int attempt = 0; attempt < kMaxAttempts; ++attempt) {
    char observed_state = '\0';
    std::uint64_t start_ticks = 0;
    if (cuwacunu::hero::runtime::read_process_state_and_start_ticks(
            static_cast<std::uint64_t>(pid), &observed_state, &start_ticks) &&
        observed_state == expected_state) {
      if (out_start_ticks)
        *out_start_ticks = start_ticks;
      return true;
    }
    ::usleep(kSleepUs);
  }
  return false;
}

bool wait_for_non_zombie_process(pid_t pid, std::uint64_t *out_start_ticks) {
  constexpr int kMaxAttempts = 200;
  constexpr useconds_t kSleepUs = 10 * 1000;
  for (int attempt = 0; attempt < kMaxAttempts; ++attempt) {
    char observed_state = '\0';
    std::uint64_t start_ticks = 0;
    if (cuwacunu::hero::runtime::read_process_state_and_start_ticks(
            static_cast<std::uint64_t>(pid), &observed_state, &start_ticks) &&
        observed_state != 'Z' && observed_state != 'X' &&
        observed_state != 'x') {
      if (out_start_ticks)
        *out_start_ticks = start_ticks;
      return true;
    }
    ::usleep(kSleepUs);
  }
  return false;
}

} // namespace

int main() {
  const std::string boot_id = cuwacunu::hero::runtime::current_boot_id();

  {
    const pid_t child = ::fork();
    assert(child >= 0);
    if (child == 0) {
      for (;;)
        ::pause();
    }

    std::uint64_t start_ticks = 0;
    assert(wait_for_non_zombie_process(child, &start_ticks));
    assert(cuwacunu::hero::runtime::process_identity_alive(
        static_cast<std::uint64_t>(child), start_ticks, boot_id));

    assert(::kill(child, SIGKILL) == 0);
    int status = 0;
    assert(::waitpid(child, &status, 0) == child);
    assert(WIFSIGNALED(status));
  }

  {
    const pid_t child = ::fork();
    assert(child >= 0);
    if (child == 0) {
      _exit(0);
    }

    std::uint64_t zombie_start_ticks = 0;
    assert(wait_for_process_state(child, 'Z', &zombie_start_ticks));
    assert(
        cuwacunu::hero::runtime::pid_alive(static_cast<std::uint64_t>(child)));
    assert(!cuwacunu::hero::runtime::process_identity_alive(
        static_cast<std::uint64_t>(child), zombie_start_ticks, boot_id));

    int status = 0;
    assert(::waitpid(child, &status, 0) == child);
    assert(WIFEXITED(status));
    assert(WEXITSTATUS(status) == 0);
  }

  return 0;
}
