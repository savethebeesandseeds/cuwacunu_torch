/* iinuji_types.h */
#include <ncursesw/ncurses.h>
#include <locale.h>
#include <vector>
#include <cmath>
#include <string>
#include <memory>
#include <algorithm>
#include <unordered_map>


namespace cuwacunu {
namespace camahjucunu {
namespace GUI {

// Forward declarations
struct iinuji_layout_t;
struct iinuji_style_t;
struct iinuji_binding_t;
struct iinuji_object_t;
struct iinuji_state_t;

// Object types
struct iinuji_data_t {
    virtual ~iinuji_data_t() = default; // Ensure polymorphism
};

struct textBox_data_t : public iinuji_data_t {
    std::string content;
    textBox_data_t(std::string content) : content(std::move(content)) {}
};

struct plotBox_data_t : public iinuji_data_t {
    std::vector<std::pair<double, double>> points;
    int density;
    plotBox_data_t(std::vector<std::pair<double, double>> points, int density)
        : points(std::move(points)), density(density) {}
};

// Action Binding
struct iinuji_binding_t {
    std::string key;                              // Key or action identifier
    void (*handler)(std::shared_ptr<void>);       // Event handler function
    std::shared_ptr<iinuji_object_t> parent;      // Associated parent object
};

// Layout Structure
struct iinuji_layout_t {
    double x;                  // X position on the screen
    double y;                  // Y position on the screen
    double width;              // Width of the object
    double height;             // Height of the object
    bool normalized;
};

// Style Structure
struct iinuji_style_t {
    std::string label_color;          // Label color (e.g., hex or color name)
    std::string background_color;     // Background color
    bool border = false;              // If true, draw a border around the object
    std::string border_color = "";    // Color for the border (optional)
};

// GUI Object
struct iinuji_object_t {
    long id;                                      // Unique identifier for the object
    bool visible;                                 // Visibility flag
    iinuji_layout_t layout;                       // Layout properties
    iinuji_style_t style;                         // Style properties
    std::shared_ptr<iinuji_data_t> data;          // Shared pointer to user-defined data
    std::vector<std::shared_ptr<iinuji_object_t>> children; // Hierarchical children
    std::unordered_map<std::string, iinuji_binding_t> bindings; // Key-action bindings

    // Modifiers
    void add_children   (const std::vector<std::shared_ptr<iinuji_object_t>>& new_children)     { children.insert(children.end(), new_children.begin(), new_children.end()); }
    void add_child      (const std::shared_ptr<iinuji_object_t>& child)                         { children.push_back(child); }
    void add_binding    (const std::string& key, const iinuji_binding_t& binding)               { bindings[key] = binding; }
    void add_bindings   (const std::unordered_map<std::string, iinuji_binding_t>& new_bindings) { bindings.insert(new_bindings.begin(), new_bindings.end()); }
    void remove_binding (const std::string& key)                                                { bindings.erase(key); }
    void show           () { visible = true; }
    void hide           () { visible = false; }
    void toggle_visible () { visible = !visible; }
};

// Application State
struct iinuji_state_t {
    std::shared_ptr<iinuji_object_t> root;       // Root GUI object
    std::shared_ptr<iinuji_object_t> focused;    // Currently focused object
    bool running;                                // Application state flag
    bool in_ncurses_mode = true;                 // Tracks current mode (true: ncurses, false: terminal)
};


// Fabrics
std::shared_ptr<iinuji_state_t> initialize_iinuji_state(
    const std::shared_ptr<iinuji_object_t>& root,
    bool in_ncurses_mode = true
) {
    auto state = std::make_shared<iinuji_state_t>();
    state->root = root;
    state->focused = nullptr; // Initially, no object is focused
    state->running = true;
    state->in_ncurses_mode = in_ncurses_mode;
    return state;
}

std::shared_ptr<iinuji_object_t> create_iinuji_object(
    bool visible = true,
    const iinuji_layout_t& layout = {0, 0, 0, 0, false},
    const iinuji_style_t& style = {"white", "black", false, ""}
) {
    static int object_count = 0; // Initialize the static counter
    auto obj = std::make_shared<iinuji_object_t>();
    obj->id = object_count++;
    obj->visible = visible;
    obj->layout = layout;
    obj->style = style;
    obj->data = nullptr;
    obj->children = {};
    obj->bindings = {};
    return obj;
}

std::shared_ptr<iinuji_object_t> create_iinuji_textBox(
    std::string content,
    const iinuji_layout_t& layout = {0, 0, 0, 0, false},
    const iinuji_style_t& style = {"white", "black", false, ""}
) {
    // Create object
    auto new_textBox = cuwacunu::camahjucunu::GUI::create_iinuji_object(true, layout, style);

    // Assign content to the data field
    new_textBox->data = std::make_shared<textBox_data_t>(textBox_data_t{std::move(content)});

    // Return
    return new_textBox;
}

std::shared_ptr<iinuji_object_t> create_iinuji_plotBox(
    std::vector<std::pair<double, double>> points, 
    int density = 10,
    const iinuji_layout_t& layout = {0, 0, 0, 0, false},
    const iinuji_style_t& style = {"white", "black", false, ""}
) {
    // Create object
    auto new_plotBox = cuwacunu::camahjucunu::GUI::create_iinuji_object(true, layout, style);

    // Assign plotBox_data_t to the data field
    new_plotBox->data = std::make_shared<plotBox_data_t>(
        plotBox_data_t{std::move(points), density}
    );

    // Return 
    return new_plotBox;
}


std::shared_ptr<iinuji_object_t> create_iinuji_panelBox(
    bool visible = false,
    const iinuji_layout_t& layout = {0, 0, 0, 0, false},
    const iinuji_style_t& style = {"white", "black", false, ""}
) {
    // Create object
    auto new_panelBox = cuwacunu::camahjucunu::GUI::create_iinuji_object(visible, layout, style);

    // Return 
    return new_panelBox;
}


} /* namespace GUI */
} /* namespace camahjucunu */
} /* namespace cuwacunu */