#pragma once

#include <vector>
#include <string>
#include <memory>
#include <algorithm>
#include <cwchar>
#include <cstdio>

/* -------------------- ANSI SGR parsing/rendering --------------------
   Supports a practical subset used by dlogs.h:
     - Reset: 0
     - Bold: 1, Normal intensity: 22, Dim: 2 (approximated)
     - Inverse: 7, Inverse off: 27
     - FG: 30-37, 90-97
     - BG: 40-47, 100-107
     - Truecolor FG: 38;2;r;g;b
     - Truecolor BG: 48;2;r;g;b
     - 256-color FG/BG: 38;5;n / 48;5;n (best-effort mapping)

   NOTE: Non-SGR CSI sequences (like ESC[2K) are ignored (stripped).
*/
namespace cuwacunu {
namespace iinuji {
namespace ansi {

inline bool has_esc(const std::string& s) {
  return s.find('\x1b') != std::string::npos;
}

struct style_t {
  std::string fg;
  std::string bg;
  bool bold{false};
  bool inverse{false};
  bool dim{false};
};

struct seg_t {
  std::string text;   // visible chars only (no escapes)
  short pair{0};
  bool bold{false};
  bool inverse{false};
};

struct row_t {
  std::vector<seg_t> segs;
  int len{0}; // visible columns (ASCII assumption)
};

inline std::string rgb_to_hex(int r, int g, int b) {
  return rgb8_to_hex(r, g, b); // from iinuji_utils.h
}

inline std::string ansi_basic_token(int idx, bool bright, bool dim) {
  // xterm-ish palette (close enough for UI logs)
  static const int base[8][3] = {
    {  0,  0,  0}, {205, 49, 49}, { 13,188,121}, {229,229, 16},
    { 36,114,200}, {188, 63,188}, { 17,168,205}, {229,229,229}
  };
  static const int brt[8][3] = {
    {102,102,102}, {241, 76, 76}, { 35,209,139}, {245,245, 67},
    { 59,142,234}, {214,112,214}, { 41,184,219}, {255,255,255}
  };
  idx = std::clamp(idx, 0, 7);
  int r = bright ? brt[idx][0] : base[idx][0];
  int g = bright ? brt[idx][1] : base[idx][1];
  int b = bright ? brt[idx][2] : base[idx][2];
  if (dim) {
    constexpr double k = 0.65;
    r = (int)std::lround(r * k);
    g = (int)std::lround(g * k);
    b = (int)std::lround(b * k);
  }
  return rgb_to_hex(r, g, b);
}

inline std::string xterm256_token(int n) {
  n = std::clamp(n, 0, 255);
  // 0..15: standard + bright
  if (n < 16) {
    bool bright = (n >= 8);
    int idx = bright ? (n - 8) : n;
    return ansi_basic_token(idx, bright, false);
  }
  // 16..231: 6x6x6 cube
  if (n >= 16 && n <= 231) {
    int v = n - 16;
    int ir = v / 36;
    int ig = (v / 6) % 6;
    int ib = v % 6;
    auto level = [](int i)->int {
      static const int L[6] = {0, 95, 135, 175, 215, 255};
      return L[std::clamp(i, 0, 5)];
    };
    return rgb_to_hex(level(ir), level(ig), level(ib));
  }
  // 232..255: grayscale ramp
  int k = n - 232; // 0..23
  int g = 8 + k * 10;
  g = std::clamp(g, 0, 255);
  return rgb_to_hex(g, g, g);
}

inline bool parse_csi(const std::string& s, std::size_t i,
                      std::size_t& next_i,
                      std::vector<int>& params,
                      char& final_byte)
{
  // Expect ESC '[' at s[i], s[i+1]
  params.clear();
  final_byte = 0;
  next_i = i;

  if (i + 1 >= s.size()) return false;
  if ((unsigned char)s[i] != 0x1b || s[i+1] != '[') return false;

  std::size_t j = i + 2;
  int cur = -1;

  auto push_cur = [&](){
    if (cur >= 0) params.push_back(cur);
    cur = -1;
  };

  while (j < s.size()) {
    unsigned char ch = (unsigned char)s[j];

    // digits build parameter
    if (ch >= '0' && ch <= '9') {
      if (cur < 0) cur = 0;
      cur = cur * 10 + (int)(ch - '0');
      ++j;
      continue;
    }

    // separator
    if (ch == ';') {
      params.push_back(cur < 0 ? 0 : cur);
      cur = -1;
      ++j;
      continue;
    }

    // ignore some CSI parameter bytes we don't care about
    if (ch == '?' || ch == ':' || ch == ' ') {
      ++j;
      continue;
    }

    // final byte (CSI final is 0x40..0x7E)
    if (ch >= 0x40 && ch <= 0x7E) {
      push_cur();
      final_byte = (char)ch;
      next_i = j + 1;
      return true;
    }

    // unknown byte: stop consuming to avoid runaway
    break;
  }

  // incomplete CSI: consume nothing
  return false;
}

inline void apply_sgr(const std::vector<int>& params, style_t& st, const style_t& base)
{
  // ESC[m is equivalent to ESC[0m
  if (params.empty()) {
    st = base;
    return;
  }

  std::size_t i = 0;
  while (i < params.size()) {
    const int p = params[i];

    if (p == 0) { st = base; ++i; continue; }
    if (p == 1) { st.bold = true; st.dim = false; ++i; continue; }
    if (p == 2) { st.dim  = true; st.bold = false; ++i; continue; }
    if (p == 22){ st.bold = false; st.dim = false; ++i; continue; }
    if (p == 7) { st.inverse = true; ++i; continue; }
    if (p == 27){ st.inverse = false; ++i; continue; }
    if (p == 39){ st.fg = base.fg; ++i; continue; }
    if (p == 49){ st.bg = base.bg; ++i; continue; }

    // basic fg
    if (p >= 30 && p <= 37) {
      st.fg = ansi_basic_token(p - 30, /*bright*/false, st.dim);
      ++i; continue;
    }
    if (p >= 90 && p <= 97) {
      st.fg = ansi_basic_token(p - 90, /*bright*/true, st.dim);
      ++i; continue;
    }

    // basic bg
    if (p >= 40 && p <= 47) {
      st.bg = ansi_basic_token(p - 40, /*bright*/false, st.dim);
      ++i; continue;
    }
    if (p >= 100 && p <= 107) {
      st.bg = ansi_basic_token(p - 100, /*bright*/true, st.dim);
      ++i; continue;
    }

    // truecolor / 256-color
    if (p == 38 || p == 48) {
      const bool is_fg = (p == 38);
      if (i + 1 < params.size()) {
        const int mode = params[i+1];
        if (mode == 2 && i + 4 < params.size()) {
          int r = std::clamp(params[i+2], 0, 255);
          int g = std::clamp(params[i+3], 0, 255);
          int b = std::clamp(params[i+4], 0, 255);
          if (is_fg) st.fg = rgb_to_hex(r,g,b);
          else       st.bg = rgb_to_hex(r,g,b);
          i += 5;
          continue;
        }
        if (mode == 5 && i + 2 < params.size()) {
          int n = params[i+2];
          if (is_fg) st.fg = xterm256_token(n);
          else       st.bg = xterm256_token(n);
          i += 3;
          continue;
        }
      }
    }

    // ignore unknown
    ++i;
  }
}

inline void push_run(row_t& row, std::string& run, const style_t& st, short fallback_pair)
{
  if (run.empty()) return;
  short pair = (short)get_color_pair(st.fg, st.bg);
  if (pair == 0) pair = fallback_pair;

  seg_t seg;
  seg.text = std::move(run);
  seg.pair = pair;
  seg.bold = st.bold;
  seg.inverse = st.inverse;

  if (!row.segs.empty()) {
    auto& last = row.segs.back();
    if (last.pair == seg.pair && last.bold == seg.bold && last.inverse == seg.inverse) {
      last.text += seg.text;
      return;
    }
  }
  row.segs.push_back(std::move(seg));
}

inline void hard_wrap(const std::string& s, int width,
                      const style_t& base,
                      short fallback_pair,
                      std::vector<row_t>& out)
{
  out.clear();
  if (width <= 0) { out.push_back(row_t{}); return; }
  if (s.empty()) { out.push_back(row_t{}); return; }

  style_t st = base;
  style_t run_style = st;
  std::string run;

  row_t row;
  int col = 0;

  auto flush = [&](){
    push_run(row, run, run_style, fallback_pair);
    run.clear();
  };
  auto new_row = [&](){
    out.push_back(std::move(row));
    row = row_t{};
    col = 0;
  };

  std::size_t i = 0;
  while (i < s.size()) {
    unsigned char c = (unsigned char)s[i];

    // CSI?
    if (c == 0x1b && i + 1 < s.size() && s[i+1] == '[') {
      flush();
      std::vector<int> params;
      char final = 0;
      std::size_t nxt = i;
      if (parse_csi(s, i, nxt, params, final)) {
        if (final == 'm') apply_sgr(params, st, base);
        // ignore other CSI sequences like 'K'
        i = nxt;
        continue;
      }
      // if parse failed, drop ESC char
      ++i;
      continue;
    }

    // ignore control chars
    if (c == '\r' || c < 32) { ++i; continue; }

    // style boundary?
    if (run.empty()) run_style = st;
    else if (run_style.fg != st.fg || run_style.bg != st.bg ||
             run_style.bold != st.bold || run_style.inverse != st.inverse) {
      flush();
      run_style = st;
    }

    // append visible char
    run.push_back((char)c);
    ++col;
    ++row.len;
    ++i;

    if (col >= width) {
      flush();
      new_row();
    }
  }

  flush();
  if (!row.segs.empty() || row.len > 0 || out.empty()) out.push_back(std::move(row));
  if (out.size() >= 2 && out.back().segs.empty() && out.back().len == 0) out.pop_back();
}

inline void render_row(int y, int x, int width,
                       const row_t& row,
                       short fallback_pair,
                       bool base_bold,
                       bool base_inverse)
{
  auto* R = get_renderer();
  if (!R || width <= 0) return;

  int col = 0;
  for (const auto& seg : row.segs) {
    if (col >= width) break;
    int rem = width - col;
    if (rem <= 0) break;

    int n = std::min(rem, (int)seg.text.size());
    if (n <= 0) continue;

    short pair = seg.pair ? seg.pair : fallback_pair;
    R->putText(y, x + col, seg.text, rem, pair, seg.bold || base_bold, seg.inverse || base_inverse);
    col += n;
  }
}

inline void append_plain(row_t& row, const std::string& s,
                         short pair, bool bold=false, bool inverse=false)
{
  if (s.empty()) return;
  seg_t seg;
  seg.text = s;
  seg.pair = pair;
  seg.bold = bold;
  seg.inverse = inverse;

  if (!row.segs.empty()) {
    auto& last = row.segs.back();
    if (last.pair == seg.pair && last.bold == seg.bold && last.inverse == seg.inverse) {
      last.text += seg.text;
      return;
    }
  }
  row.segs.push_back(std::move(seg));
  row.len += (int)s.size();
}

} // namespace ansi
} // namespace iinuji
} // namespace cuwacunu
