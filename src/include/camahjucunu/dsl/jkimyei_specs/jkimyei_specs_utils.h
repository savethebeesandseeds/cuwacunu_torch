/* jkimyei_specs_utils.h */
#pragma once

#include <algorithm>
#include <cctype>
#include <cerrno>
#include <cmath>
#include <cstdlib>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#define ROW_ID_COLUMN_HEADER "row_id"   /* every table should have this column */

namespace cuwacunu {
namespace camahjucunu {

/* ---------- Context helpers for rich error messages ---------- */

inline std::string row_context(const std::unordered_map<std::string,std::string>& row) {
  std::ostringstream oss;
  auto rid = row.find(ROW_ID_COLUMN_HEADER);
  if (rid != row.end()) oss << "[row_id=" << rid->second << "]";
  std::vector<std::string> types;
  types.reserve(row.size());
  for (const auto& kv : row) {
    const auto& k = kv.first;
    if (k.size() >= 5 && k.rfind("_type") == k.size()-5) {
      types.emplace_back(k + "=" + kv.second);
    }
  }
  if (!types.empty()) {
    oss << " {";
    for (size_t i=0;i<types.size();++i) { if (i) oss << ", "; oss << types[i]; }
    oss << "}";
  }
  if (oss.tellp() > 0) oss << " ";
  return oss.str();
}

#define RAISE_FATAL_ROW(ROW, FMT, ...) \
  do { \
    const std::string __ctx = ::cuwacunu::camahjucunu::row_context(ROW); \
    RAISE_FATAL("%s" FMT, __ctx.c_str(), ##__VA_ARGS__); \
  } while (0)

/* --------------------------- String helpers --------------------------- */

inline std::string trim_copy(std::string s) {
  auto not_space = [](unsigned char ch){ return !std::isspace(ch); };
  s.erase(s.begin(), std::find_if(s.begin(), s.end(), not_space));
  s.erase(std::find_if(s.rbegin(), s.rend(), not_space).base(), s.end());
  return s;
}

inline bool ends_with(const std::string& s, const std::string& suf) {
  return s.size() >= suf.size() &&
         std::equal(s.end()-suf.size(), s.end(), suf.begin(), suf.end());
}

/* ------------------------------ Columns ------------------------------- */

inline std::string
require_column(const std::unordered_map<std::string,std::string>& row,
               const std::string& key) {
  auto it = row.find(key);
  if (it == row.end())
    RAISE_FATAL_ROW(row, "Missing required column: \"%s\"", key.c_str());
  const auto& v = it->second;
  if (v.empty() || v == "-")
    RAISE_FATAL_ROW(row, "Empty/invalid value for column \"%s\" (got: \"%s\")", key.c_str(), v.c_str());
  return it->second;
}

/* ----------------------------- Options --------------------------------
 * Parse options of the form:  key=value, key2="val,with,commas", key3='x'
 * - Handles quotes and commas inside quotes.
 * - Last occurrence of a key wins.
 */
inline std::unordered_map<std::string,std::string>
parse_options_kvlist(const std::string& s) {
  std::unordered_map<std::string,std::string> kv;
  if (s.empty() || s == "-") return kv;

  std::string cur; cur.reserve(s.size());
  std::vector<std::string> items;
  char q = 0;  // current quote (' or "), 0 if not in quotes

  for (size_t i = 0; i < s.size(); ++i) {
    char c = s[i];
    if (q == 0 && (c == '\'' || c == '"')) {
      q = c; cur.push_back(c);
    } else if (q != 0 && c == q) {
      q = 0; cur.push_back(c);
    } else if (q == 0 && c == ',') {
      items.push_back(trim_copy(cur));
      cur.clear();
    } else {
      cur.push_back(c);
    }
  }
  if (!cur.empty()) items.push_back(trim_copy(cur));

  for (auto& item : items) {
    if (item.empty()) continue;
    auto pos = item.find('=');
    if (pos == std::string::npos)
      RAISE_FATAL("Invalid option entry (missing '='): \"%s\"", item.c_str());

    auto key = trim_copy(item.substr(0, pos));
    auto val = trim_copy(item.substr(pos + 1));

    if (key.empty())
      RAISE_FATAL("Invalid option key (empty) in entry: \"%s\"", item.c_str());

    // strip matching quotes around the whole value
    if (val.size() >= 2 &&
        ((val.front()=='"'  && val.back()=='"') ||
         (val.front()=='\'' && val.back()=='\''))) {
      val = val.substr(1, val.size()-2);
    }

    kv[key] = std::move(val); // last occurrence wins
  }
  return kv;
}

/* Require a specific option by key; returns the raw string value. */
inline bool has_option(
    const std::unordered_map<std::string,std::string>& row,
    const std::string& key) {
  const auto& opt_str = require_column(row, "options");
  auto kv = parse_options_kvlist(opt_str);
  auto it = kv.find(key);
  if (it == kv.end())
    return false;
  if (it->second.empty() || it->second == "-")
    return false;
  return true;
}

inline std::string require_option(
    const std::unordered_map<std::string,std::string>& row,
    const std::string& key) {
  const auto& opt_str = require_column(row, "options");
  auto kv = parse_options_kvlist(opt_str);
  auto it = kv.find(key);
  if (it == kv.end())
    RAISE_FATAL_ROW(row, "Missing required option: \"%s\". Options seen: \"%s\"", key.c_str(), opt_str.c_str());
  if (it->second.empty() || it->second == "-")
    RAISE_FATAL_ROW(row, "Empty/invalid value for option \"%s\" (got: \"%s\")", key.c_str(), it->second.c_str());
  return it->second;
}

/* Require one of several aliases. Example: require_any_option(row, {"epsilon","eps"}) */
inline std::string require_any_option(
    const std::unordered_map<std::string,std::string>& row,
    std::initializer_list<const char*> aliases) {
  const auto& opt_str = require_column(row, "options");
  auto kv = parse_options_kvlist(opt_str);

  for (auto* a : aliases) {
    auto it = kv.find(a);
    if (it != kv.end() && !it->second.empty() && it->second != "-")
      return it->second;
  }

  std::ostringstream want;
  bool first = true;
  for (auto* a : aliases) { if (!first) want << ", "; first = false; want << a; }

  RAISE_FATAL_ROW(row, "Missing required option (any of: %s). Options seen: \"%s\"",
                  want.str().c_str(), opt_str.c_str());
}

/* Validate that the set of options matches exactly the expected schema.
 * `expected` entries may be plain keys ("gamma") or alias groups ("epsilon|eps").
 */
inline void validate_options_exact(
    const std::unordered_map<std::string,std::string>& row,
    const std::vector<std::string>& expected)
{
  const auto& opt_str = require_column(row, "options");
  auto kv = parse_options_kvlist(opt_str);

  auto split_aliases = [](const std::string& s) {
    std::vector<std::string> out; std::string cur;
    for (char c : s) {
      if (c == '|') { auto t = trim_copy(cur); if (!t.empty()) out.push_back(std::move(t)); cur.clear(); }
      else { cur.push_back(c); }
    }
    auto t = trim_copy(cur); if (!t.empty()) out.push_back(std::move(t));
    return out;
  };

  std::unordered_set<std::string> allowed;
  allowed.reserve(expected.size()*2);

  std::vector<std::string> missing_groups;
  for (const auto& group : expected) {
    auto aliases = split_aliases(group);
    for (const auto& a : aliases) allowed.insert(a);

    bool found = false;
    for (const auto& a : aliases) {
      auto it = kv.find(a);
      if (it != kv.end() && !it->second.empty() && it->second != "-") { found = true; break; }
    }
    if (!found) missing_groups.push_back(group);
  }

  std::vector<std::string> extras;
  for (const auto& p : kv) if (!allowed.count(p.first)) extras.push_back(p.first);

  if (!missing_groups.empty() || !extras.empty()) {
    std::ostringstream miss, extra;
    for (size_t i=0;i<missing_groups.size();++i){ if(i) miss<<", "; miss<<missing_groups[i]; }
    for (size_t i=0;i<extras.size();++i){ if(i) extra<<", "; extra<<extras[i]; }
    RAISE_FATAL_ROW(row, "Options mismatch. Missing: [%s]. Unexpected: [%s]. Options seen: \"%s\"",
                    miss.str().c_str(), extra.str().c_str(), opt_str.c_str());
  }
}

/* Convenience overload */
inline void validate_options_exact(
    const std::unordered_map<std::string,std::string>& row,
    std::initializer_list<const char*> expected)
{
  std::vector<std::string> exp; exp.reserve(expected.size());
  for (auto* s : expected) exp.emplace_back(s);
  validate_options_exact(row, exp);
}

/* Require that a row's column names match `expected` EXACTLY. */
inline void require_columns_exact(
    const std::unordered_map<std::string,std::string>& row,
    const std::vector<std::string>& expected,
    bool enforce_nonempty = true)
{
  std::unordered_set<std::string> exp(expected.begin(), expected.end());

  std::vector<std::string> missing, extras;
  for (const auto& k : expected) if (!row.count(k)) missing.push_back(k);
  for (const auto& kv : row) if (!exp.count(kv.first)) extras.push_back(kv.first);

  if (!missing.empty() || !extras.empty()) {
    std::ostringstream miss, extra;
    for (size_t i=0;i<missing.size();++i){ if(i) miss<<", "; miss<<missing[i]; }
    for (size_t i=0;i<extras.size();++i){ if(i) extra<<", "; extra<<extras[i]; }
    RAISE_FATAL_ROW(row, "Column set mismatch. Missing: [%s]. Unexpected: [%s].",
                    miss.str().c_str(), extra.str().c_str());
  }

  if (enforce_nonempty) {
    for (const auto& k : expected) (void)require_column(row, k);
  }
}

/* Convenience overload */
inline void require_columns_exact(
    const std::unordered_map<std::string,std::string>& row,
    std::initializer_list<const char*> expected,
    bool enforce_nonempty = true)
{
  std::vector<std::string> exp; exp.reserve(expected.size());
  for (auto* s : expected) exp.emplace_back(s);
  require_columns_exact(row, exp, enforce_nonempty);
}

/* ----------------------------- Casting -------------------------------- */

inline double to_double(const std::string& s) {
  errno = 0;
  char* end=nullptr; const char* c=s.c_str();
  double v = std::strtod(c, &end);
  if (end==c || *end!='\0' || errno==ERANGE)
    RAISE_FATAL("Invalid double: \"%s\"", s.c_str());
  if (!std::isfinite(v))
    RAISE_FATAL("Invalid double (non-finite): \"%s\"", s.c_str());
  return v;
}
inline long to_long(const std::string& s) {
  errno = 0;
  char* end=nullptr; const char* c=s.c_str();
  long v = std::strtol(c, &end, 10);
  if (end==c || *end!='\0' || errno==ERANGE)
    RAISE_FATAL("Invalid long: \"%s\"", s.c_str());
  return v;
}
inline bool to_bool(const std::string& s) {
  if (s=="true"||s=="True"||s=="TRUE"||s=="1")  return true;
  if (s=="false"||s=="False"||s=="FALSE"||s=="0") return false;
  RAISE_FATAL("Invalid bool: \"%s\" (expected true/false/1/0)", s.c_str());
}
inline std::vector<long> to_long_list_csv(const std::string& s) {
  std::vector<long> out;
  std::stringstream ss(s);
  std::string tok;
  while (std::getline(ss, tok, ',')) {
    tok = trim_copy(tok);
    if (!tok.empty()) out.push_back(to_long(tok));
  }
  if (out.empty())
    RAISE_FATAL("Invalid long list CSV: \"%s\"", s.c_str());
  return out;
}

} /* namespace camahjucunu */
} /* namespace cuwacunu */
