// the code in this file is from my UIX benchmark demo, which is MIT licensed - honey the codewitch
#include <Arduino.h>
#include <SD.h>
#include <SPI.h>
#include <gfx.hpp>
#include <uix.hpp>
#include "spi_screen.h"
#include <tft_io.hpp>
#include <st7789.hpp>
#include <ft6236.hpp>
using namespace gfx;
using namespace uix;
using namespace arduino;

// using the SPI bus 1 (default is 0), CS of 5
using bus_t = tft_spi<1,5>;
// use the ST7789 driver with the above bus
using lcd_t = st7789<135,240,2,4,-1,bus_t,1>;
// declare the touch panel
using touch_t = ft6236<240,135>;
// creates a BGRx pixel by making each channel 
// one quarter of the whole. Any remainder bits
// are added to the green channel. One channel
// is unused. Consumed by DirectX
template<size_t BitDepth>
using bgrx_pixel = gfx::pixel<
	gfx::channel_traits<gfx::channel_name::B,(BitDepth/4)>,
    gfx::channel_traits<gfx::channel_name::G,((BitDepth/4)+(BitDepth%4))>,
    gfx::channel_traits<gfx::channel_name::R,(BitDepth/4)>,
	gfx::channel_traits<gfx::channel_name::nop,(BitDepth/4),0,(1<<(BitDepth/4))-1,(1<<(BitDepth/4))-1>    
>;
using screen_t = screen<bgrx_pixel<32>>;
using label_t = label<typename screen_t::control_surface_type>;
using color_t = color<typename screen_t::pixel_type>;
using color_ctl_t = color<rgba_pixel<32>>;
using frame_buffer_t = bitmap<typename screen_t::pixel_type,typename screen_t::palette_type>;
static bgrx_pixel<32> fire_cols[] = {
        bgrx_pixel<32>(0,0,0,255), bgrx_pixel<32>(24,0,0,255), bgrx_pixel<32>(24,0,0,255), bgrx_pixel<32>(28,0,0,255),
        bgrx_pixel<32>(32,0,0,255), bgrx_pixel<32>(32,0,0,255), bgrx_pixel<32>(36,0,0,255), bgrx_pixel<32>(40,0,0,255),
        bgrx_pixel<32>(40,0,8,255), bgrx_pixel<32>(36,0,16,255), bgrx_pixel<32>(36,0,24,255), bgrx_pixel<32>(32,0,32,255),
        bgrx_pixel<32>(28,0,40,255), bgrx_pixel<32>(28,0,48,255), bgrx_pixel<32>(24,0,56,255), bgrx_pixel<32>(20,0,64,255),
        bgrx_pixel<32>(20,0,72,255), bgrx_pixel<32>(16,0,80,255), bgrx_pixel<32>(16,0,88,255), bgrx_pixel<32>(12,0,96,255),
        bgrx_pixel<32>(8,0,104,255), bgrx_pixel<32>(8,0,112,255), bgrx_pixel<32>(4,0,120,255), bgrx_pixel<32>(0,0,128,255),
        bgrx_pixel<32>(0,0,128,255), bgrx_pixel<32>(0,0,132,255), bgrx_pixel<32>(0,0,136,255), bgrx_pixel<32>(0,0,140,255),
        bgrx_pixel<32>(0,0,144,255), bgrx_pixel<32>(0,0,144,255), bgrx_pixel<32>(0,0,148,255), bgrx_pixel<32>(0,0,152,255),
        bgrx_pixel<32>(0,0,156,255), bgrx_pixel<32>(0,0,160,255), bgrx_pixel<32>(0,0,160,255), bgrx_pixel<32>(0,0,164,255),
        bgrx_pixel<32>(0,0,168,255), bgrx_pixel<32>(0,0,172,255), bgrx_pixel<32>(0,0,176,255), bgrx_pixel<32>(0,0,180,255),
        bgrx_pixel<32>(0,4,184,255), bgrx_pixel<32>(0,4,188,255), bgrx_pixel<32>(0,8,192,255), bgrx_pixel<32>(0,8,196,255),
        bgrx_pixel<32>(0,12,200,255), bgrx_pixel<32>(0,12,204,255), bgrx_pixel<32>(0,16,208,255), bgrx_pixel<32>(0,16,212,255),
        bgrx_pixel<32>(0,20,216,255), bgrx_pixel<32>(0,20,220,255), bgrx_pixel<32>(0,24,224,255), bgrx_pixel<32>(0,24,228,255),
        bgrx_pixel<32>(0,28,232,255), bgrx_pixel<32>(0,28,236,255), bgrx_pixel<32>(0,32,240,255), bgrx_pixel<32>(0,32,244,255),
        bgrx_pixel<32>(0,36,252,255), bgrx_pixel<32>(0,36,252,255), bgrx_pixel<32>(0,40,252,255), bgrx_pixel<32>(0,40,252,255),
        bgrx_pixel<32>(0,44,252,255), bgrx_pixel<32>(0,44,252,255), bgrx_pixel<32>(0,48,252,255), bgrx_pixel<32>(0,48,252,255),
        bgrx_pixel<32>(0,52,252,255), bgrx_pixel<32>(0,52,252,255), bgrx_pixel<32>(0,56,252,255), bgrx_pixel<32>(0,56,252,255),
        bgrx_pixel<32>(0,60,252,255), bgrx_pixel<32>(0,60,252,255), bgrx_pixel<32>(0,64,252,255), bgrx_pixel<32>(0,64,252,255),
        bgrx_pixel<32>(0,68,252,255), bgrx_pixel<32>(0,68,252,255), bgrx_pixel<32>(0,72,252,255), bgrx_pixel<32>(0,72,252,255),
        bgrx_pixel<32>(0,76,252,255), bgrx_pixel<32>(0,76,252,255), bgrx_pixel<32>(0,80,252,255), bgrx_pixel<32>(0,80,252,255),
        bgrx_pixel<32>(0,84,252,255), bgrx_pixel<32>(0,84,252,255), bgrx_pixel<32>(0,88,252,255), bgrx_pixel<32>(0,88,252,255),
        bgrx_pixel<32>(0,92,252,255), bgrx_pixel<32>(0,96,252,255), bgrx_pixel<32>(0,96,252,255), bgrx_pixel<32>(0,100,252,255),
        bgrx_pixel<32>(0,100,252,255), bgrx_pixel<32>(0,104,252,255), bgrx_pixel<32>(0,104,252,255), bgrx_pixel<32>(0,108,252,255),
        bgrx_pixel<32>(0,108,252,255), bgrx_pixel<32>(0,112,252,255), bgrx_pixel<32>(0,112,252,255), bgrx_pixel<32>(0,116,252,255),
        bgrx_pixel<32>(0,116,252,255), bgrx_pixel<32>(0,120,252,255), bgrx_pixel<32>(0,120,252,255), bgrx_pixel<32>(0,124,252,255),
        bgrx_pixel<32>(0,124,252,255), bgrx_pixel<32>(0,128,252,255), bgrx_pixel<32>(0,128,252,255), bgrx_pixel<32>(0,132,252,255),
        bgrx_pixel<32>(0,132,252,255), bgrx_pixel<32>(0,136,252,255), bgrx_pixel<32>(0,136,252,255), bgrx_pixel<32>(0,140,252,255),
        bgrx_pixel<32>(0,140,252,255), bgrx_pixel<32>(0,144,252,255), bgrx_pixel<32>(0,144,252,255), bgrx_pixel<32>(0,148,252,255),
        bgrx_pixel<32>(0,152,252,255), bgrx_pixel<32>(0,152,252,255), bgrx_pixel<32>(0,156,252,255), bgrx_pixel<32>(0,156,252,255),
        bgrx_pixel<32>(0,160,252,255), bgrx_pixel<32>(0,160,252,255), bgrx_pixel<32>(0,164,252,255), bgrx_pixel<32>(0,164,252,255),
        bgrx_pixel<32>(0,168,252,255), bgrx_pixel<32>(0,168,252,255), bgrx_pixel<32>(0,172,252,255), bgrx_pixel<32>(0,172,252,255),
        bgrx_pixel<32>(0,176,252,255), bgrx_pixel<32>(0,176,252,255), bgrx_pixel<32>(0,180,252,255), bgrx_pixel<32>(0,180,252,255),
        bgrx_pixel<32>(0,184,252,255), bgrx_pixel<32>(0,184,252,255), bgrx_pixel<32>(0,188,252,255), bgrx_pixel<32>(0,188,252,255),
        bgrx_pixel<32>(0,192,252,255), bgrx_pixel<32>(0,192,252,255), bgrx_pixel<32>(0,196,252,255), bgrx_pixel<32>(0,196,252,255),
        bgrx_pixel<32>(0,200,252,255), bgrx_pixel<32>(0,200,252,255), bgrx_pixel<32>(0,204,252,255), bgrx_pixel<32>(0,208,252,255),
        bgrx_pixel<32>(0,208,252,255), bgrx_pixel<32>(0,208,252,255), bgrx_pixel<32>(0,208,252,255), bgrx_pixel<32>(0,208,252,255),
        bgrx_pixel<32>(0,212,252,255), bgrx_pixel<32>(0,212,252,255), bgrx_pixel<32>(0,212,252,255), bgrx_pixel<32>(0,212,252,255),
        bgrx_pixel<32>(0,216,252,255), bgrx_pixel<32>(0,216,252,255), bgrx_pixel<32>(0,216,252,255), bgrx_pixel<32>(0,216,252,255),
        bgrx_pixel<32>(0,216,252,255), bgrx_pixel<32>(0,220,252,255), bgrx_pixel<32>(0,220,252,255), bgrx_pixel<32>(0,220,252,255),
        bgrx_pixel<32>(0,220,252,255), bgrx_pixel<32>(0,224,252,255), bgrx_pixel<32>(0,224,252,255), bgrx_pixel<32>(0,224,252,255),
        bgrx_pixel<32>(0,224,252,255), bgrx_pixel<32>(0,228,252,255), bgrx_pixel<32>(0,228,252,255), bgrx_pixel<32>(0,228,252,255),
        bgrx_pixel<32>(0,228,252,255), bgrx_pixel<32>(0,228,252,255), bgrx_pixel<32>(0,232,252,255), bgrx_pixel<32>(0,232,252,255),
        bgrx_pixel<32>(0,232,252,255), bgrx_pixel<32>(0,232,252,255), bgrx_pixel<32>(0,236,252,255), bgrx_pixel<32>(0,236,252,255),
        bgrx_pixel<32>(0,236,252,255), bgrx_pixel<32>(0,236,252,255), bgrx_pixel<32>(0,240,252,255), bgrx_pixel<32>(0,240,252,255),
        bgrx_pixel<32>(0,240,252,255), bgrx_pixel<32>(0,240,252,255), bgrx_pixel<32>(0,240,252,255), bgrx_pixel<32>(0,244,252,255),
        bgrx_pixel<32>(0,244,252,255), bgrx_pixel<32>(0,244,252,255), bgrx_pixel<32>(0,244,252,255), bgrx_pixel<32>(0,248,252,255),
        bgrx_pixel<32>(0,248,252,255), bgrx_pixel<32>(0,248,252,255), bgrx_pixel<32>(0,248,252,255), bgrx_pixel<32>(0,252,252,255),
        bgrx_pixel<32>(4,252,252,255), bgrx_pixel<32>(8,252,252,255), bgrx_pixel<32>(12,252,252,255), bgrx_pixel<32>(16,252,252,255),
        bgrx_pixel<32>(20,252,252,255), bgrx_pixel<32>(24,252,252,255), bgrx_pixel<32>(28,252,252,255), bgrx_pixel<32>(32,252,252,255),
        bgrx_pixel<32>(36,252,252,255), bgrx_pixel<32>(40,252,252,255), bgrx_pixel<32>(40,252,252,255), bgrx_pixel<32>(44,252,252,255),
        bgrx_pixel<32>(48,252,252,255), bgrx_pixel<32>(52,252,252,255), bgrx_pixel<32>(56,252,252,255), bgrx_pixel<32>(60,252,252,255),
        bgrx_pixel<32>(64,252,252,255), bgrx_pixel<32>(68,252,252,255), bgrx_pixel<32>(72,252,252,255), bgrx_pixel<32>(76,252,252,255),
        bgrx_pixel<32>(80,252,252,255), bgrx_pixel<32>(84,252,252,255), bgrx_pixel<32>(84,252,252,255), bgrx_pixel<32>(88,252,252,255),
        bgrx_pixel<32>(92,252,252,255), bgrx_pixel<32>(96,252,252,255), bgrx_pixel<32>(100,252,252,255), bgrx_pixel<32>(104,252,252,255),
        bgrx_pixel<32>(108,252,252,255), bgrx_pixel<32>(112,252,252,255), bgrx_pixel<32>(116,252,252,255), bgrx_pixel<32>(120,252,252,255),
        bgrx_pixel<32>(124,252,252,255), bgrx_pixel<32>(124,252,252,255), bgrx_pixel<32>(128,252,252,255), bgrx_pixel<32>(132,252,252,255),
        bgrx_pixel<32>(136,252,252,255), bgrx_pixel<32>(140,252,252,255), bgrx_pixel<32>(144,252,252,255), bgrx_pixel<32>(148,252,252,255),
        bgrx_pixel<32>(152,252,252,255), bgrx_pixel<32>(156,252,252,255), bgrx_pixel<32>(160,252,252,255), bgrx_pixel<32>(164,252,252,255),
        bgrx_pixel<32>(168,252,252,255), bgrx_pixel<32>(168,252,252,255), bgrx_pixel<32>(172,252,252,255), bgrx_pixel<32>(176,252,252,255),
        bgrx_pixel<32>(180,252,252,255), bgrx_pixel<32>(184,252,252,255), bgrx_pixel<32>(188,252,252,255), bgrx_pixel<32>(192,252,252,255),
        bgrx_pixel<32>(196,252,252,255), bgrx_pixel<32>(200,252,252,255), bgrx_pixel<32>(204,252,252,255), bgrx_pixel<32>(208,252,252,255),
        bgrx_pixel<32>(208,252,252,255), bgrx_pixel<32>(212,252,252,255), bgrx_pixel<32>(216,252,252,255), bgrx_pixel<32>(220,252,252,255),
        bgrx_pixel<32>(224,252,252,255), bgrx_pixel<32>(228,252,252,255), bgrx_pixel<32>(232,252,252,255), bgrx_pixel<32>(236,252,252,255),
        bgrx_pixel<32>(240,252,252,255), bgrx_pixel<32>(244,252,252,255), bgrx_pixel<32>(248,252,252,255), bgrx_pixel<32>(252,252,252,255)
};

// downloaded from fontsquirrel.com and header generated with
// https://honeythecodewitch.com/gfx/generator
#define ARCHITECTS_DAUGHTER_IMPLEMENTATION
#include <assets/architects_daughter.hpp>
static const open_font& text_font = architects_daughter;

// declare the format of the screen
using screen_t = screen<bgrx_pixel<32>>;
using color_t = color<typename screen_t::pixel_type>;
using color2_t = color<typename lcd_t::pixel_type>;
// for access to RGB8888 colors which controls use
using color32_t = color<rgba_pixel<32>>;

lcd_t lcd2;
touch_t touch2;
// UIX allows you to use two buffers for maximum DMA efficiency
// but it's not used in Winduino, which really doesn't need it.
// So we declare one 64KB buffer for the transfer
constexpr static const int lcd_screen_size = bitmap<typename screen_t::pixel_type>::sizeof_buffer({320,240});
constexpr static const int lcd_buffer_size = lcd_screen_size > 64 * 1024 ? 64 * 1024 : lcd_screen_size;
static uint8_t lcd_buffer1[lcd_buffer_size];



// the main screen
screen_t anim_screen({320,240}, sizeof(lcd_buffer1), lcd_buffer1, nullptr);

void uix_on_touch(point16* out_locations,
                  size_t* in_out_locations_size,
                  void* state) {
    if(!*in_out_locations_size) {
        return;
    }
    int x,y;
    if(read_mouse(&x,&y)) {
        if(x<0) {
            x=0;
        }
        if(x>=320) {
            x=319;
        }
        if(y<0) {
            y=0;
        }
        if(y>=240) {
            y=239;
        }
        *in_out_locations_size =1;
        *out_locations = point16((unsigned)x,(unsigned)y);
        return;
    
    }
    *in_out_locations_size = 0;
}
void uix_on_flush(const rect16& bounds, const void* bmp, void* state) {
	flush_bitmap(bounds.x1,bounds.y1,bounds.x2-bounds.x1+1,bounds.y2-bounds.y1+1,bmp);
    anim_screen.flush_complete();
}

using label_t = label<typename screen_t::control_surface_type>;
static label_t fps(anim_screen);
static label_t summaries[] = {
    label_t(anim_screen),
    label_t(anim_screen),
    label_t(anim_screen),
    label_t(anim_screen),
    label_t(anim_screen),
    label_t(anim_screen)};
template <typename ControlSurfaceType>
class fire_box : public control<ControlSurfaceType> {
    int draw_state = 0;
	constexpr static const int V_WIDTH =320 / 4;
	constexpr static const int V_HEIGHT =240 / 4;
	constexpr static const int BUF_WIDTH =320 / 4;
	constexpr static const int BUF_HEIGHT =240 / 4+6;

    uint8_t p1[BUF_HEIGHT][BUF_WIDTH];  // VGA buffer, quarter resolution w/extra lines
    unsigned int i, j, k, l, delta;     // looping variables, counters, and data
    char ch;

   public:
    using control_surface_type = ControlSurfaceType;
    using base_type = control<control_surface_type>;
    using pixel_type = typename base_type::pixel_type;
    using palette_type = typename base_type::palette_type;
    fire_box(uix::invalidation_tracker& parent, const palette_type* palette = nullptr)
        : base_type(parent, palette) {
    }
    fire_box(fire_box&& rhs) {
        do_move_control(rhs);
        draw_state = 0;
    }
    fire_box& operator=(fire_box&& rhs) {
        do_move_control(rhs);
        draw_state = 0;
        return *this;
    }
    fire_box(const fire_box& rhs) {
        do_copy_control(rhs);
        draw_state = 0;
    }
    fire_box& operator=(const fire_box& rhs) {
        do_copy_control(rhs);
        draw_state = 0;
        return *this;
    }
    virtual void on_before_render() {
        switch (draw_state) {
            case 0:
                // Initialize the buffer to 0s
                for (i = 0; i < BUF_HEIGHT; i++) {
                    for (j = 0; j < BUF_WIDTH; j++) {
                        p1[i][j] = 0;
                    }
                }
                draw_state = 1;
                // fall through
            case 1:
                // Transform current buffer
                for (i = 1; i < BUF_HEIGHT; ++i) {
                    for (j = 0; j < BUF_WIDTH; ++j) {
                        if (j == 0)
                            p1[i - 1][j] = (p1[i][j] +
                                            p1[i - 1][BUF_WIDTH - 1] +
                                            p1[i][j + 1] +
                                            p1[i + 1][j]) >>
                                           2;
                        else if (j == 79)
                            p1[i - 1][j] = (p1[i][j] +
                                            p1[i][j - 1] +
                                            p1[i + 1][0] +
                                            p1[i + 1][j]) >>
                                           2;
                        else
                            p1[i - 1][j] = (p1[i][j] +
                                            p1[i][j - 1] +
                                            p1[i][j + 1] +
                                            p1[i + 1][j]) >>
                                           2;

                        if (p1[i][j] > 11)
                            p1[i][j] = p1[i][j] - 12;
                        else if (p1[i][j] > 3)
                            p1[i][j] = p1[i][j] - 4;
                        else {
                            if (p1[i][j] > 0) p1[i][j]--;
                            if (p1[i][j] > 0) p1[i][j]--;
                            if (p1[i][j] > 0) p1[i][j]--;
                        }
                    }
                }
                delta = 0;
                for (j = 0; j < BUF_WIDTH; j++) {
                    if (rand() % 10 < 5) {
                        delta = (rand() & 1) * 255;
                    }
                    p1[BUF_HEIGHT - 2][j] = delta;
                    p1[BUF_HEIGHT - 1][j] = delta;
                }
        }
    }
    virtual void on_paint(control_surface_type& destination, const srect16& clip) override {
        for (int y = clip.y1; y <= clip.y2; ++y) {
            for (int x = clip.x1; x <= clip.x2; ++x) {
                int i = y >> 2;
                int j = x >> 2;
                destination.point(point16(x, y), fire_cols[p1[i][j]]);
            }
        }
    }
    virtual bool on_touch(size_t locations_size, const spoint16* locations) override {
        fps.visible(true);
        return true;
    }
    virtual void on_release() override {
        fps.visible(false);
    }
};
using fire_box_t = fire_box<typename screen_t::control_surface_type>;
template <typename ControlSurfaceType>
class alpha_box : public control<ControlSurfaceType> {
    constexpr static const int horizontal_alignment = 32;
    constexpr static const int vertical_alignment = 32;
    int draw_state = 0;
    constexpr static const size_t count = 10;
    constexpr static const int16_t radius = 25;
    spoint16 pts[count];        // locations
    spoint16 dts[count];        // deltas
    rgba_pixel<32> cls[count];  // colors
    template <typename T>
    constexpr static T h_align_up(T value) {
        if (value % horizontal_alignment != 0)
            value += (T)(horizontal_alignment - value % horizontal_alignment);
        return value;
    }
    template <typename T>
    constexpr static T h_align_down(T value) {
        value -= value % horizontal_alignment;
        return value;
    }
    template <typename T>
    constexpr static T v_align_up(T value) {
        if (value % vertical_alignment != 0)
            value += (T)(vertical_alignment - value % vertical_alignment);
        return value;
    }
    template <typename T>
    constexpr static T v_align_down(T value) {
        value -= value % vertical_alignment;
        return value;
    }
    constexpr static srect16 align(const srect16& value) {
        int x2 = h_align_up(value.x2);
        if (horizontal_alignment != 1) {
            --x2;
        }
        int y2 = v_align_up(value.y2);
        if (vertical_alignment != 1) {
            --y2;
        }
        return srect16(h_align_down(value.x1), v_align_down(value.y1), x2, y2);
    }

   public:
    using control_surface_type = ControlSurfaceType;
    using base_type = control<control_surface_type>;
    using pixel_type = typename base_type::pixel_type;
    using palette_type = typename base_type::palette_type;
    alpha_box(uix::invalidation_tracker& parent, const palette_type* palette = nullptr)
        : base_type(parent, palette) {
    }
    alpha_box(alpha_box&& rhs) {
        do_move_control(rhs);
        draw_state = 0;
    }
    alpha_box& operator=(alpha_box&& rhs) {
        do_move_control(rhs);
        draw_state = 0;
        return *this;
    }
    alpha_box(const alpha_box& rhs) {
        do_copy_control(rhs);
        draw_state = 0;
    }
    alpha_box& operator=(const alpha_box& rhs) {
        do_copy_control(rhs);
        draw_state = 0;
        return *this;
    }
    virtual void on_before_render() {
        switch (draw_state) {
            case 0:
                for (size_t i = 0; i < count; ++i) {
                    // start at the center
                    pts[i] = spoint16(this->dimensions().width / 2, this->dimensions().height / 2);
                    dts[i] = {0, 0};
                    // random deltas. Retry on (dx=0||dy=0)
                    while (dts[i].x == 0 || dts[i].y == 0) {
                        dts[i].x = (rand() % 5) - 2;
                        dts[i].y = (rand() % 5) - 2;
                    }
                    // random color RGBA8888
                    cls[i] = rgba_pixel<32>((rand() % 255), (rand() % 255), (rand() % 255), (rand() % 224) + 32);
                }
                draw_state = 1;
                // fall through
        }
    }
    virtual void on_paint(control_surface_type& destination, const srect16& clip) override {
        srect16 clip_align = align(clip);
        for (int y = clip_align.y1; y <= clip_align.y2; y += vertical_alignment) {
            for (int x = clip_align.x1; x <= clip_align.x2; x += horizontal_alignment) {
                rect16 r(x, y, x + horizontal_alignment, y + vertical_alignment);
                bool w = ((x + y) % (horizontal_alignment + vertical_alignment));
                if (w) {
                    destination.fill(r, color_t::white);
                }
            }
        }
        // draw the circles
        for (size_t i = 0; i < count; ++i) {
            spoint16& pt = pts[i];
            srect16 r(pt, radius);
            if (clip.intersects(r)) {
                rgba_pixel<32>& col = cls[i];
                draw::filled_ellipse(destination, r, col, &clip);
            }
        }
    }
    virtual void on_after_render() {
        for (size_t i = 0; i < count; ++i) {
            spoint16& pt = pts[i];
            spoint16& d = dts[i];
            // move the circle
            pt.x += d.x;
            pt.y += d.y;
            // if it is about to hit the edge, invert
            // the respective deltas
            if (pt.x + d.x + -radius <= 0 || pt.x + d.x + radius >= this->dimensions().bounds().x2) {
                d.x = -d.x;
            }
            if (pt.y + d.y + -radius <= 0 || pt.y + d.y + radius >= this->dimensions().bounds().y2) {
                d.y = -d.y;
            }
        }
    }
    virtual bool on_touch(size_t locations_size, const spoint16* locations) override {
        fps.visible(true);
        return true;
    }
    virtual void on_release() override {
        fps.visible(false);
    }
};
using alpha_box_t = alpha_box<screen_t::control_surface_type>;

template <typename ControlSurfaceType>
class plaid_box : public control<ControlSurfaceType> {
    int draw_state = 0;
    constexpr static const size_t count = 10;
    constexpr static const int16_t width = 25;
    spoint16 pts[count];        // locations
    spoint16 dts[count];        // deltas
    rgba_pixel<32> cls[count];  // colors

   public:
    using control_surface_type = ControlSurfaceType;
    using base_type = control<control_surface_type>;
    using pixel_type = typename base_type::pixel_type;
    using palette_type = typename base_type::palette_type;
    plaid_box(uix::invalidation_tracker& parent, const palette_type* palette = nullptr)
        : base_type(parent, palette) {
    }
    plaid_box(plaid_box&& rhs) {
        do_move_control(rhs);
        draw_state = 0;
    }
    plaid_box& operator=(plaid_box&& rhs) {
        do_move_control(rhs);
        draw_state = 0;
        return *this;
    }
    plaid_box(const plaid_box& rhs) {
        do_copy_control(rhs);
        draw_state = 0;
    }
    plaid_box& operator=(const plaid_box& rhs) {
        do_copy_control(rhs);
        draw_state = 0;
        return *this;
    }
    virtual void on_before_render() {
        switch (draw_state) {
            case 0:
                size_t i;
                for (i = 0; i < count; ++i) {
                    if ((i & 1)) {
                        pts[i] = spoint16(0, (rand() % (this->dimensions().height - width)) + width / 2);
                        dts[i] = {0, 0};
                        // random deltas. Retry on (dy=0)
                        while (dts[i].y == 0) {
                            dts[i].y = (rand() % 5) - 2;
                        }
                    } else {
                        pts[i] = spoint16((rand() % (this->dimensions().width - width)) + width / 2, 0);
                        dts[i] = {0, 0};
                        // random deltas. Retry on (dx=0)
                        while (dts[i].x == 0) {
                            dts[i].x = (rand() % 5) - 2;
                        }
                    }
                    // random color RGBA8888
                    cls[i] = rgba_pixel<32>((rand() % 255), (rand() % 255), (rand() % 255), (rand() % 224) + 32);
                }
                draw_state = 1;
                // fall through
        }
    }
    virtual void on_after_render() {
        switch (draw_state) {
            case 0:
                break;
            case 1:
                for (size_t i = 0; i < count; ++i) {
                    spoint16& pt = pts[i];
                    spoint16& d = dts[i];
                    // move the bar
                    pt.x += d.x;
                    pt.y += d.y;
                    // if it is about to hit the edge, invert
                    // the respective deltas
                    if (pt.x + d.x + -width / 2 < 0 || pt.x + d.x + width / 2 > this->bounds().x2) {
                        d.x = -d.x;
                    }
                    if (pt.y + d.y + -width / 2 < 0 || pt.y + d.y + width / 2 > this->bounds().y2) {
                        d.y = -d.y;
                    }
                }
                break;
        }
    }
    virtual void on_paint(control_surface_type& destination, const srect16& clip) override {
        // draw the bars
        for (size_t i = 0; i < count; ++i) {
            spoint16& pt = pts[i];
            spoint16& d = dts[i];
            srect16 r;
            if (d.y == 0) {
                r = srect16(pt.x - width / 2, 0, pt.x + width / 2, this->bounds().y2);
            } else {
                r = srect16(0, pt.y - width / 2, this->bounds().x2, pt.y + width / 2);
            }
            if (clip.intersects(r)) {
                rgba_pixel<32>& col = cls[i];
                draw::filled_rectangle(destination, r, col, &clip);
            }
        }
    }
    virtual bool on_touch(size_t locations_size, const spoint16* locations) override {
        fps.visible(true);
        return true;
    }
    virtual void on_release() override {
        fps.visible(false);
    }
};
using plaid_box_t = plaid_box<screen_t::control_surface_type>;

// the controls
static fire_box_t fire(anim_screen);
static alpha_box_t alpha(anim_screen);
static plaid_box_t plaid(anim_screen);
// initialize the screens and controls
static void screen_init() {
    const rgba_pixel<32> transparent(0, 0, 0, 0);
    alpha.bounds(anim_screen.bounds());
    fire.bounds(anim_screen.bounds());
    fire.visible(false);
    plaid.bounds(anim_screen.bounds());
    plaid.visible(false);
    fps.text_color(color32_t::red);
    fps.text_open_font(&text_font);
    fps.text_line_height(40);
    fps.padding({0, 0});
    rgba_pixel<32> bg(0, 0, 0, 224);
    fps.background_color(bg);
    fps.text_justify(uix_justify::bottom_right);
    fps.bounds(srect16(0, anim_screen.bounds().y2 - fps.text_line_height() + 2, anim_screen.bounds().x2, anim_screen.bounds().y2));
    fps.visible(false);
    int y = 0;
    int lh = anim_screen.dimensions().height / 6 - 2;
    for (size_t i = 0; i < (sizeof(summaries) / sizeof(label_t)); ++i) {
        label_t& summary = summaries[i];
        summary.text_line_height(lh);
        summary.padding({0, 0});
        summary.bounds(srect16(0, y, anim_screen.bounds().x2, y + lh + 2));
        summary.text_open_font(&text_font);
        summary.border_color(transparent);
        summary.background_color(transparent);
        summary.text_color(color32_t::red);
        summary.text_justify(uix_justify::top_left);
        summary.visible(false);
        y += summary.text_line_height() + 2;
    }
    anim_screen.register_control(alpha);
    anim_screen.register_control(fire);
    anim_screen.register_control(plaid);
    anim_screen.register_control(fps);
    for (size_t i = 0; i < (sizeof(summaries) / sizeof(label_t)); ++i) {
        label_t& summary = summaries[i];
        anim_screen.register_control(summary);
    }
    anim_screen.background_color(color_t::black);
    anim_screen.on_flush_callback(uix_on_flush);
    anim_screen.on_touch_callback(uix_on_touch);
}
void setup() {
    Serial.begin(115200);
    Serial1.begin(115200);
    Serial1.println("Hello world!");
    // make some virtual GPIOs
    pinMode(17,OUTPUT);
    digitalWrite(17,HIGH);
    // attach an interrupt to 18
    // so when we put a new value
    // in winduino it causes
    // an interrupt
    attachInterrupt(18,[]() {
        Serial.println("Rise!");
    }, RISING);
    
    // init the UI screen
    screen_init();
#ifdef WINDUINO
     // finally we can initialize 
    // the screen and touch
    if(!lcd2.initialize()) {
        Serial.println("Could not find the ST7789");
    }

    if(touch2.initialize()==false) {
        Serial.println("Could not find FT6236");
    }
    
    // draw our screen
    open_text_info oti;
    oti.font = &text_font;
    oti.scale = oti.font->scale(50);
    oti.text = "Hello world!";
    
    // reading SPI displays is slow so we don't. In fact, bitmaps are really the best way to draw
    // create a bitmap with the same underlying format as the display
    auto bmp = create_bitmap_from(lcd2,lcd2.dimensions());
    if(bmp.begin()) { // make sure not out of memory
        bmp.fill(bmp.bounds(),color2_t::orange);
        draw::text(bmp,bmp.bounds(),oti,color2_t::wheat);
        draw::bitmap(lcd2,lcd2.bounds(),bmp,bmp.bounds());
        free(bmp.begin()); // free it when we're done
    }
#endif
}
void loop() {
    constexpr static const int run_seconds = 5;
    constexpr static const int summary_seconds = 10;
    static char szsummaries[sizeof(summaries) / sizeof(label_t)][128];
    static int seconds = 0;
    static int frames = 0;
    static int total_frames_alpha = 0;
    static int total_frames_fire = 0;
    static int total_frames_plaid = 0;
    static int fps_alpha[run_seconds];
    static int fps_fire[run_seconds];
    static int fps_plaid[run_seconds];
    static bool showed_summary = false;
    static int fps_index = 0;
    static char szfps[32];
    static uint32_t fps_ts = 0;
    uint32_t ms = millis();
    static bool toggle = true;
    ++frames;

    if (ms > fps_ts + 1000) {
        fps_ts = ms;
        snprintf(szfps, sizeof(szfps), "fps: %d", frames);
        Serial.println(szfps);
        fps.text(szfps);
        if (alpha.visible()) {
            fps_alpha[fps_index++] = frames;
        } else if (fire.visible()) {
            fps_fire[fps_index++] = frames;
        } else if (plaid.visible()) {
            fps_plaid[fps_index++] = frames;
        }
        frames = 0;
        ++seconds;
        // toggle every second
        digitalWrite(17,toggle?HIGH:LOW);
        toggle=!toggle;
        
    }
    if (alpha.visible()) {
        alpha.invalidate();
        ++total_frames_alpha;
    } else if (fire.visible()) {
        fire.invalidate();
        ++total_frames_fire;
    } else if (plaid.visible()) {
        plaid.invalidate();
        ++total_frames_plaid;
    }
    if (seconds == (run_seconds * 1)) {
        alpha.visible(false);
        fire.visible(true);
        plaid.visible(false);
        for (size_t i = 0; i < (sizeof(summaries) / sizeof(label_t)); ++i) {
            label_t& summary = summaries[i];
            summary.visible(false);
        }
        fps_index = 0;
    } else if (seconds == (run_seconds * 2)) {
        alpha.visible(false);
        fire.visible(false);
        plaid.visible(true);
        for (size_t i = 0; i < (sizeof(summaries) / sizeof(label_t)); ++i) {
            label_t& summary = summaries[i];
            summary.visible(false);
        }
        fps_index = 0;
    } else if (seconds == (run_seconds * 3)) {
        if (!showed_summary) {
            showed_summary = true;
            alpha.visible(false);
            fire.visible(false);
            plaid.visible(false);
            for (size_t i = 0; i < (sizeof(summaries) / sizeof(label_t)); ++i) {
                label_t& summary = summaries[i];
                summary.visible(true);
            }
            int alpha_fps_max = 0, alpha_fps_sum = 0;
            for (size_t i = 0; i < run_seconds; ++i) {
                int fps = fps_alpha[i];
                if (fps > alpha_fps_max) {
                    alpha_fps_max = fps;
                }
                alpha_fps_sum += fps;
            }
            int fire_fps_max = 0, fire_fps_sum = 0;
            for (size_t i = 0; i < run_seconds; ++i) {
                int fps = fps_fire[i];
                if (fps > fire_fps_max) {
                    fire_fps_max = fps;
                }
                fire_fps_sum += fps;
            }
            int plaid_fps_max = 0, plaid_fps_sum = 0;
            for (size_t i = 0; i < run_seconds; ++i) {
                int fps = fps_plaid[i];
                if (fps > plaid_fps_max) {
                    plaid_fps_max = fps;
                }
                plaid_fps_sum += fps;
            }
            strcpy(szsummaries[0], "alpha max/avg/frames");
            Serial.println(szsummaries[0]);
            summaries[0].text(szsummaries[0]);
            sprintf(szsummaries[1], "%d/%d/%d", alpha_fps_max,
                    (int)roundf((float)alpha_fps_sum / (float)run_seconds),
                    total_frames_alpha);
            Serial.println(szsummaries[1]);
            summaries[1].text(szsummaries[1]);
            strcpy(szsummaries[2], "fire max/avg/frames");
            Serial.println(szsummaries[2]);
            summaries[2].text(szsummaries[2]);
            sprintf(szsummaries[3], "%d/%d/%d", fire_fps_max,
                    (int)roundf((float)fire_fps_sum / (float)run_seconds),
                    total_frames_fire);
            summaries[3].text(szsummaries[3]);
            Serial.println(szsummaries[3]);
            strcpy(szsummaries[4], "plaid max/avg/frames");
            summaries[4].text(szsummaries[4]);
            Serial.println(szsummaries[4]);
            sprintf(szsummaries[5], "%d/%d/%d", plaid_fps_max,
                    (int)roundf((float)plaid_fps_sum / (float)run_seconds),
                    total_frames_plaid);
            Serial.println(szsummaries[5]);
            summaries[5].text(szsummaries[5]);
            fps_index = 0;
        }
    } else if (seconds >= (run_seconds * 3) + summary_seconds) {
        seconds = 0;
        total_frames_alpha = 0;
        total_frames_fire = 0;
        total_frames_plaid = 0;
        alpha.visible(true);
        fire.visible(false);
        plaid.visible(false);
        for (size_t i = 0; i < (sizeof(summaries) / sizeof(label_t)); ++i) {
            label_t& summary = summaries[i];
            summary.visible(false);
        }
        fps_index = 0;
        showed_summary = false;
    }
#ifdef WINDUINO
    // finger paint 
    if(touch2.update()) {
        if(touch2.touches()) {
            uint16_t x,y;
            touch2.xy(&x,&y);
            draw::filled_ellipse(lcd2,rect16(point16(x,y),5),color2_t::purple);
        }
    }
#endif
    anim_screen.update();
    char buf[1024];
    while(Serial1.available()) {
        Serial1.read(buf,sizeof(buf));
        Serial.write(buf);
    }
}
// the following code runs before setup() only when executed in Winduino
#ifdef WINDUINO
void winduino() {
    log_print("hello from winduino()\r\n");
    //hardware_set_screen_size(480,320);
    
    // load the SPI screen
    void* hw_screen = hardware_load(LIB_SPI_SCREEN);
    if(hw_screen==nullptr) {
        log_print("Unable to load external SPI screen\r\n");
    }
    // attach the log (log all messages, including debug)
    hardware_attach_log(hw_screen,"[scr]",255);
    // set the resolution
    struct {
        uint16_t width;
        uint16_t height;
    } screen_size = {240,135};
    if(!hardware_configure(hw_screen,SPI_SCREEN_PROP_RESOLUTION,&screen_size,sizeof(screen_size))) {
        log_print("Unable to configure hardware\r\n");
    }
    // set the panel offsets
    struct {
        int16_t x;
        int16_t y;
    } screen_offsets = {40,53};
    if(!hardware_configure(hw_screen,SPI_SCREEN_PROP_OFFSETS,&screen_offsets,sizeof(screen_offsets))) {
        log_print("Unable to configure hardware\r\n");
    }
    // set the screen GPIOs (aside from SPI which are fixed and virtual)
    hardware_set_pin(hw_screen,15, SPI_SCREEN_PIN_BKL);    
    hardware_set_pin(hw_screen,5, SPI_SCREEN_PIN_CS);
    hardware_set_pin(hw_screen,2,SPI_SCREEN_PIN_DC);
    hardware_set_pin(hw_screen,4,SPI_SCREEN_PIN_RST);
    // attach the SPI bus (for the screen)
    hardware_attach_spi(hw_screen,1);
    // attach the I2C bus (for touch)
    hardware_attach_i2c(hw_screen,0);
    // attach Serial1 to COM25
    //hardware_attach_serial(1,25);
   
}
#endif