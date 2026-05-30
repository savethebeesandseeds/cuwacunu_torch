#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "iinuji/iinuji_render.h"
#include "iinuji/ncurses/iinuji_app_ncurses.h"
#include "iinuji/primitives/image.h"
#include "iinuji/primitives/text_box.h"

namespace {

constexpr const char *kDefaultImagePath =
    "/cuwacunu/src/resources/waajacamaya.png";
constexpr const char *kBackgroundColor = "#101014";

struct cli_opts_t {
  std::string image_path{kDefaultImagePath};
  bool smoke{false};
};

class CaptureRenderer final : public cuwacunu::iinuji::IRend {
public:
  int glyph_calls{0};
  int nonblank_glyphs{0};

  void size(int &h, int &w) override {
    h = 24;
    w = 80;
  }

  void clear() override {}
  void flush() override {}
  void putText(int, int, const std::string &, int, short, bool, bool) override {
  }
  void putGlyph(int, int, wchar_t ch, short) override {
    ++glyph_calls;
    if (ch != L' ')
      ++nonblank_glyphs;
  }
  void fillRect(int, int, int, int, short) override {}
};

bool load_image(const std::string &path,
                cuwacunu::iinuji::rgba_image_t &image) {
  std::string error{};
  if (!cuwacunu::iinuji::decode_png_rgba_file(path, image, error)) {
    std::cerr << "[test_iinuji_image_box_render] " << path << ": " << error
              << '\n';
    return false;
  }
  return true;
}

cuwacunu::iinuji::image_grayscale_options_t
make_render_options(cuwacunu::iinuji::image_grayscale_mode_t mode,
                    bool use_color, bool preserve_aspect, bool sample_nearest,
                    int color_levels) {
  cuwacunu::iinuji::image_grayscale_options_t opts{};
  opts.mode = mode;
  opts.use_color = use_color;
  opts.preserve_aspect = preserve_aspect;
  opts.center = true;
  opts.sample_nearest = sample_nearest;
  opts.color_levels = color_levels;
  opts.background_color = kBackgroundColor;
  return opts;
}

std::shared_ptr<cuwacunu::iinuji::iinuji_object_t>
make_image_box_tree(const cuwacunu::iinuji::rgba_image_t &image,
                    const cuwacunu::iinuji::image_grayscale_options_t &opts) {
  cuwacunu::iinuji::image_box_opts_t image_opts{};
  image_opts.image = image;
  image_opts.render_options = opts;
  image_opts.style = cuwacunu::iinuji::iinuji_style_t{"white", kBackgroundColor,
                                                      false, "gray"};
  return cuwacunu::iinuji::create_image_box("image.box.render",
                                            std::move(image_opts));
}

std::shared_ptr<cuwacunu::iinuji::iinuji_object_t> make_interactive_tree(
    const cuwacunu::iinuji::rgba_image_t &image,
    const cuwacunu::iinuji::image_grayscale_options_t &opts,
    std::shared_ptr<cuwacunu::iinuji::text_box_data_t> &status_data,
    std::shared_ptr<cuwacunu::iinuji::image_box_data_t> &image_data) {
  using namespace cuwacunu::iinuji;

  auto root = create_grid_container(
      "image.render.root", {len_spec::px(3), len_spec::frac(1.0)},
      {len_spec::frac(1.0)}, 0, 0, iinuji_layout_t{},
      iinuji_style_t{"white", kBackgroundColor});

  text_box_opts_t status_opts{};
  status_opts.wrap = false;
  status_opts.style = iinuji_style_t{"#d8d8d8", "#181820"};
  auto status = create_text_box("image.render.status", std::move(status_opts));
  place_in_grid(status, 0, 0);
  root->add_child(status);

  image_box_opts_t image_opts{};
  image_opts.image = image;
  image_opts.render_options = opts;
  image_opts.style = iinuji_style_t{"white", kBackgroundColor};
  auto image_box =
      create_image_box("image.render.primitive", std::move(image_opts));
  place_in_grid(image_box, 1, 0);
  root->add_child(image_box);

  status_data = std::dynamic_pointer_cast<text_box_data_t>(status->data);
  image_data = std::dynamic_pointer_cast<image_box_data_t>(image_box->data);
  return root;
}

std::string mode_label(cuwacunu::iinuji::image_grayscale_mode_t mode) {
  return mode == cuwacunu::iinuji::image_grayscale_mode_t::Braille ? "braille"
                                                                   : "half";
}

std::string yn(bool value) { return value ? "on" : "off"; }

std::string status_text(const std::string &path,
                        const cuwacunu::iinuji::rgba_image_t &image,
                        cuwacunu::iinuji::image_grayscale_mode_t mode,
                        bool use_color, bool preserve_aspect,
                        bool sample_nearest, int color_levels) {
  return "image_box primitive render | b braille | h half | c color | a "
         "aspect | n nearest | +/- levels | q quit\n" +
         path + "\n" + std::to_string(image.width) + "x" +
         std::to_string(image.height) + " rgba | mode=" + mode_label(mode) +
         " | color=" + yn(use_color) + " | aspect=" + yn(preserve_aspect) +
         " | nearest=" + yn(sample_nearest) +
         " | levels=" + std::to_string(color_levels);
}

cli_opts_t parse_cli(int argc, char **argv) {
  cli_opts_t opts{};
  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    if (arg == "--smoke") {
      opts.smoke = true;
    } else if (arg == "--help" || arg == "-h") {
      std::cout << "usage: test_iinuji_image_box_render [--smoke] "
                   "[image.png]\n";
      std::exit(0);
    } else {
      opts.image_path = arg;
    }
  }
  return opts;
}

int run_smoke(const std::string &path,
              const cuwacunu::iinuji::rgba_image_t &image) {
  using namespace cuwacunu::iinuji;

  auto image_box = make_image_box_tree(
      image, make_render_options(image_grayscale_mode_t::Braille, false, true,
                                 false, 4));

  CaptureRenderer renderer{};
  IRend *old_renderer = set_renderer(&renderer);
  layout_tree(image_box, Rect{0, 0, 48, 16});
  render_tree(image_box);
  set_renderer(old_renderer);

  if (image_box->primitive_role != primitive_role_t::ImageBox) {
    std::cerr << "[test_iinuji_image_box_render] primitive role mismatch\n";
    return 1;
  }
  if (renderer.glyph_calls == 0 || renderer.nonblank_glyphs == 0) {
    std::cerr << "[test_iinuji_image_box_render] image render emitted no "
                 "visible glyphs\n";
    return 1;
  }

  std::cout << "smoke ok: rendered " << path << " (" << image.width << "x"
            << image.height << ") into " << renderer.glyph_calls
            << " terminal cells\n";
  return 0;
}

int run_interactive(const std::string &path,
                    const cuwacunu::iinuji::rgba_image_t &image) {
  using namespace cuwacunu::iinuji;

  image_grayscale_mode_t mode = image_grayscale_mode_t::Braille;
  bool use_color = true;
  bool preserve_aspect = true;
  bool sample_nearest = false;
  int color_levels = 4;

  NcursesAppOpts app_opts{};
  app_opts.input_timeout_ms = -1;
  NcursesApp app{app_opts};
  set_global_background(kBackgroundColor);

  std::shared_ptr<text_box_data_t> status_data{};
  std::shared_ptr<image_box_data_t> image_data{};
  auto root = make_interactive_tree(
      image,
      make_render_options(mode, use_color, preserve_aspect, sample_nearest,
                          color_levels),
      status_data, image_data);

  bool running = true;
  while (running) {
    int height = 0;
    int width = 0;
    app.renderer().size(height, width);

    if (status_data) {
      status_data->content =
          status_text(path, image, mode, use_color, preserve_aspect,
                      sample_nearest, color_levels);
    }
    if (image_data) {
      image_data->render_options = make_render_options(
          mode, use_color, preserve_aspect, sample_nearest, color_levels);
    }

    app.renderer().clear();
    layout_tree(root, Rect{0, 0, std::max(1, width), std::max(1, height)});
    render_tree(root);
    app.renderer().flush();

    const int ch = app.read_raw_key();
    switch (ch) {
    case 'q':
    case 'Q':
    case 27:
      running = false;
      break;
    case 'b':
    case 'B':
      mode = image_grayscale_mode_t::Braille;
      break;
    case 'h':
    case 'H':
      mode = image_grayscale_mode_t::HalfBlocks;
      break;
    case 'c':
    case 'C':
      use_color = !use_color;
      break;
    case 'a':
    case 'A':
      preserve_aspect = !preserve_aspect;
      break;
    case 'n':
    case 'N':
      sample_nearest = !sample_nearest;
      break;
    case '+':
    case '=':
      color_levels = std::min(256, color_levels * 2);
      break;
    case '-':
    case '_':
      color_levels = std::max(2, color_levels / 2);
      break;
    default:
      break;
    }
  }

  return 0;
}

} // namespace

int main(int argc, char **argv) {
  const cli_opts_t opts = parse_cli(argc, argv);

  cuwacunu::iinuji::rgba_image_t image{};
  if (!load_image(opts.image_path, image))
    return 1;

  if (opts.smoke)
    return run_smoke(opts.image_path, image);

  return run_interactive(opts.image_path, image);
}
