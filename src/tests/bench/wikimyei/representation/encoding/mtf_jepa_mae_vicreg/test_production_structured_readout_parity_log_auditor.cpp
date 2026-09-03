#include "production_structured_readout_parity_gate.h"

#include <algorithm>
#include <array>
#include <cerrno>
#include <charconv>
#include <clocale>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <limits>
#include <map>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

#include <sys/wait.h>

namespace gate = cuwacunu::tests::production_structured_readout_parity_gate;

namespace {

constexpr std::array<std::string_view, 3> kSeeds{"17", "31", "47"};
constexpr std::array<std::string_view, 6> kDatasets{
    "probe_train",    "probe_validation",    "test",
    "reversed_train", "reversed_validation", "reversed_test"};
constexpr std::array<std::uint64_t, 6> kDatasetRows{256, 128, 256,
                                                    256, 128, 256};
constexpr std::array<std::uint64_t, 6> kDatasetGroupBegin{
    1000000, 2000000, 3000000, 1000000, 2000000, 3000000};
constexpr std::array<bool, 6> kDatasetReversed{false, false, false,
                                               true,  true,  true};
constexpr std::string_view kSchema = "wikimyei.mtf_jepa_mae_vicreg.srr2.v1";
constexpr std::string_view kAuditSchema =
    "wikimyei.mtf_jepa_mae_vicreg.srr2_audit.v1";
constexpr std::string_view kManifestSchema =
    "wikimyei.mtf_jepa_mae_vicreg.srr2_prerun_manifest.v1";
constexpr std::string_view kProtocolSha =
    "742def90993850ab7ed381e860d60f5adbf1a258c2d9a7de0568bc0067af985e";
constexpr std::size_t kProtocolBytes = 20061;
constexpr std::string_view kProtocolSidecarSha =
    "258ef89c09ef6db995281e1c40c681a537e1fe9f985c88c6644bc285136ff2e0";
constexpr std::size_t kProtocolSidecarBytes = 115;
constexpr std::string_view kAmendmentSha =
    "03ba84fe2fa318594c2da9812aebda1d9370008e0b37ad24159388e1213c0d73";
constexpr std::size_t kAmendmentBytes = 2313;
constexpr std::string_view kAmendmentSidecarSha =
    "abb2d48fffefac920056b91fa6d2a112e56aa7a87bfbd30aa1840b179d4679a4";
constexpr std::size_t kAmendmentSidecarBytes = 128;
constexpr std::string_view kAmendmentA2Sha =
    "5cc7e519c25899d309b76df32bd15e5a24cb731a3eafc9f269ea3905eea84f11";
constexpr std::size_t kAmendmentA2Bytes = 5031;
constexpr std::string_view kAmendmentA2SidecarSha =
    "1d8091c1b538d63cb15dd90f1f5d7543f57d9685474cd52d867e03a59778981d";
constexpr std::size_t kAmendmentA2SidecarBytes = 128;
constexpr std::string_view kAmendmentA3Sha =
    "0b7cbde36a46bbade2366e5e42fcd7cbb40345766d6c9841178501cc501f6990";
constexpr std::size_t kAmendmentA3Bytes = 10854;
constexpr std::string_view kAmendmentA3SidecarSha =
    "00410b8d68dc1baa3b689fdf9f1b876fc75d0c923b07b967abba82b2c4390276";
constexpr std::size_t kAmendmentA3SidecarBytes = 128;
constexpr std::string_view kA2PrerunManifestPath =
    ".build/tests/representation_srr2_v1_prerun_a2.sha256";
constexpr std::string_view kA2PrerunManifestSha =
    "339edec05bd1d5ae686532bf9c44a1c27e3b2e664ea6df1d682059b6157ab613";
constexpr std::size_t kA2PrerunManifestBytes = 12737;
constexpr std::string_view kA2AuthoritativeLogPath =
    ".build/tests/representation_srr2_v1_authoritative.log";
constexpr std::string_view kA2AuthoritativeLogSha =
    "c60de68c496bd43a73ea7a327264a605eb3d4755df9af97b713ffe0916846768";
constexpr std::size_t kA2AuthoritativeLogBytes = 647;
constexpr std::string_view kContainerIdentitySha =
    "18370d88848d6236c14d67ff2322863b7b51925436c940c4067a5db271eb308c";
constexpr std::size_t kContainerIdentityBytes = 217;
constexpr std::string_view kBaselineSha =
    "22f53f452836583f5402d1a28d67c9b3a9ac865094444a06b9726fd0f1c7b6dd";
constexpr std::size_t kBaselineBytes = 829440;
constexpr std::string_view kParentLogSha =
    "f38c99ef1294dab5f40f57fff79a958cd214c593eedd10284531976cda20ae6a";
constexpr std::size_t kParentLogBytes = 7324951;
constexpr std::size_t kExpectedAuthoritativeKeyCount = 4541;
constexpr std::string_view kExpectedAuthoritativeKeysetSha =
    "24ab25ff06abb36a6cd59b4bbc8debc0404473b9cbd2227d3d2bb099e5d4d470";
constexpr std::string_view kAttemptLedgerPath =
    ".build/tests/representation_srr2_v1_attempt.lock";
constexpr std::string_view kAuthoritativeLogPath =
    ".build/tests/representation_srr2_v1_authoritative_a3.log";
constexpr std::string_view kParityBinaryPath =
    ".build/tests/quality_wikimyei_mtf_jepa_mae_vicreg_production_"
    "structured_readout_parity";
constexpr std::string_view kParitySourcePath =
    "src/tests/bench/wikimyei/representation/encoding/mtf_jepa_mae_vicreg/"
    "quality_wikimyei_mtf_jepa_mae_vicreg_production_structured_readout_"
    "parity.cpp";
constexpr std::size_t kParentQualityTuBytes = 552065;
constexpr std::string_view kParentQualityTuSha =
    "14ae77e2fcada70f45c2f14e69e7693db96716199d22fab28134fccb79248a56";
constexpr std::string_view kExpectedCommand =
    "docker exec unnamed_taoist /bin/bash -lc 'cd /cuwacunu && "
    "set -o noclobber && CUBLAS_WORKSPACE_CONFIG=:4096:8 "
    "./.build/tests/quality_wikimyei_mtf_"
    "jepa_mae_vicreg_production_structured_readout_parity --experiment "
    "production-structured-readout-parity --device cuda > .build/tests/"
    "representation_srr2_v1_authoritative_a3.log 2>&1'";
constexpr std::string_view kBuildCommand =
    "docker exec unnamed_taoist /bin/bash -lc 'cd /cuwacunu && make -C "
    "src/tests/bench/wikimyei/representation/encoding/mtf_jepa_mae_vicreg "
    "srr2-screen'";
constexpr std::string_view kMechanicsCommand =
    "docker exec unnamed_taoist /bin/bash -lc 'cd /cuwacunu && make -C "
    "src/tests/bench/wikimyei/representation/encoding/mtf_jepa_mae_vicreg "
    "run-srr2-mechanics'";
constexpr std::string_view kPreflightCommand =
    "docker exec unnamed_taoist /bin/bash -lc 'cd /cuwacunu && "
    "CUBLAS_WORKSPACE_CONFIG=:4096:8 ./.build/tests/quality_wikimyei_mtf_"
    "jepa_mae_vicreg_production_structured_readout_parity --experiment "
    "production-structured-readout-parity-preflight --device cuda > "
    ".build/tests/representation_srr2_v1_preflight.log 2>&1'";
constexpr std::string_view kAuditCommand =
    "docker exec unnamed_taoist /bin/bash -lc 'cd /cuwacunu && "
    "./.build/tests/test_production_structured_readout_parity_log_auditor > "
    ".build/tests/representation_srr2_v1_audit.log 2>&1'";
constexpr std::string_view kCellVector =
    "0,0,1,1,1,2,2,3,4,5,6,7,8,8,9,9,9,10,10,11,12,13,14,15";
constexpr std::string_view kFrozenLayoutHash = "741bf0dc9bc7fe3f";
constexpr std::string_view kAuthorizationTail =
    "training_authorized=false\n"
    "augmentation_change_authorized=false\n"
    "long_run_authorized=false\n"
    "active_policy_change_authorized=false\n"
    "checkpoint_migration_authorized=false\n"
    "downstream_retraining_authorized=false\n"
    "end_to_end_authorized=false\n"
    "deployment_authorized=false\n";

struct ArtifactSpec {
  const char *path;
  std::size_t bytes;
  const char *sha;
};

struct ManifestEntry {
  std::string digest{};
  std::uint64_t bytes{0};
};

constexpr std::array<ArtifactSpec, 8> kParentArtifacts{{
    {"src/tests/bench/wikimyei/representation/encoding/mtf_jepa_mae_"
     "vicreg/STRUCTURED_READOUT_REPAIR_PLAN.md",
     14893, "1f976d5da5a79323a8fce011b0b33e53b277517bd785b2fdea68aa1888338127"},
    {"src/tests/bench/wikimyei/representation/encoding/mtf_jepa_mae_"
     "vicreg/STRUCTURED_READOUT_REPAIR_PROTOCOL.md",
     21848, "ad7c9381d58a23e8f3cec27b59b44e6532aa561227ad22d57578cc6ba0a04946"},
    {"src/tests/bench/wikimyei/representation/encoding/mtf_jepa_mae_"
     "vicreg/STRUCTURED_READOUT_REPAIR_PROTOCOL.sha256",
     104, "3b97b7431e34c3b875365bad27d0b9de67a5b7fd7007760eb34ab97b125140c5"},
    {"src/tests/bench/wikimyei/representation/encoding/mtf_jepa_mae_"
     "vicreg/STRUCTURED_READOUT_REPAIR_FINDINGS.md",
     10393, "b5b3458953f2f967a0229ea910c80810303d2e709bd6d9f237966c0e6b456c6a"},
    {".build/tests/representation_srr_v1_prerun.sha256", 7166,
     "515c9c8a851b3aceb03c160e5c9c19fff5265774d51eb396c6d56123cf0d3acb"},
    {".build/tests/representation_srr_v1_authoritative.log", 7324951,
     "f38c99ef1294dab5f40f57fff79a958cd214c593eedd10284531976cda20ae6a"},
    {".build/tests/representation_srr_v1_audit.log", 2964,
     "fe943fb2aa8ad26f53953364181f7c2b452692fde17643c5a8d94ca45c9bb841"},
    {".build/tests/representation_srr_v1_receipt.sha256", 3517,
     "994be46cab5c4bbabf3b72ed30e5fa1a8ece9247722e16ae504b428dcd0fc207"},
}};

struct Records {
  std::map<std::string, std::string> values{};
  std::set<std::string> accessed{};
  std::vector<std::string> ordered_keys{};
  std::vector<std::string> errors{};
  std::size_t machine_lines{0};
  std::size_t duplicate_keys{0};
  std::size_t malformed_lines{0};
  std::size_t runtime_noise_lines{0};
  std::size_t runtime_initializing_lines{0};
  std::size_t runtime_finalizing_lines{0};
  std::size_t nonfinite_values{0};
  std::size_t critical_value_errors{0};
  std::size_t unaccessed_keys{0};
};

[[nodiscard]] bool valid_key(std::string_view key) {
  if (key.empty() || !((key.front() >= 'a' && key.front() <= 'z') ||
                       (key.front() >= 'A' && key.front() <= 'Z'))) {
    return false;
  }
  return std::all_of(key.begin(), key.end(), [](char value) {
    return (value >= 'a' && value <= 'z') || (value >= 'A' && value <= 'Z') ||
           (value >= '0' && value <= '9') || value == '_' || value == '.';
  });
}

[[nodiscard]] bool parse_double(std::string_view text, double &value) {
  if (text.empty()) {
    return false;
  }
  std::string owned(text);
  char *end = nullptr;
  errno = 0;
  value = std::strtod(owned.c_str(), &end);
  return end == owned.c_str() + owned.size() && end != owned.c_str() &&
         errno != ERANGE;
}

[[nodiscard]] bool parse_canonical_unsigned(std::string_view text,
                                            std::uint64_t &value) {
  if (text.empty() || (text.size() > 1 && text.front() == '0')) {
    return false;
  }
  const auto parsed =
      std::from_chars(text.data(), text.data() + text.size(), value);
  return parsed.ec == std::errc{} && parsed.ptr == text.data() + text.size();
}

[[nodiscard]] bool runtime_noise(std::string_view line) {
  return line.find("[source_runtime_t] initializing static-global source "
                   "snapshot") != std::string_view::npos ||
         line.find("[source_runtime_t] finalizing static-global source "
                   "snapshot") != std::string_view::npos;
}

[[nodiscard]] bool runtime_initializer(std::string_view line) {
  return line.find("[source_runtime_t] initializing static-global source "
                   "snapshot") != std::string_view::npos;
}

[[nodiscard]] bool runtime_finalizer(std::string_view line) {
  return line.find("[source_runtime_t] finalizing static-global source "
                   "snapshot") != std::string_view::npos;
}

[[nodiscard]] Records parse_records(std::string_view raw,
                                    bool reject_nonmachine) {
  Records records{};
  std::size_t begin = 0;
  while (begin < raw.size()) {
    const std::size_t newline = raw.find('\n', begin);
    const std::size_t end =
        newline == std::string_view::npos ? raw.size() : newline;
    std::string_view line = raw.substr(begin, end - begin);
    if (!line.empty() && line.back() == '\r') {
      ++records.malformed_lines;
      if (reject_nonmachine) {
        records.errors.push_back("CR byte in machine receipt");
      }
      line.remove_suffix(1);
    }
    const std::size_t separator = line.find('=');
    if (separator != std::string_view::npos &&
        valid_key(line.substr(0, separator))) {
      ++records.machine_lines;
      const std::string key(line.substr(0, separator));
      const std::string value(line.substr(separator + 1));
      records.ordered_keys.push_back(key);
      if (!records.values.emplace(key, value).second) {
        ++records.duplicate_keys;
        records.errors.push_back("duplicate key: " + key);
      }
      double numeric = 0.0;
      if (parse_double(value, numeric) && !std::isfinite(numeric)) {
        ++records.nonfinite_values;
        records.errors.push_back("non-finite value: " + key);
      }
    } else if (!line.empty() && runtime_noise(line)) {
      ++records.runtime_noise_lines;
      records.runtime_initializing_lines += runtime_initializer(line) ? 1 : 0;
      records.runtime_finalizing_lines += runtime_finalizer(line) ? 1 : 0;
    } else if (!line.empty()) {
      ++records.malformed_lines;
      if (reject_nonmachine) {
        records.errors.push_back("malformed non-machine line");
      }
    } else {
      ++records.malformed_lines;
      if (reject_nonmachine) {
        records.errors.push_back("empty line in machine receipt");
      }
    }
    if (newline == std::string_view::npos) {
      break;
    }
    begin = newline + 1;
  }
  return records;
}

[[nodiscard]] bool authorization_machine_tail_exact(std::string_view raw,
                                                    const Records &records) {
  constexpr std::array<std::string_view, 8> tail_keys{
      "training_authorized",
      "augmentation_change_authorized",
      "long_run_authorized",
      "active_policy_change_authorized",
      "checkpoint_migration_authorized",
      "downstream_retraining_authorized",
      "end_to_end_authorized",
      "deployment_authorized"};
  if (records.ordered_keys.size() < tail_keys.size()) {
    return false;
  }
  const std::size_t tail_begin = records.ordered_keys.size() - tail_keys.size();
  for (std::size_t index = 0; index < tail_keys.size(); ++index) {
    if (records.ordered_keys[tail_begin + index] != tail_keys[index]) {
      return false;
    }
    const auto value = records.values.find(std::string(tail_keys[index]));
    if (value == records.values.end() || value->second != "false") {
      return false;
    }
  }

  return raw.ends_with(kAuthorizationTail) &&
         records.runtime_initializing_lines == 1 &&
         records.runtime_finalizing_lines == 0;
}

void fail(Records &records, const std::string &message) {
  records.errors.push_back(message);
}

[[nodiscard]] const std::string &required(Records &records,
                                          const std::string &key) {
  records.accessed.insert(key);
  const auto found = records.values.find(key);
  if (found == records.values.end()) {
    fail(records, "missing key: " + key);
    ++records.critical_value_errors;
    static const std::string empty{};
    return empty;
  }
  return found->second;
}

[[nodiscard]] bool boolean(Records &records, const std::string &key) {
  const auto &value = required(records, key);
  if (value == "true") {
    return true;
  }
  if (value != "false") {
    fail(records, "invalid boolean: " + key + "=" + value);
    ++records.critical_value_errors;
  }
  return false;
}

[[nodiscard]] std::uint64_t unsigned_number(Records &records,
                                            const std::string &key) {
  const auto &text = required(records, key);
  std::uint64_t value = 0;
  if (!parse_canonical_unsigned(text, value)) {
    fail(records, "invalid unsigned integer: " + key + "=" + text);
    ++records.critical_value_errors;
    return 0;
  }
  return value;
}

[[nodiscard]] double finite_number(Records &records, const std::string &key) {
  const auto &text = required(records, key);
  double value = 0.0;
  const bool decimal_characters =
      !text.empty() && text.front() != '-' && text.front() != '+' &&
      std::all_of(text.begin(), text.end(), [](char character) {
        return (character >= '0' && character <= '9') || character == '.' ||
               character == 'e' || character == 'E' || character == '+' ||
               character == '-';
      });
  if (!decimal_characters || !parse_double(text, value) ||
      !std::isfinite(value) || value < 0.0) {
    fail(records, "invalid finite number: " + key + "=" + text);
    ++records.critical_value_errors;
    return 0.0;
  }
  return value;
}

void expect_text(Records &records, const std::string &key,
                 std::string_view expected) {
  const auto &observed = required(records, key);
  if (observed != expected) {
    fail(records, "unexpected text: " + key + "=" + observed +
                      " expected=" + std::string(expected));
  }
}

void expect_bool(Records &records, const std::string &key, bool expected) {
  if (boolean(records, key) != expected) {
    fail(records, "unexpected boolean: " + key);
  }
}

void expect_unsigned(Records &records, const std::string &key,
                     std::uint64_t expected) {
  if (unsigned_number(records, key) != expected) {
    fail(records, "unexpected integer: " + key);
  }
}

[[nodiscard]] bool lowercase_hex(std::string_view value, std::size_t size) {
  return value.size() == size &&
         std::all_of(value.begin(), value.end(), [](char item) {
           return (item >= '0' && item <= '9') || (item >= 'a' && item <= 'f');
         });
}

[[nodiscard]] std::string hash16(Records &records, const std::string &key) {
  const auto &value = required(records, key);
  if (!lowercase_hex(value, 16)) {
    fail(records, "invalid 16-hex hash: " + key + "=" + value);
    ++records.critical_value_errors;
  }
  return value;
}

[[nodiscard]] std::string hash64(Records &records, const std::string &key) {
  const auto &value = required(records, key);
  if (!lowercase_hex(value, 64)) {
    fail(records, "invalid 64-hex hash: " + key + "=" + value);
    ++records.critical_value_errors;
  }
  return value;
}

class Sha256 {
public:
  void update(const std::uint8_t *data, std::size_t size) {
    for (std::size_t index = 0; index < size; ++index) {
      block_[block_size_++] = data[index];
      if (block_size_ == block_.size()) {
        transform(block_.data());
        bit_count_ += 512;
        block_size_ = 0;
      }
    }
  }

  [[nodiscard]] std::string finish() {
    bit_count_ += static_cast<std::uint64_t>(block_size_) * 8ULL;
    block_[block_size_++] = 0x80;
    if (block_size_ > 56) {
      while (block_size_ < 64) {
        block_[block_size_++] = 0;
      }
      transform(block_.data());
      block_size_ = 0;
    }
    while (block_size_ < 56) {
      block_[block_size_++] = 0;
    }
    for (int shift = 56; shift >= 0; shift -= 8) {
      block_[block_size_++] =
          static_cast<std::uint8_t>((bit_count_ >> shift) & 0xffU);
    }
    transform(block_.data());
    std::ostringstream output;
    output << std::hex << std::setfill('0');
    for (const std::uint32_t value : state_) {
      output << std::setw(8) << value;
    }
    return output.str();
  }

private:
  [[nodiscard]] static std::uint32_t rotate(std::uint32_t value,
                                            unsigned bits) {
    return (value >> bits) | (value << (32U - bits));
  }

  void transform(const std::uint8_t *block) {
    static constexpr std::array<std::uint32_t, 64> constants{
        0x428a2f98U, 0x71374491U, 0xb5c0fbcfU, 0xe9b5dba5U, 0x3956c25bU,
        0x59f111f1U, 0x923f82a4U, 0xab1c5ed5U, 0xd807aa98U, 0x12835b01U,
        0x243185beU, 0x550c7dc3U, 0x72be5d74U, 0x80deb1feU, 0x9bdc06a7U,
        0xc19bf174U, 0xe49b69c1U, 0xefbe4786U, 0x0fc19dc6U, 0x240ca1ccU,
        0x2de92c6fU, 0x4a7484aaU, 0x5cb0a9dcU, 0x76f988daU, 0x983e5152U,
        0xa831c66dU, 0xb00327c8U, 0xbf597fc7U, 0xc6e00bf3U, 0xd5a79147U,
        0x06ca6351U, 0x14292967U, 0x27b70a85U, 0x2e1b2138U, 0x4d2c6dfcU,
        0x53380d13U, 0x650a7354U, 0x766a0abbU, 0x81c2c92eU, 0x92722c85U,
        0xa2bfe8a1U, 0xa81a664bU, 0xc24b8b70U, 0xc76c51a3U, 0xd192e819U,
        0xd6990624U, 0xf40e3585U, 0x106aa070U, 0x19a4c116U, 0x1e376c08U,
        0x2748774cU, 0x34b0bcb5U, 0x391c0cb3U, 0x4ed8aa4aU, 0x5b9cca4fU,
        0x682e6ff3U, 0x748f82eeU, 0x78a5636fU, 0x84c87814U, 0x8cc70208U,
        0x90befffaU, 0xa4506cebU, 0xbef9a3f7U, 0xc67178f2U};
    std::array<std::uint32_t, 64> words{};
    for (std::size_t index = 0; index < 16; ++index) {
      const std::size_t offset = 4 * index;
      words[index] = static_cast<std::uint32_t>(block[offset]) << 24U |
                     static_cast<std::uint32_t>(block[offset + 1]) << 16U |
                     static_cast<std::uint32_t>(block[offset + 2]) << 8U |
                     static_cast<std::uint32_t>(block[offset + 3]);
    }
    for (std::size_t index = 16; index < words.size(); ++index) {
      const std::uint32_t s0 = rotate(words[index - 15], 7) ^
                               rotate(words[index - 15], 18) ^
                               (words[index - 15] >> 3U);
      const std::uint32_t s1 = rotate(words[index - 2], 17) ^
                               rotate(words[index - 2], 19) ^
                               (words[index - 2] >> 10U);
      words[index] = words[index - 16] + s0 + words[index - 7] + s1;
    }
    std::uint32_t a = state_[0], b = state_[1], c = state_[2], d = state_[3];
    std::uint32_t e = state_[4], f = state_[5], g = state_[6], h = state_[7];
    for (std::size_t index = 0; index < words.size(); ++index) {
      const std::uint32_t s1 = rotate(e, 6) ^ rotate(e, 11) ^ rotate(e, 25);
      const std::uint32_t choose = (e & f) ^ (~e & g);
      const std::uint32_t t1 =
          h + s1 + choose + constants[index] + words[index];
      const std::uint32_t s0 = rotate(a, 2) ^ rotate(a, 13) ^ rotate(a, 22);
      const std::uint32_t majority = (a & b) ^ (a & c) ^ (b & c);
      const std::uint32_t t2 = s0 + majority;
      h = g;
      g = f;
      f = e;
      e = d + t1;
      d = c;
      c = b;
      b = a;
      a = t1 + t2;
    }
    state_[0] += a;
    state_[1] += b;
    state_[2] += c;
    state_[3] += d;
    state_[4] += e;
    state_[5] += f;
    state_[6] += g;
    state_[7] += h;
  }

  std::array<std::uint32_t, 8> state_{0x6a09e667U, 0xbb67ae85U, 0x3c6ef372U,
                                      0xa54ff53aU, 0x510e527fU, 0x9b05688cU,
                                      0x1f83d9abU, 0x5be0cd19U};
  std::array<std::uint8_t, 64> block_{};
  std::size_t block_size_{0};
  std::uint64_t bit_count_{0};
};

[[nodiscard]] std::string sha256(std::string_view bytes) {
  Sha256 hash;
  hash.update(reinterpret_cast<const std::uint8_t *>(bytes.data()),
              bytes.size());
  return hash.finish();
}

[[nodiscard]] std::string read_bytes(const std::filesystem::path &path) {
  std::ifstream input(path, std::ios::binary);
  if (!input.is_open()) {
    throw std::runtime_error("cannot open file: " + path.string());
  }
  return {std::istreambuf_iterator<char>(input),
          std::istreambuf_iterator<char>()};
}

[[nodiscard]] std::filesystem::path
find_repository_root(const std::filesystem::path &executable) {
  std::filesystem::path candidate =
      std::filesystem::canonical(executable).parent_path();
  while (!candidate.empty()) {
    if (std::filesystem::exists(
            candidate / "src/tests/bench/wikimyei/representation/encoding/"
                        "mtf_jepa_mae_vicreg/"
                        "PRODUCTION_STRUCTURED_READOUT_PARITY_PROTOCOL.md") &&
        std::filesystem::exists(candidate / ".build/tests")) {
      return std::filesystem::canonical(candidate);
    }
    const auto parent = candidate.parent_path();
    if (parent == candidate) {
      break;
    }
    candidate = parent;
  }
  candidate = std::filesystem::current_path();
  while (!candidate.empty()) {
    if (std::filesystem::exists(
            candidate / "src/tests/bench/wikimyei/representation/encoding/"
                        "mtf_jepa_mae_vicreg/"
                        "PRODUCTION_STRUCTURED_READOUT_PARITY_PROTOCOL.md")) {
      return std::filesystem::canonical(candidate);
    }
    const auto parent = candidate.parent_path();
    if (parent == candidate) {
      break;
    }
    candidate = parent;
  }
  throw std::runtime_error("cannot locate cuwacunu repository root");
}

[[nodiscard]] bool path_is_within(const std::filesystem::path &root,
                                  const std::filesystem::path &target) {
  const auto relative = target.lexically_relative(root);
  return !relative.empty() && !relative.is_absolute() &&
         *relative.begin() != "..";
}

[[nodiscard]] bool source_boundary_exact(const std::filesystem::path &root) {
  const std::string production =
      read_bytes(root / "src/include/wikimyei/representation/encoding/"
                        "mtf_jepa_mae_vicreg/mtf_jepa_mae_vicreg.h");
  const std::string shadow =
      read_bytes(root / "src/tests/bench/wikimyei/representation/encoding/"
                        "mtf_jepa_mae_vicreg/structured_readout_shadow.h");
  return production.find("structured_readout_shadow") == std::string::npos &&
         production.find("test_structured_readout") == std::string::npos &&
         production.find("structured_cdsb_v1") != std::string::npos &&
         shadow.find("structured_cdsb_v1") == std::string::npos &&
         shadow.find("select_mtf_serving_pool(") == std::string::npos &&
         shadow.find("structured_cdsb_v1_make_q0_cpu64") == std::string::npos &&
         shadow.find("structured_cdsb_v1_make_qpsm_cpu64") ==
             std::string::npos &&
         shadow.find("structured_cdsb_v1_projection_for") == std::string::npos;
}

struct BuildReceiptEvidence {
  bool exact{false};
  std::size_t bytes{0};
  std::string digest{};
  std::vector<std::string> errors{};
};

struct CandidatePatchEvidence {
  bool exact{false};
  std::vector<std::string> errors{};
};

[[nodiscard]] CandidatePatchEvidence verify_candidate_patch(
    const std::filesystem::path &root,
    const std::map<std::string, ManifestEntry> &manifest_entries) {
  CandidatePatchEvidence evidence{};
  constexpr std::string_view patch_path =
      ".build/tests/representation_srr2_v1_candidate.patch";
  constexpr std::string_view command =
      "bash /cuwacunu/src/tests/bench/wikimyei/representation/encoding/"
      "mtf_jepa_mae_vicreg/"
      "verify_production_structured_readout_parity_candidate_patch.sh "
      "--verify 2>&1";
  std::string output;
  FILE *process = ::popen(std::string(command).c_str(), "r");
  if (process == nullptr) {
    evidence.errors.push_back("cannot launch candidate-patch verifier");
    return evidence;
  }
  std::array<char, 4096> buffer{};
  while (std::fgets(buffer.data(), static_cast<int>(buffer.size()), process) !=
         nullptr) {
    output.append(buffer.data());
  }
  const int status = ::pclose(process);
  if (status == -1 || !WIFEXITED(status) || WEXITSTATUS(status) != 0) {
    evidence.errors.push_back("candidate-patch verifier did not exit zero");
    if (!output.empty()) {
      evidence.errors.push_back("candidate-patch verifier emitted failure "
                                "output");
    }
    return evidence;
  }

  Records receipt = parse_records(output, true);
  expect_text(receipt, "schema",
              "wikimyei.mtf_jepa_mae_vicreg.srr2_candidate_patch.v1");
  expect_text(receipt, "srr2.candidate_patch.path", patch_path);
  const std::string observed_digest =
      hash64(receipt, "srr2.candidate_patch.sha256");
  const auto observed_bytes =
      unsigned_number(receipt, "srr2.candidate_patch.bytes");
  expect_unsigned(receipt, "srr2.candidate_patch.baseline_entry_count", 14);
  expect_unsigned(receipt, "srr2.candidate_patch.new_entry_count", 17);
  expect_bool(receipt, "srr2.candidate_patch.apply_exact", true);
  expect_bool(receipt, "srr2.candidate_patch.live_tree_exact", true);
  expect_bool(receipt, "srr2.candidate_patch.pass", true);
  try {
    const auto manifest = manifest_entries.find(std::string(patch_path));
    const std::string live = read_bytes(root / patch_path);
    if (manifest == manifest_entries.end() || observed_bytes == 0 ||
        observed_bytes != live.size() || observed_digest != sha256(live) ||
        observed_bytes != manifest->second.bytes ||
        observed_digest != manifest->second.digest) {
      fail(receipt, "candidate-patch receipt custody mismatch");
    }
  } catch (const std::exception &error) {
    fail(receipt, std::string("candidate-patch live verification failed: ") +
                      error.what());
  }
  for (const auto &[key, value] : receipt.values) {
    (void)value;
    if (receipt.accessed.count(key) == 0) {
      ++receipt.unaccessed_keys;
      fail(receipt, "unrecognized candidate-patch receipt key: " + key);
    }
  }
  evidence.exact =
      receipt.errors.empty() && receipt.duplicate_keys == 0 &&
      receipt.malformed_lines == 0 && receipt.runtime_noise_lines == 0 &&
      receipt.nonfinite_values == 0 && receipt.critical_value_errors == 0 &&
      receipt.unaccessed_keys == 0;
  evidence.errors = std::move(receipt.errors);
  return evidence;
}

[[nodiscard]] BuildReceiptEvidence verify_build_receipt(
    const std::filesystem::path &root,
    const std::map<std::string, ManifestEntry> &manifest_entries) {
  BuildReceiptEvidence evidence{};
  constexpr std::array<std::pair<std::string_view, std::string_view>, 9>
      binaries{{
          {"production", ".build/tests/test_production_structured_readout"},
          {"shadow", ".build/tests/test_structured_readout_shadow"},
          {"gate",
           ".build/tests/test_production_structured_readout_parity_gate"},
          {"auditor", ".build/tests/"
                      "test_production_structured_readout_parity_log_auditor"},
          {"parity",
           ".build/tests/quality_wikimyei_mtf_jepa_mae_vicreg_production_"
           "structured_readout_parity"},
          {"core", ".build/tests/test_wikimyei_mtf_jepa_mae_vicreg"},
          {"contracts",
           ".build/tests/test_wikimyei_mtf_jepa_mae_vicreg_contracts"},
          {"config", ".build/tests/test_wikimyei_graph_first_specs"},
          {"adapter",
           ".build/tests/test_jkimyei_channel_graph_first_launchers"},
      }};
  try {
    const auto receipt_path =
        root / ".build/tests/representation_srr2_v1_build_receipt.txt";
    const std::string raw = read_bytes(receipt_path);
    evidence.bytes = raw.size();
    evidence.digest = sha256(raw);
    Records receipt = parse_records(raw, true);
    expect_text(receipt, "schema",
                "wikimyei.mtf_jepa_mae_vicreg.srr2_build_receipt.v1");
    expect_text(receipt, "build_command", kBuildCommand);
    for (const auto &[label, expected_path] : binaries) {
      const std::string stem = "srr2.build." + std::string(label) + ".";
      expect_text(receipt, stem + "path", expected_path);
      const auto observed_bytes = unsigned_number(receipt, stem + "bytes");
      const std::string observed_digest = hash64(receipt, stem + "sha256");
      const auto manifest = manifest_entries.find(std::string(expected_path));
      if (manifest == manifest_entries.end()) {
        fail(receipt, "build receipt binary is absent from manifest: " +
                          std::string(expected_path));
        continue;
      }
      const std::string live = read_bytes(root / expected_path);
      if (observed_bytes == 0 || observed_bytes != live.size() ||
          observed_digest != sha256(live) ||
          observed_bytes != manifest->second.bytes ||
          observed_digest != manifest->second.digest) {
        fail(receipt,
             "build receipt binary custody mismatch: " + std::string(label));
      }
    }
    expect_unsigned(receipt, "srr2.build.binary_count", binaries.size());
    expect_bool(receipt, "srr2.build.pass", true);
    for (const auto &[key, value] : receipt.values) {
      (void)value;
      if (receipt.accessed.count(key) == 0) {
        ++receipt.unaccessed_keys;
        fail(receipt, "unrecognized build receipt key: " + key);
      }
    }
    evidence.exact =
        receipt.errors.empty() && receipt.duplicate_keys == 0 &&
        receipt.malformed_lines == 0 && receipt.runtime_noise_lines == 0 &&
        receipt.nonfinite_values == 0 && receipt.critical_value_errors == 0 &&
        receipt.unaccessed_keys == 0;
    evidence.errors = std::move(receipt.errors);
  } catch (const std::exception &error) {
    evidence.errors.push_back(
        std::string("build receipt verification failed: ") + error.what());
  }
  return evidence;
}

struct ManifestEvidence {
  bool exact{false};
  bool metadata_exact{false};
  bool live_entries_exact{false};
  bool mandatory_entries_present{false};
  bool containment_exact{false};
  bool protocol_exact{false};
  bool amendment_exact{false};
  bool amendment_a2_exact{false};
  bool amendment_a3_exact{false};
  bool a2_incident_exact{false};
  bool baseline_exact{false};
  bool candidate_delta_exact{false};
  bool auditor_binary_exact{false};
  bool runtime_binding_exact{false};
  bool container_identity_exact{false};
  bool build_receipt_exact{false};
  bool parent_quality_source_exact{false};
  bool attempt_ledger_preseal_contract_exact{false};
  bool authoritative_log_preseal_contract_exact{false};
  bool mechanics_log_exact{false};
  bool preflight_log_exact{false};
  std::string token_layout_hash{};
  std::size_t bytes{0};
  std::string digest{};
  std::size_t entry_count{0};
  std::size_t error_count{0};
  std::string container_id{};
  std::map<std::string, ManifestEntry> entries{};
};

[[nodiscard]] ManifestEvidence
verify_manifest(Records &records, const std::filesystem::path &root,
                const std::filesystem::path &executable) {
  ManifestEvidence evidence{};
  const std::size_t errors_before = records.errors.size();
  const auto path = root / ".build/tests/representation_srr2_v1_prerun.sha256";
  std::string raw;
  try {
    const auto status = std::filesystem::symlink_status(path);
    if (!std::filesystem::is_regular_file(status) ||
        std::filesystem::is_symlink(status) ||
        std::filesystem::hard_link_count(path) != 1 ||
        std::filesystem::canonical(path) != path ||
        !path_is_within(root, std::filesystem::canonical(path))) {
      throw std::runtime_error(
          "SRR-2 manifest is not a contained single-link regular file");
    }
    raw = read_bytes(path);
  } catch (const std::exception &error) {
    fail(records, std::string("cannot read SRR-2 manifest: ") + error.what());
    evidence.error_count = records.errors.size() - errors_before;
    return evidence;
  }
  evidence.bytes = raw.size();
  evidence.digest = sha256(raw);
  const auto logged_manifest_bytes =
      unsigned_number(records, "srr2.prerun_manifest.bytes");
  const auto logged_manifest_digest =
      hash64(records, "srr2.prerun_manifest.sha256");
  evidence.runtime_binding_exact = logged_manifest_bytes == evidence.bytes &&
                                   logged_manifest_digest == evidence.digest;
  if (!evidence.runtime_binding_exact) {
    fail(records, "authoritative manifest bytes/hash binding failed");
  }

  std::map<std::string, std::string> metadata;
  std::map<std::string, ManifestEntry> entries;
  bool syntax_exact = true;
  bool entries_started = false;
  if (raw.empty() || raw.back() != '\n') {
    syntax_exact = false;
    fail(records, "SRR-2 manifest must end in a single LF-terminated record");
  }
  std::string previous_entry;
  std::size_t begin = 0;
  while (begin < raw.size()) {
    const std::size_t newline = raw.find('\n', begin);
    const std::size_t end = newline == std::string::npos ? raw.size() : newline;
    std::string_view line(raw.data() + begin, end - begin);
    if (!line.empty() && line.back() == '\r') {
      syntax_exact = false;
      fail(records, "SRR-2 manifest contains a CR byte");
      line.remove_suffix(1);
    }
    if (line.rfind("# ", 0) == 0) {
      if (entries_started) {
        syntax_exact = false;
        fail(records, "SRR-2 manifest metadata follows an entry");
      }
      line.remove_prefix(2);
      const auto separator = line.find('=');
      if (separator == std::string_view::npos ||
          !valid_key(line.substr(0, separator))) {
        syntax_exact = false;
        fail(records, "malformed SRR-2 manifest metadata");
      } else if (!metadata
                      .emplace(std::string(line.substr(0, separator)),
                               std::string(line.substr(separator + 1)))
                      .second) {
        syntax_exact = false;
        fail(records, "duplicate SRR-2 manifest metadata");
      }
    } else if (!line.empty()) {
      entries_started = true;
      if (line.size() <= 69 || !lowercase_hex(line.substr(0, 64), 64) ||
          line.substr(64, 2) != "  ") {
        syntax_exact = false;
        fail(records, "malformed SRR-2 manifest entry");
      } else {
        const std::string digest(line.substr(0, 64));
        const auto second_separator = line.find("  ", 66);
        std::uint64_t byte_length = 0;
        if (second_separator == std::string_view::npos ||
            !parse_canonical_unsigned(line.substr(66, second_separator - 66),
                                      byte_length)) {
          syntax_exact = false;
          fail(records, "malformed SRR-2 manifest byte length");
          if (newline == std::string::npos) {
            break;
          }
          begin = newline + 1;
          continue;
        }
        const std::string relative(line.substr(second_separator + 2));
        const std::filesystem::path relative_path(relative);
        if (relative.empty() || relative_path.is_absolute() ||
            relative.find('\\') != std::string::npos ||
            relative_path.lexically_normal().generic_string() != relative ||
            (relative_path.extension() == ".patch" &&
             relative !=
                 ".build/tests/representation_srr2_v1_candidate.patch") ||
            (!previous_entry.empty() && relative <= previous_entry) ||
            !entries
                 .emplace(relative,
                          ManifestEntry{.digest = digest, .bytes = byte_length})
                 .second) {
          syntax_exact = false;
          fail(records, "invalid, unordered, or duplicate SRR-2 manifest "
                        "path: " +
                            relative);
        } else {
          previous_entry = relative;
        }
      }
    } else {
      syntax_exact = false;
      fail(records, "SRR-2 manifest contains an empty record");
    }
    if (newline == std::string::npos) {
      break;
    }
    begin = newline + 1;
  }
  evidence.entry_count = entries.size();

  const auto metadata_is = [&](const char *key, std::string_view expected) {
    const auto found = metadata.find(key);
    if (found == metadata.end() || found->second != expected) {
      fail(records, std::string("manifest metadata mismatch: ") + key);
      return false;
    }
    return true;
  };
  evidence.metadata_exact = metadata.size() == 73;
  if (metadata.size() != 73) {
    fail(records, "unexpected SRR-2 manifest metadata key count");
  }
  evidence.metadata_exact &= metadata_is("schema", kManifestSchema);
  evidence.metadata_exact &= metadata_is(
      "manifest_format", "sha256_two_spaces_bytes_two_spaces_path_v1");
  evidence.metadata_exact &=
      metadata_is("canonical_entry_order", "lexicographic");
  evidence.metadata_exact &= metadata_is("protocol_sha256", kProtocolSha);
  evidence.metadata_exact &=
      metadata_is("protocol_amendment_sha256", kAmendmentSha);
  evidence.metadata_exact &=
      metadata_is("protocol_amendment_bytes", std::to_string(kAmendmentBytes));
  evidence.metadata_exact &=
      metadata_is("protocol_amendment_a2_sha256", kAmendmentA2Sha);
  evidence.metadata_exact &= metadata_is("protocol_amendment_a2_bytes",
                                         std::to_string(kAmendmentA2Bytes));
  evidence.metadata_exact &=
      metadata_is("protocol_amendment_a3_sha256", kAmendmentA3Sha);
  evidence.metadata_exact &= metadata_is("protocol_amendment_a3_bytes",
                                         std::to_string(kAmendmentA3Bytes));
  evidence.metadata_exact &=
      metadata_is("a2_prerun_manifest_path", kA2PrerunManifestPath);
  evidence.metadata_exact &=
      metadata_is("a2_prerun_manifest_sha256", kA2PrerunManifestSha);
  evidence.metadata_exact &= metadata_is(
      "a2_prerun_manifest_bytes", std::to_string(kA2PrerunManifestBytes));
  evidence.metadata_exact &= metadata_is(
      "a2_reserved_authoritative_log_path", kA2AuthoritativeLogPath);
  evidence.metadata_exact &= metadata_is(
      "a2_reserved_authoritative_log_sha256", kA2AuthoritativeLogSha);
  evidence.metadata_exact &= metadata_is(
      "a2_reserved_authoritative_log_bytes",
      std::to_string(kA2AuthoritativeLogBytes));
  evidence.metadata_exact &= metadata_is(
      "a2_reserved_authoritative_log_attempt_consumed", "false");
  evidence.metadata_exact &=
      metadata_is("a2_reserved_authoritative_log_attempt_count", "0");
  evidence.metadata_exact &= metadata_is(
      "a2_reserved_authoritative_log_terminal_result", "invalid_mechanics");
  evidence.metadata_exact &=
      metadata_is("a2_reserved_attempt_ledger_absent", "true");
  evidence.metadata_exact &= metadata_is("scientific_attempt_limit", "1");
  evidence.metadata_exact &=
      metadata_is("attempt_ledger_path", kAttemptLedgerPath);
  evidence.metadata_exact &=
      metadata_is("attempt_ledger_preseal_absent", "true");
  evidence.metadata_exact &=
      metadata_is("authoritative_log_preseal_absent", "true");
  evidence.metadata_exact &= metadata_is("preflight_pass", "true");
  evidence.metadata_exact &=
      metadata_is("scientific_rows_used_before_seal", "false");
  evidence.metadata_exact &=
      metadata_is("projection_q0_hash", "f8c9f35282de2ee0");
  evidence.metadata_exact &=
      metadata_is("projection_qpsm_hash", "ac8a43fd65b2c8a8");
  evidence.metadata_exact &= metadata_is("production_baseline_bytes", "829440");
  evidence.metadata_exact &=
      metadata_is("production_baseline_sha256", kBaselineSha);
  evidence.metadata_exact &=
      metadata_is("authoritative_command", kExpectedCommand);
  evidence.metadata_exact &= metadata_is("build_command", kBuildCommand);
  evidence.metadata_exact &=
      metadata_is("mechanics_command", kMechanicsCommand);
  evidence.metadata_exact &=
      metadata_is("preflight_command", kPreflightCommand);
  evidence.metadata_exact &= metadata_is("audit_command", kAuditCommand);
  evidence.metadata_exact &= metadata_is("working_directory", "/cuwacunu");
  evidence.metadata_exact &= metadata_is("container_name", "unnamed_taoist");
  evidence.metadata_exact &= metadata_is("environment_device", "cuda:0");
  evidence.metadata_exact &= metadata_is("environment_dtype", "float32");
  evidence.metadata_exact &= metadata_is("cpu_threads", "1");
  evidence.metadata_exact &= metadata_is("cpu_interop_threads", "1");
  evidence.metadata_exact &= metadata_is("deterministic_algorithms", "true");
  evidence.metadata_exact &= metadata_is("deterministic_warn_only", "false");
  evidence.metadata_exact &= metadata_is("deterministic_cudnn", "true");
  evidence.metadata_exact &= metadata_is("tf32_cublas_disabled", "true");
  evidence.metadata_exact &= metadata_is("tf32_cudnn_disabled", "true");
  evidence.metadata_exact &= metadata_is("cublas_workspace_config", ":4096:8");
  evidence.metadata_exact &= metadata_is("seed_vector", "17,31,47");
  evidence.metadata_exact &= metadata_is(
      "dataset_vector",
      "probe_train,probe_validation,test,reversed_train,reversed_validation,"
      "reversed_test");
  evidence.metadata_exact &=
      metadata_is("dataset_row_vector", "256,128,256,256,128,256");
  evidence.metadata_exact &=
      metadata_is("dataset_group_begin_vector",
                  "1000000,2000000,3000000,1000000,2000000,3000000");
  evidence.metadata_exact &= metadata_is("dataset_reversed_vector",
                                         "false,false,false,true,true,true");
  evidence.metadata_exact &= metadata_is("cell_vector", kCellVector);
  evidence.metadata_exact &= metadata_is("expected_seed_count", "3");
  evidence.metadata_exact &= metadata_is("expected_dataset_count", "6");
  evidence.metadata_exact &=
      metadata_is("expected_retained_capture_count", "18");
  evidence.metadata_exact &= metadata_is("expected_repeat_capture_count", "18");
  evidence.metadata_exact &= metadata_is("expected_retained_row_count", "3840");
  evidence.metadata_exact &= metadata_is("expected_repeat_row_count", "3840");
  evidence.metadata_exact &=
      metadata_is("expected_retained_value_count", "368640");
  evidence.metadata_exact &=
      metadata_is("expected_repeat_value_count", "368640");
  evidence.metadata_exact &=
      metadata_is("expected_retained_validity_count", "11520");
  evidence.metadata_exact &=
      metadata_is("expected_repeat_validity_count", "11520");
  evidence.metadata_exact &=
      metadata_is("candidate_patch_path",
                  ".build/tests/representation_srr2_v1_candidate.patch");
  evidence.metadata_exact &= metadata_is(
      "baseline_path",
      ".build/tests/representation_srr2_v1_production_baseline.tar");
  evidence.metadata_exact &=
      metadata_is("mechanics_log_path",
                  ".build/tests/representation_srr2_v1_mechanics.log");
  evidence.metadata_exact &=
      metadata_is("preflight_log_path",
                  ".build/tests/representation_srr2_v1_preflight.log");
  evidence.metadata_exact &=
      metadata_is("authoritative_log_path", kAuthoritativeLogPath);

  std::string authoritative_keyset;
  for (const auto &[key, value] : records.values) {
    (void)value;
    authoritative_keyset += key;
    authoritative_keyset.push_back('\n');
  }
  const std::string observed_keyset_sha = sha256(authoritative_keyset);
  evidence.metadata_exact &=
      metadata_is("expected_authoritative_key_count",
                  std::to_string(kExpectedAuthoritativeKeyCount));
  evidence.metadata_exact &= metadata_is("expected_authoritative_keyset_sha256",
                                         kExpectedAuthoritativeKeysetSha);
  if (records.values.size() != kExpectedAuthoritativeKeyCount ||
      observed_keyset_sha != kExpectedAuthoritativeKeysetSha) {
    evidence.metadata_exact = false;
    fail(records, "authoritative key schema differs from frozen keyset");
  }

  const auto container = metadata.find("container_id");
  if (container == metadata.end() || !lowercase_hex(container->second, 64)) {
    evidence.metadata_exact = false;
    fail(records, "manifest container_id is missing or malformed");
  } else {
    evidence.container_id = container->second;
  }
  const auto image = metadata.find("image_id");
  if (image == metadata.end() || image->second.size() != 71 ||
      image->second.rfind("sha256:", 0) != 0 ||
      !lowercase_hex(std::string_view(image->second).substr(7), 64)) {
    evidence.metadata_exact = false;
    fail(records, "manifest image_id is missing or malformed");
  }
  try {
    std::string hostname = read_bytes("/etc/hostname");
    while (!hostname.empty() &&
           (hostname.back() == '\n' || hostname.back() == '\r' ||
            hostname.back() == ' ' || hostname.back() == '\t')) {
      hostname.pop_back();
    }
    if (container == metadata.end() || hostname.empty() ||
        container->second.rfind(hostname, 0) != 0 ||
        std::filesystem::canonical(std::filesystem::current_path()) !=
            std::filesystem::canonical(root) ||
        std::filesystem::canonical(root).generic_string() != "/cuwacunu") {
      evidence.metadata_exact = false;
      fail(records, "manifest container/working-directory binding failed");
    }
  } catch (const std::exception &) {
    evidence.metadata_exact = false;
    fail(records, "manifest container identity is not live-verifiable");
  }
  const auto layout = metadata.find("token_layout_hash");
  if (layout == metadata.end() || !lowercase_hex(layout->second, 16) ||
      layout->second != kFrozenLayoutHash) {
    evidence.metadata_exact = false;
    fail(records, "manifest token_layout_hash is missing or malformed");
  } else {
    evidence.token_layout_hash = layout->second;
    const auto logged_layout_hash = hash16(records, "srr2.layout.hash");
    if (logged_layout_hash != layout->second) {
      evidence.metadata_exact = false;
      fail(records, "authoritative layout hash differs from manifest");
    }
  }

  const std::set<std::string> mandatory{
      ".build/tests/representation_srr2_v1_production_baseline.tar",
      ".build/tests/representation_srr2_v1_candidate.patch",
      ".build/tests/representation_srr2_v1_mechanics.log",
      ".build/tests/representation_srr2_v1_preflight.log",
      ".build/tests/representation_srr2_v1_prerun_a2.sha256",
      ".build/tests/representation_srr2_v1_authoritative.log",
      ".build/tests/representation_srr2_v1_container_identity.txt",
      ".build/tests/representation_srr2_v1_build_receipt.txt",
      ".build/tests/test_structured_readout_shadow",
      ".build/tests/test_production_structured_readout",
      ".build/tests/test_production_structured_readout_parity_gate",
      ".build/tests/test_production_structured_readout_parity_log_auditor",
      ".build/tests/quality_wikimyei_mtf_jepa_mae_vicreg_production_"
      "structured_readout_parity",
      ".build/tests/test_wikimyei_mtf_jepa_mae_vicreg",
      ".build/tests/test_wikimyei_mtf_jepa_mae_vicreg_contracts",
      ".build/tests/test_wikimyei_graph_first_specs",
      ".build/tests/test_jkimyei_channel_graph_first_launchers",
      ".build/tests/representation_srr_v1_prerun.sha256",
      ".build/tests/representation_srr_v1_authoritative.log",
      ".build/tests/representation_srr_v1_audit.log",
      ".build/tests/representation_srr_v1_receipt.sha256",
      "src/tests/bench/wikimyei/representation/encoding/"
      "mtf_jepa_mae_vicreg/STRUCTURED_READOUT_REPAIR_PLAN.md",
      "src/tests/bench/wikimyei/representation/encoding/"
      "mtf_jepa_mae_vicreg/STRUCTURED_READOUT_REPAIR_PROTOCOL.md",
      "src/tests/bench/wikimyei/representation/encoding/"
      "mtf_jepa_mae_vicreg/STRUCTURED_READOUT_REPAIR_PROTOCOL.sha256",
      "src/tests/bench/wikimyei/representation/encoding/"
      "mtf_jepa_mae_vicreg/STRUCTURED_READOUT_REPAIR_FINDINGS.md",
      "src/config/README.md",
      "src/config/grammar/wikimyei.representation.mtf_jepa_mae_vicreg.dsl."
      "bnf",
      "src/config/man/wikimyei.config.man",
      "src/config/wikimyei.representation.mtf_jepa_mae_vicreg.dsl",
      "src/config/wikimyei.representation.mtf_jepa_mae_vicreg.net",
      "src/include/jkimyei/training/inference/"
      "channel_graph_first_inference_launcher.h",
      "src/include/kikijyeba/protocol/config_bundle.h",
      "src/include/wikimyei/README.md",
      "src/include/wikimyei/representation/encoding/mtf_jepa_mae_vicreg/"
      "README.md",
      "src/include/wikimyei/representation/encoding/mtf_jepa_mae_vicreg/"
      "mtf_jepa_mae_vicreg.h",
      "src/include/wikimyei/representation/encoding/mtf_jepa_mae_vicreg/"
      "mtf_jepa_mae_vicreg_spec.h",
      "src/tests/bench/wikimyei/representation/encoding/"
      "mtf_jepa_mae_vicreg/PRODUCTION_STRUCTURED_READOUT_PARITY_PLAN.md",
      "src/tests/bench/wikimyei/representation/encoding/"
      "mtf_jepa_mae_vicreg/PRODUCTION_STRUCTURED_READOUT_PARITY_PROTOCOL.md",
      "src/tests/bench/wikimyei/representation/encoding/"
      "mtf_jepa_mae_vicreg/PRODUCTION_STRUCTURED_READOUT_PARITY_PROTOCOL."
      "sha256",
      "src/tests/bench/wikimyei/representation/encoding/"
      "mtf_jepa_mae_vicreg/"
      "PRODUCTION_STRUCTURED_READOUT_PARITY_PROTOCOL_AMENDMENT_A1.md",
      "src/tests/bench/wikimyei/representation/encoding/"
      "mtf_jepa_mae_vicreg/"
      "PRODUCTION_STRUCTURED_READOUT_PARITY_PROTOCOL_AMENDMENT_A1.sha256",
      "src/tests/bench/wikimyei/representation/encoding/"
      "mtf_jepa_mae_vicreg/"
      "PRODUCTION_STRUCTURED_READOUT_PARITY_PROTOCOL_AMENDMENT_A2.md",
      "src/tests/bench/wikimyei/representation/encoding/"
      "mtf_jepa_mae_vicreg/"
      "PRODUCTION_STRUCTURED_READOUT_PARITY_PROTOCOL_AMENDMENT_A2.sha256",
      "src/tests/bench/wikimyei/representation/encoding/"
      "mtf_jepa_mae_vicreg/"
      "PRODUCTION_STRUCTURED_READOUT_PARITY_PROTOCOL_AMENDMENT_A3.md",
      "src/tests/bench/wikimyei/representation/encoding/"
      "mtf_jepa_mae_vicreg/"
      "PRODUCTION_STRUCTURED_READOUT_PARITY_PROTOCOL_AMENDMENT_A3.sha256",
      "src/tests/bench/wikimyei/representation/encoding/"
      "mtf_jepa_mae_vicreg/production_structured_readout_parity_gate.h",
      "src/tests/bench/wikimyei/representation/encoding/"
      "mtf_jepa_mae_vicreg/structured_readout_shadow.h",
      "src/tests/bench/wikimyei/representation/encoding/"
      "mtf_jepa_mae_vicreg/test_structured_readout_shadow.cpp",
      "src/tests/bench/wikimyei/representation/encoding/"
      "mtf_jepa_mae_vicreg/test_production_structured_readout.cpp",
      "src/tests/bench/wikimyei/representation/encoding/"
      "mtf_jepa_mae_vicreg/test_production_structured_readout_parity_gate."
      "cpp",
      "src/tests/bench/wikimyei/representation/encoding/"
      "mtf_jepa_mae_vicreg/test_production_structured_readout_parity_log_"
      "auditor.cpp",
      "src/tests/bench/wikimyei/representation/encoding/"
      "mtf_jepa_mae_vicreg/quality_wikimyei_mtf_jepa_mae_vicreg_"
      "production_structured_readout_parity.cpp",
      "src/tests/bench/wikimyei/representation/encoding/"
      "mtf_jepa_mae_vicreg/quality_wikimyei_mtf_jepa_mae_vicreg_"
      "representation.cpp",
      "src/tests/bench/wikimyei/representation/encoding/"
      "mtf_jepa_mae_vicreg/"
      "run_production_structured_readout_parity_mechanics.sh",
      "src/tests/bench/wikimyei/representation/encoding/"
      "mtf_jepa_mae_vicreg/"
      "verify_production_structured_readout_parity_candidate_patch.sh",
      "src/tests/bench/wikimyei/representation/encoding/"
      "mtf_jepa_mae_vicreg/"
      "write_production_structured_readout_parity_build_receipt.sh",
      "src/tests/bench/wikimyei/representation/encoding/"
      "mtf_jepa_mae_vicreg/Makefile",
      "src/tests/bench/wikimyei/representation/encoding/"
      "mtf_jepa_mae_vicreg/test_wikimyei_mtf_jepa_mae_vicreg.cpp",
      "src/tests/bench/jkimyei/training/channel_graph_first_launchers/"
      "test_jkimyei_channel_graph_first_launchers.cpp",
      "src/tests/bench/wikimyei/config/graph_first_specs/"
      "test_wikimyei_graph_first_specs.cpp"};
  evidence.mandatory_entries_present =
      entries.size() == mandatory.size() && mandatory.size() == 60 &&
      std::all_of(mandatory.begin(), mandatory.end(),
                  [&](const auto &item) { return entries.count(item) == 1; });
  if (!evidence.mandatory_entries_present) {
    fail(records, "SRR-2 manifest is missing mandatory custody entries");
  }
  const auto ledger_path_metadata = metadata.find("attempt_ledger_path");
  const auto ledger_absent_metadata =
      metadata.find("attempt_ledger_preseal_absent");
  evidence.attempt_ledger_preseal_contract_exact =
      ledger_path_metadata != metadata.end() &&
      ledger_path_metadata->second == kAttemptLedgerPath &&
      ledger_absent_metadata != metadata.end() &&
      ledger_absent_metadata->second == "true" &&
      entries.count(std::string(kAttemptLedgerPath)) == 0;
  if (!evidence.attempt_ledger_preseal_contract_exact) {
    fail(records, "attempt ledger pre-seal exclusion contract failed");
  }
  const auto authoritative_log_absent_metadata =
      metadata.find("authoritative_log_preseal_absent");
  evidence.authoritative_log_preseal_contract_exact =
      authoritative_log_absent_metadata != metadata.end() &&
      authoritative_log_absent_metadata->second == "true" &&
      entries.count(std::string(kAuthoritativeLogPath)) == 0;
  if (!evidence.authoritative_log_preseal_contract_exact) {
    fail(records, "authoritative log pre-seal exclusion contract failed");
  }

  evidence.live_entries_exact = true;
  evidence.containment_exact = true;
  const auto canonical_root = std::filesystem::canonical(root);
  for (const auto &[relative, entry] : entries) {
    try {
      const auto target = std::filesystem::canonical(root / relative);
      if (!path_is_within(canonical_root, target)) {
        evidence.containment_exact = false;
        evidence.live_entries_exact = false;
        fail(records, "manifest target escapes repository: " + relative);
      } else if (std::filesystem::is_symlink(root / relative) ||
                 !std::filesystem::is_regular_file(target)) {
        evidence.live_entries_exact = false;
        fail(records, "manifest target is not a regular file: " + relative);
      } else {
        const auto bytes = read_bytes(target);
        if (bytes.size() != entry.bytes || sha256(bytes) != entry.digest) {
          evidence.live_entries_exact = false;
          fail(records, "manifest live size/digest mismatch: " + relative);
        }
      }
    } catch (const std::exception &) {
      evidence.live_entries_exact = false;
      fail(records, "manifest target unreadable: " + relative);
    }
  }

  const auto build_receipt = verify_build_receipt(root, entries);
  evidence.build_receipt_exact = build_receipt.exact;
  for (const auto &error : build_receipt.errors) {
    fail(records, "build receipt: " + error);
  }
  const std::string parent_quality_relative =
      "src/tests/bench/wikimyei/representation/encoding/"
      "mtf_jepa_mae_vicreg/"
      "quality_wikimyei_mtf_jepa_mae_vicreg_representation.cpp";
  const auto parent_quality = entries.find(parent_quality_relative);
  evidence.parent_quality_source_exact =
      parent_quality != entries.end() &&
      parent_quality->second.bytes == kParentQualityTuBytes &&
      parent_quality->second.digest == kParentQualityTuSha;
  if (!evidence.parent_quality_source_exact) {
    fail(records, "frozen parent quality translation unit custody failed");
  }

  try {
    const std::string container_value =
        container == metadata.end() ? std::string{} : container->second;
    const std::string image_value =
        image == metadata.end() ? std::string{} : image->second;
    const std::string expected_identity =
        "container_name=unnamed_taoist\ncontainer_id=" + container_value +
        "\nimage_id=" + image_value + "\nworking_directory=/cuwacunu\n";
    const auto identity = read_bytes(
        root / ".build/tests/representation_srr2_v1_container_identity.txt");
    evidence.container_identity_exact =
        identity == expected_identity &&
        identity.size() == kContainerIdentityBytes &&
        sha256(identity) == kContainerIdentitySha;
  } catch (const std::exception &) {
    evidence.container_identity_exact = false;
  }
  if (!evidence.container_identity_exact) {
    fail(records, "container/image identity receipt mismatch");
  }

  const std::string protocol_relative =
      "src/tests/bench/wikimyei/representation/encoding/"
      "mtf_jepa_mae_vicreg/PRODUCTION_STRUCTURED_READOUT_PARITY_PROTOCOL.md";
  const std::string protocol_sidecar_relative =
      "src/tests/bench/wikimyei/representation/encoding/"
      "mtf_jepa_mae_vicreg/PRODUCTION_STRUCTURED_READOUT_PARITY_PROTOCOL."
      "sha256";
  try {
    const auto sidecar = read_bytes(root / protocol_sidecar_relative);
    evidence.protocol_exact =
        entries[protocol_relative].digest == kProtocolSha &&
        entries[protocol_relative].bytes == kProtocolBytes &&
        read_bytes(root / protocol_relative).size() == kProtocolBytes &&
        entries[protocol_sidecar_relative].digest == kProtocolSidecarSha &&
        entries[protocol_sidecar_relative].bytes == kProtocolSidecarBytes &&
        sidecar.size() == kProtocolSidecarBytes &&
        sha256(sidecar) == kProtocolSidecarSha;
  } catch (const std::exception &) {
    evidence.protocol_exact = false;
  }
  if (!evidence.protocol_exact) {
    fail(records, "frozen SRR-2 protocol custody failed");
  }
  const std::string amendment_relative =
      "src/tests/bench/wikimyei/representation/encoding/"
      "mtf_jepa_mae_vicreg/"
      "PRODUCTION_STRUCTURED_READOUT_PARITY_PROTOCOL_AMENDMENT_A1.md";
  const std::string amendment_sidecar_relative =
      "src/tests/bench/wikimyei/representation/encoding/"
      "mtf_jepa_mae_vicreg/"
      "PRODUCTION_STRUCTURED_READOUT_PARITY_PROTOCOL_AMENDMENT_A1.sha256";
  try {
    const auto amendment = read_bytes(root / amendment_relative);
    const auto sidecar = read_bytes(root / amendment_sidecar_relative);
    evidence.amendment_exact =
        entries[amendment_relative].digest == kAmendmentSha &&
        entries[amendment_relative].bytes == kAmendmentBytes &&
        amendment.size() == kAmendmentBytes &&
        sha256(amendment) == kAmendmentSha &&
        entries[amendment_sidecar_relative].digest == kAmendmentSidecarSha &&
        entries[amendment_sidecar_relative].bytes == kAmendmentSidecarBytes &&
        sidecar.size() == kAmendmentSidecarBytes &&
        sha256(sidecar) == kAmendmentSidecarSha;
  } catch (const std::exception &) {
    evidence.amendment_exact = false;
  }
  if (!evidence.amendment_exact) {
    fail(records, "frozen SRR-2 protocol amendment custody failed");
  }
  const std::string amendment_a2_relative =
      "src/tests/bench/wikimyei/representation/encoding/"
      "mtf_jepa_mae_vicreg/"
      "PRODUCTION_STRUCTURED_READOUT_PARITY_PROTOCOL_AMENDMENT_A2.md";
  const std::string amendment_a2_sidecar_relative =
      "src/tests/bench/wikimyei/representation/encoding/"
      "mtf_jepa_mae_vicreg/"
      "PRODUCTION_STRUCTURED_READOUT_PARITY_PROTOCOL_AMENDMENT_A2.sha256";
  try {
    const auto amendment_a2 = read_bytes(root / amendment_a2_relative);
    const auto sidecar = read_bytes(root / amendment_a2_sidecar_relative);
    const auto amendment_a2_entry = entries.find(amendment_a2_relative);
    const auto sidecar_entry = entries.find(amendment_a2_sidecar_relative);
    evidence.amendment_a2_exact =
        amendment_a2_entry != entries.end() &&
        amendment_a2_entry->second.digest == kAmendmentA2Sha &&
        amendment_a2_entry->second.bytes == kAmendmentA2Bytes &&
        amendment_a2.size() == kAmendmentA2Bytes &&
        sha256(amendment_a2) == kAmendmentA2Sha &&
        sidecar_entry != entries.end() &&
        sidecar_entry->second.digest == kAmendmentA2SidecarSha &&
        sidecar_entry->second.bytes == kAmendmentA2SidecarBytes &&
        sidecar.size() == kAmendmentA2SidecarBytes &&
        sha256(sidecar) == kAmendmentA2SidecarSha;
  } catch (const std::exception &) {
    evidence.amendment_a2_exact = false;
  }
  if (!evidence.amendment_a2_exact) {
    fail(records, "frozen SRR-2 protocol amendment A2 custody failed");
  }
  const std::string amendment_a3_relative =
      "src/tests/bench/wikimyei/representation/encoding/"
      "mtf_jepa_mae_vicreg/"
      "PRODUCTION_STRUCTURED_READOUT_PARITY_PROTOCOL_AMENDMENT_A3.md";
  const std::string amendment_a3_sidecar_relative =
      "src/tests/bench/wikimyei/representation/encoding/"
      "mtf_jepa_mae_vicreg/"
      "PRODUCTION_STRUCTURED_READOUT_PARITY_PROTOCOL_AMENDMENT_A3.sha256";
  try {
    const auto amendment_a3 = read_bytes(root / amendment_a3_relative);
    const auto sidecar = read_bytes(root / amendment_a3_sidecar_relative);
    const auto amendment_a3_entry = entries.find(amendment_a3_relative);
    const auto sidecar_entry = entries.find(amendment_a3_sidecar_relative);
    evidence.amendment_a3_exact =
        amendment_a3_entry != entries.end() &&
        amendment_a3_entry->second.digest == kAmendmentA3Sha &&
        amendment_a3_entry->second.bytes == kAmendmentA3Bytes &&
        amendment_a3.size() == kAmendmentA3Bytes &&
        sha256(amendment_a3) == kAmendmentA3Sha &&
        sidecar_entry != entries.end() &&
        sidecar_entry->second.digest == kAmendmentA3SidecarSha &&
        sidecar_entry->second.bytes == kAmendmentA3SidecarBytes &&
        sidecar.size() == kAmendmentA3SidecarBytes &&
        sha256(sidecar) == kAmendmentA3SidecarSha;
  } catch (const std::exception &) {
    evidence.amendment_a3_exact = false;
  }
  if (!evidence.amendment_a3_exact) {
    fail(records, "frozen SRR-2 protocol amendment A3 custody failed");
  }
  try {
    const auto a2_manifest = read_bytes(root / kA2PrerunManifestPath);
    const auto a2_log = read_bytes(root / kA2AuthoritativeLogPath);
    const auto a2_manifest_entry =
        entries.find(std::string(kA2PrerunManifestPath));
    const auto a2_log_entry =
        entries.find(std::string(kA2AuthoritativeLogPath));
    const auto occurs_once = [&](std::string_view value) {
      const auto first = a2_log.find(value);
      return first != std::string::npos &&
             a2_log.find(value, first + value.size()) == std::string::npos;
    };
    const bool authorization_tail_exact =
        a2_log.size() >= kAuthorizationTail.size() &&
        std::string_view(a2_log).substr(a2_log.size() -
                                        kAuthorizationTail.size()) ==
            kAuthorizationTail;
    evidence.a2_incident_exact =
        a2_manifest_entry != entries.end() &&
        a2_manifest_entry->second.bytes == kA2PrerunManifestBytes &&
        a2_manifest_entry->second.digest == kA2PrerunManifestSha &&
        a2_manifest.size() == kA2PrerunManifestBytes &&
        sha256(a2_manifest) == kA2PrerunManifestSha &&
        a2_log_entry != entries.end() &&
        a2_log_entry->second.bytes == kA2AuthoritativeLogBytes &&
        a2_log_entry->second.digest == kA2AuthoritativeLogSha &&
        std::filesystem::hard_link_count(root / kA2PrerunManifestPath) == 1 &&
        std::filesystem::hard_link_count(root / kA2AuthoritativeLogPath) == 1 &&
        a2_log.size() == kA2AuthoritativeLogBytes &&
        sha256(a2_log) == kA2AuthoritativeLogSha &&
        a2_log.find('\r') == std::string::npos &&
        occurs_once("srr2.attempt.consumed=false\n") &&
        occurs_once("authoritative_attempt_count=0\n") &&
        occurs_once("failure_reason=invalid_mechanics\n") &&
        occurs_once("terminal_result=invalid_mechanics\n") &&
        occurs_once("SRR-2 production structured readout parity failure: "
                    "outer augmentation model device is not cuda:0\n") &&
        authorization_tail_exact;
  } catch (const std::exception &) {
    evidence.a2_incident_exact = false;
  }
  if (!evidence.a2_incident_exact) {
    fail(records, "preserved A2 zero-attempt incident custody failed");
  }
  const std::string baseline_relative =
      ".build/tests/representation_srr2_v1_production_baseline.tar";
  try {
    evidence.baseline_exact =
        entries[baseline_relative].digest == kBaselineSha &&
        entries[baseline_relative].bytes == kBaselineBytes &&
        read_bytes(root / baseline_relative).size() == kBaselineBytes;
  } catch (const std::exception &) {
    evidence.baseline_exact = false;
  }
  if (!evidence.baseline_exact) {
    fail(records, "production baseline archive custody failed");
  }
  const auto delta =
      entries.find(".build/tests/representation_srr2_v1_candidate.patch");
  try {
    evidence.candidate_delta_exact =
        delta != entries.end() && delta->second.bytes != 0 &&
        read_bytes(root / delta->first).size() == delta->second.bytes;
  } catch (const std::exception &) {
    evidence.candidate_delta_exact = false;
  }
  if (!evidence.candidate_delta_exact) {
    fail(records, "candidate delta custody failed");
  }
  const auto candidate_patch = verify_candidate_patch(root, entries);
  evidence.candidate_delta_exact &= candidate_patch.exact;
  for (const auto &error : candidate_patch.errors) {
    fail(records, "candidate patch: " + error);
  }
  try {
    const auto executable_relative = std::filesystem::canonical(executable)
                                         .lexically_relative(canonical_root)
                                         .generic_string();
    const auto listed = entries.find(executable_relative);
    evidence.auditor_binary_exact =
        listed != entries.end() &&
        listed->second.digest == sha256(read_bytes(executable)) &&
        listed->second.bytes == read_bytes(executable).size();
  } catch (const std::exception &) {
    evidence.auditor_binary_exact = false;
  }
  if (!evidence.auditor_binary_exact) {
    fail(records, "executing SRR-2 auditor is not the sealed binary");
  }
  evidence.exact =
      syntax_exact && evidence.metadata_exact && evidence.live_entries_exact &&
      evidence.mandatory_entries_present && evidence.containment_exact &&
      evidence.protocol_exact && evidence.amendment_exact &&
      evidence.amendment_a2_exact && evidence.amendment_a3_exact &&
      evidence.a2_incident_exact &&
      evidence.baseline_exact && evidence.candidate_delta_exact &&
      evidence.auditor_binary_exact && evidence.runtime_binding_exact &&
      evidence.container_identity_exact && evidence.build_receipt_exact &&
      evidence.parent_quality_source_exact &&
      evidence.attempt_ledger_preseal_contract_exact &&
      evidence.authoritative_log_preseal_contract_exact;
  const auto logged_entry_count =
      unsigned_number(records, "srr2.prerun_manifest.entry_count");
  const bool logged_entries_exact =
      boolean(records, "srr2.prerun_manifest.entries_exact");
  const bool logged_manifest_exact =
      boolean(records, "srr2.prerun_manifest.exact");
  const bool recomputed_entries_exact =
      syntax_exact && evidence.live_entries_exact && evidence.containment_exact;
  if (logged_entry_count != evidence.entry_count ||
      logged_entries_exact != recomputed_entries_exact ||
      logged_manifest_exact != evidence.exact) {
    fail(records, "authoritative manifest summary disagrees with auditor");
  }
  evidence.entries = entries;
  evidence.error_count = records.errors.size() - errors_before;
  return evidence;
}

struct ParentEvidence {
  bool artifacts_exact{false};
  bool hashes_exact{false};
  bool classification_exact{false};
  bool attempt_exact{false};
  bool audit_pass{false};
  bool counters_zero{false};
  bool authorizations_false{false};
  bool material_gain{false};
  bool noninferior{false};
  bool order_decodable{false};
  bool continuous_shuffle_pass{false};
  bool order_shuffle_pass{false};
  bool terminal_reproduced{false};
  std::uint64_t attempt_count{0};
  std::uint64_t audit_error_count{0};
  std::uint64_t optimizer_steps{0};
  std::uint64_t backward_calls{0};
};

[[nodiscard]] ParentEvidence verify_parent(Records &records, Records &parent,
                                           Records &parent_audit,
                                           const std::filesystem::path &root,
                                           std::string_view parent_raw) {
  ParentEvidence evidence{};
  evidence.artifacts_exact = true;
  for (const auto &artifact : kParentArtifacts) {
    try {
      const auto bytes = read_bytes(root / artifact.path);
      if (bytes.size() != artifact.bytes || sha256(bytes) != artifact.sha) {
        evidence.artifacts_exact = false;
        fail(records,
             std::string("parent artifact mismatch: ") + artifact.path);
      }
    } catch (const std::exception &) {
      evidence.artifacts_exact = false;
      fail(records,
           std::string("parent artifact unreadable: ") + artifact.path);
    }
  }
  evidence.hashes_exact = parent_raw.size() == kParentLogBytes &&
                          sha256(parent_raw) == kParentLogSha;
  if (!evidence.hashes_exact) {
    fail(records, "parent authoritative log hash/bytes mismatch");
  }
  expect_text(parent, "schema", "wikimyei.mtf_jepa_mae_vicreg.srr.v1");
  expect_text(parent, "experiment", "structured-readout-repair");
  evidence.optimizer_steps = unsigned_number(parent, "optimizer_steps");
  evidence.backward_calls = unsigned_number(parent, "backward_calls");
  const auto training_loop_calls =
      unsigned_number(parent, "training_loop_calls");
  const auto augmentation_launcher_calls =
      unsigned_number(parent, "augmentation_launcher_calls");
  const auto end_to_end_calls = unsigned_number(parent, "end_to_end_calls");
  evidence.counters_zero =
      evidence.optimizer_steps == 0 && evidence.backward_calls == 0 &&
      training_loop_calls == 0 && augmentation_launcher_calls == 0 &&
      end_to_end_calls == 0;
  const bool training_authorized = boolean(parent, "training_authorized");
  const bool augmentation_change_authorized =
      boolean(parent, "augmentation_change_authorized");
  const bool long_run_authorized = boolean(parent, "long_run_authorized");
  const bool production_or_end_to_end_authorized =
      boolean(parent, "production_or_end_to_end_authorized");
  const bool follow_on_production_repair_authorized =
      boolean(parent, "follow_on_production_repair_authorized");
  evidence.authorizations_false =
      !training_authorized && !augmentation_change_authorized &&
      !long_run_authorized && !production_or_end_to_end_authorized &&
      !follow_on_production_repair_authorized;
  evidence.attempt_exact = boolean(parent, "srr.attempt.consumed");
  evidence.material_gain =
      required(parent,
               "srr.summary.contrast.shadow_minus_channel.classification") ==
      "material_gain";
  const auto &noninferior = required(
      parent, "srr.summary.contrast.shadow_minus_encoder.classification");
  evidence.noninferior = noninferior == "noninferior";
  evidence.order_decodable =
      required(parent, "srr.summary.arm.shadow.order.classification") ==
      "order_decodable";
  evidence.continuous_shuffle_pass =
      boolean(parent, "srr.summary.arm.shadow.continuous_shuffle_pass");
  evidence.order_shuffle_pass =
      boolean(parent, "srr.summary.arm.shadow.order_shuffle_pass");
  evidence.terminal_reproduced =
      required(parent, "execution_status") == "srr_measurements_complete";

  expect_text(parent_audit, "schema",
              "wikimyei.mtf_jepa_mae_vicreg.srr_audit.v1");
  evidence.attempt_count =
      unsigned_number(parent_audit, "audit.authoritative_attempt_count");
  evidence.audit_error_count =
      unsigned_number(parent_audit, "audit.error_count");
  evidence.audit_pass =
      boolean(parent_audit, "audit.pass") && evidence.audit_error_count == 0;
  const bool final_classification_exact =
      required(parent_audit, "audit.final_classification") ==
      "structured_readout_reproduced";
  const bool gate_classification_exact =
      required(parent_audit, "audit.gate.classification") ==
      "structured_readout_reproduced";
  const bool failure_reason_exact =
      required(parent_audit, "audit.failure_reason") == "none";
  evidence.classification_exact = final_classification_exact &&
                                  gate_classification_exact &&
                                  failure_reason_exact;
  evidence.attempt_exact =
      evidence.attempt_exact && evidence.attempt_count == 1;
  return evidence;
}

struct CaptureEvidence {
  gate::ProductionShadowParityInput parity{};
  gate::DeviceTranslationInput device{};
  bool capture_contracts_exact{true};
  bool purity_exact{true};
  bool finite_exact{true};
  bool deterministic_exact{true};
  bool input_hashes_exact{true};
  bool cpu64_hashes_exact{true};
  bool canonical_layout_metadata_exact{true};
  bool parent_source_hashes_exact{true};
  bool parent_reference_hashes_exact{true};
  bool parent_shadow_hashes_exact{true};
  std::size_t parent_hash_comparisons{0};
};

struct NormalizerEvidence {
  bool exact{false};
  std::size_t parent_hash_comparisons{0};
};

struct PrimitiveCaptureEvidence {
  bool counts_exact{false};
  bool same_encoded_object{false};
  bool public_selector_sandwich_exact{false};
  bool input_unchanged{false};
  bool parameter_unchanged{false};
  bool buffer_unchanged{false};
  bool cpu_rng_unchanged{false};
  bool cuda_rng_unchanged{false};
  bool model_mode_unchanged{false};
  bool finite{false};
  bool invalid_zero_exact{false};
  bool cpu64_masks_match_reference{false};
  bool complete_inputs{false};
  std::string cpu64_production_mask_hash{};
  std::string cpu64_shadow_mask_hash{};
};

[[nodiscard]] bool required_true(Records &records, const std::string &key) {
  return boolean(records, key);
}

void compare_emitted_bool(Records &records, const std::string &key,
                          bool recomputed) {
  const bool emitted = boolean(records, key);
  if (emitted != recomputed) {
    fail(records, "emitted boolean disagrees with audit: " + key);
  }
}

void compare_emitted_unsigned(Records &records, const std::string &key,
                              std::uint64_t recomputed) {
  const auto emitted = unsigned_number(records, key);
  if (emitted != recomputed) {
    fail(records, "emitted count disagrees with audit: " + key);
  }
}

void compare_emitted_double(Records &records, const std::string &key,
                            double recomputed) {
  const double emitted = finite_number(records, key);
  if (emitted != recomputed) {
    fail(records, "emitted maximum disagrees with audit: " + key);
  }
}

[[nodiscard]] bool
recompute_canonical_plan(std::string_view cell_vector,
                         std::string_view observed_layout_hash,
                         std::string_view authenticated_layout_hash) {
  return cell_vector == kCellVector &&
         lowercase_hex(observed_layout_hash, 16) &&
         lowercase_hex(authenticated_layout_hash, 16) &&
         observed_layout_hash == kFrozenLayoutHash &&
         authenticated_layout_hash == kFrozenLayoutHash &&
         observed_layout_hash == authenticated_layout_hash;
}

struct AttemptLedgerBindings {
  std::string command_sha256{};
  std::uint64_t manifest_bytes{0};
  std::string manifest_sha256{};
  std::uint64_t binary_bytes{0};
  std::string binary_sha256{};
  std::string container_id{};
};

struct AttemptLedgerEvidence {
  bool exact{false};
  bool content_exact{false};
  bool exclusive_create_exact{false};
  bool durable_exact{false};
  bool path_exact{false};
  bool mode_exact{false};
  bool single_link_exact{false};
  bool source_manifest_binding_exact{false};
  bool binary_manifest_binding_exact{false};
  bool emission_order_exact{false};
  std::uint64_t attempt_count{0};
  std::size_t bytes{0};
  std::string digest{};
  std::vector<std::string> errors{};
};

[[nodiscard]] std::string
canonical_attempt_ledger_bytes(const AttemptLedgerBindings &bindings) {
  return "schema=wikimyei.mtf_jepa_mae_vicreg.srr2_attempt_ledger.v1\n"
         "experiment=production-structured-readout-parity\n"
         "attempt_count=1\n"
         "state=consumed\n"
         "authoritative_command_sha256=" +
         bindings.command_sha256 +
         "\nprerun_manifest_bytes=" + std::to_string(bindings.manifest_bytes) +
         "\nprerun_manifest_sha256=" + bindings.manifest_sha256 +
         "\nexecuting_binary_bytes=" + std::to_string(bindings.binary_bytes) +
         "\nexecuting_binary_sha256=" + bindings.binary_sha256 +
         "\ncontainer_id=" + bindings.container_id + "\n";
}

[[nodiscard]] AttemptLedgerEvidence
verify_attempt_ledger_content(std::string_view raw,
                              const AttemptLedgerBindings &bindings) {
  AttemptLedgerEvidence evidence{};
  evidence.bytes = raw.size();
  evidence.digest = sha256(raw);
  Records ledger = parse_records(raw, true);
  constexpr std::array<std::string_view, 10> expected_order{
      "schema",
      "experiment",
      "attempt_count",
      "state",
      "authoritative_command_sha256",
      "prerun_manifest_bytes",
      "prerun_manifest_sha256",
      "executing_binary_bytes",
      "executing_binary_sha256",
      "container_id"};
  const std::string canonical = canonical_attempt_ledger_bytes(bindings);
  if (raw != canonical) {
    fail(ledger, "attempt ledger bytes are not the canonical LF-only receipt");
  }
  if (!raw.ends_with("\n") ||
      ledger.ordered_keys.size() != expected_order.size() ||
      !std::equal(ledger.ordered_keys.begin(), ledger.ordered_keys.end(),
                  expected_order.begin(), expected_order.end())) {
    fail(ledger, "attempt ledger record order/EOF is not canonical");
  }
  expect_text(ledger, "schema",
              "wikimyei.mtf_jepa_mae_vicreg.srr2_attempt_ledger.v1");
  expect_text(ledger, "experiment", "production-structured-readout-parity");
  evidence.attempt_count = unsigned_number(ledger, "attempt_count");
  if (evidence.attempt_count != 1) {
    fail(ledger, "attempt ledger count is not exactly one");
  }
  expect_text(ledger, "state", "consumed");
  expect_text(ledger, "authoritative_command_sha256", bindings.command_sha256);
  expect_unsigned(ledger, "prerun_manifest_bytes", bindings.manifest_bytes);
  expect_text(ledger, "prerun_manifest_sha256", bindings.manifest_sha256);
  expect_unsigned(ledger, "executing_binary_bytes", bindings.binary_bytes);
  expect_text(ledger, "executing_binary_sha256", bindings.binary_sha256);
  expect_text(ledger, "container_id", bindings.container_id);
  for (const auto &[key, value] : ledger.values) {
    (void)value;
    if (ledger.accessed.count(key) == 0) {
      ++ledger.unaccessed_keys;
      fail(ledger, "unrecognized attempt ledger key: " + key);
    }
  }
  evidence.content_exact =
      ledger.errors.empty() && ledger.machine_lines == expected_order.size() &&
      ledger.duplicate_keys == 0 && ledger.malformed_lines == 0 &&
      ledger.runtime_noise_lines == 0 && ledger.nonfinite_values == 0 &&
      ledger.critical_value_errors == 0 && ledger.unaccessed_keys == 0;
  evidence.errors = std::move(ledger.errors);
  return evidence;
}

[[nodiscard]] AttemptLedgerEvidence
verify_attempt_ledger(Records &records, const std::filesystem::path &root,
                      const ManifestEvidence &manifest) {
  AttemptLedgerBindings bindings{};
  bindings.command_sha256 = sha256(kExpectedCommand);
  bindings.manifest_bytes = manifest.bytes;
  bindings.manifest_sha256 = manifest.digest;
  bindings.container_id = manifest.container_id;
  std::string first_read;
  std::string second_read;
  bool implementation_bound = false;
  bool path_exact = false;
  bool mode_exact = false;
  bool single_link_exact = false;
  bool source_manifest_binding_exact = false;
  bool binary_manifest_binding_exact = false;
  try {
    const auto parity_path = root / std::string(kParityBinaryPath);
    const std::string parity_binary = read_bytes(parity_path);
    bindings.binary_bytes = parity_binary.size();
    bindings.binary_sha256 = sha256(parity_binary);
    const auto parity_entry =
        manifest.entries.find(std::string(kParityBinaryPath));
    binary_manifest_binding_exact =
        parity_entry != manifest.entries.end() &&
        parity_entry->second.bytes == bindings.binary_bytes &&
        parity_entry->second.digest == bindings.binary_sha256;

    const std::string parity_source =
        read_bytes(root / std::string(kParitySourcePath));
    const auto source_entry =
        manifest.entries.find(std::string(kParitySourcePath));
    source_manifest_binding_exact =
        source_entry != manifest.entries.end() &&
        source_entry->second.bytes == parity_source.size() &&
        source_entry->second.digest == sha256(parity_source);
    implementation_bound =
        manifest.live_entries_exact && manifest.candidate_delta_exact &&
        binary_manifest_binding_exact && source_manifest_binding_exact;

    const auto ledger_path = root / std::string(kAttemptLedgerPath);
    const auto canonical_root = std::filesystem::canonical(root);
    const auto canonical_ledger = std::filesystem::canonical(ledger_path);
    const auto ledger_status = std::filesystem::symlink_status(ledger_path);
    const auto read_only_permissions = std::filesystem::perms::owner_read |
                                       std::filesystem::perms::group_read |
                                       std::filesystem::perms::others_read;
    path_exact =
        path_is_within(canonical_root, canonical_ledger) &&
        canonical_ledger == (canonical_root / std::string(kAttemptLedgerPath))
                                .lexically_normal() &&
        ledger_status.type() == std::filesystem::file_type::regular &&
        !std::filesystem::is_symlink(ledger_status);
    mode_exact = ledger_status.permissions() == read_only_permissions;
    single_link_exact = std::filesystem::hard_link_count(canonical_ledger) == 1;
    first_read = read_bytes(canonical_ledger);
    second_read = read_bytes(canonical_ledger);
  } catch (const std::exception &error) {
    fail(records, std::string("attempt ledger live verification failed: ") +
                      error.what());
  }

  AttemptLedgerEvidence evidence =
      verify_attempt_ledger_content(first_read, bindings);
  evidence.path_exact = path_exact;
  evidence.mode_exact = mode_exact;
  evidence.single_link_exact = single_link_exact;
  evidence.source_manifest_binding_exact = source_manifest_binding_exact;
  evidence.binary_manifest_binding_exact = binary_manifest_binding_exact;
  for (const auto &error : evidence.errors) {
    fail(records, "attempt ledger: " + error);
  }
  constexpr std::array<std::string_view, 6> emitted_ledger_order{
      "srr2.attempt.ledger_path",    "srr2.attempt.ledger_bytes",
      "srr2.attempt.ledger_sha256",  "srr2.attempt.ledger_exclusive_create",
      "srr2.attempt.ledger_durable", "srr2.attempt.ledger_content_exact"};
  const auto consumed =
      std::find(records.ordered_keys.begin(), records.ordered_keys.end(),
                "srr2.attempt.consumed");
  evidence.emission_order_exact =
      consumed != records.ordered_keys.end() &&
      std::distance(records.ordered_keys.begin(), consumed) >=
          static_cast<std::ptrdiff_t>(emitted_ledger_order.size()) &&
      std::equal(
          consumed - static_cast<std::ptrdiff_t>(emitted_ledger_order.size()),
          consumed, emitted_ledger_order.begin(), emitted_ledger_order.end());
  if (!evidence.emission_order_exact) {
    fail(records, "attempt ledger log primitives are not adjacent/ordered");
  }
  evidence.exclusive_create_exact =
      manifest.attempt_ledger_preseal_contract_exact && implementation_bound &&
      evidence.content_exact && evidence.attempt_count == 1;
  evidence.durable_exact = implementation_bound && evidence.content_exact &&
                           evidence.path_exact && evidence.mode_exact &&
                           evidence.single_link_exact &&
                           first_read == second_read && !first_read.empty();
  evidence.exact = evidence.content_exact && evidence.exclusive_create_exact &&
                   evidence.durable_exact && evidence.emission_order_exact;

  expect_text(records, "srr2.attempt.ledger_path", kAttemptLedgerPath);
  compare_emitted_unsigned(records, "srr2.attempt.ledger_bytes",
                           evidence.bytes);
  const std::string emitted_digest =
      hash64(records, "srr2.attempt.ledger_sha256");
  if (emitted_digest != evidence.digest) {
    fail(records, "emitted attempt ledger digest disagrees with live file");
  }
  compare_emitted_bool(records, "srr2.attempt.ledger_exclusive_create",
                       evidence.exclusive_create_exact);
  compare_emitted_bool(records, "srr2.attempt.ledger_durable",
                       evidence.durable_exact);
  compare_emitted_bool(records, "srr2.attempt.ledger_content_exact",
                       evidence.content_exact);
  return evidence;
}

[[nodiscard]] PrimitiveCaptureEvidence
verify_capture_primitives(Records &records, const std::string &stem,
                          std::uint64_t rows,
                          const std::string &expected_input_data,
                          const std::string &expected_input_mask,
                          const std::string &reference_mask_hash) {
  PrimitiveCaptureEvidence evidence{};
  const std::uint64_t expected_values = rows * 96;
  const std::uint64_t expected_masks = rows * 3;
  const auto captured_rows =
      unsigned_number(records, stem + "captured_row_count");
  constexpr std::array<std::string_view, 5> outputs{
      "production", "shadow", "reference", "cpu64_production", "cpu64_shadow"};
  std::array<std::uint64_t, 5> value_counts{};
  std::array<std::uint64_t, 5> mask_counts{};
  std::array<std::uint64_t, 5> valid_counts{};
  std::array<std::uint64_t, 5> finite_counts{};
  std::array<bool, 5> finite_flags{};
  std::array<bool, 5> invalid_zero_flags{};
  for (std::size_t index = 0; index < outputs.size(); ++index) {
    const std::string name(outputs[index]);
    value_counts[index] =
        unsigned_number(records, stem + name + "_value_count");
    mask_counts[index] = unsigned_number(records, stem + name + "_mask_count");
    valid_counts[index] =
        unsigned_number(records, stem + name + "_valid_count");
    finite_counts[index] =
        unsigned_number(records, stem + name + "_finite_count");
    finite_flags[index] = boolean(records, stem + name + "_finite");
    invalid_zero_flags[index] =
        boolean(records, stem + name + "_invalid_zero_exact");
  }
  evidence.counts_exact = captured_rows == rows;
  evidence.finite = true;
  evidence.invalid_zero_exact = true;
  for (std::size_t index = 0; index < outputs.size(); ++index) {
    const bool value_count_exact = value_counts[index] == expected_values;
    const bool mask_count_exact = mask_counts[index] == expected_masks;
    const bool finite_exact =
        value_count_exact && finite_counts[index] == value_counts[index];
    const bool fully_valid =
        mask_count_exact && valid_counts[index] == mask_counts[index];
    evidence.counts_exact &= value_count_exact && mask_count_exact &&
                             valid_counts[index] <= mask_counts[index];
    evidence.finite &= finite_exact;
    evidence.invalid_zero_exact &= fully_valid;
    if (finite_flags[index] != finite_exact) {
      fail(records, "emitted finite fact disagrees with counts: " + stem +
                        std::string(outputs[index]));
    }
    if (invalid_zero_flags[index] != fully_valid) {
      fail(records, "emitted invalid-zero fact is not independently proved: " +
                        stem + std::string(outputs[index]));
    }
  }

  const std::string input_data_before =
      hash16(records, stem + "input_data_before_hash");
  const std::string input_data_after =
      hash16(records, stem + "input_data_after_hash");
  const std::string input_mask_before =
      hash16(records, stem + "input_mask_before_hash");
  const std::string input_mask_after =
      hash16(records, stem + "input_mask_after_hash");
  const bool target_before = boolean(records, stem + "target_defined_before");
  const bool target_after = boolean(records, stem + "target_defined_after");
  evidence.input_unchanged = input_data_before == input_data_after &&
                             input_data_before == expected_input_data &&
                             input_mask_before == input_mask_after &&
                             input_mask_before == expected_input_mask &&
                             !target_before && !target_after;

  const std::string parameter_before =
      hash16(records, stem + "parameter_before_hash");
  const std::string parameter_after =
      hash16(records, stem + "parameter_after_hash");
  const double parameter_max_abs =
      finite_number(records, stem + "parameter_max_abs");
  evidence.parameter_unchanged =
      parameter_before == parameter_after && parameter_max_abs == 0.0;
  const std::string buffer_before =
      hash16(records, stem + "buffer_before_hash");
  const std::string buffer_after = hash16(records, stem + "buffer_after_hash");
  evidence.buffer_unchanged = buffer_before == buffer_after;
  const std::string cpu_rng_before =
      hash16(records, stem + "cpu_rng_before_hash");
  const std::string cpu_rng_after =
      hash16(records, stem + "cpu_rng_after_hash");
  evidence.cpu_rng_unchanged = cpu_rng_before == cpu_rng_after;
  const std::string cuda_rng_before =
      hash16(records, stem + "cuda_rng_before_hash");
  const std::string cuda_rng_after =
      hash16(records, stem + "cuda_rng_after_hash");
  evidence.cuda_rng_unchanged = cuda_rng_before == cuda_rng_after;
  const bool training_before = boolean(records, stem + "training_mode_before");
  const bool training_after = boolean(records, stem + "training_mode_after");
  evidence.model_mode_unchanged =
      !training_before && training_before == training_after;

  const std::string encoded_before_production =
      hash16(records, stem + "encoded_before_production_hash");
  const std::string encoded_after_production =
      hash16(records, stem + "encoded_after_production_hash");
  const std::string encoded_before_shadow =
      hash16(records, stem + "encoded_before_shadow_hash");
  const std::string encoded_after_shadow =
      hash16(records, stem + "encoded_after_shadow_hash");
  evidence.same_encoded_object =
      encoded_before_production == encoded_after_production &&
      encoded_after_production == encoded_before_shadow &&
      encoded_before_shadow == encoded_after_shadow;
  const std::string old_selector_before =
      hash16(records, stem + "old_selector_before_hash");
  const std::string old_selector_after =
      hash16(records, stem + "old_selector_after_hash");
  const std::string token_before = hash16(records, stem + "token_before_hash");
  const std::string token_after = hash16(records, stem + "token_after_hash");
  const std::string token_mask_before =
      hash16(records, stem + "token_before_mask_hash");
  const std::string token_mask_after =
      hash16(records, stem + "token_after_mask_hash");
  const std::string token_metadata_before =
      hash16(records, stem + "token_before_metadata_hash");
  const std::string token_metadata_after =
      hash16(records, stem + "token_after_metadata_hash");
  const std::string encoded_token_mask =
      hash16(records, stem + "encoded_token_mask_hash");
  const std::string encoded_metadata =
      hash16(records, stem + "encoded_metadata_hash");
  evidence.public_selector_sandwich_exact =
      old_selector_before == old_selector_after &&
      token_before == token_after && token_mask_before == token_mask_after &&
      token_metadata_before == token_metadata_after &&
      token_mask_before == encoded_token_mask &&
      token_metadata_before == encoded_metadata;
  evidence.cpu64_production_mask_hash =
      hash16(records, stem + "cpu64_production_mask_hash");
  evidence.cpu64_shadow_mask_hash =
      hash16(records, stem + "cpu64_shadow_mask_hash");
  evidence.cpu64_masks_match_reference =
      evidence.cpu64_production_mask_hash == reference_mask_hash &&
      evidence.cpu64_shadow_mask_hash == reference_mask_hash;

  const auto compare = [&](std::string_view suffix, bool recomputed) {
    compare_emitted_bool(records, stem + std::string(suffix), recomputed);
  };
  compare("same_encoded_object", evidence.same_encoded_object);
  compare("public_selector_sandwich_exact",
          evidence.public_selector_sandwich_exact);
  compare("input_unchanged", evidence.input_unchanged);
  compare("parameter_unchanged", evidence.parameter_unchanged);
  compare("buffer_unchanged", evidence.buffer_unchanged);
  compare("cpu_rng_unchanged", evidence.cpu_rng_unchanged);
  compare("cuda_rng_unchanged", evidence.cuda_rng_unchanged);
  compare("model_mode_unchanged", evidence.model_mode_unchanged);
  compare("finite", evidence.finite);
  compare("invalid_zero_exact", evidence.invalid_zero_exact);
  evidence.complete_inputs =
      evidence.counts_exact && evidence.same_encoded_object &&
      evidence.public_selector_sandwich_exact && evidence.input_unchanged &&
      evidence.parameter_unchanged && evidence.buffer_unchanged &&
      evidence.cpu_rng_unchanged && evidence.cuda_rng_unchanged &&
      evidence.model_mode_unchanged && evidence.finite &&
      evidence.invalid_zero_exact;
  return evidence;
}

void close_auxiliary_schema(Records &records) {
  for (const auto &[key, value] : records.values) {
    (void)value;
    if (records.accessed.count(key) == 0) {
      ++records.unaccessed_keys;
      fail(records, "unrecognized auxiliary key: " + key);
    }
  }
}

struct MechanicsEvidence {
  gate::BackwardCompatibilityInput compatibility{};
  bool exact{false};
  std::size_t bytes{0};
  std::string digest{};
  std::vector<std::string> errors{};
};

[[nodiscard]] MechanicsEvidence
verify_mechanics_log(const std::filesystem::path &root) {
  MechanicsEvidence evidence{};
  const auto path = root / ".build/tests/representation_srr2_v1_mechanics.log";
  std::string raw;
  try {
    raw = read_bytes(path);
  } catch (const std::exception &error) {
    evidence.errors.push_back(error.what());
    return evidence;
  }
  evidence.bytes = raw.size();
  evidence.digest = sha256(raw);
  Records receipt = parse_records(raw, true);
  expect_text(receipt, "schema",
              "wikimyei.mtf_jepa_mae_vicreg.srr2_mechanics.v1");
  expect_text(receipt, "experiment", "production-structured-readout-mechanics");
  constexpr std::array<std::pair<std::string_view, std::string_view>, 6> cases{{
      {"production", "CUBLAS_WORKSPACE_CONFIG=:4096:8 "
                     "./.build/tests/test_production_structured_readout"},
      {"shadow", "./.build/tests/test_structured_readout_shadow"},
      {"gate", "./.build/tests/test_production_structured_readout_parity_gate"},
      {"auditor", "./.build/tests/"
                  "test_production_structured_readout_parity_log_auditor "
                  "--self-test"},
      {"config", "./.build/tests/test_wikimyei_graph_first_specs"},
      {"adapter", "./.build/tests/test_jkimyei_channel_graph_first_launchers"},
  }};
  for (const auto &[name, command] : cases) {
    const std::string prefix = "srr2.mechanics." + std::string(name) + ".";
    expect_text(receipt, prefix + "command", command);
    expect_unsigned(receipt, prefix + "exit_code", 0);
    expect_bool(receipt, prefix + "pass_marker", true);
  }
  expect_bool(receipt, "srr2.mechanics.training_or_augmentation_used", false);
  expect_bool(receipt, "srr2.mechanics.local_contracts_exact", true);
  expect_bool(receipt, "srr2.mechanics.pass", true);
  const auto value = [&](const char *name) {
    const bool observed =
        boolean(receipt, std::string("srr2.compatibility.") + name);
    if (!observed) {
      fail(receipt, std::string("mechanics compatibility false: ") + name);
    }
    return observed;
  };
  evidence.compatibility = {
      .legacy_enum_ordinals_exact = value("legacy_enum_ordinals_exact"),
      .legacy_policy_names_exact = value("legacy_policy_names_exact"),
      .structured_policy_appended = value("structured_policy_appended"),
      .structured_policy_name_exact = value("structured_policy_name_exact"),
      .parser_round_trip_exact = value("parser_round_trip_exact"),
      .unknown_policy_rejected = value("unknown_policy_rejected"),
      .cpp_default_all_tokens = value("cpp_default_all_tokens"),
      .omitted_dsl_all_tokens = value("omitted_dsl_all_tokens"),
      .active_dsl_all_tokens = value("active_dsl_all_tokens"),
      .protocol_fingerprint_distinct = value("protocol_fingerprint_distinct"),
      .structured_checkpoint_round_trip_exact =
          value("structured_checkpoint_round_trip_exact"),
      .legacy_checkpoint_all_tokens = value("legacy_checkpoint_all_tokens"),
      .legacy_checkpoint_does_not_inherit_structured =
          value("legacy_checkpoint_does_not_inherit_structured"),
      .checkpoint_mismatch_rejected = value("checkpoint_mismatch_rejected"),
      .malformed_checkpoint_rejected = value("malformed_checkpoint_rejected"),
      .legacy_policy_bytes_exact = value("legacy_policy_bytes_exact"),
      .public_selector_contract_exact = value("public_selector_contract_exact"),
      .adapter_reaches_structured_selector =
          value("adapter_reaches_structured_selector")};
  close_auxiliary_schema(receipt);
  evidence.exact =
      receipt.errors.empty() && receipt.duplicate_keys == 0 &&
      receipt.malformed_lines == 0 && receipt.runtime_noise_lines == 0 &&
      receipt.nonfinite_values == 0 && receipt.critical_value_errors == 0 &&
      receipt.unaccessed_keys == 0;
  evidence.errors = std::move(receipt.errors);
  return evidence;
}

struct PreflightEvidence {
  bool exact{false};
  std::size_t bytes{0};
  std::string digest{};
  std::vector<std::string> errors{};
};

struct PreflightCaptureEvidence {
  bool exact{false};
  bool complete{false};
  bool purity_exact{false};
  bool production_shadow_values_exact{false};
  bool production_shadow_masks_exact{false};
  bool cpu64_production_reference_exact{false};
  bool cpu64_shadow_reference_exact{false};
  PrimitiveCaptureEvidence primitives{};
  std::string encoder{};
  std::string served{};
  std::string metadata{};
  std::string production{};
  std::string shadow{};
  std::string reference{};
  std::string cpu64_production{};
  std::string cpu64_shadow{};
  std::string production_mask{};
  std::string shadow_mask{};
  std::string reference_mask{};
  double production_shadow_max_abs{0.0};
  double cpu64_production_shadow_max_abs{0.0};
  double cpu64_production_reference_max_abs{0.0};
  double cpu64_shadow_reference_max_abs{0.0};
  double device_production_reference_max_abs{0.0};
  double device_shadow_reference_max_abs{0.0};
};

[[nodiscard]] PreflightCaptureEvidence
verify_preflight_capture(Records &records, const std::string &prefix,
                         const std::string &expected_input_data,
                         const std::string &expected_input_mask,
                         bool expected_layout_exact) {
  PreflightCaptureEvidence evidence{};
  constexpr std::uint64_t rows = 101;
  const auto observed_rows = unsigned_number(records, prefix + "rows");
  const auto batch_size = unsigned_number(records, prefix + "batch_size");
  const auto value_count = unsigned_number(records, prefix + "value_count");
  const auto validity_count =
      unsigned_number(records, prefix + "validity_count");
  const bool counts_exact = observed_rows == rows && batch_size == 96 &&
                            value_count == rows * 96 &&
                            validity_count == rows * 3;
  const bool shape = required(records, prefix + "shape") == "101,3,32" &&
                     required(records, prefix + "mask_shape") == "101,3";
  const bool strides =
      required(records, prefix + "values_stride") == "96,32,1" &&
      required(records, prefix + "mask_stride") == "3,1" &&
      boolean(records, prefix + "values_contiguous") &&
      boolean(records, prefix + "mask_contiguous");
  const bool dtype = required(records, prefix + "dtype") == "float32";
  const bool device = required(records, prefix + "device") == "cuda:0";
  const bool layout = boolean(records, prefix + "layout_exact");
  if (layout != expected_layout_exact) {
    fail(records, "preflight capture layout fact disagrees: " + prefix);
  }

  evidence.encoder = hash16(records, prefix + "encoder_hash");
  evidence.served = hash16(records, prefix + "served_hash");
  evidence.metadata = hash16(records, prefix + "metadata_structure_hash");
  evidence.production = hash16(records, prefix + "production_hash");
  evidence.shadow = hash16(records, prefix + "shadow_hash");
  evidence.reference = hash16(records, prefix + "reference_hash");
  evidence.cpu64_production = hash16(records, prefix + "cpu64_production_hash");
  evidence.cpu64_shadow = hash16(records, prefix + "cpu64_shadow_hash");
  evidence.production_mask = hash16(records, prefix + "production_mask_hash");
  evidence.shadow_mask = hash16(records, prefix + "shadow_mask_hash");
  evidence.reference_mask = hash16(records, prefix + "reference_mask_hash");
  evidence.primitives =
      verify_capture_primitives(records, prefix, rows, expected_input_data,
                                expected_input_mask, evidence.reference_mask);

  evidence.production_shadow_max_abs =
      finite_number(records, prefix + "production_shadow_device_max_abs");
  evidence.cpu64_production_shadow_max_abs =
      finite_number(records, prefix + "cpu64_production_shadow_max_abs");
  evidence.cpu64_production_reference_max_abs =
      finite_number(records, prefix + "cpu64_production_reference_max_abs");
  evidence.cpu64_shadow_reference_max_abs =
      finite_number(records, prefix + "cpu64_shadow_reference_max_abs");
  evidence.device_production_reference_max_abs =
      finite_number(records, prefix + "device_production_reference_max_abs");
  evidence.device_shadow_reference_max_abs =
      finite_number(records, prefix + "device_shadow_reference_max_abs");
  const std::array<double, 6> maxima{
      evidence.production_shadow_max_abs,
      evidence.cpu64_production_shadow_max_abs,
      evidence.cpu64_production_reference_max_abs,
      evidence.cpu64_shadow_reference_max_abs,
      evidence.device_production_reference_max_abs,
      evidence.device_shadow_reference_max_abs};
  if (std::any_of(maxima.begin(), maxima.end(),
                  [](double value) { return value < 0.0; })) {
    fail(records, "negative preflight capture maximum: " + prefix);
  }

  const bool masks_equal = evidence.production_mask == evidence.shadow_mask &&
                           evidence.shadow_mask == evidence.reference_mask;
  evidence.production_shadow_values_exact =
      evidence.production == evidence.shadow &&
      evidence.production_shadow_max_abs == 0.0;
  evidence.production_shadow_masks_exact = masks_equal;
  const bool cpu64_production_shadow_values =
      evidence.cpu64_production == evidence.cpu64_shadow &&
      evidence.cpu64_production_shadow_max_abs == 0.0;
  const bool cpu64_masks =
      evidence.primitives.cpu64_masks_match_reference && masks_equal;
  evidence.cpu64_production_reference_exact =
      evidence.cpu64_production == evidence.reference &&
      evidence.cpu64_production_reference_max_abs == 0.0;
  evidence.cpu64_shadow_reference_exact =
      evidence.cpu64_shadow == evidence.reference &&
      evidence.cpu64_shadow_reference_max_abs == 0.0;
  compare_emitted_bool(records, prefix + "production_shadow_value_bytes_exact",
                       evidence.production_shadow_values_exact);
  compare_emitted_bool(records, prefix + "production_shadow_mask_bytes_exact",
                       evidence.production_shadow_masks_exact);
  compare_emitted_bool(records,
                       prefix + "cpu64_production_shadow_value_bytes_exact",
                       cpu64_production_shadow_values);
  compare_emitted_bool(records,
                       prefix + "cpu64_production_shadow_mask_bytes_exact",
                       cpu64_masks);
  compare_emitted_bool(records,
                       prefix + "cpu64_production_reference_value_bytes_exact",
                       evidence.cpu64_production_reference_exact);
  compare_emitted_bool(records,
                       prefix + "cpu64_production_reference_mask_bytes_exact",
                       cpu64_masks);
  compare_emitted_bool(records,
                       prefix + "cpu64_shadow_reference_value_bytes_exact",
                       evidence.cpu64_shadow_reference_exact);
  compare_emitted_bool(
      records, prefix + "cpu64_shadow_reference_mask_bytes_exact", cpu64_masks);

  evidence.complete = counts_exact && shape && strides && dtype && device &&
                      layout && evidence.primitives.complete_inputs;
  compare_emitted_bool(records, prefix + "complete", evidence.complete);
  evidence.purity_exact = evidence.primitives.input_unchanged &&
                          evidence.primitives.parameter_unchanged &&
                          evidence.primitives.buffer_unchanged &&
                          evidence.primitives.cpu_rng_unchanged &&
                          evidence.primitives.cuda_rng_unchanged &&
                          evidence.primitives.model_mode_unchanged;
  evidence.exact = evidence.complete && evidence.purity_exact &&
                   evidence.production_shadow_values_exact &&
                   evidence.production_shadow_masks_exact &&
                   cpu64_production_shadow_values && cpu64_masks &&
                   evidence.cpu64_production_reference_exact &&
                   evidence.cpu64_shadow_reference_exact &&
                   evidence.device_production_reference_max_abs <=
                       gate::kDeviceTranslationTolerance &&
                   evidence.device_shadow_reference_max_abs <=
                       gate::kDeviceTranslationTolerance;
  return evidence;
}

[[nodiscard]] PreflightEvidence
verify_preflight_log(const std::filesystem::path &root,
                     std::string_view expected_layout_hash,
                     const MechanicsEvidence &mechanics) {
  PreflightEvidence evidence{};
  std::string raw;
  try {
    raw =
        read_bytes(root / ".build/tests/representation_srr2_v1_preflight.log");
  } catch (const std::exception &error) {
    evidence.errors.push_back(error.what());
    return evidence;
  }
  evidence.bytes = raw.size();
  evidence.digest = sha256(raw);
  Records receipt = parse_records(raw, true);
  expect_text(receipt, "schema",
              "wikimyei.mtf_jepa_mae_vicreg.srr2_preflight.v2");
  expect_text(receipt, "experiment",
              "production-structured-readout-parity-preflight");
  expect_text(receipt, "device", "cuda:0");
  expect_text(receipt, "dtype", "float32");
  expect_bool(receipt, "srr2.preflight.manifest_read", false);
  expect_unsigned(receipt, "srr2.preflight.normalizer_group_begin", 4700000);
  expect_unsigned(receipt, "srr2.preflight.normalizer_count", 32);
  expect_unsigned(receipt, "srr2.preflight.capture_group_begin", 4800000);
  expect_unsigned(receipt, "srr2.preflight.capture_count", 101);
  expect_unsigned(receipt, "srr2.preflight.batch_size", 96);
  expect_unsigned(receipt, "srr2.preflight.seed", 17);
  expect_unsigned(receipt, "srr2.preflight.expected_authoritative_key_count",
                  kExpectedAuthoritativeKeyCount);
  expect_text(receipt, "srr2.preflight.expected_authoritative_keyset_sha256",
              kExpectedAuthoritativeKeysetSha);
  expect_bool(receipt, "srr2.preflight.target_constructed", false);
  expect_text(receipt, "srr2.preflight.compatibility.parsed_device_type",
              "cuda");
  const auto parsed_device_index = required(
      receipt, "srr2.preflight.compatibility.parsed_device_index");
  if (parsed_device_index != "-1" && parsed_device_index != "0") {
    fail(receipt, "preflight parsed CUDA alias index is neither -1 nor 0");
  }
  expect_text(receipt, "srr2.preflight.compatibility.active_alias_device",
              "cuda:0");
  expect_text(receipt,
              "srr2.preflight.compatibility.structured_alias_device",
              "cuda:0");
  expect_text(receipt, "srr2.preflight.compatibility.active_alias_policy",
              "all_tokens");
  expect_text(receipt,
              "srr2.preflight.compatibility.structured_alias_policy",
              "structured_cdsb_v1");
  const auto active_manifest =
      hash16(receipt, "srr2.preflight.compatibility.active_manifest_hash");
  const auto structured_manifest = hash16(
      receipt, "srr2.preflight.compatibility.structured_manifest_hash");
  const auto active_nonpolicy = hash16(
      receipt,
      "srr2.preflight.compatibility.active_nonpolicy_manifest_hash");
  const auto structured_nonpolicy = hash16(
      receipt,
      "srr2.preflight.compatibility.structured_nonpolicy_manifest_hash");
  expect_unsigned(receipt,
                  "srr2.preflight.compatibility.receipt_fact_count", 18);
  const bool emitted_compatibility_setup =
      boolean(receipt, "srr2.preflight.compatibility.setup_complete");
  const auto compatibility_value = [&](const char *name) {
    return boolean(receipt, std::string("srr2.compatibility.") + name);
  };
  const gate::BackwardCompatibilityInput preflight_compatibility{
      .legacy_enum_ordinals_exact =
          compatibility_value("legacy_enum_ordinals_exact"),
      .legacy_policy_names_exact =
          compatibility_value("legacy_policy_names_exact"),
      .structured_policy_appended =
          compatibility_value("structured_policy_appended"),
      .structured_policy_name_exact =
          compatibility_value("structured_policy_name_exact"),
      .parser_round_trip_exact = compatibility_value("parser_round_trip_exact"),
      .unknown_policy_rejected =
          compatibility_value("unknown_policy_rejected"),
      .cpp_default_all_tokens =
          compatibility_value("cpp_default_all_tokens"),
      .omitted_dsl_all_tokens =
          compatibility_value("omitted_dsl_all_tokens"),
      .active_dsl_all_tokens =
          compatibility_value("active_dsl_all_tokens"),
      .protocol_fingerprint_distinct =
          compatibility_value("protocol_fingerprint_distinct"),
      .structured_checkpoint_round_trip_exact =
          compatibility_value("structured_checkpoint_round_trip_exact"),
      .legacy_checkpoint_all_tokens =
          compatibility_value("legacy_checkpoint_all_tokens"),
      .legacy_checkpoint_does_not_inherit_structured = compatibility_value(
          "legacy_checkpoint_does_not_inherit_structured"),
      .checkpoint_mismatch_rejected =
          compatibility_value("checkpoint_mismatch_rejected"),
      .malformed_checkpoint_rejected =
          compatibility_value("malformed_checkpoint_rejected"),
      .legacy_policy_bytes_exact =
          compatibility_value("legacy_policy_bytes_exact"),
      .public_selector_contract_exact =
          compatibility_value("public_selector_contract_exact"),
      .adapter_reaches_structured_selector =
          compatibility_value("adapter_reaches_structured_selector")};
  const auto compatibility_all_true = [](const auto &value) {
    return value.legacy_enum_ordinals_exact &&
           value.legacy_policy_names_exact &&
           value.structured_policy_appended &&
           value.structured_policy_name_exact &&
           value.parser_round_trip_exact && value.unknown_policy_rejected &&
           value.cpp_default_all_tokens && value.omitted_dsl_all_tokens &&
           value.active_dsl_all_tokens &&
           value.protocol_fingerprint_distinct &&
           value.structured_checkpoint_round_trip_exact &&
           value.legacy_checkpoint_all_tokens &&
           value.legacy_checkpoint_does_not_inherit_structured &&
           value.checkpoint_mismatch_rejected &&
           value.malformed_checkpoint_rejected &&
           value.legacy_policy_bytes_exact &&
           value.public_selector_contract_exact &&
           value.adapter_reaches_structured_selector;
  };
  const bool compatibility_preboundary_exact =
      mechanics.exact && compatibility_all_true(mechanics.compatibility) &&
      compatibility_all_true(preflight_compatibility) &&
      active_manifest != structured_manifest &&
      active_manifest == active_nonpolicy &&
      active_nonpolicy == structured_nonpolicy;
  compare_emitted_bool(receipt,
                       "srr2.preflight.compatibility.setup_complete",
                       compatibility_preboundary_exact);
  expect_text(receipt, "srr2.environment.device", "cuda:0");
  expect_text(receipt, "srr2.environment.dtype", "float32");
  expect_unsigned(receipt, "srr2.environment.cpu_threads", 1);
  expect_unsigned(receipt, "srr2.environment.cpu_interop_threads", 1);
  expect_bool(receipt, "srr2.environment.deterministic_algorithms", true);
  expect_bool(receipt, "srr2.environment.deterministic_warn_only", false);
  expect_bool(receipt, "srr2.environment.deterministic_cudnn", true);
  expect_bool(receipt, "srr2.environment.tf32_cublas_disabled", true);
  expect_bool(receipt, "srr2.environment.tf32_cudnn_disabled", true);
  expect_bool(receipt, "srr2.environment.cublas_workspace_exact", true);
  expect_bool(receipt, "srr2.environment.cuda_available", true);
  expect_text(receipt, "srr2.projection.q0_hash", "f8c9f35282de2ee0");
  expect_text(receipt, "srr2.projection.qpsm_hash", "ac8a43fd65b2c8a8");
  const double orthogonality =
      finite_number(receipt, "srr2.projection.orthogonality_error");
  const double contrast =
      finite_number(receipt, "srr2.projection.contrast_mean_error");
  const double block_sum =
      finite_number(receipt, "srr2.projection.block_sum_error");
  if (orthogonality < 0.0 || orthogonality > 1.0e-10 || contrast < 0.0 ||
      contrast > 1.0e-10 || block_sum < 0.0 || block_sum > 1.0e-10) {
    fail(receipt, "preflight projection invariant failure");
  }
  expect_text(receipt, "srr2.layout.hash", expected_layout_hash);
  expect_text(receipt, "srr2.layout.cell_vector", kCellVector);
  const bool layout_exact = boolean(receipt, "srr2.layout.exact") &&
                            !expected_layout_hash.empty() &&
                            lowercase_hex(expected_layout_hash, 16);
  const std::string retained_input_data =
      hash16(receipt, "srr2.preflight.retained.input_data_before_hash");
  const std::string retained_input_mask =
      hash16(receipt, "srr2.preflight.retained.input_mask_before_hash");
  const auto retained = verify_preflight_capture(
      receipt, "srr2.preflight.retained.", retained_input_data,
      retained_input_mask, layout_exact);
  const auto repeat = verify_preflight_capture(
      receipt, "srr2.preflight.repeat.", retained_input_data,
      retained_input_mask, layout_exact);
  const bool repeat_identity =
      retained.complete && repeat.complete &&
      retained.encoder == repeat.encoder && retained.served == repeat.served &&
      retained.metadata == repeat.metadata &&
      retained.production == repeat.production &&
      retained.shadow == repeat.shadow &&
      retained.reference == repeat.reference &&
      retained.cpu64_production == repeat.cpu64_production &&
      retained.cpu64_shadow == repeat.cpu64_shadow &&
      retained.production_mask == repeat.production_mask &&
      retained.shadow_mask == repeat.shadow_mask &&
      retained.reference_mask == repeat.reference_mask &&
      retained.primitives.cpu64_production_mask_hash ==
          repeat.primitives.cpu64_production_mask_hash &&
      retained.primitives.cpu64_shadow_mask_hash ==
          repeat.primitives.cpu64_shadow_mask_hash;

  compare_emitted_bool(receipt, "srr2.preflight.complete", retained.complete);
  compare_emitted_bool(receipt, "srr2.preflight.repeat_complete",
                       repeat.complete);
  compare_emitted_bool(receipt, "srr2.preflight.same_encoded_object",
                       retained.primitives.same_encoded_object);
  compare_emitted_bool(receipt, "srr2.preflight.public_selector_sandwich_exact",
                       retained.primitives.public_selector_sandwich_exact);
  compare_emitted_bool(receipt, "srr2.preflight.layout_exact", layout_exact);
  compare_emitted_bool(receipt,
                       "srr2.preflight.production_shadow_value_bytes_exact",
                       retained.production_shadow_values_exact);
  compare_emitted_bool(receipt,
                       "srr2.preflight.production_shadow_mask_bytes_exact",
                       retained.production_shadow_masks_exact);
  compare_emitted_bool(receipt,
                       "srr2.preflight.cpu64_production_reference_bytes_exact",
                       retained.cpu64_production_reference_exact);
  compare_emitted_bool(receipt,
                       "srr2.preflight.cpu64_shadow_reference_bytes_exact",
                       retained.cpu64_shadow_reference_exact);
  compare_emitted_double(receipt,
                         "srr2.preflight.production_shadow_device_max_abs",
                         retained.production_shadow_max_abs);
  compare_emitted_double(receipt,
                         "srr2.preflight.cpu64_production_reference_max_abs",
                         retained.cpu64_production_reference_max_abs);
  compare_emitted_double(receipt,
                         "srr2.preflight.cpu64_shadow_reference_max_abs",
                         retained.cpu64_shadow_reference_max_abs);
  compare_emitted_double(receipt,
                         "srr2.preflight.device_production_reference_max_abs",
                         retained.device_production_reference_max_abs);
  compare_emitted_double(receipt,
                         "srr2.preflight.device_shadow_reference_max_abs",
                         retained.device_shadow_reference_max_abs);
  compare_emitted_bool(receipt, "srr2.preflight.input_unchanged",
                       retained.primitives.input_unchanged);
  compare_emitted_bool(receipt, "srr2.preflight.parameter_unchanged",
                       retained.primitives.parameter_unchanged);
  compare_emitted_bool(receipt, "srr2.preflight.buffer_unchanged",
                       retained.primitives.buffer_unchanged);
  compare_emitted_bool(receipt, "srr2.preflight.cpu_rng_unchanged",
                       retained.primitives.cpu_rng_unchanged);
  compare_emitted_bool(receipt, "srr2.preflight.cuda_rng_unchanged",
                       retained.primitives.cuda_rng_unchanged);
  compare_emitted_bool(receipt, "srr2.preflight.model_mode_unchanged",
                       retained.primitives.model_mode_unchanged);
  compare_emitted_bool(receipt, "srr2.preflight.repeat_identity_exact",
                       repeat_identity);

  bool parent_exact = false;
  try {
    const std::string parent_raw = read_bytes(
        root / ".build/tests/representation_srr_v1_authoritative.log");
    const std::string parent_audit_raw =
        read_bytes(root / ".build/tests/representation_srr_v1_audit.log");
    Records parent = parse_records(parent_raw, false);
    Records parent_audit = parse_records(parent_audit_raw, false);
    const auto parent_evidence =
        verify_parent(receipt, parent, parent_audit, root, parent_raw);
    parent_exact =
        parent_evidence.artifacts_exact && parent_evidence.hashes_exact &&
        parent_evidence.classification_exact && parent_evidence.attempt_exact &&
        parent_evidence.audit_pass && parent_evidence.counters_zero &&
        parent_evidence.authorizations_false &&
        parent_evidence.attempt_count == 1 &&
        parent_evidence.audit_error_count == 0 &&
        parent_evidence.optimizer_steps == 0 &&
        parent_evidence.backward_calls == 0;
  } catch (const std::exception &error) {
    fail(receipt,
         std::string("preflight parent verification failed: ") + error.what());
  }
  compare_emitted_bool(receipt, "srr2.preflight.parent_evidence_exact",
                       parent_exact);

  const bool environment_exact =
      required(receipt, "srr2.environment.device") == "cuda:0" &&
      required(receipt, "srr2.environment.dtype") == "float32" &&
      required(receipt, "srr2.environment.cpu_threads") == "1" &&
      required(receipt, "srr2.environment.cpu_interop_threads") == "1" &&
      required(receipt, "srr2.environment.deterministic_algorithms") ==
          "true" &&
      required(receipt, "srr2.environment.deterministic_warn_only") ==
          "false" &&
      required(receipt, "srr2.environment.deterministic_cudnn") == "true" &&
      required(receipt, "srr2.environment.tf32_cublas_disabled") == "true" &&
      required(receipt, "srr2.environment.tf32_cudnn_disabled") == "true" &&
      required(receipt, "srr2.environment.cublas_workspace_exact") == "true" &&
      required(receipt, "srr2.environment.cuda_available") == "true";
  const bool projection_exact =
      orthogonality >= 0.0 && orthogonality <= 1.0e-10 && contrast >= 0.0 &&
      contrast <= 1.0e-10 && block_sum >= 0.0 && block_sum <= 1.0e-10;
  bool source_independent = false;
  bool baseline_exact = false;
  try {
    source_independent = source_boundary_exact(root);
    const std::string baseline = read_bytes(
        root / ".build/tests/representation_srr2_v1_production_baseline.tar");
    baseline_exact =
        baseline.size() == kBaselineBytes && sha256(baseline) == kBaselineSha;
  } catch (const std::exception &error) {
    fail(receipt,
         std::string("preflight source/baseline verification failed: ") +
             error.what());
  }
  const bool preflight_pass =
      parent_exact && environment_exact && projection_exact && layout_exact &&
      compatibility_preboundary_exact && emitted_compatibility_setup &&
      retained.exact && repeat.exact && repeat_identity &&
      retained.purity_exact && repeat.purity_exact && source_independent &&
      baseline_exact && !boolean(receipt, "srr2.preflight.manifest_read") &&
      !boolean(receipt, "srr2.preflight.target_constructed");
  compare_emitted_bool(receipt, "srr2.preflight.pass", preflight_pass);
  expect_bool(receipt, "srr2.attempt.consumed", false);
  expect_unsigned(receipt, "authoritative_attempt_count", 0);
  constexpr std::array<std::string_view, 16> counter_names{
      "training_step_count",     "optimizer_construction_count",
      "optimizer_step_count",    "backward_call_count",
      "weight_update_count",     "augmentation_change_count",
      "target_generation_count", "probe_construction_count",
      "probe_fit_count",         "validation_selection_count",
      "prediction_count",        "permutation_count",
      "bootstrap_count",         "downstream_retraining_count",
      "end_to_end_count",        "deployment_count"};
  for (const auto name : counter_names) {
    expect_unsigned(receipt, std::string(name), 0);
  }
  constexpr std::array<std::string_view, 8> authorization_names{
      "training_authorized",
      "augmentation_change_authorized",
      "long_run_authorized",
      "active_policy_change_authorized",
      "checkpoint_migration_authorized",
      "downstream_retraining_authorized",
      "end_to_end_authorized",
      "deployment_authorized"};
  for (const auto name : authorization_names) {
    expect_bool(receipt, std::string(name), false);
  }
  if (!authorization_machine_tail_exact(raw, receipt)) {
    fail(receipt, "preflight authorization tail is not literal/exact");
  }
  close_auxiliary_schema(receipt);
  evidence.exact =
      receipt.errors.empty() && receipt.duplicate_keys == 0 &&
      receipt.malformed_lines == 0 && receipt.nonfinite_values == 0 &&
      receipt.critical_value_errors == 0 && receipt.unaccessed_keys == 0;
  evidence.errors = std::move(receipt.errors);
  return evidence;
}

[[nodiscard]] NormalizerEvidence verify_normalizer(Records &records,
                                                   Records &parent) {
  NormalizerEvidence evidence{};
  constexpr std::string_view prefix = "srr2.data.normalizer.";
  constexpr std::string_view parent_prefix = "srr.data.normalizer.normalized.";
  const auto group_begin =
      unsigned_number(records, std::string(prefix) + "group_begin");
  const auto rows = unsigned_number(records, std::string(prefix) + "rows");
  const auto parent_group_begin =
      unsigned_number(parent, std::string(parent_prefix) + "group_begin");
  const auto parent_rows =
      unsigned_number(parent, std::string(parent_prefix) + "groups");

  const std::string data_hash =
      hash16(records, std::string(prefix) + "data_hash");
  const std::string mask_hash =
      hash16(records, std::string(prefix) + "mask_hash");
  const std::string mean_hash =
      hash16(records, std::string(prefix) + "mean_hash");
  const std::string inv_std_hash =
      hash16(records, std::string(prefix) + "inv_std_hash");
  const std::string parent_data_hash =
      hash16(parent, std::string(parent_prefix) + "data_hash");
  const std::string parent_mask_hash =
      hash16(parent, std::string(parent_prefix) + "mask_hash");
  const std::string parent_mean_hash =
      hash16(parent, "srr.normalization.mean_hash");
  const std::string parent_inv_std_hash =
      hash16(parent, "srr.normalization.inv_std_hash");
  const std::string logged_parent_data =
      hash16(records, std::string(prefix) + "parent_data_hash");
  const std::string logged_parent_mask =
      hash16(records, std::string(prefix) + "parent_mask_hash");
  const std::string logged_parent_mean =
      hash16(records, std::string(prefix) + "parent_mean_hash");
  const std::string logged_parent_inv_std =
      hash16(records, std::string(prefix) + "parent_inv_std_hash");
  evidence.parent_hash_comparisons = 4;
  evidence.exact =
      group_begin == 0 && rows == 256 && group_begin == parent_group_begin &&
      rows == parent_rows && data_hash == parent_data_hash &&
      mask_hash == parent_mask_hash && mean_hash == parent_mean_hash &&
      inv_std_hash == parent_inv_std_hash &&
      logged_parent_data == parent_data_hash &&
      logged_parent_mask == parent_mask_hash &&
      logged_parent_mean == parent_mean_hash &&
      logged_parent_inv_std == parent_inv_std_hash;
  compare_emitted_bool(records, std::string(prefix) + "exact", evidence.exact);
  return evidence;
}

[[nodiscard]] CaptureEvidence verify_captures(Records &records,
                                              Records &parent) {
  CaptureEvidence evidence{};
  auto &parity = evidence.parity;
  auto &device = evidence.device;
  parity.shape_exact = true;
  parity.strides_and_contiguity_exact = true;
  parity.dtype_exact = true;
  parity.device_exact = true;
  parity.valid_mask_bytes_exact = true;
  parity.value_bytes_exact = true;
  parity.cpu64_valid_mask_bytes_exact = true;
  parity.cpu64_value_bytes_exact = true;
  parity.stable_hashes_exact = true;
  parity.repeat_capture_identity_exact = true;
  parity.per_capture_coverage_exact = true;
  parity.coverage_counts_recomputed_from_records = true;
  device.cpu64_reference_shape_exact = true;
  device.cpu64_reference_mask_bytes_exact = true;
  device.cpu64_production_reference_bytes_exact = true;
  device.cpu64_shadow_reference_bytes_exact = true;
  device.device_reference_contract_exact = true;

  std::set<std::string> observed_seeds;
  std::set<std::string> observed_datasets;
  for (std::size_t seed_index = 0; seed_index < kSeeds.size(); ++seed_index) {
    const std::string seed(kSeeds[seed_index]);
    observed_seeds.insert(seed);
    for (std::size_t dataset_index = 0; dataset_index < kDatasets.size();
         ++dataset_index) {
      const std::string dataset(kDatasets[dataset_index]);
      observed_datasets.insert(dataset);
      const std::uint64_t rows = kDatasetRows[dataset_index];
      const std::uint64_t expected_group_begin =
          kDatasetGroupBegin[dataset_index];
      const bool expected_reversed = kDatasetReversed[dataset_index];
      const std::string prefix =
          "srr2.seed_" + seed + ".capture." + dataset + ".";
      const std::string parent_prefix =
          "srr.seed_" + seed + ".capture." + dataset + ".";
      const std::string parent_data_prefix =
          "srr.data." + dataset + ".normalized.";

      const auto observed_rows = unsigned_number(records, prefix + "rows");
      const auto values = unsigned_number(records, prefix + "value_count");
      const auto validity = unsigned_number(records, prefix + "validity_count");
      const auto batch_size = unsigned_number(records, prefix + "batch_size");
      const bool coverage = observed_rows == rows && values == rows * 96 &&
                            validity == rows * 3 && batch_size == 96;
      parity.per_capture_coverage_exact &= coverage;
      ++parity.retained_capture_count;
      parity.retained_row_count += observed_rows;
      parity.retained_value_count += values;
      parity.retained_validity_count += validity;

      const auto group_begin = unsigned_number(records, prefix + "group_begin");
      const bool reversed = boolean(records, prefix + "reversed");
      const auto parent_group_begin =
          unsigned_number(parent, parent_data_prefix + "group_begin");
      const auto parent_groups =
          unsigned_number(parent, parent_data_prefix + "groups");
      const std::string input_data_hash =
          hash16(records, prefix + "input_data_hash");
      const std::string input_mask_hash =
          hash16(records, prefix + "input_mask_hash");
      const std::string parent_input_data_hash =
          hash16(parent, parent_data_prefix + "data_hash");
      const std::string parent_input_mask_hash =
          hash16(parent, parent_data_prefix + "mask_hash");
      const std::string logged_parent_input_data_hash =
          hash16(records, prefix + "parent_input_data_hash");
      const std::string logged_parent_input_mask_hash =
          hash16(records, prefix + "parent_input_mask_hash");
      evidence.parent_hash_comparisons += 2;
      const bool input_hashes =
          group_begin == expected_group_begin &&
          group_begin == parent_group_begin && parent_groups == rows &&
          reversed == expected_reversed &&
          input_data_hash == parent_input_data_hash &&
          input_mask_hash == parent_input_mask_hash &&
          logged_parent_input_data_hash == parent_input_data_hash &&
          logged_parent_input_mask_hash == parent_input_mask_hash;
      evidence.input_hashes_exact &= input_hashes;
      evidence.parent_source_hashes_exact &= input_hashes;
      compare_emitted_bool(records, prefix + "parent_input_hashes_exact",
                           input_hashes);

      const std::string rows_text = std::to_string(rows);
      const bool values_shape =
          required(records, prefix + "shape") == rows_text + ",3,32";
      const bool mask_shape =
          required(records, prefix + "mask_shape") == rows_text + ",3";
      const bool shape = values_shape && mask_shape;
      parity.shape_exact &= shape;
      device.cpu64_reference_shape_exact &= shape;
      const bool values_stride =
          required(records, prefix + "values_stride") == "96,32,1";
      const bool mask_stride =
          required(records, prefix + "mask_stride") == "3,1";
      const bool values_contiguous =
          boolean(records, prefix + "values_contiguous");
      const bool mask_contiguous = boolean(records, prefix + "mask_contiguous");
      const bool strides =
          values_stride && mask_stride && values_contiguous && mask_contiguous;
      parity.strides_and_contiguity_exact &= strides;
      const bool dtype = required(records, prefix + "dtype") == "float32";
      const bool on_device = required(records, prefix + "device") == "cuda:0";
      parity.dtype_exact &= dtype;
      parity.device_exact &= on_device;
      device.device_reference_contract_exact &=
          shape && strides && dtype && on_device;

      const bool capture_layout =
          required_true(records, prefix + "layout_exact");
      const bool complete = required_true(records, prefix + "complete");

      const std::string parent_encoder =
          hash16(parent, parent_prefix + "encoder_hash");
      const std::string parent_served =
          hash16(parent, parent_prefix + "served_hash");
      const std::string parent_metadata =
          hash16(parent, parent_prefix + "metadata_structure_hash");
      const std::string parent_reference =
          hash16(parent, parent_prefix + "audit_hash");
      const std::string parent_shadow =
          hash16(parent, parent_prefix + "shadow_hash");
      evidence.parent_hash_comparisons += 5;

      const std::string encoder = hash16(records, prefix + "encoder_hash");
      const std::string served = hash16(records, prefix + "served_hash");
      const std::string metadata =
          hash16(records, prefix + "metadata_structure_hash");
      const std::string logged_parent_encoder =
          hash16(records, prefix + "parent_encoder_hash");
      const std::string logged_parent_served =
          hash16(records, prefix + "parent_served_hash");
      const std::string logged_parent_reference =
          hash16(records, prefix + "parent_reference_hash");
      const std::string logged_parent_shadow =
          hash16(records, prefix + "parent_shadow_hash");
      const std::string production =
          hash16(records, prefix + "production_hash");
      const std::string shadow = hash16(records, prefix + "shadow_hash");
      const std::string reference = hash16(records, prefix + "reference_hash");
      const std::string cpu64_production =
          hash16(records, prefix + "cpu64_production_hash");
      const std::string cpu64_shadow =
          hash16(records, prefix + "cpu64_shadow_hash");
      const bool source_hashes = input_hashes && encoder == parent_encoder &&
                                 served == parent_served &&
                                 metadata == parent_metadata &&
                                 logged_parent_encoder == parent_encoder &&
                                 logged_parent_served == parent_served;
      evidence.parent_source_hashes_exact &= source_hashes;
      const bool reference_hash = reference == parent_reference &&
                                  cpu64_production == parent_reference &&
                                  cpu64_shadow == parent_reference &&
                                  logged_parent_reference == parent_reference;
      const bool shadow_hash = production == parent_shadow &&
                               shadow == parent_shadow &&
                               logged_parent_shadow == parent_shadow;
      const bool production_shadow_hash = production == shadow;
      const bool cpu64_production_shadow_hash =
          cpu64_production == cpu64_shadow;
      const bool cpu64_production_reference_hash =
          cpu64_production == reference;
      const bool cpu64_shadow_reference_hash = cpu64_shadow == reference;
      evidence.cpu64_hashes_exact &= cpu64_production_shadow_hash &&
                                     cpu64_production_reference_hash &&
                                     cpu64_shadow_reference_hash;
      evidence.parent_reference_hashes_exact &= reference_hash;
      evidence.parent_shadow_hashes_exact &= shadow_hash;
      parity.stable_hashes_exact &= source_hashes && reference_hash &&
                                    shadow_hash && production_shadow_hash &&
                                    input_hashes;
      const bool emitted_parent_source =
          boolean(records, prefix + "parent_source_hashes_exact");
      const bool emitted_parent_reference =
          boolean(records, prefix + "parent_reference_hash_exact");
      const bool emitted_parent_shadow =
          boolean(records, prefix + "parent_shadow_hash_exact");
      if (emitted_parent_source != source_hashes ||
          emitted_parent_reference != reference_hash ||
          emitted_parent_shadow != shadow_hash) {
        fail(records, "emitted parent hash fact disagrees: " + prefix);
      }

      const std::string production_mask =
          hash16(records, prefix + "production_mask_hash");
      const std::string shadow_mask =
          hash16(records, prefix + "shadow_mask_hash");
      const std::string reference_mask =
          hash16(records, prefix + "reference_mask_hash");
      const auto retained_primitives = verify_capture_primitives(
          records, prefix, rows, parent_input_data_hash, parent_input_mask_hash,
          reference_mask);
      const bool retained_layout_metadata =
          coverage && shape && strides && dtype && on_device && source_hashes &&
          retained_primitives.counts_exact &&
          retained_primitives.same_encoded_object &&
          retained_primitives.public_selector_sandwich_exact;
      evidence.canonical_layout_metadata_exact &= retained_layout_metadata;
      const bool recomputed_complete = retained_primitives.complete_inputs &&
                                       shape && strides && dtype && on_device &&
                                       capture_layout;
      if (complete != recomputed_complete) {
        fail(records, "emitted capture completeness disagrees: " + prefix);
      }
      evidence.capture_contracts_exact &= recomputed_complete;
      evidence.purity_exact &= retained_primitives.input_unchanged &&
                               retained_primitives.parameter_unchanged &&
                               retained_primitives.buffer_unchanged &&
                               retained_primitives.cpu_rng_unchanged &&
                               retained_primitives.cuda_rng_unchanged &&
                               retained_primitives.model_mode_unchanged;
      evidence.finite_exact &= retained_primitives.finite;
      const bool masks_equal =
          production_mask == shadow_mask && shadow_mask == reference_mask;
      const bool production_shadow_mask_bytes =
          boolean(records, prefix + "production_shadow_mask_bytes_exact");
      parity.valid_mask_bytes_exact &=
          production_shadow_mask_bytes && masks_equal;
      const bool production_shadow_value_bytes =
          boolean(records, prefix + "production_shadow_value_bytes_exact");
      parity.value_bytes_exact &= production_shadow_value_bytes;
      const bool cpu64_production_shadow_value_bytes = required_true(
          records, prefix + "cpu64_production_shadow_value_bytes_exact");
      parity.cpu64_value_bytes_exact &=
          cpu64_production_shadow_value_bytes && cpu64_production_shadow_hash;
      const bool cpu64_production_shadow_mask_bytes = required_true(
          records, prefix + "cpu64_production_shadow_mask_bytes_exact");
      parity.cpu64_valid_mask_bytes_exact &=
          cpu64_production_shadow_mask_bytes && masks_equal &&
          retained_primitives.cpu64_masks_match_reference;
      const bool cpu64_production_reference_value_bytes = required_true(
          records, prefix + "cpu64_production_reference_value_bytes_exact");
      const bool cpu64_shadow_reference_value_bytes = required_true(
          records, prefix + "cpu64_shadow_reference_value_bytes_exact");
      device.cpu64_production_reference_bytes_exact &=
          cpu64_production_reference_value_bytes &&
          cpu64_production_reference_hash;
      device.cpu64_shadow_reference_bytes_exact &=
          cpu64_shadow_reference_value_bytes && cpu64_shadow_reference_hash;
      const bool cpu64_production_reference_mask_bytes = required_true(
          records, prefix + "cpu64_production_reference_mask_bytes_exact");
      const bool cpu64_shadow_reference_mask_bytes = required_true(
          records, prefix + "cpu64_shadow_reference_mask_bytes_exact");
      device.cpu64_reference_mask_bytes_exact &=
          cpu64_production_reference_mask_bytes &&
          cpu64_shadow_reference_mask_bytes && masks_equal &&
          retained_primitives.cpu64_masks_match_reference;
      parity.value_bytes_exact &= production == shadow;

      const double production_shadow_device =
          finite_number(records, prefix + "production_shadow_device_max_abs");
      const double cpu64_production_shadow =
          finite_number(records, prefix + "cpu64_production_shadow_max_abs");
      const double cpu64_production_reference =
          finite_number(records, prefix + "cpu64_production_reference_max_abs");
      const double cpu64_shadow_reference =
          finite_number(records, prefix + "cpu64_shadow_reference_max_abs");
      const double device_production_reference = finite_number(
          records, prefix + "device_production_reference_max_abs");
      const double device_shadow_reference =
          finite_number(records, prefix + "device_shadow_reference_max_abs");
      const std::array<double, 6> maxima{
          production_shadow_device,    cpu64_production_shadow,
          cpu64_production_reference,  cpu64_shadow_reference,
          device_production_reference, device_shadow_reference};
      if (std::any_of(maxima.begin(), maxima.end(),
                      [](double value) { return value < 0.0; })) {
        fail(records, "negative maximum error: " + prefix);
      }
      const bool recomputed_production_shadow_values =
          production == shadow && production_shadow_device == 0.0;
      const bool recomputed_production_shadow_masks = masks_equal;
      const bool recomputed_cpu64_production_shadow_values =
          cpu64_production_shadow_hash && cpu64_production_shadow == 0.0;
      const bool recomputed_cpu64_masks =
          retained_primitives.cpu64_masks_match_reference && masks_equal;
      const bool recomputed_cpu64_production_reference_values =
          cpu64_production_reference_hash && cpu64_production_reference == 0.0;
      const bool recomputed_cpu64_shadow_reference_values =
          cpu64_shadow_reference_hash && cpu64_shadow_reference == 0.0;
      compare_emitted_bool(records,
                           prefix + "production_shadow_value_bytes_exact",
                           recomputed_production_shadow_values);
      compare_emitted_bool(records,
                           prefix + "production_shadow_mask_bytes_exact",
                           recomputed_production_shadow_masks);
      compare_emitted_bool(records,
                           prefix + "cpu64_production_shadow_value_bytes_exact",
                           recomputed_cpu64_production_shadow_values);
      compare_emitted_bool(records,
                           prefix + "cpu64_production_shadow_mask_bytes_exact",
                           recomputed_cpu64_masks);
      compare_emitted_bool(
          records, prefix + "cpu64_production_reference_value_bytes_exact",
          recomputed_cpu64_production_reference_values);
      compare_emitted_bool(
          records, prefix + "cpu64_production_reference_mask_bytes_exact",
          recomputed_cpu64_masks);
      compare_emitted_bool(records,
                           prefix + "cpu64_shadow_reference_value_bytes_exact",
                           recomputed_cpu64_shadow_reference_values);
      compare_emitted_bool(records,
                           prefix + "cpu64_shadow_reference_mask_bytes_exact",
                           recomputed_cpu64_masks);
      parity.value_bytes_exact &= recomputed_production_shadow_values;
      parity.valid_mask_bytes_exact &= recomputed_production_shadow_masks;
      parity.cpu64_value_bytes_exact &=
          recomputed_cpu64_production_shadow_values;
      parity.cpu64_valid_mask_bytes_exact &= recomputed_cpu64_masks;
      device.cpu64_production_reference_bytes_exact &=
          recomputed_cpu64_production_reference_values;
      device.cpu64_shadow_reference_bytes_exact &=
          recomputed_cpu64_shadow_reference_values;
      device.cpu64_reference_mask_bytes_exact &= recomputed_cpu64_masks;
      parity.device_max_abs =
          std::max(parity.device_max_abs, production_shadow_device);
      parity.cpu64_max_abs =
          std::max(parity.cpu64_max_abs, cpu64_production_shadow);
      device.cpu64_production_reference_max_abs =
          std::max(device.cpu64_production_reference_max_abs,
                   cpu64_production_reference);
      device.cpu64_shadow_reference_max_abs = std::max(
          device.cpu64_shadow_reference_max_abs, cpu64_shadow_reference);
      device.device_production_reference_max_abs =
          std::max(device.device_production_reference_max_abs,
                   device_production_reference);
      device.device_shadow_reference_max_abs = std::max(
          device.device_shadow_reference_max_abs, device_shadow_reference);

      parity.value_bytes_exact &= retained_primitives.invalid_zero_exact;

      const auto repeat_rows = unsigned_number(records, prefix + "repeat_rows");
      const auto repeat_values =
          unsigned_number(records, prefix + "repeat_value_count");
      const auto repeat_validity =
          unsigned_number(records, prefix + "repeat_validity_count");
      const auto repeat_batch_size =
          unsigned_number(records, prefix + "repeat_batch_size");
      const bool repeat_coverage =
          repeat_rows == rows && repeat_values == rows * 96 &&
          repeat_validity == rows * 3 && repeat_batch_size == 96;
      parity.per_capture_coverage_exact &= repeat_coverage;
      ++parity.repeat_capture_count;
      parity.repeat_row_count += repeat_rows;
      parity.repeat_value_count += repeat_values;
      parity.repeat_validity_count += repeat_validity;

      const bool repeat_values_shape =
          required(records, prefix + "repeat_shape") == rows_text + ",3,32";
      const bool repeat_mask_shape =
          required(records, prefix + "repeat_mask_shape") == rows_text + ",3";
      const bool repeat_shape = repeat_values_shape && repeat_mask_shape;
      const bool repeat_values_stride =
          required(records, prefix + "repeat_values_stride") == "96,32,1";
      const bool repeat_mask_stride =
          required(records, prefix + "repeat_mask_stride") == "3,1";
      const bool repeat_values_contiguous =
          boolean(records, prefix + "repeat_values_contiguous");
      const bool repeat_mask_contiguous =
          boolean(records, prefix + "repeat_mask_contiguous");
      const bool repeat_strides = repeat_values_stride && repeat_mask_stride &&
                                  repeat_values_contiguous &&
                                  repeat_mask_contiguous;
      const bool repeat_dtype =
          required(records, prefix + "repeat_dtype") == "float32";
      const bool repeat_device =
          required(records, prefix + "repeat_device") == "cuda:0";
      const bool repeat_layout =
          boolean(records, prefix + "repeat_layout_exact");
      const bool emitted_repeat_complete =
          boolean(records, prefix + "repeat_complete");
      parity.shape_exact &= repeat_shape;
      parity.strides_and_contiguity_exact &= repeat_strides;
      parity.dtype_exact &= repeat_dtype;
      parity.device_exact &= repeat_device;

      const std::string repeat_encoder =
          hash16(records, prefix + "repeat_encoder_hash");
      const std::string repeat_served =
          hash16(records, prefix + "repeat_served_hash");
      const std::string repeat_metadata =
          hash16(records, prefix + "repeat_metadata_structure_hash");
      const std::string repeat_production =
          hash16(records, prefix + "repeat_production_hash");
      const std::string repeat_shadow =
          hash16(records, prefix + "repeat_shadow_hash");
      const std::string repeat_reference =
          hash16(records, prefix + "repeat_reference_hash");
      const std::string repeat_production_mask =
          hash16(records, prefix + "repeat_production_mask_hash");
      const std::string repeat_shadow_mask =
          hash16(records, prefix + "repeat_shadow_mask_hash");
      const std::string repeat_reference_mask =
          hash16(records, prefix + "repeat_reference_mask_hash");
      const std::string repeat_cpu64_production =
          hash16(records, prefix + "repeat_cpu64_production_hash");
      const std::string repeat_cpu64_shadow =
          hash16(records, prefix + "repeat_cpu64_shadow_hash");
      const auto repeat_primitives = verify_capture_primitives(
          records, prefix + "repeat_", rows, parent_input_data_hash,
          parent_input_mask_hash, repeat_reference_mask);
      const bool repeat_layout_metadata =
          repeat_coverage && repeat_shape && repeat_strides && repeat_dtype &&
          repeat_device && repeat_encoder == encoder &&
          repeat_served == served && repeat_metadata == metadata &&
          repeat_primitives.counts_exact &&
          repeat_primitives.same_encoded_object &&
          repeat_primitives.public_selector_sandwich_exact;
      evidence.canonical_layout_metadata_exact &= repeat_layout_metadata;
      const bool recomputed_repeat_complete =
          repeat_coverage && repeat_primitives.complete_inputs &&
          repeat_shape && repeat_strides && repeat_dtype && repeat_device &&
          repeat_layout;
      if (emitted_repeat_complete != recomputed_repeat_complete) {
        fail(records, "emitted repeat completeness disagrees: " + prefix);
      }
      const bool repeat_purity = repeat_primitives.input_unchanged &&
                                 repeat_primitives.parameter_unchanged &&
                                 repeat_primitives.buffer_unchanged &&
                                 repeat_primitives.cpu_rng_unchanged &&
                                 repeat_primitives.cuda_rng_unchanged &&
                                 repeat_primitives.model_mode_unchanged;

      const bool repeat_masks_equal =
          repeat_production_mask == repeat_shadow_mask &&
          repeat_shadow_mask == repeat_reference_mask;
      const bool repeat_cpu64_production_shadow_hash =
          repeat_cpu64_production == repeat_cpu64_shadow;
      const bool repeat_cpu64_production_reference_hash =
          repeat_cpu64_production == repeat_reference;
      const bool repeat_cpu64_shadow_reference_hash =
          repeat_cpu64_shadow == repeat_reference;
      const double repeat_production_shadow_device = finite_number(
          records, prefix + "repeat_production_shadow_device_max_abs");
      const double repeat_cpu64_production_shadow = finite_number(
          records, prefix + "repeat_cpu64_production_shadow_max_abs");
      const double repeat_cpu64_production_reference = finite_number(
          records, prefix + "repeat_cpu64_production_reference_max_abs");
      const double repeat_cpu64_shadow_reference = finite_number(
          records, prefix + "repeat_cpu64_shadow_reference_max_abs");
      const double repeat_device_production_reference = finite_number(
          records, prefix + "repeat_device_production_reference_max_abs");
      const double repeat_device_shadow_reference = finite_number(
          records, prefix + "repeat_device_shadow_reference_max_abs");
      const std::array<double, 6> repeat_maxima{
          repeat_production_shadow_device,    repeat_cpu64_production_shadow,
          repeat_cpu64_production_reference,  repeat_cpu64_shadow_reference,
          repeat_device_production_reference, repeat_device_shadow_reference};
      if (std::any_of(repeat_maxima.begin(), repeat_maxima.end(),
                      [](double value) { return value < 0.0; })) {
        fail(records, "negative repeat maximum error: " + prefix);
      }
      const bool repeat_production_shadow_values =
          repeat_production == repeat_shadow &&
          repeat_production_shadow_device == 0.0;
      const bool repeat_production_shadow_masks = repeat_masks_equal;
      const bool repeat_cpu64_production_shadow_values =
          repeat_cpu64_production_shadow_hash &&
          repeat_cpu64_production_shadow == 0.0;
      const bool repeat_cpu64_masks =
          repeat_primitives.cpu64_masks_match_reference && repeat_masks_equal;
      const bool repeat_cpu64_production_reference_values =
          repeat_cpu64_production_reference_hash &&
          repeat_cpu64_production_reference == 0.0;
      const bool repeat_cpu64_shadow_reference_values =
          repeat_cpu64_shadow_reference_hash &&
          repeat_cpu64_shadow_reference == 0.0;
      compare_emitted_bool(
          records, prefix + "repeat_production_shadow_value_bytes_exact",
          repeat_production_shadow_values);
      compare_emitted_bool(records,
                           prefix + "repeat_production_shadow_mask_bytes_exact",
                           repeat_production_shadow_masks);
      compare_emitted_bool(
          records, prefix + "repeat_cpu64_production_shadow_value_bytes_exact",
          repeat_cpu64_production_shadow_values);
      compare_emitted_bool(
          records, prefix + "repeat_cpu64_production_shadow_mask_bytes_exact",
          repeat_cpu64_masks);
      compare_emitted_bool(
          records,
          prefix + "repeat_cpu64_production_reference_value_bytes_exact",
          repeat_cpu64_production_reference_values);
      compare_emitted_bool(
          records,
          prefix + "repeat_cpu64_production_reference_mask_bytes_exact",
          repeat_cpu64_masks);
      compare_emitted_bool(
          records, prefix + "repeat_cpu64_shadow_reference_value_bytes_exact",
          repeat_cpu64_shadow_reference_values);
      compare_emitted_bool(
          records, prefix + "repeat_cpu64_shadow_reference_mask_bytes_exact",
          repeat_cpu64_masks);

      parity.valid_mask_bytes_exact &= repeat_production_shadow_masks;
      parity.value_bytes_exact &= repeat_production_shadow_values;
      parity.cpu64_valid_mask_bytes_exact &= repeat_cpu64_masks;
      parity.cpu64_value_bytes_exact &= repeat_cpu64_production_shadow_values;
      device.cpu64_reference_shape_exact &= repeat_shape;
      device.cpu64_reference_mask_bytes_exact &= repeat_cpu64_masks;
      device.cpu64_production_reference_bytes_exact &=
          repeat_cpu64_production_reference_values;
      device.cpu64_shadow_reference_bytes_exact &=
          repeat_cpu64_shadow_reference_values;
      device.device_reference_contract_exact &=
          repeat_shape && repeat_strides && repeat_dtype && repeat_device &&
          repeat_primitives.finite;
      parity.cpu64_max_abs =
          std::max(parity.cpu64_max_abs, repeat_cpu64_production_shadow);
      parity.device_max_abs =
          std::max(parity.device_max_abs, repeat_production_shadow_device);
      device.cpu64_production_reference_max_abs =
          std::max(device.cpu64_production_reference_max_abs,
                   repeat_cpu64_production_reference);
      device.cpu64_shadow_reference_max_abs = std::max(
          device.cpu64_shadow_reference_max_abs, repeat_cpu64_shadow_reference);
      device.device_production_reference_max_abs =
          std::max(device.device_production_reference_max_abs,
                   repeat_device_production_reference);
      device.device_shadow_reference_max_abs =
          std::max(device.device_shadow_reference_max_abs,
                   repeat_device_shadow_reference);

      const bool repeat_identity =
          recomputed_repeat_complete && repeat_encoder == encoder &&
          repeat_served == served && repeat_metadata == metadata &&
          repeat_production == production && repeat_shadow == shadow &&
          repeat_reference == reference &&
          repeat_cpu64_production == cpu64_production &&
          repeat_cpu64_shadow == cpu64_shadow &&
          repeat_production_mask == production_mask &&
          repeat_shadow_mask == shadow_mask &&
          repeat_reference_mask == reference_mask &&
          repeat_primitives.cpu64_production_mask_hash ==
              retained_primitives.cpu64_production_mask_hash &&
          repeat_primitives.cpu64_shadow_mask_hash ==
              retained_primitives.cpu64_shadow_mask_hash &&
          repeat_production_shadow_device == production_shadow_device &&
          repeat_cpu64_production_shadow == cpu64_production_shadow &&
          repeat_cpu64_production_reference == cpu64_production_reference &&
          repeat_cpu64_shadow_reference == cpu64_shadow_reference &&
          repeat_device_production_reference == device_production_reference &&
          repeat_device_shadow_reference == device_shadow_reference;
      const bool emitted_repeat =
          boolean(records, prefix + "repeat_identity_exact");
      if (emitted_repeat != repeat_identity) {
        fail(records, "repeat identity fact disagrees: " + prefix);
      }
      parity.repeat_capture_identity_exact &= repeat_identity;
      parity.stable_hashes_exact &= repeat_identity;
      evidence.deterministic_exact &= repeat_identity;
      evidence.capture_contracts_exact &= recomputed_repeat_complete;
      evidence.purity_exact &= repeat_purity;
      evidence.finite_exact &= repeat_primitives.finite;
    }
  }
  parity.seed_count = observed_seeds.size();
  parity.dataset_count = observed_datasets.size();
  return evidence;
}

struct InputEvidence {
  gate::GateInput input{};
  CaptureEvidence captures{};
  NormalizerEvidence normalizer{};
  bool projection_exact{false};
  bool layout_exact{false};
  bool source_independent{false};
};

[[nodiscard]] InputEvidence build_gate_input(
    Records &records, Records &parent, const std::filesystem::path &root,
    const ManifestEvidence &manifest, const ParentEvidence &parent_evidence,
    const MechanicsEvidence &mechanics_evidence,
    const AttemptLedgerEvidence &attempt_ledger, bool audit_input_exact) {
  InputEvidence evidence{};
  auto &input = evidence.input;

  expect_text(records, "schema", kSchema);
  expect_text(records, "experiment", "production-structured-readout-parity");
  expect_text(records, "device", "cuda:0");
  expect_text(records, "dtype", "float32");
  expect_text(records, "authoritative_command", kExpectedCommand);
  expect_text(records, "srr2.environment.device", "cuda:0");
  expect_text(records, "srr2.environment.dtype", "float32");
  const auto cpu_threads =
      unsigned_number(records, "srr2.environment.cpu_threads");
  const auto cpu_interop_threads =
      unsigned_number(records, "srr2.environment.cpu_interop_threads");
  const bool deterministic_algorithms =
      boolean(records, "srr2.environment.deterministic_algorithms");
  const bool deterministic_warn_only =
      boolean(records, "srr2.environment.deterministic_warn_only");
  const bool deterministic_cudnn =
      boolean(records, "srr2.environment.deterministic_cudnn");
  const bool tf32_cublas_disabled =
      boolean(records, "srr2.environment.tf32_cublas_disabled");
  const bool tf32_cudnn_disabled =
      boolean(records, "srr2.environment.tf32_cudnn_disabled");
  const bool cublas_workspace_exact =
      boolean(records, "srr2.environment.cublas_workspace_exact");
  const bool cuda_available =
      boolean(records, "srr2.environment.cuda_available");
  const bool environment_exact =
      cpu_threads == 1 && cpu_interop_threads == 1 &&
      deterministic_algorithms && !deterministic_warn_only &&
      deterministic_cudnn && tf32_cublas_disabled && tf32_cudnn_disabled &&
      cublas_workspace_exact && cuda_available;

  const std::string q0_hash = hash16(records, "srr2.projection.q0_hash");
  const std::string qpsm_hash = hash16(records, "srr2.projection.qpsm_hash");
  const double orthogonality =
      finite_number(records, "srr2.projection.orthogonality_error");
  const double contrast =
      finite_number(records, "srr2.projection.contrast_mean_error");
  const double block_sum =
      finite_number(records, "srr2.projection.block_sum_error");
  evidence.projection_exact =
      q0_hash == "f8c9f35282de2ee0" && qpsm_hash == "ac8a43fd65b2c8a8" &&
      orthogonality >= 0.0 && contrast >= 0.0 && block_sum >= 0.0 &&
      orthogonality <= 1.0e-10 && contrast <= 1.0e-10 && block_sum <= 1.0e-10;
  const std::string cell_vector = required(records, "srr2.layout.cell_vector");
  const std::string layout_hash = hash16(records, "srr2.layout.hash");
  evidence.layout_exact = recompute_canonical_plan(cell_vector, layout_hash,
                                                   manifest.token_layout_hash);
  compare_emitted_bool(records, "srr2.layout.exact", evidence.layout_exact);
  try {
    evidence.source_independent = source_boundary_exact(root);
  } catch (const std::exception &) {
    evidence.source_independent = false;
    fail(records, "production source is unreadable");
  }

  evidence.normalizer = verify_normalizer(records, parent);
  evidence.captures = verify_captures(records, parent);
  input.parity = evidence.captures.parity;
  input.device_translation = evidence.captures.device;

  input.mechanics.authoritative_attempt_count =
      unsigned_number(records, "authoritative_attempt_count");
  const bool attempt_consumed = boolean(records, "srr2.attempt.consumed");
  const auto marker =
      std::find(records.ordered_keys.begin(), records.ordered_keys.end(),
                "srr2.attempt.consumed");
  const auto first_capture = std::find_if(
      records.ordered_keys.begin(), records.ordered_keys.end(),
      [](const std::string &key) { return key.rfind("srr2.seed_", 0) == 0; });
  const bool attempt_order_exact =
      marker != records.ordered_keys.end() &&
      first_capture != records.ordered_keys.end() &&
      marker + 1 == first_capture;
  input.mechanics.authorizations = {
      .training_authorized = boolean(records, "training_authorized"),
      .augmentation_change_authorized =
          boolean(records, "augmentation_change_authorized"),
      .long_run_authorized = boolean(records, "long_run_authorized"),
      .active_policy_change_authorized =
          boolean(records, "active_policy_change_authorized"),
      .checkpoint_migration_authorized =
          boolean(records, "checkpoint_migration_authorized"),
      .downstream_retraining_authorized =
          boolean(records, "downstream_retraining_authorized"),
      .end_to_end_authorized = boolean(records, "end_to_end_authorized"),
      .deployment_authorized = boolean(records, "deployment_authorized")};
  input.mechanics.counters = {
      .training_step_count = unsigned_number(records, "training_step_count"),
      .optimizer_construction_count =
          unsigned_number(records, "optimizer_construction_count"),
      .optimizer_step_count = unsigned_number(records, "optimizer_step_count"),
      .backward_call_count = unsigned_number(records, "backward_call_count"),
      .weight_update_count = unsigned_number(records, "weight_update_count"),
      .augmentation_change_count =
          unsigned_number(records, "augmentation_change_count"),
      .target_generation_count =
          unsigned_number(records, "target_generation_count"),
      .probe_construction_count =
          unsigned_number(records, "probe_construction_count"),
      .probe_fit_count = unsigned_number(records, "probe_fit_count"),
      .validation_selection_count =
          unsigned_number(records, "validation_selection_count"),
      .prediction_count = unsigned_number(records, "prediction_count"),
      .permutation_count = unsigned_number(records, "permutation_count"),
      .bootstrap_count = unsigned_number(records, "bootstrap_count"),
      .downstream_retraining_count =
          unsigned_number(records, "downstream_retraining_count"),
      .end_to_end_count = unsigned_number(records, "end_to_end_count"),
      .deployment_count = unsigned_number(records, "deployment_count")};

  input.mechanics.local_contracts_exact = mechanics_evidence.exact;
  input.mechanics.source_boundary_exact = evidence.source_independent;
  input.mechanics.command_exact =
      required(records, "authoritative_command") == kExpectedCommand;
  input.mechanics.environment_exact = environment_exact;
  input.mechanics.cuda_available = cuda_available;
  input.mechanics.attempt_marker_exact =
      attempt_consumed && attempt_order_exact && attempt_ledger.exact &&
      attempt_ledger.attempt_count == 1 &&
      input.mechanics.authoritative_attempt_count ==
          attempt_ledger.attempt_count;
  input.mechanics.capture_contracts_exact =
      evidence.captures.capture_contracts_exact;
  input.mechanics.purity_exact = evidence.captures.purity_exact;
  input.mechanics.finite_outputs_exact = evidence.captures.finite_exact;
  input.mechanics.deterministic_execution_exact =
      evidence.captures.deterministic_exact;
  input.mechanics.manifest_exact = manifest.exact;
  input.mechanics.audit_input_exact = audit_input_exact;

  compare_emitted_bool(records, "srr2.mechanics.local_contracts_exact",
                       input.mechanics.local_contracts_exact);
  compare_emitted_bool(records, "srr2.mechanics.source_boundary_exact",
                       input.mechanics.source_boundary_exact);
  compare_emitted_bool(records, "srr2.mechanics.command_exact",
                       input.mechanics.command_exact);
  compare_emitted_bool(records, "srr2.mechanics.environment_exact",
                       input.mechanics.environment_exact);
  compare_emitted_bool(records, "srr2.mechanics.cuda_available",
                       input.mechanics.cuda_available);
  compare_emitted_bool(records, "srr2.mechanics.attempt_marker_exact",
                       input.mechanics.attempt_marker_exact);
  compare_emitted_unsigned(records,
                           "srr2.mechanics.authoritative_attempt_count",
                           input.mechanics.authoritative_attempt_count);
  compare_emitted_bool(records, "srr2.mechanics.capture_contracts_exact",
                       input.mechanics.capture_contracts_exact);
  compare_emitted_bool(records, "srr2.mechanics.purity_exact",
                       input.mechanics.purity_exact);
  compare_emitted_bool(records, "srr2.mechanics.finite_outputs_exact",
                       input.mechanics.finite_outputs_exact);
  compare_emitted_bool(records, "srr2.mechanics.deterministic_execution_exact",
                       input.mechanics.deterministic_execution_exact);
  compare_emitted_bool(records, "srr2.mechanics.manifest_exact",
                       input.mechanics.manifest_exact);

  input.parent = {.artifacts_exact = parent_evidence.artifacts_exact,
                  .hashes_exact = parent_evidence.hashes_exact,
                  .terminal_classification_exact =
                      parent_evidence.classification_exact,
                  .audit_pass = parent_evidence.audit_pass,
                  .authorizations_false = parent_evidence.authorizations_false,
                  .authoritative_attempt_count = parent_evidence.attempt_count,
                  .audit_error_count = parent_evidence.audit_error_count,
                  .optimizer_step_count = parent_evidence.optimizer_steps,
                  .backward_call_count = parent_evidence.backward_calls};
  compare_emitted_bool(records, "srr2.parent.artifacts_exact",
                       input.parent.artifacts_exact);
  compare_emitted_bool(records, "srr2.parent.hashes_exact",
                       input.parent.hashes_exact);
  compare_emitted_bool(records, "srr2.parent.terminal_classification_exact",
                       input.parent.terminal_classification_exact);
  compare_emitted_bool(records, "srr2.parent.audit_pass",
                       input.parent.audit_pass);
  compare_emitted_bool(records, "srr2.parent.authorizations_false",
                       input.parent.authorizations_false);
  compare_emitted_unsigned(records, "srr2.parent.authoritative_attempt_count",
                           input.parent.authoritative_attempt_count);
  compare_emitted_unsigned(records, "srr2.parent.audit_error_count",
                           input.parent.audit_error_count);
  compare_emitted_unsigned(records, "srr2.parent.optimizer_step_count",
                           input.parent.optimizer_step_count);
  compare_emitted_unsigned(records, "srr2.parent.backward_call_count",
                           input.parent.backward_call_count);

  input.compatibility = mechanics_evidence.compatibility;
  bool live_dsl_all_tokens = false;
  try {
    const auto active_dsl = read_bytes(
        root / "src/config/wikimyei.representation.mtf_jepa_mae_vicreg.dsl");
    live_dsl_all_tokens =
        active_dsl.find("SERVING_POOL_POLICY = all_tokens;") !=
            std::string::npos &&
        active_dsl.find("SERVING_POOL_POLICY = structured_cdsb_v1;") ==
            std::string::npos;
  } catch (const std::exception &) {
    fail(records, "active DSL is unreadable");
  }
  if (input.compatibility.active_dsl_all_tokens != live_dsl_all_tokens) {
    fail(records, "active DSL compatibility fact disagrees with live file");
  }
  input.compatibility.active_dsl_all_tokens &= live_dsl_all_tokens;
  const auto compare_compatibility = [&](const char *name, bool value) {
    compare_emitted_bool(records, std::string("srr2.compatibility.") + name,
                         value);
  };
  compare_compatibility("legacy_enum_ordinals_exact",
                        input.compatibility.legacy_enum_ordinals_exact);
  compare_compatibility("legacy_policy_names_exact",
                        input.compatibility.legacy_policy_names_exact);
  compare_compatibility("structured_policy_appended",
                        input.compatibility.structured_policy_appended);
  compare_compatibility("structured_policy_name_exact",
                        input.compatibility.structured_policy_name_exact);
  compare_compatibility("parser_round_trip_exact",
                        input.compatibility.parser_round_trip_exact);
  compare_compatibility("unknown_policy_rejected",
                        input.compatibility.unknown_policy_rejected);
  compare_compatibility("cpp_default_all_tokens",
                        input.compatibility.cpp_default_all_tokens);
  compare_compatibility("omitted_dsl_all_tokens",
                        input.compatibility.omitted_dsl_all_tokens);
  compare_compatibility("active_dsl_all_tokens",
                        input.compatibility.active_dsl_all_tokens);
  compare_compatibility("protocol_fingerprint_distinct",
                        input.compatibility.protocol_fingerprint_distinct);
  compare_compatibility(
      "structured_checkpoint_round_trip_exact",
      input.compatibility.structured_checkpoint_round_trip_exact);
  compare_compatibility("legacy_checkpoint_all_tokens",
                        input.compatibility.legacy_checkpoint_all_tokens);
  compare_compatibility(
      "legacy_checkpoint_does_not_inherit_structured",
      input.compatibility.legacy_checkpoint_does_not_inherit_structured);
  compare_compatibility("checkpoint_mismatch_rejected",
                        input.compatibility.checkpoint_mismatch_rejected);
  compare_compatibility("malformed_checkpoint_rejected",
                        input.compatibility.malformed_checkpoint_rejected);
  compare_compatibility("legacy_policy_bytes_exact",
                        input.compatibility.legacy_policy_bytes_exact);
  compare_compatibility("public_selector_contract_exact",
                        input.compatibility.public_selector_contract_exact);
  compare_compatibility(
      "adapter_reaches_structured_selector",
      input.compatibility.adapter_reaches_structured_selector);

  input.sealed_reference = {
      .archived_base_custody_exact = manifest.baseline_exact,
      .candidate_delta_custody_exact = manifest.candidate_delta_exact,
      .production_shadow_source_independent = evidence.source_independent,
      .q0_identity_exact = evidence.projection_exact,
      .qpsm_identity_exact = evidence.projection_exact,
      .projection_invariants_exact = evidence.projection_exact,
      .layout_and_metadata_exact =
          evidence.layout_exact &&
          evidence.captures.canonical_layout_metadata_exact,
      .canonical_plan_exact =
          recompute_canonical_plan(cell_vector, layout_hash,
                                   manifest.token_layout_hash) &&
          evidence.captures.canonical_layout_metadata_exact,
      .parent_shadow_identities_exact =
          evidence.captures.parent_shadow_hashes_exact,
      .canonical_reference_identity_exact =
          evidence.captures.parent_reference_hashes_exact,
      .all_reference_keys_exact =
          evidence.normalizer.exact &&
          evidence.captures.parent_source_hashes_exact &&
          evidence.captures.parent_reference_hashes_exact &&
          evidence.captures.parent_shadow_hashes_exact};
  compare_emitted_bool(records,
                       "srr2.sealed_reference.archived_base_custody_exact",
                       input.sealed_reference.archived_base_custody_exact);
  compare_emitted_bool(records,
                       "srr2.sealed_reference.candidate_delta_custody_exact",
                       input.sealed_reference.candidate_delta_custody_exact);
  compare_emitted_bool(
      records, "srr2.sealed_reference.production_shadow_source_independent",
      input.sealed_reference.production_shadow_source_independent);
  compare_emitted_bool(records, "srr2.sealed_reference.q0_identity_exact",
                       input.sealed_reference.q0_identity_exact);
  compare_emitted_bool(records, "srr2.sealed_reference.qpsm_identity_exact",
                       input.sealed_reference.qpsm_identity_exact);
  compare_emitted_bool(records,
                       "srr2.sealed_reference.projection_invariants_exact",
                       input.sealed_reference.projection_invariants_exact);
  compare_emitted_bool(records,
                       "srr2.sealed_reference.layout_and_metadata_exact",
                       input.sealed_reference.layout_and_metadata_exact);
  compare_emitted_bool(records, "srr2.sealed_reference.canonical_plan_exact",
                       input.sealed_reference.canonical_plan_exact);
  compare_emitted_bool(records,
                       "srr2.sealed_reference.parent_shadow_identities_exact",
                       input.sealed_reference.parent_shadow_identities_exact);
  compare_emitted_bool(
      records, "srr2.sealed_reference.canonical_reference_identity_exact",
      input.sealed_reference.canonical_reference_identity_exact);
  compare_emitted_bool(records,
                       "srr2.sealed_reference.all_reference_keys_exact",
                       input.sealed_reference.all_reference_keys_exact);

  const auto &parity = input.parity;
  compare_emitted_bool(records, "srr2.summary.parity.shape_exact",
                       parity.shape_exact);
  compare_emitted_bool(records,
                       "srr2.summary.parity.strides_and_contiguity_exact",
                       parity.strides_and_contiguity_exact);
  compare_emitted_bool(records, "srr2.summary.parity.dtype_exact",
                       parity.dtype_exact);
  compare_emitted_bool(records, "srr2.summary.parity.device_exact",
                       parity.device_exact);
  compare_emitted_bool(records, "srr2.summary.parity.valid_mask_bytes_exact",
                       parity.valid_mask_bytes_exact);
  compare_emitted_bool(records, "srr2.summary.parity.value_bytes_exact",
                       parity.value_bytes_exact);
  compare_emitted_bool(records,
                       "srr2.summary.parity.cpu64_valid_mask_bytes_exact",
                       parity.cpu64_valid_mask_bytes_exact);
  compare_emitted_bool(records, "srr2.summary.parity.cpu64_value_bytes_exact",
                       parity.cpu64_value_bytes_exact);
  compare_emitted_bool(records, "srr2.summary.parity.stable_hashes_exact",
                       parity.stable_hashes_exact);
  compare_emitted_bool(records,
                       "srr2.summary.parity.repeat_capture_identity_exact",
                       parity.repeat_capture_identity_exact);
  compare_emitted_bool(records,
                       "srr2.summary.parity.per_capture_coverage_exact",
                       parity.per_capture_coverage_exact);
  compare_emitted_bool(
      records, "srr2.summary.parity.coverage_counts_recomputed_from_records",
      parity.coverage_counts_recomputed_from_records);
  compare_emitted_double(records, "srr2.summary.parity.cpu64_max_abs",
                         parity.cpu64_max_abs);
  compare_emitted_double(records, "srr2.summary.parity.device_max_abs",
                         parity.device_max_abs);
  compare_emitted_unsigned(records, "srr2.summary.parity.seed_count",
                           parity.seed_count);
  compare_emitted_unsigned(records, "srr2.summary.parity.dataset_count",
                           parity.dataset_count);
  compare_emitted_unsigned(records,
                           "srr2.summary.parity.retained_capture_count",
                           parity.retained_capture_count);
  compare_emitted_unsigned(records, "srr2.summary.parity.repeat_capture_count",
                           parity.repeat_capture_count);
  compare_emitted_unsigned(records, "srr2.summary.parity.retained_row_count",
                           parity.retained_row_count);
  compare_emitted_unsigned(records, "srr2.summary.parity.repeat_row_count",
                           parity.repeat_row_count);
  compare_emitted_unsigned(records, "srr2.summary.parity.retained_value_count",
                           parity.retained_value_count);
  compare_emitted_unsigned(records, "srr2.summary.parity.repeat_value_count",
                           parity.repeat_value_count);
  compare_emitted_unsigned(records,
                           "srr2.summary.parity.retained_validity_count",
                           parity.retained_validity_count);
  compare_emitted_unsigned(records, "srr2.summary.parity.repeat_validity_count",
                           parity.repeat_validity_count);

  compare_emitted_unsigned(records, "srr2.coverage.seed_count",
                           parity.seed_count);
  compare_emitted_unsigned(records, "srr2.coverage.dataset_count",
                           parity.dataset_count);
  compare_emitted_unsigned(records, "srr2.coverage.batch_size", 96);
  compare_emitted_unsigned(records, "srr2.coverage.retained_capture_count",
                           parity.retained_capture_count);
  compare_emitted_unsigned(records, "srr2.coverage.repeat_capture_count",
                           parity.repeat_capture_count);
  compare_emitted_unsigned(records, "srr2.coverage.retained_row_count",
                           parity.retained_row_count);
  compare_emitted_unsigned(records, "srr2.coverage.repeat_row_count",
                           parity.repeat_row_count);
  compare_emitted_unsigned(records, "srr2.coverage.retained_value_count",
                           parity.retained_value_count);
  compare_emitted_unsigned(records, "srr2.coverage.repeat_value_count",
                           parity.repeat_value_count);
  compare_emitted_unsigned(records, "srr2.coverage.retained_validity_count",
                           parity.retained_validity_count);
  compare_emitted_unsigned(records, "srr2.coverage.repeat_validity_count",
                           parity.repeat_validity_count);
  compare_emitted_bool(records, "srr2.coverage.counts_recomputed_from_records",
                       parity.coverage_counts_recomputed_from_records);
  const bool coverage_exact =
      parity.per_capture_coverage_exact &&
      parity.coverage_counts_recomputed_from_records &&
      parity.seed_count == gate::kExpectedSeedCount &&
      parity.dataset_count == gate::kExpectedDatasetCount &&
      parity.retained_capture_count == gate::kExpectedRetainedCaptureCount &&
      parity.repeat_capture_count == gate::kExpectedRepeatCaptureCount &&
      parity.retained_row_count == gate::kExpectedRetainedRowCount &&
      parity.repeat_row_count == gate::kExpectedRepeatRowCount &&
      parity.retained_value_count == gate::kExpectedRetainedValueCount &&
      parity.repeat_value_count == gate::kExpectedRepeatValueCount &&
      parity.retained_validity_count == gate::kExpectedRetainedValidityCount &&
      parity.repeat_validity_count == gate::kExpectedRepeatValidityCount;
  compare_emitted_bool(records, "srr2.coverage.exact", coverage_exact);

  const auto &translation = input.device_translation;
  compare_emitted_bool(
      records, "srr2.summary.device_translation.cpu64_reference_shape_exact",
      translation.cpu64_reference_shape_exact);
  compare_emitted_bool(
      records,
      "srr2.summary.device_translation.cpu64_reference_mask_bytes_exact",
      translation.cpu64_reference_mask_bytes_exact);
  compare_emitted_bool(records,
                       "srr2.summary.device_translation."
                       "cpu64_production_reference_bytes_exact",
                       translation.cpu64_production_reference_bytes_exact);
  compare_emitted_bool(records,
                       "srr2.summary.device_translation."
                       "cpu64_shadow_reference_bytes_exact",
                       translation.cpu64_shadow_reference_bytes_exact);
  compare_emitted_bool(
      records,
      "srr2.summary.device_translation.device_reference_contract_exact",
      translation.device_reference_contract_exact);
  compare_emitted_double(records,
                         "srr2.summary.device_translation."
                         "cpu64_production_reference_max_abs",
                         translation.cpu64_production_reference_max_abs);
  compare_emitted_double(records,
                         "srr2.summary.device_translation."
                         "cpu64_shadow_reference_max_abs",
                         translation.cpu64_shadow_reference_max_abs);
  compare_emitted_double(records,
                         "srr2.summary.device_translation."
                         "device_production_reference_max_abs",
                         translation.device_production_reference_max_abs);
  compare_emitted_double(records,
                         "srr2.summary.device_translation."
                         "device_shadow_reference_max_abs",
                         translation.device_shadow_reference_max_abs);

  auto &quality = input.quality_transport;
  const bool frozen_parent =
      parent_evidence.artifacts_exact && parent_evidence.hashes_exact &&
      parent_evidence.audit_pass && parent_evidence.classification_exact;
  const bool parent_data_identity = boolean(parent, "srr.data.identity_exact");
  const bool parent_normalization_identity =
      boolean(parent, "srr.data.normalization_preserved_identity");
  const bool parent_ridge_exact = boolean(parent, "srr.ridge.pass");
  const bool parent_order_shuffle_balanced =
      boolean(parent, "srr.order_shuffle_balanced");
  quality.features_and_masks_cover_parent_domain =
      evidence.normalizer.exact && evidence.captures.input_hashes_exact &&
      coverage_exact && parity.value_bytes_exact &&
      parity.valid_mask_bytes_exact && parity.stable_hashes_exact;
  quality.targets_exact = frozen_parent && parent_data_identity;
  quality.group_splits_exact = frozen_parent && parent_data_identity &&
                               evidence.captures.input_hashes_exact;
  quality.sample_ladder_exact =
      frozen_parent && parent_ridge_exact && manifest.protocol_exact;
  quality.alpha_grid_exact =
      frozen_parent && parent_ridge_exact && manifest.protocol_exact;
  quality.standardization_exact = frozen_parent &&
                                  parent_normalization_identity &&
                                  evidence.normalizer.exact;
  quality.target_centering_exact = frozen_parent && parent_ridge_exact;
  quality.fit_and_validation_selection_exact =
      frozen_parent && parent_ridge_exact;
  quality.test_rows_exact = frozen_parent && parent_data_identity &&
                            evidence.captures.input_hashes_exact &&
                            coverage_exact;
  const bool parent_permutations_valid =
      boolean(parent, "srr.permutations_valid");
  const bool parent_bootstrap_valid = boolean(parent, "srr.bootstrap.valid");
  quality.permutations_exact = frozen_parent && parent_permutations_valid;
  quality.bootstrap_rows_exact = frozen_parent && parent_bootstrap_valid;
  quality.decision_thresholds_exact = frozen_parent && manifest.protocol_exact;
  quality.parent_material_gain_over_channel = parent_evidence.material_gain;
  quality.parent_noninferior_to_encoder = parent_evidence.noninferior;
  quality.parent_order_decodable = parent_evidence.order_decodable;
  quality.parent_continuous_shuffle_pass =
      parent_evidence.continuous_shuffle_pass;
  quality.parent_order_shuffle_pass = parent_evidence.order_shuffle_pass;
  quality.parent_order_shuffle_pass &= parent_order_shuffle_balanced;
  quality.parent_terminal_reproduced = parent_evidence.terminal_reproduced &&
                                       parent_evidence.classification_exact;

  const auto compare_quality = [&](const char *name, bool value) {
    compare_emitted_bool(records, std::string("srr2.quality_transport.") + name,
                         value);
  };
  compare_quality("features_and_masks_cover_parent_domain",
                  quality.features_and_masks_cover_parent_domain);
  compare_quality("targets_exact", quality.targets_exact);
  compare_quality("group_splits_exact", quality.group_splits_exact);
  compare_quality("sample_ladder_exact", quality.sample_ladder_exact);
  compare_quality("alpha_grid_exact", quality.alpha_grid_exact);
  compare_quality("standardization_exact", quality.standardization_exact);
  compare_quality("target_centering_exact", quality.target_centering_exact);
  compare_quality("fit_and_validation_selection_exact",
                  quality.fit_and_validation_selection_exact);
  compare_quality("test_rows_exact", quality.test_rows_exact);
  compare_quality("permutations_exact", quality.permutations_exact);
  compare_quality("bootstrap_rows_exact", quality.bootstrap_rows_exact);
  compare_quality("decision_thresholds_exact",
                  quality.decision_thresholds_exact);
  compare_quality("parent_material_gain_over_channel",
                  quality.parent_material_gain_over_channel);
  compare_quality("parent_noninferior_to_encoder",
                  quality.parent_noninferior_to_encoder);
  compare_quality("parent_order_decodable", quality.parent_order_decodable);
  compare_quality("parent_continuous_shuffle_pass",
                  quality.parent_continuous_shuffle_pass);
  compare_quality("parent_order_shuffle_pass",
                  quality.parent_order_shuffle_pass);
  compare_quality("parent_terminal_reproduced",
                  quality.parent_terminal_reproduced);
  return evidence;
}

struct IndependentResult {
  std::string classification{"invalid_mechanics"};
  std::string reason{"invalid_numeric"};
};

[[nodiscard]] bool authorizations_clear(const gate::AuthorizationInput &input) {
  return !input.training_authorized && !input.augmentation_change_authorized &&
         !input.long_run_authorized && !input.active_policy_change_authorized &&
         !input.checkpoint_migration_authorized &&
         !input.downstream_retraining_authorized &&
         !input.end_to_end_authorized && !input.deployment_authorized;
}

[[nodiscard]] bool counters_zero(const gate::ForbiddenCounterInput &input) {
  return input.training_step_count == 0 &&
         input.optimizer_construction_count == 0 &&
         input.optimizer_step_count == 0 && input.backward_call_count == 0 &&
         input.weight_update_count == 0 &&
         input.augmentation_change_count == 0 &&
         input.target_generation_count == 0 &&
         input.probe_construction_count == 0 && input.probe_fit_count == 0 &&
         input.validation_selection_count == 0 && input.prediction_count == 0 &&
         input.permutation_count == 0 && input.bootstrap_count == 0 &&
         input.downstream_retraining_count == 0 &&
         input.end_to_end_count == 0 && input.deployment_count == 0;
}

[[nodiscard]] bool
coverage_exact(const gate::ProductionShadowParityInput &input) {
  return input.per_capture_coverage_exact &&
         input.coverage_counts_recomputed_from_records &&
         input.seed_count == gate::kExpectedSeedCount &&
         input.dataset_count == gate::kExpectedDatasetCount &&
         input.retained_capture_count == gate::kExpectedRetainedCaptureCount &&
         input.repeat_capture_count == gate::kExpectedRepeatCaptureCount &&
         input.retained_row_count == gate::kExpectedRetainedRowCount &&
         input.repeat_row_count == gate::kExpectedRepeatRowCount &&
         input.retained_value_count == gate::kExpectedRetainedValueCount &&
         input.repeat_value_count == gate::kExpectedRepeatValueCount &&
         input.retained_validity_count ==
             gate::kExpectedRetainedValidityCount &&
         input.repeat_validity_count == gate::kExpectedRepeatValidityCount;
}

[[nodiscard]] IndependentResult independent_gate(const gate::GateInput &input) {
  const auto failure = [](std::string classification, std::string reason) {
    return IndependentResult{.classification = std::move(classification),
                             .reason = std::move(reason)};
  };
  const std::array<double, 6> numeric{
      input.parity.cpu64_max_abs,
      input.parity.device_max_abs,
      input.device_translation.cpu64_production_reference_max_abs,
      input.device_translation.cpu64_shadow_reference_max_abs,
      input.device_translation.device_production_reference_max_abs,
      input.device_translation.device_shadow_reference_max_abs};
  if (std::any_of(numeric.begin(), numeric.end(), [](double value) {
        return !std::isfinite(value) || value < 0.0;
      })) {
    return failure("invalid_mechanics", "invalid_numeric");
  }
  const auto &mechanics = input.mechanics;
  if (!mechanics.local_contracts_exact)
    return failure("invalid_mechanics", "local_contract");
  if (!mechanics.source_boundary_exact)
    return failure("invalid_mechanics", "source_boundary");
  if (!mechanics.command_exact)
    return failure("invalid_mechanics", "command");
  if (!mechanics.environment_exact)
    return failure("invalid_mechanics", "environment");
  if (!mechanics.cuda_available)
    return failure("invalid_mechanics", "cuda_unavailable");
  if (!mechanics.attempt_marker_exact ||
      mechanics.authoritative_attempt_count != 1)
    return failure("invalid_mechanics", "attempt_contract");
  if (!mechanics.capture_contracts_exact)
    return failure("invalid_mechanics", "capture_contract");
  if (!mechanics.purity_exact)
    return failure("invalid_mechanics", "purity_contract");
  if (!mechanics.finite_outputs_exact)
    return failure("invalid_mechanics", "finite_output_contract");
  if (!mechanics.deterministic_execution_exact)
    return failure("invalid_mechanics", "deterministic_contract");
  if (!mechanics.manifest_exact)
    return failure("invalid_mechanics", "manifest");
  if (!mechanics.audit_input_exact)
    return failure("invalid_mechanics", "audit_input");
  if (!authorizations_clear(mechanics.authorizations))
    return failure("invalid_mechanics", "authorization");
  if (!counters_zero(mechanics.counters))
    return failure("invalid_mechanics", "nonzero_counter");

  const auto &parent = input.parent;
  if (!parent.artifacts_exact)
    return failure("parent_evidence_failure", "parent_artifact");
  if (!parent.hashes_exact)
    return failure("parent_evidence_failure", "parent_hash");
  if (!parent.terminal_classification_exact)
    return failure("parent_evidence_failure", "parent_classification");
  if (parent.authoritative_attempt_count != 1)
    return failure("parent_evidence_failure", "parent_attempt_count");
  if (!parent.audit_pass || parent.audit_error_count != 0)
    return failure("parent_evidence_failure", "parent_audit");
  if (parent.optimizer_step_count != 0 || parent.backward_call_count != 0)
    return failure("parent_evidence_failure", "parent_counter");
  if (!parent.authorizations_false)
    return failure("parent_evidence_failure", "parent_authorization");

  const auto &compatibility = input.compatibility;
  if (!compatibility.legacy_enum_ordinals_exact ||
      !compatibility.structured_policy_appended)
    return failure("backward_compatibility_failure", "enum_contract");
  if (!compatibility.legacy_policy_names_exact ||
      !compatibility.structured_policy_name_exact)
    return failure("backward_compatibility_failure", "policy_name");
  if (!compatibility.parser_round_trip_exact ||
      !compatibility.unknown_policy_rejected)
    return failure("backward_compatibility_failure", "parser_contract");
  if (!compatibility.cpp_default_all_tokens)
    return failure("backward_compatibility_failure", "default_contract");
  if (!compatibility.omitted_dsl_all_tokens ||
      !compatibility.active_dsl_all_tokens)
    return failure("backward_compatibility_failure", "dsl_contract");
  if (!compatibility.protocol_fingerprint_distinct)
    return failure("backward_compatibility_failure", "fingerprint_contract");
  if (!compatibility.structured_checkpoint_round_trip_exact ||
      !compatibility.legacy_checkpoint_all_tokens ||
      !compatibility.legacy_checkpoint_does_not_inherit_structured ||
      !compatibility.checkpoint_mismatch_rejected ||
      !compatibility.malformed_checkpoint_rejected)
    return failure("backward_compatibility_failure", "checkpoint_contract");
  if (!compatibility.legacy_policy_bytes_exact)
    return failure("backward_compatibility_failure",
                   "legacy_policy_regression");
  if (!compatibility.public_selector_contract_exact ||
      !compatibility.adapter_reaches_structured_selector)
    return failure("backward_compatibility_failure",
                   "selector_adapter_contract");

  const auto &sealed = input.sealed_reference;
  if (!sealed.archived_base_custody_exact)
    return failure("sealed_reference_failure", "archived_base_custody");
  if (!sealed.candidate_delta_custody_exact)
    return failure("sealed_reference_failure", "candidate_delta_custody");
  if (!sealed.production_shadow_source_independent)
    return failure("sealed_reference_failure",
                   "production_shadow_source_boundary");
  if (!sealed.q0_identity_exact || !sealed.qpsm_identity_exact ||
      !sealed.projection_invariants_exact)
    return failure("sealed_reference_failure", "projection_contract");
  if (!sealed.layout_and_metadata_exact || !sealed.canonical_plan_exact)
    return failure("sealed_reference_failure", "layout_contract");
  if (!sealed.parent_shadow_identities_exact)
    return failure("sealed_reference_failure", "shadow_identity");
  if (!sealed.canonical_reference_identity_exact)
    return failure("sealed_reference_failure", "canonical_reference_identity");
  if (!sealed.all_reference_keys_exact)
    return failure("sealed_reference_failure", "reference_keys");

  const auto &parity = input.parity;
  if (!parity.shape_exact || !parity.strides_and_contiguity_exact ||
      !parity.dtype_exact || !parity.device_exact)
    return failure("production_shadow_parity_failure",
                   "production_shadow_tensor_contract");
  if (!parity.valid_mask_bytes_exact || !parity.cpu64_valid_mask_bytes_exact)
    return failure("production_shadow_parity_failure",
                   "production_shadow_mask_identity");
  if (!parity.value_bytes_exact || !parity.cpu64_value_bytes_exact)
    return failure("production_shadow_parity_failure",
                   "production_shadow_value_identity");
  if (!parity.stable_hashes_exact)
    return failure("production_shadow_parity_failure",
                   "production_shadow_hash_identity");
  if (!parity.repeat_capture_identity_exact)
    return failure("production_shadow_parity_failure",
                   "production_shadow_repeat_identity");
  if (parity.cpu64_max_abs != 0.0 || parity.device_max_abs != 0.0)
    return failure("production_shadow_parity_failure",
                   "production_shadow_exact_zero");
  if (!coverage_exact(parity))
    return failure("production_shadow_parity_failure",
                   "production_shadow_coverage");

  const auto &translation = input.device_translation;
  if (!translation.cpu64_reference_shape_exact ||
      !translation.cpu64_reference_mask_bytes_exact ||
      !translation.cpu64_production_reference_bytes_exact ||
      !translation.cpu64_shadow_reference_bytes_exact)
    return failure("device_translation_failure", "cpu64_reference_contract");
  if (translation.cpu64_production_reference_max_abs != 0.0 ||
      translation.cpu64_shadow_reference_max_abs != 0.0)
    return failure("device_translation_failure", "cpu64_reference_exact_zero");
  if (!translation.device_reference_contract_exact)
    return failure("device_translation_failure", "device_contract");
  if (translation.device_production_reference_max_abs >
          gate::kDeviceTranslationTolerance ||
      translation.device_shadow_reference_max_abs >
          gate::kDeviceTranslationTolerance)
    return failure("device_translation_failure", "device_tolerance");

  const auto &quality = input.quality_transport;
  if (!quality.features_and_masks_cover_parent_domain ||
      !quality.targets_exact || !quality.group_splits_exact ||
      !quality.sample_ladder_exact || !quality.alpha_grid_exact ||
      !quality.standardization_exact || !quality.target_centering_exact ||
      !quality.fit_and_validation_selection_exact || !quality.test_rows_exact ||
      !quality.permutations_exact || !quality.bootstrap_rows_exact ||
      !quality.decision_thresholds_exact)
    return failure("production_readout_gate_failure",
                   "quality_transport_identity");
  if (!quality.parent_material_gain_over_channel)
    return failure("production_readout_gate_failure", "parent_material_gain");
  if (!quality.parent_noninferior_to_encoder)
    return failure("production_readout_gate_failure", "parent_noninferiority");
  if (!quality.parent_order_decodable)
    return failure("production_readout_gate_failure", "parent_order");
  if (!quality.parent_continuous_shuffle_pass)
    return failure("production_readout_gate_failure",
                   "parent_continuous_control");
  if (!quality.parent_order_shuffle_pass)
    return failure("production_readout_gate_failure", "parent_order_control");
  if (!quality.parent_terminal_reproduced)
    return failure("production_readout_gate_failure", "parent_terminal");
  return {.classification = "production_structured_readout_parity_reproduced",
          .reason = "none"};
}

void compare_gate_result(Records &records, const gate::GateResult &result) {
  compare_emitted_bool(records, "srr2.gate.numeric_inputs_valid",
                       result.numeric_inputs_valid);
  compare_emitted_bool(records, "srr2.gate.authorization_boundary_valid",
                       result.authorization_boundary_valid);
  compare_emitted_bool(records, "srr2.gate.zero_counters_valid",
                       result.zero_counters_valid);
  compare_emitted_bool(records, "srr2.gate.mechanics_valid",
                       result.mechanics_valid);
  compare_emitted_bool(records, "srr2.gate.parent_evidence_valid",
                       result.parent_evidence_valid);
  compare_emitted_bool(records, "srr2.gate.backward_compatibility_valid",
                       result.backward_compatibility_valid);
  compare_emitted_bool(records, "srr2.gate.sealed_reference_valid",
                       result.sealed_reference_valid);
  compare_emitted_bool(records, "srr2.gate.coverage_valid",
                       result.coverage_valid);
  compare_emitted_bool(records, "srr2.gate.production_shadow_parity_valid",
                       result.production_shadow_parity_valid);
  compare_emitted_bool(records, "srr2.gate.cpu64_reference_valid",
                       result.cpu64_reference_valid);
  compare_emitted_bool(records, "srr2.gate.device_translation_valid",
                       result.device_translation_valid);
  compare_emitted_bool(records, "srr2.gate.production_readout_gate_valid",
                       result.production_readout_gate_valid);
  const std::string classification =
      gate::terminal_classification_name(result.classification);
  const std::string reason = gate::failure_reason_name(result.failure_reason);
  expect_text(records, "srr2.gate.classification", classification);
  expect_text(records, "srr2.gate.failure_reason", reason);
  expect_text(records, "terminal_result", classification);
  expect_text(records, "failure_reason", reason);
}

void expect(bool condition, const std::string &message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

[[nodiscard]] gate::GateInput valid_gate_fixture() {
  gate::GateInput input{};
  input.mechanics.local_contracts_exact = true;
  input.mechanics.source_boundary_exact = true;
  input.mechanics.command_exact = true;
  input.mechanics.environment_exact = true;
  input.mechanics.cuda_available = true;
  input.mechanics.attempt_marker_exact = true;
  input.mechanics.capture_contracts_exact = true;
  input.mechanics.purity_exact = true;
  input.mechanics.finite_outputs_exact = true;
  input.mechanics.deterministic_execution_exact = true;
  input.mechanics.manifest_exact = true;
  input.mechanics.audit_input_exact = true;
  input.mechanics.authoritative_attempt_count = 1;
  input.parent = {.artifacts_exact = true,
                  .hashes_exact = true,
                  .terminal_classification_exact = true,
                  .audit_pass = true,
                  .authorizations_false = true,
                  .authoritative_attempt_count = 1,
                  .audit_error_count = 0,
                  .optimizer_step_count = 0,
                  .backward_call_count = 0};
  input.compatibility = {.legacy_enum_ordinals_exact = true,
                         .legacy_policy_names_exact = true,
                         .structured_policy_appended = true,
                         .structured_policy_name_exact = true,
                         .parser_round_trip_exact = true,
                         .unknown_policy_rejected = true,
                         .cpp_default_all_tokens = true,
                         .omitted_dsl_all_tokens = true,
                         .active_dsl_all_tokens = true,
                         .protocol_fingerprint_distinct = true,
                         .structured_checkpoint_round_trip_exact = true,
                         .legacy_checkpoint_all_tokens = true,
                         .legacy_checkpoint_does_not_inherit_structured = true,
                         .checkpoint_mismatch_rejected = true,
                         .malformed_checkpoint_rejected = true,
                         .legacy_policy_bytes_exact = true,
                         .public_selector_contract_exact = true,
                         .adapter_reaches_structured_selector = true};
  input.sealed_reference = {.archived_base_custody_exact = true,
                            .candidate_delta_custody_exact = true,
                            .production_shadow_source_independent = true,
                            .q0_identity_exact = true,
                            .qpsm_identity_exact = true,
                            .projection_invariants_exact = true,
                            .layout_and_metadata_exact = true,
                            .canonical_plan_exact = true,
                            .parent_shadow_identities_exact = true,
                            .canonical_reference_identity_exact = true,
                            .all_reference_keys_exact = true};
  input.parity = {.shape_exact = true,
                  .strides_and_contiguity_exact = true,
                  .dtype_exact = true,
                  .device_exact = true,
                  .valid_mask_bytes_exact = true,
                  .value_bytes_exact = true,
                  .cpu64_valid_mask_bytes_exact = true,
                  .cpu64_value_bytes_exact = true,
                  .stable_hashes_exact = true,
                  .repeat_capture_identity_exact = true,
                  .per_capture_coverage_exact = true,
                  .coverage_counts_recomputed_from_records = true,
                  .cpu64_max_abs = 0.0,
                  .device_max_abs = 0.0,
                  .seed_count = gate::kExpectedSeedCount,
                  .dataset_count = gate::kExpectedDatasetCount,
                  .retained_capture_count = gate::kExpectedRetainedCaptureCount,
                  .repeat_capture_count = gate::kExpectedRepeatCaptureCount,
                  .retained_row_count = gate::kExpectedRetainedRowCount,
                  .repeat_row_count = gate::kExpectedRepeatRowCount,
                  .retained_value_count = gate::kExpectedRetainedValueCount,
                  .repeat_value_count = gate::kExpectedRepeatValueCount,
                  .retained_validity_count =
                      gate::kExpectedRetainedValidityCount,
                  .repeat_validity_count = gate::kExpectedRepeatValidityCount};
  input.device_translation = {.cpu64_reference_shape_exact = true,
                              .cpu64_reference_mask_bytes_exact = true,
                              .cpu64_production_reference_bytes_exact = true,
                              .cpu64_shadow_reference_bytes_exact = true,
                              .device_reference_contract_exact = true,
                              .cpu64_production_reference_max_abs = 0.0,
                              .cpu64_shadow_reference_max_abs = 0.0,
                              .device_production_reference_max_abs = 1.0e-6,
                              .device_shadow_reference_max_abs = 1.0e-6};
  input.quality_transport = {.features_and_masks_cover_parent_domain = true,
                             .targets_exact = true,
                             .group_splits_exact = true,
                             .sample_ladder_exact = true,
                             .alpha_grid_exact = true,
                             .standardization_exact = true,
                             .target_centering_exact = true,
                             .fit_and_validation_selection_exact = true,
                             .test_rows_exact = true,
                             .permutations_exact = true,
                             .bootstrap_rows_exact = true,
                             .decision_thresholds_exact = true,
                             .parent_material_gain_over_channel = true,
                             .parent_noninferior_to_encoder = true,
                             .parent_order_decodable = true,
                             .parent_continuous_shuffle_pass = true,
                             .parent_order_shuffle_pass = true,
                             .parent_terminal_reproduced = true};
  return input;
}

void expect_gate(const gate::GateInput &input, std::string_view classification,
                 std::string_view reason, const std::string &fixture) {
  const auto official = gate::evaluate(input);
  const auto independent = independent_gate(input);
  expect(gate::terminal_classification_name(official.classification) ==
                 classification &&
             gate::failure_reason_name(official.failure_reason) == reason,
         "official gate fixture: " + fixture);
  expect(independent.classification == classification &&
             independent.reason == reason,
         "independent gate fixture: " + fixture);
}

void run_self_test() {
  const auto clean = parse_records("schema=x\na=1\nb=true\n", true);
  expect(clean.errors.empty() && clean.machine_lines == 3,
         "clean parser fixture");
  const auto duplicate = parse_records("a=1\na=2\n", true);
  expect(duplicate.duplicate_keys == 1 && !duplicate.errors.empty(),
         "duplicate parser fixture");
  const auto nonfinite = parse_records("a=nan\n", true);
  expect(nonfinite.nonfinite_values == 1 && !nonfinite.errors.empty(),
         "non-finite parser fixture");
  const auto malformed = parse_records("a=1\nnot-a-record\n", true);
  expect(malformed.malformed_lines == 1 && !malformed.errors.empty(),
         "malformed parser fixture");
  const auto blank = parse_records("a=1\n\nb=2\n", true);
  expect(blank.malformed_lines == 1 && !blank.errors.empty(),
         "blank-line parser fixture");
  const auto carriage_return = parse_records("a=1\r\n", true);
  expect(carriage_return.malformed_lines == 1 &&
             !carriage_return.errors.empty(),
         "CR parser fixture");
  Records typed = parse_records("b=maybe\nn=-1\nh=ABC\n", true);
  (void)boolean(typed, "b");
  (void)unsigned_number(typed, "n");
  (void)hash16(typed, "h");
  (void)required(typed, "missing");
  expect(typed.critical_value_errors == 4, "critical typed-value fixture");
  Records noncanonical_unsigned = parse_records("n=01\n", true);
  (void)unsigned_number(noncanonical_unsigned, "n");
  expect(noncanonical_unsigned.critical_value_errors == 1,
         "canonical unsigned fixture");
  const std::string initializer =
      "[source_runtime_t] initializing static-global source snapshot\n";
  const std::string valid_tail = initializer + std::string(kAuthorizationTail);
  const auto valid_tail_records = parse_records(valid_tail, true);
  expect(authorization_machine_tail_exact(valid_tail, valid_tail_records),
         "literal authorization EOF fixture");
  const std::string trailing_finalizer =
      valid_tail +
      "[source_runtime_t] finalizing static-global source snapshot\n";
  const auto trailing_finalizer_records =
      parse_records(trailing_finalizer, true);
  expect(!authorization_machine_tail_exact(trailing_finalizer,
                                           trailing_finalizer_records),
         "trailing finalizer rejection fixture");
  expect(sha256("abc") ==
             "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad",
         "SHA-256 fixture");

  const AttemptLedgerBindings ledger_bindings{
      .command_sha256 = std::string(64, 'a'),
      .manifest_bytes = 12345,
      .manifest_sha256 = std::string(64, 'b'),
      .binary_bytes = 67890,
      .binary_sha256 = std::string(64, 'c'),
      .container_id = std::string(64, 'd')};
  const std::string ledger = canonical_attempt_ledger_bytes(ledger_bindings);
  expect(verify_attempt_ledger_content(ledger, ledger_bindings).content_exact,
         "canonical attempt ledger fixture");
  const auto replace_once = [](std::string value, std::string_view from,
                               std::string_view to) {
    const auto at = value.find(from);
    if (at == std::string::npos) {
      throw std::runtime_error("self-test replacement source missing");
    }
    value.replace(at, from.size(), to);
    return value;
  };
  expect(!verify_attempt_ledger_content(
              replace_once(ledger, "state=consumed\n", "state=open\n"),
              ledger_bindings)
              .content_exact,
         "attempt ledger tamper fixture");
  expect(!verify_attempt_ledger_content(
              replace_once(ledger, "attempt_count=1\n",
                           "attempt_count=1\nattempt_count=1\n"),
              ledger_bindings)
              .content_exact,
         "attempt ledger duplicate fixture");
  expect(!verify_attempt_ledger_content(
              replace_once(ledger, "state=consumed\n", ""), ledger_bindings)
              .content_exact,
         "attempt ledger missing fixture");
  expect(!verify_attempt_ledger_content(
              replace_once(ledger, "state=consumed\n", "state consumed\n"),
              ledger_bindings)
              .content_exact,
         "attempt ledger malformed fixture");
  auto wrong_ledger_bindings = ledger_bindings;
  wrong_ledger_bindings.binary_sha256 = std::string(64, 'e');
  expect(!verify_attempt_ledger_content(ledger, wrong_ledger_bindings)
              .content_exact,
         "attempt ledger wrong-binding fixture");
  expect(!verify_attempt_ledger_content(
              replace_once(ledger, "schema=", "schema=\r"), ledger_bindings)
              .content_exact,
         "attempt ledger LF-only fixture");

  constexpr std::string_view layout_hash = kFrozenLayoutHash;
  expect(recompute_canonical_plan(kCellVector, layout_hash, layout_hash),
         "canonical-plan primitive fixture");
  expect(!recompute_canonical_plan("0,1", layout_hash, layout_hash),
         "canonical-plan cell tamper fixture");
  expect(
      !recompute_canonical_plan(kCellVector, layout_hash, "fedcba9876543210"),
      "canonical-plan layout tamper fixture");
  Records false_canonical =
      parse_records("srr2.sealed_reference.canonical_plan_exact=false\n", true);
  compare_emitted_bool(false_canonical,
                       "srr2.sealed_reference.canonical_plan_exact", true);
  expect(!false_canonical.errors.empty(),
         "canonical-plan false summary rejection fixture");
  Records forged_canonical =
      parse_records("srr2.sealed_reference.canonical_plan_exact=true\n", true);
  compare_emitted_bool(forged_canonical,
                       "srr2.sealed_reference.canonical_plan_exact", false);
  expect(!forged_canonical.errors.empty(),
         "canonical-plan forged-true rejection fixture");
  Records honest_canonical =
      parse_records("srr2.sealed_reference.canonical_plan_exact=false\n", true);
  compare_emitted_bool(honest_canonical,
                       "srr2.sealed_reference.canonical_plan_exact", false);
  expect(honest_canonical.errors.empty(),
         "canonical-plan recomputed-false fixture");

  expect_gate(valid_gate_fixture(),
              "production_structured_readout_parity_reproduced", "none",
              "success");
  {
    auto input = valid_gate_fixture();
    input.parity.cpu64_max_abs = std::numeric_limits<double>::quiet_NaN();
    input.mechanics.local_contracts_exact = false;
    expect_gate(input, "invalid_mechanics", "invalid_numeric",
                "numeric precedence");
  }
  {
    auto input = valid_gate_fixture();
    input.mechanics.local_contracts_exact = false;
    input.parent.artifacts_exact = false;
    expect_gate(input, "invalid_mechanics", "local_contract",
                "mechanics precedence");
  }
  {
    auto input = valid_gate_fixture();
    input.parent.artifacts_exact = false;
    input.compatibility.legacy_enum_ordinals_exact = false;
    expect_gate(input, "parent_evidence_failure", "parent_artifact",
                "parent precedence");
  }
  {
    auto input = valid_gate_fixture();
    input.compatibility.legacy_enum_ordinals_exact = false;
    input.sealed_reference.q0_identity_exact = false;
    expect_gate(input, "backward_compatibility_failure", "enum_contract",
                "compatibility precedence");
  }
  {
    auto input = valid_gate_fixture();
    input.sealed_reference.q0_identity_exact = false;
    input.parity.shape_exact = false;
    expect_gate(input, "sealed_reference_failure", "projection_contract",
                "sealed reference precedence");
  }
  {
    auto input = valid_gate_fixture();
    input.parity.shape_exact = false;
    input.device_translation.cpu64_reference_shape_exact = false;
    expect_gate(input, "production_shadow_parity_failure",
                "production_shadow_tensor_contract", "parity precedence");
  }
  {
    auto input = valid_gate_fixture();
    input.device_translation.cpu64_reference_shape_exact = false;
    input.quality_transport.targets_exact = false;
    expect_gate(input, "device_translation_failure", "cpu64_reference_contract",
                "device precedence");
  }
  {
    auto input = valid_gate_fixture();
    input.quality_transport.targets_exact = false;
    expect_gate(input, "production_readout_gate_failure",
                "quality_transport_identity", "quality precedence");
  }
}

void close_authoritative_schema(Records &records) {
  for (const auto &[key, value] : records.values) {
    (void)value;
    if (records.accessed.count(key) == 0) {
      ++records.unaccessed_keys;
      if (records.unaccessed_keys <= 16) {
        fail(records, "unrecognized authoritative key: " + key);
      }
    }
  }
  if (records.unaccessed_keys > 16) {
    fail(records, "additional unrecognized authoritative keys: " +
                      std::to_string(records.unaccessed_keys - 16));
  }
}

[[nodiscard]] bool structural_input_exact(const Records &records) {
  return records.duplicate_keys == 0 && records.malformed_lines == 0 &&
         records.runtime_initializing_lines == 1 &&
         records.runtime_finalizing_lines == 0 &&
         records.nonfinite_values == 0 && records.critical_value_errors == 0;
}

[[nodiscard]] bool independent_agrees(const gate::GateResult &official,
                                      const IndependentResult &independent) {
  return independent.classification ==
             gate::terminal_classification_name(official.classification) &&
         independent.reason ==
             gate::failure_reason_name(official.failure_reason);
}

} // namespace

int main(int argc, char **argv) {
  try {
    std::setlocale(LC_NUMERIC, "C");
    if (argc == 2 && std::string_view(argv[1]) == "--self-test") {
      run_self_test();
      std::cout << "srr2.audit.self_test=PASS\n";
      return 0;
    }
    if (argc > 2) {
      throw std::runtime_error(
          "usage: test_production_structured_readout_parity_log_auditor "
          "[authoritative-log]|--self-test");
    }

    std::filesystem::path executable;
    if (std::filesystem::exists("/proc/self/exe")) {
      executable = std::filesystem::canonical("/proc/self/exe");
    } else {
      executable = std::filesystem::canonical(argv[0]);
    }
    const auto root = find_repository_root(executable);
    const auto canonical_log = root / std::string(kAuthoritativeLogPath);
    const std::filesystem::path log_path =
        argc == 2 ? std::filesystem::path(argv[1]) : canonical_log;
    const auto supplied_log =
        (log_path.is_absolute() ? log_path : std::filesystem::absolute(log_path))
            .lexically_normal();
    const auto expected_log = canonical_log.lexically_normal();
    const auto log_status = std::filesystem::symlink_status(expected_log);
    const bool authoritative_log_envelope_exact =
        supplied_log == expected_log &&
        std::filesystem::is_regular_file(log_status) &&
        !std::filesystem::is_symlink(log_status) &&
        std::filesystem::hard_link_count(expected_log) == 1 &&
        std::filesystem::canonical(expected_log) == expected_log &&
        path_is_within(root, std::filesystem::canonical(expected_log));
    if (!authoritative_log_envelope_exact) {
      throw std::runtime_error(
          "authoritative log is not the canonical single-link regular A3 "
          "evidence envelope");
    }
    const std::string authoritative_raw = read_bytes(log_path);
    const std::string parent_raw = read_bytes(
        root / ".build/tests/representation_srr_v1_authoritative.log");
    const std::string parent_audit_raw =
        read_bytes(root / ".build/tests/representation_srr_v1_audit.log");
    Records records = parse_records(authoritative_raw, true);
    Records parent = parse_records(parent_raw, false);
    Records parent_audit = parse_records(parent_audit_raw, false);
    const bool authoritative_tail_exact =
        authorization_machine_tail_exact(authoritative_raw, records);
    if (!authoritative_tail_exact) {
      fail(records,
           "authoritative authorization tail is not literal/exact/ordered");
    }
    ManifestEvidence manifest = verify_manifest(records, root, executable);
    const MechanicsEvidence mechanics = verify_mechanics_log(root);
    const PreflightEvidence preflight =
        verify_preflight_log(root, manifest.token_layout_hash, mechanics);
    manifest.mechanics_log_exact = mechanics.exact;
    manifest.preflight_log_exact = preflight.exact;
    manifest.exact &= mechanics.exact && preflight.exact;
    for (const auto &error : mechanics.errors) {
      fail(records, "mechanics receipt: " + error);
    }
    for (const auto &error : preflight.errors) {
      fail(records, "preflight receipt: " + error);
    }
    compare_emitted_bool(records, "srr2.prerun_manifest.exact", manifest.exact);
    const AttemptLedgerEvidence attempt_ledger =
        verify_attempt_ledger(records, root, manifest);
    const ParentEvidence parent_evidence =
        verify_parent(records, parent, parent_audit, root, parent_raw);
    const bool provisional_audit_input =
        structural_input_exact(records) && authoritative_tail_exact;
    InputEvidence evidence =
        build_gate_input(records, parent, root, manifest, parent_evidence,
                         mechanics, attempt_ledger, provisional_audit_input);

    const bool emitted_audit_input =
        boolean(records, "srr2.mechanics.audit_input_exact");
    const auto provisional_gate = gate::evaluate(evidence.input);
    compare_gate_result(records, provisional_gate);
    close_authoritative_schema(records);
    const bool schema_closed = records.unaccessed_keys == 0;
    const bool final_audit_input = structural_input_exact(records) &&
                                   schema_closed && authoritative_tail_exact;
    evidence.input.mechanics.audit_input_exact = final_audit_input;
    if (emitted_audit_input != final_audit_input) {
      fail(records, "emitted boolean disagrees with audit: "
                    "srr2.mechanics.audit_input_exact");
    }

    const auto official_gate = gate::evaluate(evidence.input);
    compare_gate_result(records, official_gate);
    const auto independent = independent_gate(evidence.input);
    const bool gate_agreement = independent_agrees(official_gate, independent);
    if (!gate_agreement) {
      fail(records,
           "frozen gate and independent precedence evaluation disagree");
    }

    const bool parent_parse_exact =
        parent.errors.empty() && parent_audit.errors.empty() &&
        parent.duplicate_keys == 0 && parent_audit.duplicate_keys == 0 &&
        parent.nonfinite_values == 0 && parent_audit.nonfinite_values == 0;
    const bool pass =
        records.errors.empty() && parent_parse_exact && gate_agreement;
    const std::string gate_classification =
        gate::terminal_classification_name(official_gate.classification);
    const std::string gate_reason =
        gate::failure_reason_name(official_gate.failure_reason);
    const std::string final_classification =
        pass ? gate_classification : "invalid_mechanics";
    const std::string final_reason = pass ? gate_reason : "audit_input";
    const std::size_t parent_hash_comparisons =
        evidence.normalizer.parent_hash_comparisons +
        evidence.captures.parent_hash_comparisons;
    const std::size_t error_count = records.errors.size() +
                                    parent.errors.size() +
                                    parent_audit.errors.size();

    std::cout << std::boolalpha << std::setprecision(17);
    std::cout << "schema=" << kAuditSchema << '\n';
    std::cout << "srr2.audit.authoritative_log.bytes="
              << authoritative_raw.size() << '\n';
    std::cout << "srr2.audit.authoritative_log.sha256="
              << sha256(authoritative_raw) << '\n';
    std::cout << "srr2.audit.authoritative_log.envelope_exact="
              << authoritative_log_envelope_exact << '\n';
    std::cout << "srr2.audit.machine_line_count=" << records.machine_lines
              << '\n';
    std::cout << "srr2.audit.duplicate_key_count=" << records.duplicate_keys
              << '\n';
    std::cout << "srr2.audit.malformed_line_count=" << records.malformed_lines
              << '\n';
    std::cout << "srr2.audit.nonfinite_value_count=" << records.nonfinite_values
              << '\n';
    std::cout << "srr2.audit.critical_value_error_count="
              << records.critical_value_errors << '\n';
    std::cout << "srr2.audit.unaccessed_key_count=" << records.unaccessed_keys
              << '\n';
    std::cout << "srr2.audit.schema_closed=" << schema_closed << '\n';
    std::cout << "srr2.audit.authoritative_authorization_tail_exact="
              << authoritative_tail_exact << '\n';
    std::cout << "srr2.audit.manifest.bytes=" << manifest.bytes << '\n';
    std::cout << "srr2.audit.manifest.sha256=" << manifest.digest << '\n';
    std::cout << "srr2.audit.manifest.entry_count=" << manifest.entry_count
              << '\n';
    std::cout << "srr2.audit.manifest.runtime_binding_exact="
              << manifest.runtime_binding_exact << '\n';
    std::cout << "srr2.audit.manifest.live_entries_exact="
              << manifest.live_entries_exact << '\n';
    std::cout << "srr2.audit.manifest.canonical_containment_exact="
              << manifest.containment_exact << '\n';
    std::cout << "srr2.audit.manifest.baseline_exact="
              << manifest.baseline_exact << '\n';
    std::cout << "srr2.audit.manifest.candidate_delta_exact="
              << manifest.candidate_delta_exact << '\n';
    std::cout << "srr2.audit.manifest.build_receipt_exact="
              << manifest.build_receipt_exact << '\n';
    std::cout << "srr2.audit.manifest.parent_quality_source_exact="
              << manifest.parent_quality_source_exact << '\n';
    std::cout << "srr2.audit.manifest.auditor_binary_exact="
              << manifest.auditor_binary_exact << '\n';
    std::cout << "srr2.audit.manifest.mechanics_log_exact="
              << manifest.mechanics_log_exact << '\n';
    std::cout << "srr2.audit.manifest.preflight_log_exact="
              << manifest.preflight_log_exact << '\n';
    std::cout << "srr2.audit.manifest.protocol_amendment_exact="
              << manifest.amendment_exact << '\n';
    std::cout << "srr2.audit.manifest.protocol_amendment_a2_exact="
              << manifest.amendment_a2_exact << '\n';
    std::cout << "srr2.audit.manifest.protocol_amendment_a3_exact="
              << manifest.amendment_a3_exact << '\n';
    std::cout << "srr2.audit.manifest.a2_incident_exact="
              << manifest.a2_incident_exact << '\n';
    std::cout << "srr2.audit.manifest.attempt_ledger_preseal_contract_exact="
              << manifest.attempt_ledger_preseal_contract_exact << '\n';
    std::cout
        << "srr2.audit.manifest.authoritative_log_preseal_contract_exact="
        << manifest.authoritative_log_preseal_contract_exact << '\n';
    std::cout << "srr2.audit.manifest.exact=" << manifest.exact << '\n';
    std::cout << "srr2.audit.attempt_ledger.bytes=" << attempt_ledger.bytes
              << '\n';
    std::cout << "srr2.audit.attempt_ledger.sha256=" << attempt_ledger.digest
              << '\n';
    std::cout << "srr2.audit.attempt_ledger.attempt_count="
              << attempt_ledger.attempt_count << '\n';
    std::cout << "srr2.audit.attempt_ledger.content_exact="
              << attempt_ledger.content_exact << '\n';
    std::cout << "srr2.audit.attempt_ledger.exclusive_create_exact="
              << attempt_ledger.exclusive_create_exact << '\n';
    std::cout << "srr2.audit.attempt_ledger.durable_exact="
              << attempt_ledger.durable_exact << '\n';
    std::cout << "srr2.audit.attempt_ledger.path_exact="
              << attempt_ledger.path_exact << '\n';
    std::cout << "srr2.audit.attempt_ledger.mode_0444_exact="
              << attempt_ledger.mode_exact << '\n';
    std::cout << "srr2.audit.attempt_ledger.single_link_exact="
              << attempt_ledger.single_link_exact << '\n';
    std::cout << "srr2.audit.attempt_ledger.source_manifest_binding_exact="
              << attempt_ledger.source_manifest_binding_exact << '\n';
    std::cout << "srr2.audit.attempt_ledger.binary_manifest_binding_exact="
              << attempt_ledger.binary_manifest_binding_exact << '\n';
    std::cout << "srr2.audit.attempt_ledger.emission_order_exact="
              << attempt_ledger.emission_order_exact << '\n';
    std::cout << "srr2.audit.attempt_ledger.exact=" << attempt_ledger.exact
              << '\n';
    std::cout << "srr2.audit.parent.artifact_count=" << kParentArtifacts.size()
              << '\n';
    std::cout << "srr2.audit.parent.log_bytes_exact="
              << (parent_raw.size() == kParentLogBytes) << '\n';
    std::cout << "srr2.audit.parent.log_sha256_exact="
              << (sha256(parent_raw) == kParentLogSha) << '\n';
    std::cout << "srr2.audit.parent.artifacts_exact="
              << parent_evidence.artifacts_exact << '\n';
    std::cout << "srr2.audit.parent.classification_exact="
              << parent_evidence.classification_exact << '\n';
    std::cout << "srr2.audit.parent.attempt_exact="
              << parent_evidence.attempt_exact << '\n';
    std::cout << "srr2.audit.parent.audit_pass=" << parent_evidence.audit_pass
              << '\n';
    std::cout << "srr2.audit.parent.authorizations_false="
              << parent_evidence.authorizations_false << '\n';
    std::cout << "srr2.audit.normalizer_exact=" << evidence.normalizer.exact
              << '\n';
    std::cout << "srr2.audit.capture.retained_count="
              << evidence.captures.parity.retained_capture_count << '\n';
    std::cout << "srr2.audit.capture.repeat_count="
              << evidence.captures.parity.repeat_capture_count << '\n';
    std::cout << "srr2.audit.capture.input_hashes_exact="
              << evidence.captures.input_hashes_exact << '\n';
    std::cout << "srr2.audit.capture.parent_hash_comparisons="
              << parent_hash_comparisons << '\n';
    std::cout << "srr2.audit.capture.parent_source_hashes_exact="
              << evidence.captures.parent_source_hashes_exact << '\n';
    std::cout << "srr2.audit.capture.parent_reference_hashes_exact="
              << evidence.captures.parent_reference_hashes_exact << '\n';
    std::cout << "srr2.audit.capture.parent_shadow_hashes_exact="
              << evidence.captures.parent_shadow_hashes_exact << '\n';
    std::cout << "srr2.audit.capture.cpu64_hashes_exact="
              << evidence.captures.cpu64_hashes_exact << '\n';
    std::cout << "srr2.audit.parity.cpu64_max_abs="
              << evidence.input.parity.cpu64_max_abs << '\n';
    std::cout << "srr2.audit.parity.device_max_abs="
              << evidence.input.parity.device_max_abs << '\n';
    std::cout
        << "srr2.audit.device.production_reference_max_abs="
        << evidence.input.device_translation.device_production_reference_max_abs
        << '\n';
    std::cout
        << "srr2.audit.device.shadow_reference_max_abs="
        << evidence.input.device_translation.device_shadow_reference_max_abs
        << '\n';
    std::cout << "srr2.audit.gate.independent_agreement=" << gate_agreement
              << '\n';
    std::cout << "srr2.audit.gate.classification=" << gate_classification
              << '\n';
    std::cout << "srr2.audit.gate.failure_reason=" << gate_reason << '\n';
    std::cout << "srr2.audit.final_classification=" << final_classification
              << '\n';
    std::cout << "srr2.audit.failure_reason=" << final_reason << '\n';
    std::cout << "srr2.audit.scientific_success="
              << (gate_classification ==
                  "production_structured_readout_parity_reproduced")
              << '\n';
    std::cout << "srr2.audit.error_count=" << error_count << '\n';
    std::size_t error_index = 0;
    const auto print_errors = [&](std::string_view source,
                                  const std::vector<std::string> &errors) {
      for (const auto &error : errors) {
        if (error_index < 32) {
          std::cout << "srr2.audit.error_" << error_index << '=' << source
                    << ": " << error << '\n';
        }
        ++error_index;
      }
    };
    print_errors("parent", parent.errors);
    print_errors("parent_audit", parent_audit.errors);
    print_errors("srr2", records.errors);
    if (error_index > 32) {
      std::cout << "srr2.audit.suppressed_error_count=" << error_index - 32
                << '\n';
    }
    std::cout << "srr2.audit.pass=" << pass << '\n';
    std::cout << "audit_pass=" << pass << '\n';
    std::cout << "audit_error_count=" << error_count << '\n';
    std::cout << kAuthorizationTail;
    return pass ? 0 : 3;
  } catch (const std::exception &error) {
    std::cout << std::boolalpha;
    std::cout << "srr2.audit.pass=false\n"
              << "srr2.audit.fatal_error=" << error.what() << '\n'
              << "audit_pass=false\n"
              << "audit_error_count=1\n"
              << kAuthorizationTail;
    return 2;
  }
}
