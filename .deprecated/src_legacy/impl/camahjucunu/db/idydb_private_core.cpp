
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <unistd.h>
#include <sys/mman.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <stdarg.h>
#include <ctype.h>

#ifdef __linux__
#include <sys/syscall.h>
#endif

#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <stdint.h>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <set>
#include <sstream>
#include <string_view>
#include <thread>
#include <unordered_set>

#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/crypto.h>

#include "camahjucunu/db/idydb.h"

/* IdyDB database operations (internal) */
static int idydb_new(idydb **handler);
static void idydb_destroy(idydb **handler);
static int idydb_connection_setup(idydb **handler, const char *filename, int flags);
static int idydb_connection_setup_stream(idydb **handler, FILE* stream, int flags);
static char *idydb_get_err_message(idydb **handler);
static unsigned char idydb_read_at(idydb **handler, idydb_column_row_sizing column_position, idydb_column_row_sizing row_position);
static void idydb_clear_values(idydb **handler);
static unsigned char idydb_insert_at(idydb **handler, idydb_column_row_sizing column_position, idydb_column_row_sizing row_position);
static unsigned char idydb_insert_value_int(idydb **handler, int set_value);
static unsigned char idydb_insert_value_float(idydb **handler, float set_value);
static unsigned char idydb_insert_value_char(idydb **handler, char *set_value);
static unsigned char idydb_insert_value_bool(idydb **handler, bool set_value);
static unsigned char idydb_insert_value_vector(idydb **handler, const float* data, unsigned short dims);
static void idydb_insert_reset(idydb **handler);
static int idydb_retrieve_value_int(idydb **handler);
static float idydb_retrieve_value_float(idydb **handler);
static char *idydb_retrieve_value_char(idydb **handler);
static bool idydb_retrieve_value_bool(idydb **handler);
static const float* idydb_retrieve_value_vector(idydb **handler, unsigned short* out_dims);
static unsigned char idydb_retrieve_value_type(idydb **handler);
static void idydb_error_state(idydb **handler, unsigned char error_id);

union idydb_read_mmap_response;
static union idydb_read_mmap_response idydb_read_mmap(unsigned int position, unsigned char size, void *mmapped_char);

/* ----- constants and layout helpers ----- */
#define IDYDB_MAX_BUFFER_SIZE 1024
#define IDYDB_MAX_CHAR_LENGTH (0xFFFF - sizeof(short))   /* reader expects (stored_len + 1) <= IDYDB_MAX_CHAR_LENGTH */
#define IDYDB_MAX_VECTOR_DIM   16383
#define IDYDB_MAX_ERR_SIZE 100
#define IDYDB_SEGMENT_SIZE 3
#define IDYDB_PARTITION_SIZE 4
#define IDYDB_PARTITION_AND_SEGMENT (IDYDB_SEGMENT_SIZE + IDYDB_PARTITION_SIZE)
#define IDYDB_MMAP_MAX_SIZE 0x1400000

/* on-disk type tags for values that follow a segment header */
#define IDYDB_READ_INT         1
#define IDYDB_READ_FLOAT       2
#define IDYDB_READ_CHAR        3
#define IDYDB_READ_BOOL_TRUE   4
#define IDYDB_READ_BOOL_FALSE  5
#define IDYDB_READ_VECTOR      6

#define IDYDB_READ_AND_WRITE 0
#define IDYDB_READONLY_MMAPPED 2

#ifndef IDYDB_SIZING_MODE
#error No sizing mode type was defined to IDYDB_SIZING_MODE
#elif IDYDB_SIZING_MODE == IDYDB_SIZING_MODE_TINY
#define IDYDB_COLUMN_POSITION_MAX 0x000F
#define IDYDB_ROW_POSITION_MAX 0x000F
typedef unsigned int idydb_size_selection_type;
typedef unsigned short idydb_sizing_max;
#elif IDYDB_SIZING_MODE == IDYDB_SIZING_MODE_SMALL
#define IDYDB_COLUMN_POSITION_MAX 0x00FF
#define IDYDB_ROW_POSITION_MAX 0x00FF
typedef unsigned int idydb_size_selection_type;
typedef unsigned int idydb_sizing_max;
#elif IDYDB_SIZING_MODE == IDYDB_SIZING_MODE_BIG
#define IDYDB_COLUMN_POSITION_MAX 0xFFFF
#define IDYDB_ROW_POSITION_MAX 0xFFFF
#define IDYDB_ALLOW_UNSAFE
typedef unsigned long long idydb_size_selection_type;
typedef unsigned long long idydb_sizing_max;
#else
#error An invalid sizing mode was attributed to IDYDB_SIZING_MODE
#endif

#if IDYDB_MMAP_ALLOWED
#define IDYDB_MMAP_OK
#endif

/* ---------------- Encrypted-at-rest format ----------------
 * Header layout (little-endian ints):
 *  [0..7]   magic "IDYDBENC"
 *  [8..11]  version (u32) = 1
 *  [12..15] pbkdf2_iter (u32)
 *  [16..31] salt (16 bytes)
 *  [32..43] iv (12 bytes) (GCM nonce)
 *  [44..51] plaintext_len (u64)
 *  [52..67] tag (16 bytes) (GCM tag over AAD+ciphertext)
 *  [68..]   ciphertext (plaintext_len bytes)
 *
 * AAD = header bytes [0..51] (everything except the tag).
 */
#define IDYDB_ENC_MAGIC      "IDYDBENC"
#define IDYDB_ENC_MAGIC_LEN  8
#define IDYDB_ENC_VERSION    1u
#define IDYDB_ENC_SALT_LEN   16
#define IDYDB_ENC_IV_LEN     12
#define IDYDB_ENC_TAG_LEN    16
#define IDYDB_ENC_HDR_LEN    (IDYDB_ENC_MAGIC_LEN + 4 + 4 + IDYDB_ENC_SALT_LEN + IDYDB_ENC_IV_LEN + 8 + IDYDB_ENC_TAG_LEN)
#define IDYDB_ENC_AAD_LEN    (IDYDB_ENC_HDR_LEN - IDYDB_ENC_TAG_LEN)
#define IDYDB_ENC_KEY_LEN    32
#define IDYDB_ENC_DEFAULT_PBKDF2_ITER 200000u

/* Reasonable PBKDF2 bounds (DoS mitigation + sanity). Tune as you like. */
#define IDYDB_ENC_MIN_PBKDF2_ITER 10000u
#define IDYDB_ENC_MAX_PBKDF2_ITER 5000000u

static inline void idydb_u32_le_write(unsigned char* p, uint32_t v) {
	p[0] = (unsigned char)(v & 0xFFu);
	p[1] = (unsigned char)((v >> 8) & 0xFFu);
	p[2] = (unsigned char)((v >> 16) & 0xFFu);
	p[3] = (unsigned char)((v >> 24) & 0xFFu);
}
static inline uint32_t idydb_u32_le_read(const unsigned char* p) {
	return (uint32_t)p[0]
		| ((uint32_t)p[1] << 8)
		| ((uint32_t)p[2] << 16)
		| ((uint32_t)p[3] << 24);
}
static inline void idydb_u64_le_write(unsigned char* p, uint64_t v) {
	for (int i = 0; i < 8; ++i) p[i] = (unsigned char)((v >> (8 * i)) & 0xFFu);
}
static inline uint64_t idydb_u64_le_read(const unsigned char* p) {
	uint64_t v = 0;
	for (int i = 0; i < 8; ++i) v |= ((uint64_t)p[i] << (8 * i));
	return v;
}

static inline int idydb_crypto_iter_ok(uint32_t iter) {
	return iter >= IDYDB_ENC_MIN_PBKDF2_ITER && iter <= IDYDB_ENC_MAX_PBKDF2_ITER;
}

static int idydb_crypto_derive_key_pbkdf2(const char* passphrase,
                                         const unsigned char salt[IDYDB_ENC_SALT_LEN],
                                         uint32_t iter,
                                         unsigned char out_key[IDYDB_ENC_KEY_LEN])
{
	if (!passphrase || !out_key || iter == 0) return 0;
	if (!idydb_crypto_iter_ok(iter)) return 0;

	int ok = PKCS5_PBKDF2_HMAC(passphrase,
	                          (int)strlen(passphrase),
	                          salt,
	                          IDYDB_ENC_SALT_LEN,
	                          (int)iter,
	                          EVP_sha256(),
	                          IDYDB_ENC_KEY_LEN,
	                          out_key);
	return ok == 1;
}

/* ---------------- Secure plaintext working storage ----------------
 * Goal: do NOT write plaintext to a filesystem-backed temp file.
 *
 * Linux: memfd_create (anonymous RAM-backed FD).
 * Fallback: shm_open + shm_unlink (usually tmpfs-backed; anonymous after unlink).
 */
#ifdef __linux__
#ifndef MFD_CLOEXEC
#define MFD_CLOEXEC 0x0001U
#endif
static int idydb_memfd_create_compat(const char* name, unsigned int flags)
{
#ifdef SYS_memfd_create
	return (int)syscall(SYS_memfd_create, name, flags);
#else
	errno = ENOSYS;
	return -1;
#endif
}
#endif

static FILE* idydb_secure_plain_stream(const char** out_kind)
{
	if (out_kind) *out_kind = "unknown";

#ifdef __linux__
	{
		int fd = idydb_memfd_create_compat("idydb_plain", MFD_CLOEXEC);
		if (fd >= 0) {
			(void)fchmod(fd, 0600);
			FILE* f = fdopen(fd, "w+b");
			if (!f) { close(fd); return NULL; }
			if (out_kind) *out_kind = "memfd";
			return f;
		}
	}
#endif

	{
		unsigned char rnd[16];
		if (RAND_bytes(rnd, sizeof(rnd)) != 1) return NULL;

		char name[64];
		snprintf(name, sizeof(name),
		         "/idydb_%02x%02x%02x%02x%02x%02x%02x%02x",
		         rnd[0], rnd[1], rnd[2], rnd[3], rnd[4], rnd[5], rnd[6], rnd[7]);

		int fd = shm_open(name, O_CREAT | O_EXCL | O_RDWR, 0600);
		if (fd < 0) return NULL;

		(void)shm_unlink(name);

		FILE* f = fdopen(fd, "w+b");
		if (!f) { close(fd); return NULL; }
		if (out_kind) *out_kind = "shm";
		return f;
	}
}

static int idydb_crypto_decrypt_locked_file_to_stream(FILE* in,
                                                      const char* passphrase,
                                                      FILE* out_plain,
                                                      unsigned char out_salt[IDYDB_ENC_SALT_LEN],
                                                      uint32_t* out_iter,
                                                      unsigned char out_key[IDYDB_ENC_KEY_LEN])
{
	if (!in || !passphrase || !out_plain || !out_salt || !out_iter || !out_key) return 0;

	/* Get total size for sanity checks. */
	fseek(in, 0L, SEEK_END);
	long total_sz = ftell(in);
	if (total_sz < 0) return 0;
	fseek(in, 0L, SEEK_SET);

	if (total_sz < (long)IDYDB_ENC_HDR_LEN) return 0;

	unsigned char hdr[IDYDB_ENC_HDR_LEN];
	size_t r = fread(hdr, 1, IDYDB_ENC_HDR_LEN, in);
	if (r != IDYDB_ENC_HDR_LEN) return 0;

	if (memcmp(hdr, IDYDB_ENC_MAGIC, IDYDB_ENC_MAGIC_LEN) != 0) return 0;
	uint32_t ver = idydb_u32_le_read(hdr + 8);
	if (ver != IDYDB_ENC_VERSION) return 0;

	uint32_t iter = idydb_u32_le_read(hdr + 12);
	if (!idydb_crypto_iter_ok(iter)) return 0;

	memcpy(out_salt, hdr + 16, IDYDB_ENC_SALT_LEN);

	unsigned char iv[IDYDB_ENC_IV_LEN];
	memcpy(iv, hdr + 32, IDYDB_ENC_IV_LEN);

	uint64_t plaintext_len = idydb_u64_le_read(hdr + 44);

	unsigned char tag[IDYDB_ENC_TAG_LEN];
	memcpy(tag, hdr + 52, IDYDB_ENC_TAG_LEN);

	/* Since GCM ciphertext length equals plaintext length, ensure file length matches. */
	uint64_t cipher_len = (uint64_t)(total_sz - (long)IDYDB_ENC_HDR_LEN);
	if (cipher_len != plaintext_len) return 0;

	if (!idydb_crypto_derive_key_pbkdf2(passphrase, out_salt, iter, out_key)) return 0;
	*out_iter = iter;

	EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
	if (!ctx) return 0;

	int ok = 1;
	int len = 0;

	if (EVP_DecryptInit_ex(ctx, EVP_aes_256_gcm(), NULL, NULL, NULL) != 1) ok = 0;
	if (ok && EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, IDYDB_ENC_IV_LEN, NULL) != 1) ok = 0;
	if (ok && EVP_DecryptInit_ex(ctx, NULL, NULL, out_key, iv) != 1) ok = 0;

	if (ok && EVP_DecryptUpdate(ctx, NULL, &len, hdr, (int)IDYDB_ENC_AAD_LEN) != 1) ok = 0;

	fseek(in, IDYDB_ENC_HDR_LEN, SEEK_SET);

	unsigned char inbuf[16 * 1024];
	unsigned char outbuf[16 * 1024 + 16];
	uint64_t written = 0;

	while (ok) {
		size_t n = fread(inbuf, 1, sizeof(inbuf), in);
		if (n == 0) break;
		int outl = 0;
		if (EVP_DecryptUpdate(ctx, outbuf, &outl, inbuf, (int)n) != 1) { ok = 0; break; }
		if (outl > 0) {
			if (fwrite(outbuf, 1, (size_t)outl, out_plain) != (size_t)outl) { ok = 0; break; }
			written += (uint64_t)outl;
		}
	}

	if (ok) {
		if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, IDYDB_ENC_TAG_LEN, tag) != 1) ok = 0;
		unsigned char finalbuf[16];
		int finallen = 0;
		int final_ok = EVP_DecryptFinal_ex(ctx, finalbuf, &finallen);
		if (final_ok != 1) ok = 0;
	}

	EVP_CIPHER_CTX_free(ctx);

	if (ok && plaintext_len != written) ok = 0;

	fflush(out_plain);
	fseek(out_plain, 0L, SEEK_SET);
	return ok ? 1 : 0;
}

static int idydb_crypto_encrypt_stream_to_locked_file(FILE* plain,
                                                      FILE* out,
                                                      const unsigned char salt[IDYDB_ENC_SALT_LEN],
                                                      uint32_t iter,
                                                      const unsigned char key[IDYDB_ENC_KEY_LEN])
{
	if (!plain || !out || !salt || !key || iter == 0) return 0;
	if (!idydb_crypto_iter_ok(iter)) return 0;

	fflush(plain);
	long cur = ftell(plain);
	if (cur < 0) cur = 0;
	fseek(plain, 0L, SEEK_END);
	long plen_long = ftell(plain);
	if (plen_long < 0) return 0;
	uint64_t plaintext_len = (uint64_t)plen_long;
	fseek(plain, 0L, SEEK_SET);

	unsigned char iv[IDYDB_ENC_IV_LEN];
	if (RAND_bytes(iv, IDYDB_ENC_IV_LEN) != 1) return 0;

	unsigned char hdr[IDYDB_ENC_HDR_LEN];
	memset(hdr, 0, sizeof(hdr));
	memcpy(hdr, IDYDB_ENC_MAGIC, IDYDB_ENC_MAGIC_LEN);
	idydb_u32_le_write(hdr + 8, IDYDB_ENC_VERSION);
	idydb_u32_le_write(hdr + 12, iter);
	memcpy(hdr + 16, salt, IDYDB_ENC_SALT_LEN);
	memcpy(hdr + 32, iv, IDYDB_ENC_IV_LEN);
	idydb_u64_le_write(hdr + 44, plaintext_len);
	/* tag placeholder at hdr+52..+67 remains zero */

	/* truncate & write placeholder header */
	fflush(out);
	if (ftruncate(fileno(out), 0) != 0) return 0;

	fseek(out, 0L, SEEK_SET);
	if (fwrite(hdr, 1, IDYDB_ENC_HDR_LEN, out) != IDYDB_ENC_HDR_LEN) return 0;

	EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
	if (!ctx) return 0;

	int ok = 1;
	int len = 0;

	if (EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), NULL, NULL, NULL) != 1) ok = 0;
	if (ok && EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, IDYDB_ENC_IV_LEN, NULL) != 1) ok = 0;
	if (ok && EVP_EncryptInit_ex(ctx, NULL, NULL, key, iv) != 1) ok = 0;

	if (ok && EVP_EncryptUpdate(ctx, NULL, &len, hdr, (int)IDYDB_ENC_AAD_LEN) != 1) ok = 0;

	unsigned char inbuf[16 * 1024];
	unsigned char outbuf[16 * 1024 + 16];

	while (ok) {
		size_t n = fread(inbuf, 1, sizeof(inbuf), plain);
		if (n == 0) break;
		int outl = 0;
		if (EVP_EncryptUpdate(ctx, outbuf, &outl, inbuf, (int)n) != 1) { ok = 0; break; }
		if (outl > 0) {
			if (fwrite(outbuf, 1, (size_t)outl, out) != (size_t)outl) { ok = 0; break; }
		}
	}

	if (ok) {
		int finallen = 0;
		unsigned char finalbuf[16];
		if (EVP_EncryptFinal_ex(ctx, finalbuf, &finallen) != 1) ok = 0;
	}

	unsigned char tag[IDYDB_ENC_TAG_LEN];
	if (ok) {
		if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, IDYDB_ENC_TAG_LEN, tag) != 1) ok = 0;
	}

	EVP_CIPHER_CTX_free(ctx);

	if (!ok) return 0;

	/* write tag into header */
	fseek(out, 52, SEEK_SET);
	if (fwrite(tag, 1, IDYDB_ENC_TAG_LEN, out) != IDYDB_ENC_TAG_LEN) return 0;

	fflush(out);
	fsync(fileno(out));

	fseek(plain, cur, SEEK_SET);
	return 1;
}

/* --------------------------- Core object --------------------------- */

typedef struct idydb
{
	void *buffer;
	bool configured;
	FILE *file_descriptor;
	char err_message[IDYDB_MAX_ERR_SIZE];
	union value
	{
		int   int_value;
		float float_value;
		char  char_value[(IDYDB_MAX_CHAR_LENGTH + 1)];
		bool  bool_value;
	} value;
	float*         vector_value;
	unsigned short vector_dims;

	unsigned char value_type;
	bool value_retrieved;
	idydb_sizing_max size;
	unsigned char read_only;
#ifdef IDYDB_ALLOW_UNSAFE
	bool unsafe;
#endif

	idydb_embed_fn embedder;
	void* embedder_user;

	/* encryption runtime state */
	bool encryption_enabled;
	bool dirty;

	FILE* backing_descriptor; /* locked file handle for encrypted backing (or NULL) */
	char* backing_filename;

	unsigned char enc_salt[IDYDB_ENC_SALT_LEN];
	uint32_t enc_iter;
	unsigned char enc_key[IDYDB_ENC_KEY_LEN];
	bool enc_key_set;

	/* debug: where plaintext lives when encrypted mode is enabled */
	const char* plain_storage_kind; /* "memfd" / "shm" / NULL */

} idydb;

typedef struct idydb_named_lock
{
	int fd;
	bool shared;
	char* path;
} idydb_named_lock;

static std::uint64_t idydb_now_ms_utc()
{
	using namespace std::chrono;
	return static_cast<std::uint64_t>(
		duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count());
}

static std::string idydb_lock_sanitize_line(std::string_view value)
{
	std::string out;
	out.reserve(value.size());
	for (char c : value)
	{
		if (c == '\0' || c == '\n' || c == '\r' || c == '\t')
			out.push_back(' ');
		else
			out.push_back(c);
	}
	const auto first = out.find_first_not_of(' ');
	if (first == std::string::npos) return std::string{};
	const auto last = out.find_last_not_of(' ');
	return out.substr(first, last - first + 1);
}

static std::string idydb_lock_command_line()
{
#ifdef __linux__
	int fd = ::open("/proc/self/cmdline", O_RDONLY | O_CLOEXEC);
	if (fd < 0) return std::string{};

	std::vector<char> buffer(4096, '\0');
	const ssize_t nread = ::read(fd, buffer.data(), buffer.size());
	(void)::close(fd);
	if (nread <= 0) return std::string{};

	for (ssize_t i = 0; i < nread; ++i)
	{
		if (buffer[static_cast<std::size_t>(i)] == '\0')
			buffer[static_cast<std::size_t>(i)] = ' ';
	}
	return idydb_lock_sanitize_line(
		std::string_view(buffer.data(), static_cast<std::size_t>(nread)));
#else
	return std::string{};
#endif
}

static bool idydb_named_lock_write_all(int fd, const char* data, std::size_t size)
{
	std::size_t written = 0;
	while (written < size)
	{
		const ssize_t rc = ::write(fd, data + written, size - written);
		if (rc < 0)
		{
			if (errno == EINTR) continue;
			return false;
		}
		if (rc == 0) return false;
		written += static_cast<std::size_t>(rc);
	}
	return true;
}

static bool idydb_named_lock_read_all(int fd, std::string* out)
{
	if (!out) return false;
	out->clear();
	if (::lseek(fd, 0, SEEK_SET) < 0) return false;

	char chunk[512];
	while (true)
	{
		const ssize_t rc = ::read(fd, chunk, sizeof(chunk));
		if (rc < 0)
		{
			if (errno == EINTR) continue;
			return false;
		}
		if (rc == 0) break;
		out->append(chunk, static_cast<std::size_t>(rc));
	}
	return true;
}

static std::string idydb_named_lock_metadata(
	const idydb_named_lock_options* options)
{
	const bool shared = options && options->shared;
	const std::string owner_label =
		(options && options->owner_label)
			? idydb_lock_sanitize_line(options->owner_label)
			: std::string{};
	const std::string command = idydb_lock_command_line();

	std::ostringstream out;
	out << "pid=" << static_cast<long long>(::getpid()) << "\n";
	out << "uid=" << static_cast<unsigned long long>(::getuid()) << "\n";
	out << "mode=" << (shared ? "shared" : "exclusive") << "\n";
	out << "acquired_at_ms=" << static_cast<unsigned long long>(idydb_now_ms_utc())
	    << "\n";
	if (!owner_label.empty()) out << "owner_label=" << owner_label << "\n";
	if (!command.empty()) out << "command=" << command << "\n";
	return out.str();
}

extern "C" int idydb_named_lock_acquire(const char* lock_path,
                                        idydb_named_lock** out_lock,
                                        const idydb_named_lock_options* options)
{
	if (!lock_path || !out_lock || *out_lock != NULL) return IDYDB_ERROR;

	if (options && options->create_parent_directories)
	{
		std::error_code ec;
		const std::filesystem::path path(lock_path);
		const auto parent = path.parent_path();
		if (!parent.empty())
		{
			std::filesystem::create_directories(parent, ec);
			if (ec) return IDYDB_PERM;
		}
	}

	int fd = ::open(lock_path, O_RDWR | O_CREAT | O_CLOEXEC, 0600);
	if (fd < 0) return (errno == ENOENT ? IDYDB_NOT_FOUND : IDYDB_PERM);

	const int lock_mode =
		((options && options->shared) ? LOCK_SH : LOCK_EX) | LOCK_NB;
	const unsigned int retry_count = options ? options->retry_count : 0U;
	const unsigned int retry_delay_ms = options ? options->retry_delay_ms : 0U;

	bool locked = false;
	for (unsigned int attempt = 0; attempt <= retry_count; ++attempt)
	{
		if (::flock(fd, lock_mode) == 0)
		{
			locked = true;
			break;
		}
		if (errno == EINTR) continue;
		if ((errno != EWOULDBLOCK && errno != EAGAIN) || attempt >= retry_count)
		{
			(void)::close(fd);
			return (errno == EWOULDBLOCK || errno == EAGAIN) ? IDYDB_BUSY : IDYDB_PERM;
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(retry_delay_ms));
	}
	if (!locked)
	{
		(void)::close(fd);
		return IDYDB_BUSY;
	}

	const std::string metadata = idydb_named_lock_metadata(options);
	if (::ftruncate(fd, 0) != 0 ||
	    ::lseek(fd, 0, SEEK_SET) < 0 ||
	    !idydb_named_lock_write_all(fd, metadata.data(), metadata.size()) ||
	    ::fsync(fd) != 0)
	{
		(void)::flock(fd, LOCK_UN);
		(void)::close(fd);
		return IDYDB_ERROR;
	}

	idydb_named_lock* lock =
		static_cast<idydb_named_lock*>(::malloc(sizeof(idydb_named_lock)));
	if (!lock)
	{
		(void)::flock(fd, LOCK_UN);
		(void)::close(fd);
		return IDYDB_ERROR;
	}

	lock->fd = fd;
	lock->shared = (options && options->shared);
	lock->path = static_cast<char*>(::malloc(::strlen(lock_path) + 1));
	if (!lock->path)
	{
		(void)::flock(fd, LOCK_UN);
		(void)::close(fd);
		::free(lock);
		return IDYDB_ERROR;
	}
	::strcpy(lock->path, lock_path);
	*out_lock = lock;
	return IDYDB_SUCCESS;
}

extern "C" int idydb_named_lock_release(idydb_named_lock** lock)
{
	if (!lock || !*lock) return IDYDB_DONE;

	int rc = IDYDB_DONE;
	if ((*lock)->fd >= 0)
	{
		if (::flock((*lock)->fd, LOCK_UN) != 0) rc = IDYDB_ERROR;
		if (::close((*lock)->fd) != 0) rc = IDYDB_ERROR;
	}
	if ((*lock)->path) ::free((*lock)->path);
	::free(*lock);
	*lock = NULL;
	return rc;
}

extern "C" int idydb_named_lock_describe_owner(const char* lock_path,
                                               char* buffer,
                                               size_t buffer_size)
{
	if (!lock_path || !buffer || buffer_size == 0) return IDYDB_ERROR;
	buffer[0] = '\0';

	int fd = ::open(lock_path, O_RDONLY | O_CLOEXEC);
	if (fd < 0) return (errno == ENOENT ? IDYDB_NOT_FOUND : IDYDB_PERM);

	std::string metadata;
	const bool ok = idydb_named_lock_read_all(fd, &metadata);
	(void)::close(fd);
	if (!ok) return IDYDB_ERROR;

	std::snprintf(buffer, buffer_size, "%s", metadata.c_str());
	return IDYDB_SUCCESS;
}

/* ---------------- Verbose debug (compile-time) ---------------- */

#if CUWACUNU_CAMAHJUCUNU_DB_VERBOSE_DEBUG

static const char* idydb_ro_str(unsigned char ro)
{
	switch (ro)
	{
		case IDYDB_READ_AND_WRITE:   return "rw";
		case IDYDB_READONLY:         return "ro";
		case IDYDB_READONLY_MMAPPED: return "ro(mmap)";
		default:                     return "unknown";
	}
}

static void db_debugf(idydb **handler, const char* fmt, ...)
{
	(void)handler; /* currently unused, but kept in signature for future context */
	fprintf(stdout, "[DB] ");

	va_list ap;
	va_start(ap, fmt);
	vfprintf(stdout, fmt, ap);
	va_end(ap);

	fputc('\n', stdout);
	fflush(stdout);
}

#define DB_DEBUGF(db, fmt, ...) do { db_debugf((db), (fmt), ##__VA_ARGS__); } while(0)
#define DB_DEBUG(db, msg_literal) do { DB_DEBUGF((db), "%s", (msg_literal)); } while(0)

/* --- formatting helpers --- */

static void idydb_dbg_sha256_8bytes_hex16(const void* data, size_t len, char out_hex16[17])
{
	/* out = 8 bytes of sha256 => 16 hex chars + '\0' */
	static const char* hexd = "0123456789abcdef";
	unsigned char digest[32];
	unsigned int out_len = 0;
	EVP_MD_CTX* ctx = EVP_MD_CTX_new();
	if (!ctx) { strncpy(out_hex16, "noctx", 17); out_hex16[16] = '\0'; return; }

	int ok = (EVP_DigestInit_ex(ctx, EVP_sha256(), NULL) == 1)
	      && (EVP_DigestUpdate(ctx, data, len) == 1)
	      && (EVP_DigestFinal_ex(ctx, digest, &out_len) == 1)
	      && (out_len == 32);

	EVP_MD_CTX_free(ctx);

	if (!ok) { strncpy(out_hex16, "shaerr", 17); out_hex16[16] = '\0'; return; }

	for (int i = 0; i < 8; ++i) {
		out_hex16[2*i + 0] = hexd[(digest[i] >> 4) & 0xF];
		out_hex16[2*i + 1] = hexd[(digest[i] >> 0) & 0xF];
	}
	out_hex16[16] = '\0';
}

static void idydb_dbg_escape_preview(const char* in, char* out, size_t out_cap, size_t max_in_chars)
{
	static const char* hexd = "0123456789abcdef";
	if (!out || out_cap == 0) return;
	out[0] = '\0';
	if (!in) { snprintf(out, out_cap, "<null>"); return; }

	size_t o = 0;
	for (size_t i = 0; in[i] != '\0' && i < max_in_chars; ++i)
	{
		unsigned char ch = (unsigned char)in[i];
		if (o + 2 >= out_cap) break;

		switch (ch)
		{
			case '\\': if (o + 2 < out_cap) { out[o++]='\\'; out[o++]='\\'; } break;
			case '"':  if (o + 2 < out_cap) { out[o++]='\\'; out[o++]='"';  } break;
			case '\n': if (o + 2 < out_cap) { out[o++]='\\'; out[o++]='n';  } break;
			case '\r': if (o + 2 < out_cap) { out[o++]='\\'; out[o++]='r';  } break;
			case '\t': if (o + 2 < out_cap) { out[o++]='\\'; out[o++]='t';  } break;
			default:
				if (isprint(ch)) {
					out[o++] = (char)ch;
				} else {
					/* \xHH */
					if (o + 4 >= out_cap) break;
					out[o++]='\\'; out[o++]='x';
					out[o++]=hexd[(ch >> 4) & 0xF];
					out[o++]=hexd[(ch >> 0) & 0xF];
				}
				break;
		}
		if (o >= out_cap) break;
	}
	out[(o < out_cap) ? o : (out_cap - 1)] = '\0';
}

static void idydb_dbg_format_value_from_handler(const idydb* h, char* out, size_t cap)
{
	if (!out || cap == 0) return;
	if (!h) { snprintf(out, cap, "<nohandler>"); return; }

	switch (h->value_type)
	{
		case IDYDB_NULL:
			snprintf(out, cap, "NULL");
			return;

		case IDYDB_INTEGER:
			snprintf(out, cap, "INT(%d)", h->value.int_value);
			return;

		case IDYDB_FLOAT:
			snprintf(out, cap, "FLOAT(%.9g)", (double)h->value.float_value);
			return;

		case IDYDB_BOOL:
			snprintf(out, cap, "BOOL(%s)", (h->value.bool_value ? "true" : "false"));
			return;

		case IDYDB_CHAR:
		{
			const char* s = h->value.char_value;
			size_t len = (s ? strlen(s) : 0);
			char prev[96];
			idydb_dbg_escape_preview(s, prev, sizeof(prev), 48);
			const char* ell = (len > 48 ? "…" : "");
			snprintf(out, cap, "CHAR(len=%zu,\"%s%s\")", len, prev, ell);
			return;
		}

		case IDYDB_VECTOR:
		{
			unsigned short d = h->vector_dims;
			if (!h->vector_value || d == 0) {
				snprintf(out, cap, "VEC(d=%u,<null>)", (unsigned)d);
				return;
			}
			char sha16[17];
			idydb_dbg_sha256_8bytes_hex16(h->vector_value, (size_t)d * sizeof(float), sha16);
			snprintf(out, cap, "VEC(d=%u,sha=%s)", (unsigned)d, sha16);
			return;
		}

		default:
			snprintf(out, cap, "TYPE(%u)", (unsigned)h->value_type);
			return;
	}
}

/* Reads the current stored value of a cell and formats it to out. */
static unsigned char idydb_dbg_peek_cell_repr(idydb **handler,
                                             idydb_column_row_sizing c,
                                             idydb_column_row_sizing r,
                                             char* out,
                                             size_t cap)
{
	if (!out || cap == 0) return 0xFF;
	out[0] = '\0';

	int rc = idydb_read_at(handler, c, r);
	if (rc == IDYDB_DONE) {
		unsigned char t = (*handler)->value_type;
		idydb_dbg_format_value_from_handler(*handler, out, cap);
		idydb_clear_values(handler);
		return t;
	}

	if (rc == IDYDB_NULL) {
		snprintf(out, cap, "NULL");
		idydb_clear_values(handler);
		return IDYDB_NULL;
	}

	snprintf(out, cap, "ERR(rc=%d,%s)", rc, idydb_get_err_message(handler));
	idydb_clear_values(handler);
	return 0xFF;
}

#else

#define DB_DEBUGF(db, fmt, ...) do { (void)(db); } while(0)
#define DB_DEBUG(db, msg_literal) do { (void)(db); (void)(msg_literal); } while(0)

#endif


/* ---------------- Error state ---------------- */

static void idydb_error_state(idydb **handler, unsigned char error_id)
{
	const char *errors[] = {
		"\0",
		"The minimum buffer size has encroached beyond suitable definitions\0",
		"The maximum buffer size has encroached beyond suitable definitions\0",
		"The database handler has already been attributed to handle another database\0",
		"No database exists to be exclusively read\0",
		"Failed to open the database\0",
		"Exclusive rights to access the database could not be obtained\0",
		"The database attempted to access has a larger size than what this object can read\0",
		"The database handler has not been attributed to handle a database\0",
		"The database was opened in readonly mode\0",
		"Data insertion avoided due to unexpected tennant\0",
		"Data insertion avoided due to the length of a string being too large (or vector too large)\0",
#if IDYDB_SIZING_MODE == IDYDB_SIZING_MODE_TINY
		"The requested range was outside of the database's range (sizing mode parameter is: tiny)\0",
#elif IDYDB_SIZING_MODE == IDYDB_SIZING_MODE_SMALL
		"The requested range was outside of the database's range (sizing mode parameter is: small)\0",
#elif IDYDB_SIZING_MODE == IDYDB_SIZING_MODE_BIG
		"The requested range was outside of the database's range\0",
#endif
		"The database contracted a malformed structure declaration\0",
		"An error occurred in attempting to read data from the database\0",
		"An error occurred in attempting to write data to the database\0",
		"An error occurred in attempting to write data to an updating skip offset notation in the database\0",
		"Failed database truncation occurred\0",
		"An error occurred in attempting to retrieve data from the database\0",
		"Data retrieval avoided due to the length of a string being too large\0",
		"The database yielded an invalid datatype\0",
		"The requested range must have a valid starting range of at least 1\0",
		"The database declares ranges that exceed the current sizing mode parameter set\0",
		"Unable to enable unsafe mode due to compilation sizing mode parameter set\0",
		"Unable to allocate memory for the creation of the database handler\0",
		"An unknown error occurred\0",
		"Encryption requested but no passphrase supplied\0",
		"Database decryption failed (wrong passphrase, tampered file, or unsupported parameters)\0",
		"Database encryption writeback failed\0",
		"Failed to create secure in-memory plaintext working storage\0",
		"Encrypted READONLY open cannot migrate plaintext db; open writable once to migrate\0"
	};

	const unsigned char max_id = (unsigned char)(sizeof(errors) / sizeof(errors[0]) - 1);
	if (error_id > max_id) error_id = max_id;
	strncpy((*handler)->err_message, errors[error_id], IDYDB_MAX_ERR_SIZE);
}

static inline void idydb_error_state_if_available(idydb **handler, unsigned char error_id)
{
	if (handler && *handler) idydb_error_state(handler, error_id);
}

/* ---------------- mmap helper ---------------- */

#ifdef IDYDB_MMAP_OK
union idydb_read_mmap_response
{
	int integer;
	short short_integer;
	float floating_point;
	unsigned char bytes[4];
};

static union idydb_read_mmap_response idydb_read_mmap(unsigned int position, unsigned char size, void *mmapped_char)
{
	union idydb_read_mmap_response value;
	memset(value.bytes, 0, sizeof(value.bytes));
	unsigned char byte_position = 0;
	for (unsigned int i = position; i < (size + position); i++)
		value.bytes[byte_position++] = (char)(((char *)mmapped_char)[i]);
	return value;
}
#endif

/* ---------------- core helpers ---------------- */

unsigned int idydb_version_check()
{
	return IDYDB_VERSION;
}

static void idydb_clear_values(idydb **handler)
{
	if (!handler || !*handler) return;
	(*handler)->value.int_value = 0;
	(*handler)->value_type = IDYDB_NULL;
	(*handler)->value_retrieved = false;

	if ((*handler)->vector_value != NULL) {
		free((*handler)->vector_value);
		(*handler)->vector_value = NULL;
	}
	(*handler)->vector_dims = 0;
}

static int idydb_new(idydb **handler)
{
	*handler = (idydb *)malloc(sizeof(idydb));
	if (*handler == NULL)
		return IDYDB_ERROR;

	(*handler)->buffer = NULL; /* important */
	(*handler)->configured = (IDYDB_MAX_BUFFER_SIZE < 50 || IDYDB_MAX_BUFFER_SIZE > 1024);
	(*handler)->size = 0;
	(*handler)->read_only = IDYDB_READ_AND_WRITE;
#ifdef IDYDB_ALLOW_UNSAFE
	(*handler)->unsafe = false;
#endif
	(*handler)->value_type = 0;
	(*handler)->value_retrieved = false;
	(*handler)->file_descriptor = NULL;
	(*handler)->vector_value = NULL;
	(*handler)->vector_dims = 0;
	(*handler)->embedder = NULL;
	(*handler)->embedder_user = NULL;

	(*handler)->encryption_enabled = false;
	(*handler)->dirty = false;
	(*handler)->backing_descriptor = NULL;
	(*handler)->backing_filename = NULL;
	memset((*handler)->enc_salt, 0, sizeof((*handler)->enc_salt));
	(*handler)->enc_iter = 0;
	memset((*handler)->enc_key, 0, sizeof((*handler)->enc_key));
	(*handler)->enc_key_set = false;
	(*handler)->plain_storage_kind = NULL;

	if (IDYDB_MAX_BUFFER_SIZE < 50)
	{
		idydb_error_state(handler, 1);
		return IDYDB_ERROR;
	}
	else if (IDYDB_MAX_BUFFER_SIZE > 1024)
	{
		idydb_error_state(handler, 2);
		return IDYDB_ERROR;
	}
	else
		idydb_error_state(handler, 0);
	return IDYDB_SUCCESS;
}

static void idydb_destroy(idydb **handler)
{
	if (!handler || !*handler) return;

	if ((*handler)->configured)
	{
		(*handler)->configured = false;
		if ((*handler)->file_descriptor != NULL)
		{
			flock(fileno((*handler)->file_descriptor), LOCK_UN);
			fclose((*handler)->file_descriptor);
			(*handler)->file_descriptor = NULL;
		}
	}

	if ((*handler)->backing_descriptor != NULL)
	{
		flock(fileno((*handler)->backing_descriptor), LOCK_UN);
		fclose((*handler)->backing_descriptor);
		(*handler)->backing_descriptor = NULL;
	}
	if ((*handler)->backing_filename != NULL)
	{
		free((*handler)->backing_filename);
		(*handler)->backing_filename = NULL;
	}
	if ((*handler)->enc_key_set)
	{
		OPENSSL_cleanse((*handler)->enc_key, sizeof((*handler)->enc_key));
		(*handler)->enc_key_set = false;
	}

	if ((*handler)->read_only != IDYDB_READONLY_MMAPPED)
	{
		if ((*handler)->buffer != NULL) free((*handler)->buffer);
	}
	else
	{
		if ((*handler)->buffer != NULL && (*handler)->buffer != MAP_FAILED)
			munmap((*handler)->buffer, (*handler)->size);
	}
	(*handler)->buffer = NULL;

	if ((*handler)->vector_value != NULL) {
		free((*handler)->vector_value);
		(*handler)->vector_value = NULL;
	}
}

static inline const idydb_sizing_max idydb_max_size()
{
	idydb_sizing_max insertion_area[2] = {0, 0};
	insertion_area[0] = IDYDB_COLUMN_POSITION_MAX;
	insertion_area[0] *= IDYDB_ROW_POSITION_MAX;
	insertion_area[0] *= (IDYDB_MAX_CHAR_LENGTH - 1);
	if (IDYDB_ROW_POSITION_MAX > 1)
	{
		insertion_area[1] = IDYDB_COLUMN_POSITION_MAX;
		insertion_area[1] *= IDYDB_ROW_POSITION_MAX;
		insertion_area[1] *= IDYDB_SEGMENT_SIZE;
	}
	return (insertion_area[0] + insertion_area[1] + (IDYDB_COLUMN_POSITION_MAX * IDYDB_PARTITION_AND_SEGMENT));
}

static int idydb_connection_setup_stream(idydb **handler, FILE* stream, int flags)
{
	if ((*handler)->configured)
	{
		if (IDYDB_MAX_BUFFER_SIZE >= 50 && IDYDB_MAX_BUFFER_SIZE <= 1024)
			idydb_error_state(handler, 3);
		return IDYDB_ERROR;
	}

	(*handler)->size = 0;
	(*handler)->read_only = IDYDB_READ_AND_WRITE;
	if ((flags & IDYDB_READONLY) == IDYDB_READONLY)
		(*handler)->read_only = IDYDB_READONLY;

	(*handler)->file_descriptor = stream;
	if ((*handler)->file_descriptor == NULL)
	{
		idydb_error_state(handler, 5);
		return IDYDB_PERM;
	}

	fseek((*handler)->file_descriptor, 0L, SEEK_END);
	(*handler)->size = (idydb_sizing_max)ftell((*handler)->file_descriptor);
	fseek((*handler)->file_descriptor, 0L, SEEK_SET);

	(*handler)->configured = true;

	const int lock_mode =
		(((flags & IDYDB_READONLY) == IDYDB_READONLY) ? LOCK_SH : LOCK_EX) | LOCK_NB;
	if (flock(fileno((*handler)->file_descriptor), lock_mode) != 0)
	{
		idydb_error_state(handler, 6);
		return IDYDB_BUSY;
	}

	if ((flags & IDYDB_UNSAFE) == IDYDB_UNSAFE)
#ifdef IDYDB_ALLOW_UNSAFE
		(*handler)->unsafe = true;
#else
	{
		idydb_error_state(handler, 23);
		return IDYDB_ERROR;
	}
#endif
	else if ((*handler)->size > idydb_max_size())
	{
		idydb_error_state(handler, 7);
		return IDYDB_RANGE;
	}

	(*handler)->buffer = malloc((sizeof(char) * IDYDB_MAX_BUFFER_SIZE));
	if ((*handler)->buffer == NULL)
	{
		idydb_error_state(handler, 24);
		return IDYDB_ERROR;
	}
	return IDYDB_SUCCESS;
}

static int idydb_connection_setup(idydb **handler, const char *filename, int flags)
{
	if ((*handler)->configured)
	{
		if (IDYDB_MAX_BUFFER_SIZE >= 50 && IDYDB_MAX_BUFFER_SIZE <= 1024)
			idydb_error_state(handler, 3);
		return IDYDB_ERROR;
	}
	(*handler)->size = 0;
	(*handler)->read_only = IDYDB_READ_AND_WRITE;
	if ((flags & IDYDB_READONLY) == IDYDB_READONLY)
		(*handler)->read_only = IDYDB_READONLY;

	bool file_exists = true;
	if (access(filename, F_OK) != 0)
	{
		file_exists = false;
		if ((*handler)->read_only == IDYDB_READONLY)
		{
			if ((flags & IDYDB_CREATE) == 0)
			{
				idydb_error_state(handler, 4);
				return IDYDB_NOT_FOUND;
			}
		}
	}

	(*handler)->file_descriptor = fopen(filename,
		(((flags & IDYDB_READONLY) == IDYDB_READONLY)
			? "r"
			: ((((flags & IDYDB_CREATE) == IDYDB_CREATE) && !file_exists) ? "w+" : "r+")));

	if ((*handler)->file_descriptor == NULL)
	{
		idydb_error_state(handler, 5);
		return IDYDB_PERM;
	}
	else
	{
		fseek((*handler)->file_descriptor, 0L, SEEK_END);
		(*handler)->size = ftell((*handler)->file_descriptor);
		fseek((*handler)->file_descriptor, 0L, SEEK_SET);
	}

	(*handler)->configured = true;

	const int lock_mode =
		(((flags & IDYDB_READONLY) == IDYDB_READONLY) ? LOCK_SH : LOCK_EX) | LOCK_NB;
	if (flock(fileno((*handler)->file_descriptor), lock_mode) != 0)
	{
		idydb_error_state(handler, 6);
		return IDYDB_BUSY;
	}

	if ((flags & IDYDB_UNSAFE) == IDYDB_UNSAFE)
#ifdef IDYDB_ALLOW_UNSAFE
		(*handler)->unsafe = true;
#else
	{
		idydb_error_state(handler, 23);
		return IDYDB_ERROR;
	}
#endif
	else if ((*handler)->size > idydb_max_size())
	{
		idydb_error_state(handler, 7);
		return IDYDB_RANGE;
	}

#ifdef IDYDB_MMAP_OK
	if ((*handler)->read_only == IDYDB_READONLY)
	{
		if ((*handler)->size <= IDYDB_MMAP_MAX_SIZE && (*handler)->size > 0)
		{
			(*handler)->buffer = mmap(NULL, (*handler)->size, PROT_READ, MAP_PRIVATE, fileno((*handler)->file_descriptor), 0);
			if ((*handler)->buffer != MAP_FAILED)
				(*handler)->read_only = IDYDB_READONLY_MMAPPED;
			else
				(*handler)->buffer = malloc((sizeof(char) * IDYDB_MAX_BUFFER_SIZE));
		}
		else
			(*handler)->buffer = malloc((sizeof(char) * IDYDB_MAX_BUFFER_SIZE));
	}
	else
#endif
		(*handler)->buffer = malloc((sizeof(char) * IDYDB_MAX_BUFFER_SIZE));

	if ((*handler)->buffer == NULL)
	{
		idydb_error_state(handler, 24);
		return IDYDB_ERROR;
	}

	return IDYDB_SUCCESS;
}

static char *idydb_get_err_message(idydb **handler)
{
	if (!handler || *handler == NULL)
		return (char *)"This handler failed to be setup\0";
	return (*handler)->err_message;
}

static int idydb_open_fail_cleanup(idydb **handler, int rc)
{
	if (handler && *handler)
	{
		idydb_destroy(handler);
		free(*handler);
		*handler = NULL;
	}
	return rc;
}

/* ---------------- Public open/close (runtime encryption) ---------------- */

int idydb_open_with_options(const char *filename, idydb **handler, const idydb_open_options* options)
{
	if (!filename || !handler || !options) return IDYDB_ERROR;
	if (*handler != NULL) return IDYDB_ERROR;

	if (idydb_new(handler) == IDYDB_ERROR)
	{
		*handler = NULL;
		return IDYDB_ERROR;
	}

	int flags = options->flags;

	if (!options->encrypted_at_rest)
	{
		int rc = idydb_connection_setup(handler, filename, flags);
		if (rc != IDYDB_SUCCESS) return idydb_open_fail_cleanup(handler, rc);
		(*handler)->encryption_enabled = false;
		(*handler)->dirty = false;
		idydb_clear_values(handler);
		DB_DEBUGF(handler, "opened PLAINTEXT db file=\"%s\" flags=0x%x", filename, flags);
		return IDYDB_SUCCESS;
	}

	/* encrypted-at-rest path */
	if (!options->passphrase)
	{
		idydb_error_state(handler, 27);
		DB_DEBUGF(handler, "encrypted open refused: passphrase is NULL (file=\"%s\")", filename);
		return idydb_open_fail_cleanup(handler, IDYDB_ERROR);
	}

	bool file_exists = (access(filename, F_OK) == 0);
	if (!file_exists && ((flags & IDYDB_CREATE) == 0))
	{
		idydb_error_state(handler, 4);
		return idydb_open_fail_cleanup(handler, IDYDB_NOT_FOUND);
	}

	/* open + lock backing file */
	const bool ro = ((flags & IDYDB_READONLY) == IDYDB_READONLY);

	FILE* backing = fopen(filename,
		(ro ? "rb" : (file_exists ? "rb+" : "wb+")));

	if (!backing)
	{
		idydb_error_state(handler, 5);
		return idydb_open_fail_cleanup(handler, IDYDB_PERM);
	}

	const int backing_lock_mode = (ro ? LOCK_SH : LOCK_EX) | LOCK_NB;
	if (flock(fileno(backing), backing_lock_mode) != 0)
	{
		fclose(backing);
		idydb_error_state(handler, 6);
		return idydb_open_fail_cleanup(handler, IDYDB_BUSY);
	}

	(*handler)->encryption_enabled = true;
	(*handler)->backing_descriptor = backing;
	(*handler)->backing_filename = (char*)malloc(strlen(filename) + 1);
	if ((*handler)->backing_filename) strcpy((*handler)->backing_filename, filename);
	(*handler)->dirty = false;

	const char* kind = NULL;
	FILE* plain = idydb_secure_plain_stream(&kind);
	if (!plain)
	{
		idydb_error_state(handler, 30);
		DB_DEBUGF(handler, "failed to create secure in-memory plaintext working storage (backing=\"%s\")", filename);
		return idydb_open_fail_cleanup(handler, IDYDB_ERROR);
	}
	(*handler)->plain_storage_kind = kind;

	DB_DEBUGF(handler, "opened ENCRYPTED-AT-REST db backing=\"%s\" ro=%s exists=%s working_plain=%s",
	          filename, (ro ? "yes" : "no"), (file_exists ? "yes" : "no"), (kind ? kind : "unknown"));

	/* detect encrypted header */
	fseek(backing, 0L, SEEK_END);
	long bsz = ftell(backing);
	fseek(backing, 0L, SEEK_SET);

	bool is_enc = false;
	if (bsz >= (long)IDYDB_ENC_MAGIC_LEN)
	{
		unsigned char magic[IDYDB_ENC_MAGIC_LEN];
		size_t rr = fread(magic, 1, IDYDB_ENC_MAGIC_LEN, backing);
		fseek(backing, 0L, SEEK_SET);
		if (rr == IDYDB_ENC_MAGIC_LEN && memcmp(magic, IDYDB_ENC_MAGIC, IDYDB_ENC_MAGIC_LEN) == 0) is_enc = true;
	}

	/* IMPORTANT: encrypted-at-rest should not silently accept a plaintext backing in READONLY mode */
	if (!is_enc && ro && bsz > 0)
	{
		idydb_error_state(handler, 31);
		DB_DEBUGF(handler, "refusing encrypted READONLY open on PLAINTEXT backing; open writable once to migrate");
		fclose(plain);
		return idydb_open_fail_cleanup(handler, IDYDB_ERROR);
	}

	if (is_enc)
	{
		uint32_t iter = 0;
		DB_DEBUGF(handler, "encrypted container detected; decrypting...");
		if (!idydb_crypto_decrypt_locked_file_to_stream(backing, options->passphrase, plain,
		                                                (*handler)->enc_salt, &iter, (*handler)->enc_key))
		{
			idydb_error_state(handler, 28);
			DB_DEBUGF(handler, "decrypt FAILED (wrong passphrase, tampered file, or unsupported params)");
			fclose(plain);
			return idydb_open_fail_cleanup(handler, IDYDB_ERROR);
		}
		(*handler)->enc_iter = iter;
		(*handler)->enc_key_set = true;

		fseek(plain, 0L, SEEK_END);
		long psz = ftell(plain);
		fseek(plain, 0L, SEEK_SET);
		(void)psz;
		DB_DEBUGF(handler, "decrypt OK -> plaintext bytes=%ld pbkdf2_iter=%u", psz, iter);
	}
	else
	{
		/* plaintext backing (migration) */
		if (bsz > 0)
		{
			DB_DEBUGF(handler, "PLAINTEXT backing detected; copying into working plaintext stream (migration)");
			unsigned char buf[16 * 1024];
			while (1) {
				size_t n = fread(buf, 1, sizeof(buf), backing);
				if (n == 0) break;
				if (fwrite(buf, 1, n, plain) != n) {
					idydb_error_state(handler, 26);
					fclose(plain);
					return idydb_open_fail_cleanup(handler, IDYDB_ERROR);
				}
			}
			fflush(plain);
			fseek(plain, 0L, SEEK_SET);
			fseek(backing, 0L, SEEK_SET);
		}

		/* generate new key/salt for migration or new encrypted creation */
		uint32_t iter = (options->pbkdf2_iter == 0 ? IDYDB_ENC_DEFAULT_PBKDF2_ITER : (uint32_t)options->pbkdf2_iter);
		if (!idydb_crypto_iter_ok(iter)) {
			idydb_error_state(handler, 26);
			fclose(plain);
			return idydb_open_fail_cleanup(handler, IDYDB_ERROR);
		}

		(*handler)->enc_iter = iter;

		if (RAND_bytes((*handler)->enc_salt, IDYDB_ENC_SALT_LEN) != 1)
		{
			idydb_error_state(handler, 26);
			fclose(plain);
			return idydb_open_fail_cleanup(handler, IDYDB_ERROR);
		}
		if (!idydb_crypto_derive_key_pbkdf2(options->passphrase, (*handler)->enc_salt, (*handler)->enc_iter, (*handler)->enc_key))
		{
			idydb_error_state(handler, 26);
			fclose(plain);
			return idydb_open_fail_cleanup(handler, IDYDB_ERROR);
		}
		(*handler)->enc_key_set = true;

		/* If readonly, do not writeback. If writeable, mark dirty to force encryption on close. */
		if (!ro) (*handler)->dirty = true;

		DB_DEBUGF(handler, "migration/new-encrypted setup: pbkdf2_iter=%u dirty=%s", (*handler)->enc_iter, ((*handler)->dirty ? "yes" : "no"));
	}

	/* Setup db handler to operate on plaintext stream */
	int setup_rc = idydb_connection_setup_stream(handler, plain, flags);
	if (setup_rc != IDYDB_SUCCESS)
		return idydb_open_fail_cleanup(handler, setup_rc);

	idydb_clear_values(handler);
	DB_DEBUGF(handler, "ready: db opened against secure working plaintext stream kind=%s", (kind ? kind : "unknown"));
	return IDYDB_SUCCESS;
}

int idydb_open(const char *filename, idydb **handler, int flags)
{
	idydb_open_options opt;
	opt.flags = flags;
	opt.encrypted_at_rest = false;
	opt.passphrase = NULL;
	opt.pbkdf2_iter = 0;
	return idydb_open_with_options(filename, handler, &opt);
}

int idydb_open_encrypted(const char *filename, idydb **handler, int flags, const char* passphrase)
{
	idydb_open_options opt;
	opt.flags = flags;
	opt.encrypted_at_rest = true;
	opt.passphrase = passphrase;
	opt.pbkdf2_iter = 0;
	return idydb_open_with_options(filename, handler, &opt);
}

int idydb_close(idydb **handler)
{
	if (!handler || !*handler) return IDYDB_DONE;

	/* If encrypted-at-rest and dirty and writable, writeback ciphertext */
	if ((*handler)->encryption_enabled &&
	    (*handler)->enc_key_set &&
	    (*handler)->backing_descriptor != NULL &&
	    (*handler)->read_only == IDYDB_READ_AND_WRITE &&
	    (*handler)->dirty)
	{
		DB_DEBUGF(handler, "close: encrypting writeback -> backing=\"%s\" pbkdf2_iter=%u",
		          ((*handler)->backing_filename ? (*handler)->backing_filename : "(unknown)"),
		          (*handler)->enc_iter);

		if (!idydb_crypto_encrypt_stream_to_locked_file((*handler)->file_descriptor,
		                                                (*handler)->backing_descriptor,
		                                                (*handler)->enc_salt,
		                                                (*handler)->enc_iter,
		                                                (*handler)->enc_key))
		{
			idydb_error_state(handler, 29);
			DB_DEBUGF(handler, "close: writeback FAILED (backing not updated safely)");
			idydb_destroy(handler);
			free(*handler);
			*handler = NULL;
			return IDYDB_ERROR;
		}
		DB_DEBUGF(handler, "close: writeback OK");
	}
	else
	{
		DB_DEBUGF(handler, "close: no writeback (enc=%s dirty=%s mode=%s)",
		          ((*handler)->encryption_enabled ? "yes" : "no"),
		          ((*handler)->dirty ? "yes" : "no"),
		          idydb_ro_str((*handler)->read_only));
	}

	idydb_destroy(handler);
	free(*handler);
	*handler = NULL;
	return IDYDB_DONE;
}

/* ---------------- Public API thin wrappers ---------------- */

char *idydb_errmsg(idydb **handler) { return idydb_get_err_message(handler); }
int idydb_extract(idydb **handler, idydb_column_row_sizing c, idydb_column_row_sizing r) { return idydb_read_at(handler, c, r); }
int idydb_retrieved_type(idydb **handler) { return idydb_retrieve_value_type(handler); }

/* ---------------- insert value staging ---------------- */

