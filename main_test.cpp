#include <Arduino.h>
#include <gfx.hpp>
#include <uix.hpp>
// downloaded from fontsquirrel.com and header generated with
// https://honeythecodewitch.com/gfx/generator
#define ARCHITECTS_DAUGHTER_IMPLEMENTATION
#include <assets/architects_daughter.hpp>
static const gfx::open_font& text_font = architects_daughter;
using namespace gfx;
using namespace uix;
// creates a BGRx pixel by making each channel
// one quarter of the whole. Any remainder bits
// are added to the green channel. One channel
// is unused. Consumed by DirectX
template <size_t BitDepth>
using bgrx_pixel = gfx::pixel<
    gfx::channel_traits<gfx::channel_name::B, (BitDepth / 4)>,
    gfx::channel_traits<gfx::channel_name::G, ((BitDepth / 4) + (BitDepth % 4))>,
    gfx::channel_traits<gfx::channel_name::R, (BitDepth / 4)>,
    gfx::channel_traits<gfx::channel_name::nop, (BitDepth / 4), 0, (1 << (BitDepth / 4)) - 1, (1 << (BitDepth / 4)) - 1> >;
using screen_t = screen<bgrx_pixel<32>>;
using color_t = color<typename screen_t::pixel_type>;
// for access to RGB8888 colors which controls use
using color32_t = color<rgba_pixel<32>>;

using button_t = uix::svg_button<screen_t::control_surface_type>;

// UIX allows you to use two buffers for maximum DMA efficiency
// but it's not used in Winduino, which really doesn't need it.
// So we declare one 64KB buffer for the transfer
constexpr static const int lcd_screen_size = bitmap<typename screen_t::pixel_type>::sizeof_buffer({320, 240});
constexpr static const int lcd_buffer_size = lcd_screen_size > 64 * 1024 ? 64 * 1024 : lcd_screen_size;
static uint8_t lcd_buffer1[lcd_buffer_size];

// the main screen
screen_t main_screen({320, 240}, sizeof(lcd_buffer1), lcd_buffer1, nullptr);
button_t test_button(main_screen);

void uix_on_touch(point16* out_locations,
                  size_t* in_out_locations_size,
                  void* state) {
    if (!*in_out_locations_size) {
        return;
    }
    int x, y;
    if (read_mouse(&x, &y)) {
        if (x < 0) {
            x = 0;
        }
        if (x >= 320) {
            x = 319;
        }
        if (y < 0) {
            y = 0;
        }
        if (y >= 240) {
            y = 239;
        }
        *in_out_locations_size = 1;
        *out_locations = point16((unsigned)x, (unsigned)y);
        return;
    }
    *in_out_locations_size = 0;
}
void uix_on_flush(const rect16& bounds, const void* bmp, void* state) {
    flush_bitmap(bounds.x1, bounds.y1, bounds.x2 - bounds.x1 + 1, bounds.y2 - bounds.y1 + 1, bmp);
    main_screen.flush_complete();
}
// initialize the screens and controls
static void screen_init() {
    const rgba_pixel<32> transparent(0, 0, 0, 0);
    main_screen.background_color(color_t::white);

    test_button.background_color(color32_t::gray,true);
    test_button.border_color(color32_t::black,true);
    test_button.text_color(color32_t::black);
    test_button.text_open_font(&text_font);
    test_button.text_line_height(25);
    test_button.text("Hello!");
    test_button.radiuses({0,0});
    test_button.bounds(srect16(0,0,99,99).center_horizontal(main_screen.bounds()).offset(100,0));
    main_screen.register_control(test_button);

    main_screen.on_flush_callback(uix_on_flush);
    main_screen.on_touch_callback(uix_on_touch);
}

void setup() {
    Serial.begin(115200);
    screen_init();
}
void loop() {
    main_screen.update();
}