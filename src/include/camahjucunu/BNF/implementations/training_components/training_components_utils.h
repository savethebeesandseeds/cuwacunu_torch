/* training_components_utils.h */
#pragma once

#include <algorithm>
#include <cctype>
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

inline const std::string&
require_column(const std::unordered_map<std::string,std::string>& row,
               const std::string& key) {
  auto it = row.find(key);
  if (it == row.end()) throw std::runtime_error("Missing required column: " + key);
  const auto& v = it->second;
  if (v.empty() || v == "-") throw std::runtime_error("Empty/invalid value for column: " + key);
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
      throw std::runtime_error("Invalid option entry (missing '='): " + item);

    auto key = trim_copy(item.substr(0, pos));
    auto val = trim_copy(item.substr(pos + 1));

    if (key.empty())
      throw std::runtime_error("Invalid option key (empty) in: " + item);

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
inline std::string require_option(
    const std::unordered_map<std::string,std::string>& row,
    const std::string& key) {
  const auto& opt_str = require_column(row, "options");
  auto kv = parse_options_kvlist(opt_str);
  auto it = kv.find(key);
  if (it == kv.end())
    throw std::runtime_error("Missing required option: " + key);
  if (it->second.empty() || it->second == "-")
    throw std::runtime_error("Empty/invalid value for option: " + key);
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

  // Build a helpful error
  std::ostringstream oss;
  oss << "Missing required option (any of): ";
  bool first = true;
  for (auto* a : aliases) { if (!first) oss << ", "; first = false; oss << a; }

  // Context (row_id and any *_type columns)
  auto rid = row.find(ROW_ID_COLUMN_HEADER);
  if (rid != row.end()) oss << " [row_id=" << rid->second << "]";

  std::vector<std::string> types;
  for (const auto& p : row) if (ends_with(p.first, "_type"))
    types.push_back(p.first + "=" + p.second);
  if (!types.empty()) {
    oss << " {";
    for (size_t i=0;i<types.size();++i){ if(i) oss<<", "; oss<<types[i]; }
    oss << "}";
  }
  throw std::runtime_error(oss.str());
}

/* Validate that the set of options matches exactly the expected schema.
 * `expected` entries may be plain keys ("gamma") or alias groups ("epsilon|eps").
 * - Throws if any required group is missing.
 * - Throws if there are extra keys not listed in any group.
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
  for (const auto& p : kv) {
    if (!allowed.count(p.first)) extras.push_back(p.first);
  }

  if (!missing_groups.empty() || !extras.empty()) {
    std::ostringstream oss;
    oss << "Options mismatch";

    auto rid = row.find(ROW_ID_COLUMN_HEADER);
    if (rid != row.end()) oss << " [row_id=" << rid->second << "]";

    std::vector<std::string> types;
    for (const auto& pr : row) if (ends_with(pr.first, "_type"))
      types.push_back(pr.first + "=" + pr.second);
    if (!types.empty()) {
      oss << " {";
      for (size_t i=0;i<types.size();++i){ if(i) oss<<", "; oss<<types[i]; }
      oss << "}";
    }
    oss << ". ";

    if (!missing_groups.empty()) {
      oss << "Missing: [";
      for (size_t i=0;i<missing_groups.size();++i){ if(i) oss<<", "; oss<<missing_groups[i]; }
      oss << "]. ";
    }
    if (!extras.empty()) {
      oss << "Unexpected: [";
      for (size_t i=0;i<extras.size();++i){ if(i) oss<<", "; oss<<extras[i]; }
      oss << "].";
    }
    throw std::runtime_error(oss.str());
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


/* Require that a row's column names match `expected` EXACTLY.
 * - If `enforce_nonempty` is true (default), also validates each expected
 *   column with require_column(row, col) to reject empty/"-".
 * - Error includes row_id and any *_type columns present.
 */
inline void require_columns_exact(
    const std::unordered_map<std::string,std::string>& row,
    const std::vector<std::string>& expected,
    bool enforce_nonempty = true)
{
  std::unordered_set<std::string> exp(expected.begin(), expected.end());

  // Compute missing / extras
  std::vector<std::string> missing, extras;
  for (const auto& k : expected) {
    if (!row.count(k)) missing.push_back(k);
  }
  for (const auto& kv : row) {
    if (!exp.count(kv.first)) extras.push_back(kv.first);
  }

  if (!missing.empty() || !extras.empty()) {
    std::ostringstream oss;
    oss << "Column set mismatch";

    auto rid = row.find(ROW_ID_COLUMN_HEADER);
    if (rid != row.end()) oss << " [row_id=" << rid->second << "]";

    std::vector<std::string> types;
    for (const auto& pr : row) if (ends_with(pr.first, "_type"))
      types.push_back(pr.first + "=" + pr.second);
    if (!types.empty()) {
      oss << " {";
      for (size_t i=0;i<types.size();++i){ if(i) oss<<", "; oss<<types[i]; }
      oss << "}";
    }
    oss << ". ";

    if (!missing.empty()) {
      oss << "Missing columns: [";
      for (size_t i=0;i<missing.size();++i){ if(i) oss<<", "; oss<<missing[i]; }
      oss << "]. ";
    }
    if (!extras.empty()) {
      oss << "Unexpected columns: [";
      for (size_t i=0;i<extras.size();++i){ if(i) oss<<", "; oss<<extras[i]; }
      oss << "].";
    }
    throw std::runtime_error(oss.str());
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
  char* end=nullptr; const char* c=s.c_str();
  double v = std::strtod(c, &end);
  if (end==c || *end!='\0') throw std::runtime_error("Invalid double: " + s);
  return v;
}
inline long to_long(const std::string& s) {
  char* end=nullptr; const char* c=s.c_str();
  long v = std::strtol(c, &end, 10);
  if (end==c || *end!='\0') throw std::runtime_error("Invalid long: " + s);
  return v;
}
inline bool to_bool(const std::string& s) {
  if (s=="true"||s=="True"||s=="TRUE"||s=="1")  return true;
  if (s=="false"||s=="False"||s=="FALSE"||s=="0") return false;
  throw std::runtime_error("Invalid bool: " + s);
}
inline std::vector<long> to_long_list_csv(const std::string& s) {
  std::vector<long> out;
  std::stringstream ss(s);
  std::string tok;
  while (std::getline(ss, tok, ',')) {
    tok = trim_copy(tok);
    if (!tok.empty()) out.push_back(to_long(tok));
  }
  if (out.empty()) throw std::runtime_error("Invalid long list CSV: " + s);
  return out;
}

} /* namespace camahjucunu */
} /* namespace cuwacunu */
