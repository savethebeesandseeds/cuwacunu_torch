#include "piaabo/dconfig.h"

namespace cuwacunu {
namespace piaabo {
namespace dconfig {

std::mutex          config_mutex;
exchange_type_e     config_space_t::exchange_type;
std::string         config_space_t::config_folder;
std::string         config_space_t::config_file_path;
parsed_config_t     config_space_t::config;

/*──────────────────────── raw string helpers ─────────────────────────────*/
std::string config_space_t::raw(const std::string& section, const std::string& key)
{
  LOCK_GUARD(config_mutex);
  auto s = config.find(section);
  if (s == config.end())
    throw bad_config_access("Missing section [" + section + ']');

  auto k = s->second.find(key);
  if (k == s->second.end())
    throw bad_config_access("Missing key <" + key + "> in [" + section + ']');

  return k->second;
}

/* helper so we can write static-assert in an 'else' branch */
template<class> inline constexpr bool always_false_v = false;

/*────────── convert string to arbitrary type T ──────────*/
template<class T>
T config_space_t::from_string(const std::string& s)
{
  /* 1 ─ raw string */
  if constexpr (std::is_same_v<T, std::string>) {
    return s;

  /* 2 ─ boolean (accept many spellings) */
  } else if constexpr (std::is_same_v<T, bool>) {
    std::string v; v.reserve(s.size());
    std::transform(s.begin(), s.end(), std::back_inserter(v),
             [](unsigned char c){ return std::tolower(c); });
    return v=="1"||v=="true"||v=="yes"||v=="y"||v=="on";

  /* 3 ─ integers  (exclude bool explicitly) */
  } else if constexpr (std::is_integral_v<T> && !std::is_same_v<T,bool>) {
    T v{};
    auto [ptr, ec] = std::from_chars(s.data(), s.data()+s.size(), v);
    if (ec != std::errc())
      throw bad_config_access("Invalid integer '"+s+'\'');
    return v;

  /* 4 ─ floating point */
  } else if constexpr (std::is_floating_point_v<T>) {
    char* end{}; errno = 0;
    long double ld = std::strtold(s.c_str(), &end);
    if (errno || end != s.data()+s.size())
      throw bad_config_access("Invalid float '"+s+'\'');
    return static_cast<T>(ld);

  /* 5 ─ unsupported type → hard compile-time error */
  } else {
    static_assert(always_false_v<T>, "from_string<T>() not specialised for this T");
  }
}

/*────────────── public accessor — throws unless fallback provided ────────*/
template<class T>
T config_space_t::get(const std::string& section, const std::string& key, std::optional<T> fallback)
{
  try { return from_string<T>( raw(section, key) ); }
  catch (const bad_config_access&) {
    if (fallback) return *fallback;
    throw;
  }
}

/*────────────────────── INI-file reading (unchanged) ─────────────────────*/
// -----------------------------------------------------------------------------
// helper: strip comments, but keep anything inside single or double quotes
// -----------------------------------------------------------------------------
static std::string strip_comment(std::string_view line)
{
  bool in_single = false, in_double = false;

  for (size_t i = 0; i < line.size(); ++i) {
    char c = line[i];

    if    (c == '\'' && !in_double) in_single = !in_single;
    else if (c == '"'  && !in_single) in_double = !in_double;
    else if ((c == '#' || c == ';')   // potential comment char
         && !in_single && !in_double) {
      return std::string(line.substr(0, i));    // trim at comment
    }
  }
  return std::string(line);         // no comment found
}

// -----------------------------------------------------------------------------
// read_config
// -----------------------------------------------------------------------------
parsed_config_t config_space_t::read_config(const std::string& path)
{
  LOCK_GUARD(config_mutex);

  std::ifstream file = cuwacunu::piaabo::dfiles::readFileToStream(path);
  if (!file) {
    log_fatal("Cannot open config file: %s\n", path.c_str());
  }

  std::string raw, current;
  parsed_config_t parsed;

  while (std::getline(file, raw)) {
    // 1) remove comments, 2) trim whitespace
    std::string line = piaabo::trim_string( strip_comment(raw) );
    if (line.empty()) continue;

    // [SECTION]
    if (line.front() == '[' && line.back() == ']') {
      current = piaabo::trim_string(line.substr(1, line.size() - 2));
      parsed[current];       // ensure the section exists
      continue;
    }

    // key = value
    size_t pos = line.find('=');
    if (pos == std::string::npos) {
      log_warn("Skipping malformed line in %s: %s\n", path.c_str(), raw.c_str());
      continue;
    }

    std::string key   = piaabo::trim_string(line.substr(0, pos));
    std::string value = piaabo::trim_string(line.substr(pos + 1));

    parsed[current][key] = value;
  }
  return parsed;
}

/*──────────────────────────── config maintenance ─────────────────────────*/
void config_space_t::change_config_file(const char* folder, const char* file)
{
  config_folder  = folder;
  config_file_path  = config_folder + file;
  update_config();
}

void config_space_t::update_config()
{
  if (!std::filesystem::exists(config_file_path))
    log_warn("[dconfig] config file %s does not exist\n", config_file_path.c_str());

  config = read_config(config_file_path);
  validate_config();

  /* excahnge type */
  auto t = config["GENERAL"]["exchange_type"];
  auto exchange_type_temp = (t=="REAL") ? exchange_type_e::REAL : exchange_type_e::TEST;
  log_info("[dconfig] exchange_type = %s\n", t.c_str());

  if (exchange_type != exchange_type_e::NONE && exchange_type_temp != exchange_type) {
    log_fatal("(dconfig.cpp)[update_condig] mid course changes to exchange_type are not permitted\n");
  }

  exchange_type = exchange_type_temp;
}

/*────────────────────── validation (kept as before) ──────────────────────*/
bool config_space_t::validate_config()
{
  bool ok = true;
  #define TEST_CONFIG_SECTION(P,S,K) \
    if(config[S].find(K)==config[S].end()){ \
      log_warn("Missing field <%s> in section [%s]\n",K,S); *(P)=false;}
  #define TEST_CONFIG_OPTION(P,S,K,...) \
    TEST_CONFIG_SECTION(P,S,K) else { \
      std::string v=config[S][K]; \
      std::vector<std::string> choices={__VA_ARGS__}; \
      if(std::find(choices.begin(),choices.end(),v)==choices.end()){ \
        log_warn("Invalid value %s for [%s]<%s>\n",v.c_str(),S,K); *(P)=false;}}
  /* sections */
  // TEST_CONFIG_OPTION(&ok,"GENERAL","exchange_type","REAL","TEST");
  // TEST_CONFIG_SECTION(&ok,"VICReg","n_epochs");
  // TEST_CONFIG_SECTION(&ok,"VICReg","n_iters");
  // TEST_CONFIG_SECTION(&ok,"VICReg","lr");
  // TEST_CONFIG_SECTION(&ok,"GENERAL","absolute_base_symbol");
  // TEST_CONFIG_SECTION(&ok,"REAL_EXCHANGE","AES_salt");
  // TEST_CONFIG_SECTION(&ok,"REAL_EXCHANGE","Ed25519_pkey");
  // TEST_CONFIG_SECTION(&ok,"REAL_EXCHANGE","EXCHANGE_api_filename");
  // TEST_CONFIG_SECTION(&ok,"REAL_EXCHANGE","websocket_url");
  // TEST_CONFIG_SECTION(&ok,"TEST_EXCHANGE","AES_salt");
  // TEST_CONFIG_SECTION(&ok,"TEST_EXCHANGE","Ed25519_pkey");
  // TEST_CONFIG_SECTION(&ok,"TEST_EXCHANGE","EXCHANGE_api_filename");
  // TEST_CONFIG_SECTION(&ok,"TEST_EXCHANGE","websocket_url");
  // TEST_CONFIG_SECTION(&ok,"BNF","observation_pipeline_bnf_filename");
  // TEST_CONFIG_SECTION(&ok,"BNF","observation_pipeline_instruction_filename");

  if (!ok)
    log_terminate_gracefully("Invalid configuration, aborting.\n");

  return ok;
}

/*────────────────────── helper files (unchanged) ─────────────────────────*/
std::string config_space_t::websocket_url() {
  return config_space_t::config[config_space_t::exchange_type == exchange_type_e::REAL ? "REAL_EXCHANGE" : "TEST_EXCHANGE"]["websocket_url"];
}

std::string config_space_t::api_key() {
  return config_space_t::config[config_space_t::exchange_type == exchange_type_e::REAL ? "REAL_EXCHANGE" : "TEST_EXCHANGE"]["EXCHANGE_api_filename"];
}

std::string config_space_t::aes_salt() {
  return config_space_t::config[config_space_t::exchange_type == exchange_type_e::REAL ? "REAL_EXCHANGE" : "TEST_EXCHANGE"]["AES_salt"];
}

std::string config_space_t::Ed25519_pkey() {
  return config_space_t::config[config_space_t::exchange_type == exchange_type_e::REAL ? "REAL_EXCHANGE" : "TEST_EXCHANGE"]["Ed25519_pkey"];
}

std::string config_space_t::observation_pipeline_bnf() {
  return piaabo::dfiles::readFileToString(
    config["BNF"]["observation_pipeline_bnf_filename"]);
}
std::string config_space_t::observation_pipeline_instruction() {
  return piaabo::dfiles::readFileToString(
    config["BNF"]["observation_pipeline_instruction_filename"]);
}
std::string config_space_t::training_components_bnf() {
  return piaabo::dfiles::readFileToString(
    config["BNF"]["training_components_bnf_filename"]);
}
std::string config_space_t::training_components_instruction() {
  return piaabo::dfiles::readFileToString(
    config["BNF"]["training_components_instruction_filename"]);
}

/*──────────────────────────── life-cycle hooks ───────────────────────────*/
void config_space_t::finit()  { log_info("[dconfig] finalising\n"); }
void config_space_t::init() {
  log_info("[dconfig] initialising\n");
  exchange_type = exchange_type_e::NONE;
  change_config_file();     // defaults
}
config_space_t::_init config_space_t::_initializer;

/*──────────────── explicit template instantiations ───────────────────────*/
template int          config_space_t::get<int>        (const std::string&, const std::string&, std::optional<int>);
template double       config_space_t::get<double>     (const std::string&, const std::string&, std::optional<double>);
template bool         config_space_t::get<bool>       (const std::string&, const std::string&, std::optional<bool>);
template std::string  config_space_t::get<std::string>(const std::string&, const std::string&, std::optional<std::string>);

} // namespace dconfig
} // namespace piaabo
} // namespace cuwacunu
