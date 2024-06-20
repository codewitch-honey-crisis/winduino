#include <Windows.h>
#include <stddef.h>
#include <stdint.h>

#define DLL_API __declspec(dllexport)

#define CALL __cdecl

static const uint8_t TFT_CASET = 0x2A;
static const uint8_t TFT_PASET = 0x2B;
static const uint8_t TFT_RAMWR = 0x2C;
static const uint8_t TFT_RAMRD = 0x2E;

constexpr static const uint8_t TOUCH_REG_MODE = 0x00;
constexpr static const uint8_t TOUCH_REG_XL = 0x04;
constexpr static const uint8_t TOUCH_REG_XH = 0x03;
constexpr static const uint8_t TOUCH_REG_YL = 0x06;
constexpr static const uint8_t TOUCH_REG_YH = 0x05;
constexpr static const uint8_t TOUCH_REG_NUMTOUCHES = 0x2;
constexpr static const uint8_t TOUCH_REG_THRESHHOLD = 0x80;
constexpr static const uint8_t TOUCH_REG_VENDID = 0xA8;
constexpr static const uint8_t TOUCH_REG_CHIPID = 0xA3;
constexpr static const uint8_t FT6236_VENDID = 0x11;
constexpr static const uint8_t FT6206_CHIPID = 0x6;
constexpr static const uint8_t FT6236_CHIPID = 0x36;
constexpr static const uint8_t FT6236U_CHIPID = 0x64;
typedef __cdecl void (*gpio_set_callback)(uint32_t value, void* state);
typedef __cdecl uint8_t (*gpio_get_callback)(void* state);
typedef __cdecl void(*log_callback)(const char* text);
typedef struct input {
    void* read_state;
    bool has_cached_value;
    uint8_t cached_value;
    gpio_get_callback read;
    void (*on_change_callback)(void* state);
    void* on_change_callback_state;
    input() : read_state(nullptr), has_cached_value(false), cached_value(false), read(nullptr), on_change_callback(nullptr), on_change_callback_state(nullptr) {
    }
    void on_changed(uint8_t value) {
        has_cached_value = true;
        cached_value = value;
        if (on_change_callback != nullptr) {
            on_change_callback(on_change_callback_state);
        }
    }
    uint8_t value() const {
        if (!has_cached_value) {
            if (read == nullptr) {
                return 0;
            }
            return read(read_state);
        }
        return cached_value;
    }
} input_t;

typedef enum screen_states {
    SCREEN_STATE_INITIAL = 0,
    SCREEN_STATE_IGNORING,
    SCREEN_STATE_COLSET1,
    SCREEN_STATE_COLSET2,
    SCREEN_STATE_ROWSET1,
    SCREEN_STATE_ROWSET2,
    SCREEN_STATE_WRITE,
    SCREEN_STATE_READ1,
    SCREEN_STATE_READ2
} screen_states_t;
typedef enum touch_states {
    TOUCH_STATE_INITIAL = 0,
    TOUCH_STATE_IGNORING,
    TOUCH_STATE_ADDRESS,
    TOUCH_STATE_READ,
    TOUCH_STATE_WRITE
} touch_states_t;

class hardware_interface {
public:
    virtual int CALL CanConfigure() =0;
    virtual int CALL Configure(int prop, 
                            void* data, 
                            size_t size) =0;
    virtual int CALL CanConnect() =0;
    virtual int CALL Connect(uint8_t pin, 
                            gpio_get_callback getter, 
                            gpio_set_callback setter, 
                            void* state) =0;
    virtual int CALL CanUpdate() =0;
    virtual int CALL Update() =0;
    virtual int CALL CanPinChange() =0;
    virtual int CALL PinChange(uint8_t pin, 
                            uint32_t value) =0;
    virtual int CALL CanTransferBitsSPI() =0;
    virtual int CALL TransferBitsSPI(uint8_t* data, 
                                    size_t size_bits) =0;
    virtual int CALL CanTransferBytesI2C() =0;
    virtual int CALL TransferBytesI2C(const uint8_t* in, 
                                    size_t in_size, 
                                    uint8_t* out, 
                                    size_t* in_out_out_size) =0;
    virtual int CALL CanAttachLog() =0;
    virtual int CALL AttachLog(log_callback logger, 
                            const char* prefix, 
                            uint8_t level) =0;
    virtual int CALL Destroy() = 0;
};
class spi_screen : public hardware_interface {
    struct {
        uint16_t width;
        uint16_t height;
    } screen_size; //= {320, 240};
    struct {
        int16_t x;
        int16_t y;
    } screen_offsets; // = {0,0};
    screen_states_t screen_st;// = SCREEN_STATE_INITIAL;
    uint8_t touch_address;// = 0x38;
    bool touch_factory_mode;// = false;
    int touch_current_reg;// = -1;
    uint8_t touch_threshhold;// = 0x80;
    bool bkl_low;// = false;
    uint8_t colset;// = TFT_CASET;
    uint8_t rowset;// = TFT_PASET;
    uint8_t write;// = TFT_RAMWR;
    uint8_t read;// = TFT_RAMRD;
    uint16_t col_start, col_end, row_start, row_end;
    int cmd;// = -1;
    uint8_t in_byte;// = 0;
    int bit_count;// = 0;
    input_t cs;
    input_t dc;
    input_t rst;
    input_t bkl;
    uint8_t* frame_buffer;
    uint32_t data_word;// = 0;
    size_t bytes_written, bytes_read;
    uint16_t column, row;
    int offset;
    bool can_configure;// = true;
    HANDLE render_mutex;// = NULL;
    HANDLE render_thread;// = NULL;
    HANDLE quit_event;// = NULL;
    int reset_state;// = false;
    HANDLE screen_ready;// = NULL;
    HANDLE touch_mutex;// = NULL;
    log_callback logger;// = NULL;
    const char* log_prefix;
    uint8_t log_level;
    bool in_pixel_transfer;// = false;
    volatile long render_changed;// = 1;
    // mouse mess
    struct {
        int x;
        int y;
    } mouse_loc;
    int mouse_state;// = 0;  // 0 = released, 1 = pressed
    int old_mouse_state;// = 0;
    int mouse_req;// = 0;
    // directX stuff
    ID2D1HwndRenderTarget* render_target;// = nullptr;
    ID2D1Factory* d2d_factory;// = nullptr;
    ID2D1Bitmap* render_bitmap;// = nullptr;
    HWND hwnd_screen;// = nullptr;
    bool created_wndcls;// = false;
    void logfmt(uint8_t level,const char* format, ...);
    void update_title(HWND hwnd);
    uint8_t process_byte_spi(uint8_t val);
    static unsigned long CALLBACK window_thread_proc(void* state);
    static unsigned long CALLBACK render_thread_proc(void* state);
    static LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    static void on_rst_changed(void* state);
public:
    void initialize();
    virtual int CALL CanConfigure();
    virtual int CALL Configure(int prop, void* data, size_t size);
    virtual int CALL CanConnect();
    virtual int CALL Connect(uint8_t pin, gpio_get_callback getter, gpio_set_callback setter, void* state);
    virtual int CALL CanUpdate();
    virtual int CALL Update();
    virtual int CALL CanPinChange();
    virtual int CALL PinChange(uint8_t pin, uint32_t value);
    virtual int CALL CanTransferBitsSPI();
    virtual int CALL TransferBitsSPI(uint8_t* data, size_t size_bits);
    virtual int CALL CanTransferBytesI2C();
    virtual int CALL TransferBytesI2C(const uint8_t* in, size_t in_size, uint8_t* out, size_t* in_out_out_size);
    virtual int CALL CanAttachLog();
    virtual int CALL AttachLog(log_callback logger,const char* prefix, uint8_t level);
    virtual int CALL Destroy();
};

typedef CALL void (*gpio_set_callback)(uint32_t value, void* state);
typedef CALL uint8_t (*gpio_get_callback)(void* state);
typedef CALL void (*log_callback)(const char* value);
#ifdef __cplusplus
extern "C" {
#endif
DLL_API int CALL CreateHardware(hardware_interface** out_hardware);
#ifdef __cplusplus
}  // __cplusplus defined.
#endif