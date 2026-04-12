/* idydb.h */
/* based on the original work of: https://github.com/bradley499/flitdb/tree/main */
/* also used in project idyicyanere */
#ifndef idydb_h
#define idydb_h

#ifndef CUWACUNU_CAMAHJUCUNU_DB_VERBOSE_DEBUG
#define CUWACUNU_CAMAHJUCUNU_DB_VERBOSE_DEBUG 0 // default: disabled; define to 1 for verbose DB logs
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
#include <string>
#include <vector>
#endif

#define IDYDB_SUCCESS       0  // Successful operation
#define IDYDB_ERROR         1  // Unsuccessful operation
#define IDYDB_PERM          2  // Permission denied
#define IDYDB_BUSY          3  // The database file is locked
#define IDYDB_NOT_FOUND     4  // The database file is not found
#define IDYDB_CORRUPT       5  // The database file is malformed
#define IDYDB_RANGE         6  // The requested range is outside the range of the database
#define IDYDB_CREATE        7  // Create a database if not existent
#define IDYDB_READONLY      8  // Only allow the reading of the database
#define IDYDB_DONE          9  // The operation was completed successfully
#define IDYDB_NULL          10 // The operation resulted in a null lookup
#define IDYDB_INTEGER       11 // The value type of int
#define IDYDB_FLOAT         12 // The value type of float
#define IDYDB_CHAR          13 // The value type of char
#define IDYDB_BOOL          14 // The value type of bool
#define IDYDB_VECTOR        15 // The value type of vector<float>
#define IDYDB_UNSAFE        16 // Discard safety protocols to allow for larger database
#define IDYDB_VERSION  0x117ee // The current IdyDB version magic number

// Database sizing options
#define IDYDB_SIZING_MODE_TINY  1 // Handle databases up to 14.74 megabytes in size
#define IDYDB_SIZING_MODE_SMALL 2 // Handle databases up to 4.26 gigabytes in size
#define IDYDB_SIZING_MODE_BIG   3 // Handle databases up to 281.47 terabytes in size

// Database memory mapping
#define IDYDB_MMAP_ALLOWED 1 // Allows the database to memory map files - if possible (1 - allowed, 0 - disallowed)

// Database sizing selection
#define IDYDB_SIZING_MODE IDYDB_SIZING_MODE_BIG // The sizing mode for this compilation

#ifdef __cplusplus
#define idydb_extern extern "C"
#else
#define idydb_extern extern
#endif

typedef struct idydb idydb;

#if IDYDB_SIZING_MODE == IDYDB_SIZING_MODE_BIG
typedef unsigned long long idydb_column_row_sizing;
#else
typedef unsigned short idydb_column_row_sizing;
#endif

/* ---------------- Runtime open options (encryption is runtime-controlled) ---------------- */

typedef struct idydb_open_options {
    int flags;                  /* existing flags: IDYDB_CREATE, IDYDB_READONLY, IDYDB_UNSAFE */
    bool encrypted_at_rest;     /* true => encrypted backing file using OpenSSL AES-256-GCM */
    const char* passphrase;     /* required if encrypted_at_rest == true */
    unsigned int pbkdf2_iter;   /* 0 => default (recommended); otherwise custom */
} idydb_open_options;

typedef struct idydb_named_lock idydb_named_lock;

typedef struct idydb_named_lock_options {
    bool shared;                     /* false => exclusive lock, true => shared lock */
    bool create_parent_directories;  /* create the lock path parent directory when needed */
    unsigned int retry_count;        /* number of retries after an initial busy result */
    unsigned int retry_delay_ms;     /* sleep between retries when retry_count > 0 */
    const char* owner_label;         /* optional metadata label written into the lock file */
} idydb_named_lock_options;

/**
 * @brief Unified runtime open entrypoint
 * @note On failure, *handler is left NULL (no close required).
 */
idydb_extern int idydb_open_with_options(const char *filename, idydb **handler, const idydb_open_options* options);

/**
 * @brief Configures the IdyDB handler to point to and operate on a specific file (PLAINTEXT)
 */
idydb_extern int idydb_open(const char *filename, idydb **handler, int flags);

/**
 * @brief Open an encrypted-at-rest database (AES-256-GCM + PBKDF2-HMAC-SHA256 via OpenSSL).
 */
idydb_extern int idydb_open_encrypted(const char *filename, idydb **handler, int flags, const char* passphrase);

/**
 * @brief Close the connection to the database, and delete the IdyDB handler object
 */
idydb_extern int idydb_close(idydb **handler);

/**
 * @brief Acquire a named OS-backed lock represented by `lock_path`.
 * @note The lock file is intentionally persistent; the kernel lock is released
 *       when `idydb_named_lock_release()` is called or when the process exits.
 */
idydb_extern int idydb_named_lock_acquire(const char* lock_path,
                                          idydb_named_lock** out_lock,
                                          const idydb_named_lock_options* options);

/**
 * @brief Release a named lock acquired with `idydb_named_lock_acquire()`.
 */
idydb_extern int idydb_named_lock_release(idydb_named_lock** lock);

/**
 * @brief Read the current owner metadata stored in a named lock file.
 * @note This is best-effort diagnostics intended for busy-lock messages.
 */
idydb_extern int idydb_named_lock_describe_owner(const char* lock_path,
                                                 char* buffer,
                                                 size_t buffer_size);

/**
 * @brief Return the current version of the IdyDB API
 */
idydb_extern unsigned int idydb_version_check();

/**
 * @brief Retrieve an error message from the IdyDB handler if once exists
 */
idydb_extern char* idydb_errmsg(idydb **handler);

/**
 * @brief Extract a value stored within the IdyDB handler
 */
idydb_extern int idydb_extract(idydb **handler, idydb_column_row_sizing column_position, idydb_column_row_sizing row_position);

/**
 * @brief Retrieve a numeric representation of what data type was retrieved:
 *        IDYDB_NULL, IDYDB_INTEGER, IDYDB_FLOAT, IDYDB_CHAR, IDYDB_BOOL, IDYDB_VECTOR
 */
idydb_extern int idydb_retrieved_type(idydb **handler);

idydb_extern int idydb_insert_int(idydb **handler, idydb_column_row_sizing column_position, idydb_column_row_sizing row_position, int value);
idydb_extern int idydb_insert_float(idydb **handler, idydb_column_row_sizing column_position, idydb_column_row_sizing row_position, float value);
idydb_extern int idydb_insert_char(idydb **handler, idydb_column_row_sizing column_position, idydb_column_row_sizing row_position, char* value);
idydb_extern int idydb_insert_const_char(idydb **handler, idydb_column_row_sizing column_position, idydb_column_row_sizing row_position, const char* value);
idydb_extern int idydb_insert_bool(idydb **handler, idydb_column_row_sizing column_position, idydb_column_row_sizing row_position, bool value);

typedef struct idydb_bulk_cell {
    idydb_column_row_sizing column;
    idydb_column_row_sizing row;
    unsigned char type; /* IDYDB_INTEGER/FLOAT/CHAR/BOOL/VECTOR */
    union {
        int i;
        float f;
        bool b;
        const char* s;
        const float* vec;
    } value;
    unsigned short vector_dims;
} idydb_bulk_cell;

/**
 * @brief Replace the writable database contents with a fresh set of cells.
 * @note Cells may be unsorted on input; the implementation sorts them by
 *       column,row before writing a brand-new snapshot.
 */
idydb_extern int idydb_replace_with_cells(idydb **handler,
                                          const idydb_bulk_cell* cells,
                                          size_t cell_count);

/**
 * @brief Insert a vector<float> (embedding)
 *
 * Layout on disk: [type=6][uint16 dims][dims * float32 bytes]
 * @param dims must be <= 16383
 */
idydb_extern int idydb_insert_vector(idydb **handler, idydb_column_row_sizing column_position, idydb_column_row_sizing row_position, const float* data, unsigned short dims);

/**
 * @brief Delete a value stored within the IdyDB handler
 */
idydb_extern int idydb_delete(idydb **handler, idydb_column_row_sizing column_position, idydb_column_row_sizing row_position);

/**
 * @brief Retrieve primitives from the last extracted value
 */
idydb_extern int   idydb_retrieve_int(idydb **handler);
idydb_extern float idydb_retrieve_float(idydb **handler);
idydb_extern char* idydb_retrieve_char(idydb **handler);
idydb_extern bool  idydb_retrieve_bool(idydb **handler);

/**
 * @brief Retrieve a vector from the last extracted value.
 * Returns an internal pointer valid until the next extract/insert/clear; do not free it.
 * On success, *out_dims is set to the vector dimensionality.
 */
idydb_extern const float* idydb_retrieve_vector(idydb **handler, unsigned short* out_dims);

/* --------------------------- Vector DB + RAG extensions --------------------------- */

typedef enum { IDYDB_SIM_COSINE = 1, IDYDB_SIM_L2 = 2 } idydb_similarity_metric;

typedef struct {
    idydb_column_row_sizing row; // 1-based row index
    float score;                 // higher is better
} idydb_knn_result;

/* --------------------------- Query Filters + Metadata --------------------------- */

typedef enum {
    IDYDB_FILTER_OP_EQ         = 1,
    IDYDB_FILTER_OP_NEQ        = 2,
    IDYDB_FILTER_OP_GT         = 3,
    IDYDB_FILTER_OP_GTE        = 4,
    IDYDB_FILTER_OP_LT         = 5,
    IDYDB_FILTER_OP_LTE        = 6,
    IDYDB_FILTER_OP_IS_NULL    = 7,
    IDYDB_FILTER_OP_IS_NOT_NULL= 8
} idydb_filter_op;

typedef union {
    int         i;
    float       f;
    bool        b;
    const char* s;  /* not owned */
} idydb_filter_value;

typedef struct {
    idydb_column_row_sizing column;
    unsigned char           type;  /* IDYDB_INTEGER/FLOAT/CHAR/BOOL/NULL */
    idydb_filter_op         op;
    idydb_filter_value      value;
} idydb_filter_term;

typedef struct {
    const idydb_filter_term* terms; /* not owned */
    size_t                   nterms;
} idydb_filter;

/* Returned metadata values (deep-copies for CHAR/VECTOR). */
typedef struct {
    unsigned char type; /* IDYDB_NULL/INTEGER/FLOAT/CHAR/BOOL/VECTOR */
    union {
        int   i;
        float f;
        bool  b;
        char* s; /* malloc'd if type==IDYDB_CHAR */
        struct { float* v; unsigned short dims; } vec; /* malloc'd if type==IDYDB_VECTOR */
    } as;
} idydb_value;

/* Convenience free helpers for heap allocations returned by IdyDB APIs. */
idydb_extern void idydb_free(void* p);
idydb_extern void idydb_value_free(idydb_value* v);
idydb_extern void idydb_values_free(idydb_value* values, size_t count);

idydb_extern int idydb_knn_search_vector_column(idydb **handler,
                                                idydb_column_row_sizing vector_column,
                                                const float* query,
                                                unsigned short dims,
                                                unsigned short k,
                                                idydb_similarity_metric metric,
                                                idydb_knn_result* out_results);

/* Filtered kNN: only rows passing `filter` are considered. */
idydb_extern int idydb_knn_search_vector_column_filtered(idydb **handler,
                                                        idydb_column_row_sizing vector_column,
                                                        const float* query,
                                                        unsigned short dims,
                                                        unsigned short k,
                                                        idydb_similarity_metric metric,
                                                        const idydb_filter* filter,
                                                        idydb_knn_result* out_results);

 
idydb_extern idydb_column_row_sizing idydb_column_next_row(idydb **handler, idydb_column_row_sizing column);

idydb_extern int idydb_rag_upsert_text(idydb **handler,
                                       idydb_column_row_sizing text_column,
                                       idydb_column_row_sizing vector_column,
                                       idydb_column_row_sizing row,
                                       const char* text,
                                       const float* embedding,
                                       unsigned short dims);

typedef int (*idydb_embed_fn)(const char* text, float** out_vector, unsigned short* out_dims, void* user);

idydb_extern void idydb_set_embedder(idydb **handler, idydb_embed_fn fn, void* user);

idydb_extern int idydb_rag_upsert_text_auto_embed(idydb **handler,
                                                  idydb_column_row_sizing text_column,
                                                  idydb_column_row_sizing vector_column,
                                                  idydb_column_row_sizing row,
                                                  const char* text);

idydb_extern int idydb_rag_query_topk(idydb **handler,
                                      idydb_column_row_sizing text_column,
                                      idydb_column_row_sizing vector_column,
                                      const float* query_embedding,
                                      unsigned short dims,
                                      unsigned short k,
                                      idydb_similarity_metric metric,
                                      idydb_knn_result* out_results,
                                      char** out_texts);

idydb_extern int idydb_rag_query_topk_filtered(idydb **handler,
                                              idydb_column_row_sizing text_column,
                                              idydb_column_row_sizing vector_column,
                                              const float* query_embedding,
                                              unsigned short dims,
                                              unsigned short k,
                                              idydb_similarity_metric metric,
                                              const idydb_filter* filter,
                                              idydb_knn_result* out_results,
                                              char** out_texts);

/* TopK with structured metadata:
 * out_meta is a flat array of size (k * meta_columns_count).
 * Indexing: out_meta[i*meta_columns_count + j] corresponds to result i, meta column j.
 * Caller frees CHAR/VECTOR in out_meta via idydb_values_free(out_meta, k*meta_columns_count).
 */
idydb_extern int idydb_rag_query_topk_with_metadata(idydb **handler,
                                                   idydb_column_row_sizing text_column,
                                                   idydb_column_row_sizing vector_column,
                                                   const float* query_embedding,
                                                   unsigned short dims,
                                                   unsigned short k,
                                                   idydb_similarity_metric metric,
                                                   const idydb_filter* filter,
                                                   const idydb_column_row_sizing* meta_columns,
                                                   size_t meta_columns_count,
                                                   idydb_knn_result* out_results,
                                                   char** out_texts,
                                                   idydb_value* out_meta);

idydb_extern int idydb_rag_query_context(idydb **handler,
                                         idydb_column_row_sizing text_column,
                                         idydb_column_row_sizing vector_column,
                                         const float* query_embedding,
                                         unsigned short dims,
                                         unsigned short k,
                                         idydb_similarity_metric metric,
                                         size_t max_chars,
                                         char** out_context);

idydb_extern int idydb_rag_query_context_filtered(idydb **handler,
                                                 idydb_column_row_sizing text_column,
                                                 idydb_column_row_sizing vector_column,
                                                 const float* query_embedding,
                                                 unsigned short dims,
                                                 unsigned short k,
                                                 idydb_similarity_metric metric,
                                                 const idydb_filter* filter,
                                                 size_t max_chars,
                                                 char** out_context);

#undef idydb_extern

#ifdef __cplusplus
namespace db {

using ::idydb;
using ::idydb_column_row_sizing;
using ::idydb_open_options;

using ::idydb_open_with_options;
using ::idydb_open;
using ::idydb_open_encrypted;
using ::idydb_close;
using ::idydb_named_lock;
using ::idydb_named_lock_options;
using ::idydb_named_lock_acquire;
using ::idydb_named_lock_release;
using ::idydb_named_lock_describe_owner;
using ::idydb_version_check;
using ::idydb_errmsg;
using ::idydb_extract;
using ::idydb_retrieved_type;

using ::idydb_insert_int;
using ::idydb_insert_float;
using ::idydb_insert_char;
using ::idydb_insert_const_char;
using ::idydb_insert_bool;
using ::idydb_insert_vector;

using ::idydb_delete;

using ::idydb_retrieve_int;
using ::idydb_retrieve_float;
using ::idydb_retrieve_char;
using ::idydb_retrieve_bool;
using ::idydb_retrieve_vector;

using ::idydb_similarity_metric;
using ::idydb_knn_result;
using ::idydb_knn_search_vector_column;
using ::idydb_knn_search_vector_column_filtered;
using ::idydb_column_next_row;

using ::idydb_rag_upsert_text;
using ::idydb_embed_fn;
using ::idydb_set_embedder;
using ::idydb_rag_upsert_text_auto_embed;
using ::idydb_rag_query_topk;
using ::idydb_rag_query_topk_filtered;
using ::idydb_rag_query_topk_with_metadata;
using ::idydb_rag_query_context;
using ::idydb_rag_query_context_filtered;

using ::idydb_filter_op;
using ::idydb_filter_value;
using ::idydb_filter_term;
using ::idydb_filter;
using ::idydb_value;
using ::idydb_free;
using ::idydb_value_free;
using ::idydb_values_free;

namespace wave_cursor {

// Packed wave-cursor index:
// [ run_id | episode_k | batch_j ]
// The layout is configurable by bit-widths and must fit in 63 bits.
struct layout_t {
    std::uint8_t run_bits{21};
    std::uint8_t episode_bits{21};
    std::uint8_t batch_bits{21};
};

struct parts_t {
    std::uint64_t run_id{0};
    std::uint64_t episode_k{0};
    std::uint64_t batch_j{0};
};

struct masked_query_t {
    std::uint64_t value{0};
    std::uint64_t mask{0};
};

inline constexpr std::uint8_t field_run = 0x01;
inline constexpr std::uint8_t field_episode = 0x02;
inline constexpr std::uint8_t field_batch = 0x04;

[[nodiscard]] inline constexpr std::uint64_t bitmask(std::uint8_t bits) noexcept {
    if (bits == 0) return 0ULL;
    if (bits >= 64) return ~0ULL;
    return (1ULL << bits) - 1ULL;
}

[[nodiscard]] inline constexpr bool valid_layout(const layout_t &layout) noexcept {
    const unsigned total = static_cast<unsigned>(layout.run_bits) +
                           static_cast<unsigned>(layout.episode_bits) +
                           static_cast<unsigned>(layout.batch_bits);
    if (layout.run_bits == 0 || layout.episode_bits == 0 || layout.batch_bits == 0) {
        return false;
    }
    // Keep sign bit clear for compatibility with signed transports.
    return total <= 63U;
}

[[nodiscard]] inline constexpr bool fits_bits(std::uint64_t value,
                                              std::uint8_t bits) noexcept {
    const std::uint64_t m = bitmask(bits);
    return (value & ~m) == 0ULL;
}

[[nodiscard]] inline constexpr bool pack(const layout_t &layout,
                                         const parts_t &parts,
                                         std::uint64_t *out) noexcept {
    if (!out || !valid_layout(layout)) return false;
    if (!fits_bits(parts.run_id, layout.run_bits)) return false;
    if (!fits_bits(parts.episode_k, layout.episode_bits)) return false;
    if (!fits_bits(parts.batch_j, layout.batch_bits)) return false;

    const std::uint8_t shift_episode = layout.batch_bits;
    const std::uint8_t shift_run = static_cast<std::uint8_t>(layout.batch_bits +
                                                              layout.episode_bits);
    *out = (parts.run_id << shift_run) |
           (parts.episode_k << shift_episode) |
           parts.batch_j;
    return true;
}

[[nodiscard]] inline constexpr bool unpack(const layout_t &layout,
                                           std::uint64_t packed,
                                           parts_t *out) noexcept {
    if (!out || !valid_layout(layout)) return false;
    const std::uint64_t batch_m = bitmask(layout.batch_bits);
    const std::uint64_t episode_m = bitmask(layout.episode_bits);
    const std::uint64_t run_m = bitmask(layout.run_bits);

    parts_t parts{};
    parts.batch_j = packed & batch_m;
    packed >>= layout.batch_bits;
    parts.episode_k = packed & episode_m;
    packed >>= layout.episode_bits;
    parts.run_id = packed & run_m;
    *out = parts;
    return true;
}

[[nodiscard]] inline constexpr bool build_masked_query(
    const layout_t &layout,
    const parts_t &parts,
    std::uint8_t field_mask,
    masked_query_t *out) noexcept {
    if (!out || !valid_layout(layout)) return false;
    constexpr std::uint8_t known = field_run | field_episode | field_batch;
    if ((field_mask & ~known) != 0) return false;

    parts_t masked_parts = parts;
    if ((field_mask & field_run) == 0) masked_parts.run_id = 0;
    if ((field_mask & field_episode) == 0) masked_parts.episode_k = 0;
    if ((field_mask & field_batch) == 0) masked_parts.batch_j = 0;

    std::uint64_t packed = 0;
    if (!pack(layout, masked_parts, &packed)) return false;

    const std::uint8_t shift_episode = layout.batch_bits;
    const std::uint8_t shift_run = static_cast<std::uint8_t>(layout.batch_bits +
                                                              layout.episode_bits);
    std::uint64_t m = 0;
    if ((field_mask & field_batch) != 0) {
        m |= bitmask(layout.batch_bits);
    }
    if ((field_mask & field_episode) != 0) {
        m |= (bitmask(layout.episode_bits) << shift_episode);
    }
    if ((field_mask & field_run) != 0) {
        m |= (bitmask(layout.run_bits) << shift_run);
    }

    out->mask = m;
    out->value = packed & m;
    return true;
}

[[nodiscard]] inline constexpr bool match_masked(
    std::uint64_t packed,
    const masked_query_t &query) noexcept {
    return (packed & query.mask) == query.value;
}

} // namespace wave_cursor

namespace query {

struct query_spec_t {
    std::vector<idydb_filter_term> filters{};
    std::vector<idydb_column_row_sizing> select_columns{};
    std::size_t limit{0};
    std::size_t offset{0};
    bool newest_first{false};
};

struct query_row_t {
    idydb_column_row_sizing row_id{0};
    std::vector<idydb_value> values{};
};

/* Row ids are deterministic: ascending by default, descending if newest_first.
 * A limit of zero means "no limit".
 */
[[nodiscard]] bool select_rows(idydb **handler, const query_spec_t &spec,
                               std::vector<idydb_column_row_sizing> *out_rows,
                               std::string *error = nullptr);

[[nodiscard]] bool fetch_row(idydb **handler, idydb_column_row_sizing row,
                             const std::vector<idydb_column_row_sizing> &columns,
                             std::vector<idydb_value> *out_values,
                             std::string *error = nullptr);
/* Ownership: caller must free `out_values` deep allocations via
 * `idydb_values_free(out_values->data(), out_values->size())`.
 */

[[nodiscard]] bool fetch_rows(idydb **handler,
                              const std::vector<idydb_column_row_sizing> &rows,
                              const std::vector<idydb_column_row_sizing> &columns,
                              std::vector<query_row_t> *out_rows,
                              std::string *error = nullptr);
/* Ownership: caller must free each `query_row_t.values` via
 * `idydb_values_free(row.values.data(), row.values.size())`.
 */

[[nodiscard]] bool count_rows(idydb **handler,
                              const std::vector<idydb_filter_term> &filters,
                              std::size_t *out_count,
                              std::string *error = nullptr);

/* `key_column` is expected to hold char/string values. */
[[nodiscard]] bool latest_row_by_key(idydb **handler,
                                     idydb_column_row_sizing key_column,
                                     const std::string &key_value,
                                     idydb_column_row_sizing *out_row,
                                     std::string *error = nullptr);

[[nodiscard]] bool exists_row(idydb **handler,
                              const std::vector<idydb_filter_term> &filters,
                              bool *out_exists,
                              std::string *error = nullptr);

} // namespace query

// C++ overload conveniences (header-only)
static inline int idydb_insert(idydb **handler, idydb_column_row_sizing c, idydb_column_row_sizing r, int v)
{ return ::idydb_insert_int(handler, c, r, v); }

static inline int idydb_insert(idydb **handler, idydb_column_row_sizing c, idydb_column_row_sizing r, float v)
{ return ::idydb_insert_float(handler, c, r, v); }

static inline int idydb_insert(idydb **handler, idydb_column_row_sizing c, idydb_column_row_sizing r, char* v)
{ return ::idydb_insert_char(handler, c, r, v); }

static inline int idydb_insert(idydb **handler, idydb_column_row_sizing c, idydb_column_row_sizing r, const char* v)
{ return ::idydb_insert_const_char(handler, c, r, v); }

static inline int idydb_insert(idydb **handler, idydb_column_row_sizing c, idydb_column_row_sizing r, bool v)
{ return ::idydb_insert_bool(handler, c, r, v); }

} // namespace db
#endif

#endif /* idydb_h */
