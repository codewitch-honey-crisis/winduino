#define SCREEN_SIZE {320,240}
#include <winduino.hpp>
#include <gfx.hpp>
#include <uix.hpp>
using namespace gfx;
using namespace uix;

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

uint8_t * transfer_buffer = nullptr;
//constexpr static const size_t transfer_buffer_size = frame_buffer_t::sizeof_buffer(screen_size.width , screen_size.height );
constexpr static const size_t transfer_buffer_size = 128*1024;
screen_t main_screen;
static int seconds=0;
static int printed = 0;
template <typename ControlSurfaceType>
class fire_box : public control<ControlSurfaceType> {
    int draw_state = 0;
	constexpr static const int V_WIDTH =screen_size.width / 4;
	constexpr static const int V_HEIGHT =screen_size.height / 4;
	constexpr static const int BUF_WIDTH =screen_size.width / 4;
	constexpr static const int BUF_HEIGHT =screen_size.height / 4+6;

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
        //fps.visible(true);
        return true;
    }
    virtual void on_release() override {
        //fps.visible(false);
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
            spoint16& d = dts[i];
            srect16 r(pt, radius);
            if (clip.intersects(r)) {
                rgba_pixel<32>& col = cls[i];
                draw::filled_ellipse(destination, r, col, &clip);
            }
            // move the circle
            pt.x += d.x;
            pt.y += d.y;
            // if it is about to hit the edge, invert
            // the respective deltas
            if (pt.x + d.x + -radius <= 0 || pt.x + d.x + radius >= destination.bounds().x2) {
                d.x = -d.x;
            }
            if (pt.y + d.y + -radius <= 0 || pt.y + d.y + radius >= destination.bounds().y2) {
                d.y = -d.y;
            }
        }
    }
    virtual bool on_touch(size_t locations_size, const spoint16* locations) override {
        //fps.visible(true);
        return true;
    }
    virtual void on_release() override {
        //fps.visible(false);
    }
};
using alpha_box_t = alpha_box<screen_t::control_surface_type>;

template <typename ControlSurfaceType>
class plaid_box : public control<ControlSurfaceType> {
    int draw_state = 0;
    spoint16 pts[10];        // locations
    spoint16 dts[10];        // deltas
    rgba_pixel<32> cls[10];  // colors

   public:
    using control_surface_type = ControlSurfaceType;
    using base_type = control<control_surface_type>;
    using pixel_type = typename base_type::pixel_type;
    using palette_type = typename base_type::palette_type;
    plaid_box(uix::invalidation_tracker& parent, const palette_type* palette = nullptr)
        : base_type(parent, palette) {
    }
    plaid_box()
        : base_type() {
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
        constexpr static const size_t count = 10;
    constexpr static const int16_t width = 25;
    
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
                    //convert(fire_cols[6+(rand()%250)],&cls[i]);
                    //cls[i].template channel<channel_name::A>((rand()%224)+32);
                    cls[i] = rgba_pixel<32>((rand() % 256), (rand() % 256), (rand() % 256), (rand() % 224) + 32);
                }
                draw_state = 1;
                // fall through
        }
    }
    virtual void on_after_render() {
    constexpr static const size_t count = 10;
    constexpr static const int16_t width = 25;
    
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
    constexpr static const size_t count = 10;
    constexpr static const int16_t width = 25;
    
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
                draw::filled_rectangle(destination, r,cls[i], &clip);
            }
        }
    }
    virtual bool on_touch(size_t locations_size, const spoint16* locations) override {
        //fps.visible(true);
        return true;
    }
    virtual void on_release() override {
        //fps.visible(false);
    }
};
using plaid_box_t = plaid_box<screen_t::control_surface_type>;

// the controls
static fire_box_t fire(main_screen);
static alpha_box_t alpha(main_screen);
static plaid_box_t plaid(main_screen);

void uix_on_touch(point16* out_locations,
                  size_t* in_out_locations_size,
                  void* state) {
    if(!*in_out_locations_size) {
        return;
    }
    int x,y;
    if(read_mouse(&x,&y)) {
        if(ssize16(SCREEN_SIZE).bounds().intersects(spoint16(x,y))) {
            *in_out_locations_size =1;
            *out_locations = point16((unsigned)x,(unsigned)y);
            return;
        }
    }
    *in_out_locations_size = 0;
}
void uix_on_flush(const rect16& bounds, const void* bmp, void* state) {
	flush_bitmap(bounds.x1,bounds.y1,bounds.x2-bounds.x1+1,bounds.y2-bounds.y1+1,bmp);
    main_screen.flush_complete();
}
void setup() {
    Serial.begin(115200);
    transfer_buffer = (uint8_t *)malloc(transfer_buffer_size);
    main_screen.dimensions((ssize16)SCREEN_SIZE);
    main_screen.buffer_size(transfer_buffer_size);
    main_screen.buffer1(transfer_buffer);
    main_screen.on_flush_callback(uix_on_flush);
    main_screen.on_touch_callback(uix_on_touch);
    main_screen.background_color(color_t::black);
    alpha.bounds(main_screen.bounds());
    main_screen.register_control(alpha);
    fire.bounds(main_screen.bounds());
    fire.visible(false);
    main_screen.register_control(fire);
    plaid.bounds(main_screen.bounds());
    plaid.visible(false);
    main_screen.register_control(plaid);
    Serial.println("alpha");
}
void loop() {
    static int ts = 0;
    if(ts==0) { ts = millis(); }
    if(millis()>ts+1000) {
        ts = millis();
        ++seconds;
    }
    if(seconds==5) {
        alpha.visible(false);
        fire.visible(true);
        plaid.visible(false);
        if(printed!=5) {
            Serial.println("fire");
            printed = 5;
        }
    }
    if(seconds==10) {
        alpha.visible(false);
        fire.visible(false);
        plaid.visible(true);
        if(printed!=10) {
            Serial.println("plaid");
            printed = 10;
        }
    }
    if(seconds>=15) {
        printed = 0;
        alpha.visible(true);
        fire.visible(false);
        plaid.visible(false);
        Serial.println("alpha");
        seconds = 0;
    }
    main_screen.invalidate();
    main_screen.update();
}
