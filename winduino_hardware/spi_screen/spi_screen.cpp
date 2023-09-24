
#define UNICODE
#if defined(UNICODE) && !defined(_UNICODE)
#define _UNICODE
#elif defined(_UNICODE) && !defined(UNICODE)
#define UNICODE
#endif

#include "spi_screen.h"

#include <Windows.h>
#include <d2d1.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "spi_screen_lib.h"

#pragma comment(lib, "d2d1.lib")

#define TFT_CASET 0x2A
#define TFT_PASET 0x2B
#define TFT_RAMWR 0x2C
#define TFT_RAMRD 0x2E

static struct {
    uint16_t width;
    uint16_t height;
} screen_size = {320, 240};
static struct {
    int16_t x;
    int16_t y;
} screen_offsets = {0, 0};

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

typedef enum states {
    STATE_INITIAL = 0,
    STATE_IGNORING,
    STATE_COLSET1,
    STATE_COLSET2,
    STATE_ROWSET1,
    STATE_ROWSET2,
    STATE_WRITE,
    STATE_READ
} states_t;
static states_t st = STATE_INITIAL;
bool bkl_low = false;
static uint8_t colset = TFT_CASET;
static uint8_t rowset = TFT_PASET;
static uint8_t write = TFT_RAMWR;
static uint8_t read = TFT_RAMRD;
static uint16_t col_start = 0, col_end = 0, row_start = 0, row_end = 0;
static int cmd = -1;
static uint8_t in_byte = 0;
static int bit_count = 0;
static input_t cs;
static input_t dc;
static input_t rst;
static input_t bkl;
static uint8_t* frame_buffer;
static uint16_t data_word = 0;
static size_t bytes_written = 0, bytes_read = 0;
static uint16_t column = 0, row = 0;
static int offset = 0;
static bool can_configure = true;
static HANDLE render_mutex = NULL;
static HANDLE render_thread = NULL;
static HANDLE quit_event = NULL;
static int reset_state = false;
static HANDLE screen_ready = NULL;
static log_callback logger = NULL;
static HMODULE hInstance = NULL;
static bool in_pixel_transfer = false;
static volatile DWORD render_changed = 1;
// directX stuff
static ID2D1HwndRenderTarget* render_target = nullptr;
static ID2D1Factory* d2d_factory = nullptr;
static ID2D1Bitmap* render_bitmap = nullptr;

HWND hwnd_screen = nullptr;
static bool created_wndcls = false;
static void logfmt(const char* format, ...) {
    if (logger == nullptr) {
        return;
    }
    char loc_buf[1024];
    char* temp = loc_buf;
    va_list arg;
    va_list copy;
    va_start(arg, format);
    va_copy(copy, arg);
    int len = vsnprintf(temp, sizeof(loc_buf), format, copy);
    va_end(copy);
    if (len < 0) {
        va_end(arg);
        return;
    }
    if (len >= (int)sizeof(loc_buf)) {  // comparation of same sign type for the compiler
        temp = (char*)malloc(len + 1);
        if (temp == NULL) {
            va_end(arg);
            return;
        }
        len = vsnprintf(temp, len + 1, format, arg);
    }
    va_end(arg);
    logger(temp);
    if (temp != loc_buf) {
        free(temp);
    }
    return;
}
static LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    if (uMsg == WM_CLOSE) {
        SetEvent(quit_event);
    }
    return DefWindowProcW(hWnd, uMsg, wParam, lParam);
}

static DWORD render_thread_proc(void* state);
static DWORD window_thread_proc(void* state) {
    HRESULT hr;
    ResetEvent(screen_ready);

    if (frame_buffer == nullptr) {
        frame_buffer = (uint8_t*)malloc(4 * screen_size.width * screen_size.height);
        if (frame_buffer == nullptr) {
            return 1;
        }
    }
    if (render_mutex == nullptr) {
        if (!created_wndcls) {
            WNDCLASSW wc;
            wc.style = CS_HREDRAW | CS_VREDRAW;
            wc.lpfnWndProc = WindowProc;
            wc.cbClsExtra = 0;
            wc.cbWndExtra = 0;
            wc.hInstance = hInstance;  // GetModuleHandle(NULL);
            wc.hbrBackground = NULL;
            wc.lpszMenuName = NULL;
            wc.hIcon = NULL;
            wc.hCursor = LoadCursor(NULL, IDC_ARROW);
            wc.lpszClassName = L"spi_screen";
            RegisterClassW(&wc);
            created_wndcls = true;
        }
        if (quit_event == NULL) {
            // for signalling when to exit
            quit_event = CreateEventW(
                NULL,         // default security attributes
                TRUE,         // manual-reset event
                FALSE,        // initial state is nonsignaled
                L"QuitEvent"  // object name
            );
            if (quit_event == NULL) {
                return 1;
            }
        }
        if (d2d_factory == nullptr) {
            // start DirectX
            hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &d2d_factory);
            if (!SUCCEEDED(hr)) {
                return 1;
            }
        }
        RECT r = {0, 0, screen_size.width, screen_size.height};
        AdjustWindowRectEx(&r, WS_CAPTION | WS_BORDER, FALSE, WS_EX_TOOLWINDOW);
        hwnd_screen = CreateWindowExW(WS_EX_TOOLWINDOW, L"spi_screen", L"SPI Screen", WS_CAPTION | WS_BORDER, CW_USEDEFAULT, CW_USEDEFAULT, r.right - r.left, r.bottom - r.top, NULL, NULL, /* GetModuleHandleW(NULL)*/ hInstance, NULL);
        if (hwnd_screen == nullptr) {
            return 1;
        }
        if (render_target == nullptr) {
            RECT rc;
            GetClientRect(hwnd_screen, &rc);
            D2D1_SIZE_U size = D2D1::SizeU(
                (rc.right - rc.left),
                rc.bottom - rc.top);

            hr = d2d_factory->CreateHwndRenderTarget(
                D2D1::RenderTargetProperties(),
                D2D1::HwndRenderTargetProperties(hwnd_screen, size),
                &render_target);

            if (!SUCCEEDED(hr)) {
                return 1;
            }
        }
        if (render_bitmap == nullptr) {
            // initialize the render bitmap
            D2D1_SIZE_U size = {0};
            D2D1_BITMAP_PROPERTIES props;
            render_target->GetDpi(&props.dpiX, &props.dpiY);
            D2D1_PIXEL_FORMAT pixelFormat = D2D1::PixelFormat(
                DXGI_FORMAT_B8G8R8A8_UNORM,
                D2D1_ALPHA_MODE_IGNORE);
            props.pixelFormat = pixelFormat;
            size.width = screen_size.width;
            size.height = screen_size.height;

            hr = render_target->CreateBitmap(size,
                                             props,
                                             &render_bitmap);
            if (!SUCCEEDED(hr)) {
                return 1;
            }
        }
        if (render_mutex == NULL) {
            render_mutex = CreateMutexW(NULL, FALSE, NULL);
            if (render_mutex == NULL) {
                return 1;
            }
        }
        if (render_thread == NULL) {
            render_thread = CreateThread(NULL, 4000, render_thread_proc, NULL, 0, NULL);
            if (render_thread == NULL) {
                return 1;
            }
        }
        ShowWindow(hwnd_screen, SW_SHOW);
        UpdateWindow(hwnd_screen);
    }
    SetEvent(screen_ready);
    bool quit = false;
    while (!quit) {
        DWORD result = 0;
        MSG msg = {0};
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) {
                quit = true;
                break;
            }
            // handle out of band
            // window messages here
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        if (WAIT_OBJECT_0 == WaitForSingleObject(quit_event, 0)) {
            quit = true;
        }
    }
    ResetEvent(screen_ready);
    if (quit_event != NULL) {
        SetEvent(quit_event);
    }
    if (render_thread != NULL) {
        CloseHandle(render_thread);
    }
    if (render_bitmap != nullptr) {
        render_bitmap->Release();
        render_bitmap = nullptr;
    }
    if (render_target != nullptr) {
        render_target->Release();
        render_target = nullptr;
    }
    if (hwnd_screen != NULL) {
        DestroyWindow(hwnd_screen);
        hwnd_screen = NULL;
    }
    if (render_mutex != NULL) {
        CloseHandle(render_mutex);
    }
    if (quit_event != NULL) {
        CloseHandle(quit_event);
    }
    return 0;
}
static uint8_t process_byte(uint8_t val) {
    if (can_configure) return val;
    if (dc.value()) {
        if (st == STATE_INITIAL) {
            return 0;
        }
        switch (st) {
            case STATE_WRITE:
                switch (offset) {
                    case 0:
                        data_word = val << 8;
                        offset=1;
                        ++bytes_written;
                        break;
                    case 1:
                        if (in_pixel_transfer) {
                            data_word |= val;
                            int x = column- screen_offsets.x;
                            int y = row - screen_offsets.y;
                            if (x >= 0 && x < screen_size.width && y >= 0 && y < screen_size.height) {
                                uint8_t r = ((float)((data_word >> 11) & 31) / 31.0f) * 255;
                                uint8_t g = ((float)((data_word >> 5) & 63) / 63.0f) * 255;
                                uint8_t b = ((float)((data_word >> 0) & 31) / 31.0f) * 255;

                                uint8_t* p = frame_buffer + ((x + screen_size.width * y) * 4);
                                *p++ = b;
                                *p++ = g;
                                *p++ = r;
                                *p = 0xFF;
                                InterlockedExchange(&render_changed,1);
                            }
                            ++column;
                            if (column > col_end) {
                                ++row;
                                column = col_start;
                                if (row > row_end) {
                                    if (in_pixel_transfer) {
                                        ReleaseMutex(render_mutex);
                                    }
                                    in_pixel_transfer = false;
                                    st = STATE_IGNORING;
                                    break;
                                }
                            }
                            ++bytes_written;
                            offset = 0;
                            if (bytes_written == sizeof(uint16_t) * (row_end - row_start + 1) * (col_start - col_end + 1)) {
                                if (in_pixel_transfer) {
                                    ReleaseMutex(render_mutex);
                                }
                                in_pixel_transfer = false;
                                st = STATE_IGNORING;
                            }
                        }
                        break;
                }

                break;

            case STATE_COLSET1:
                if (offset == 0) {
                    data_word = val << 8;
                    offset = 1;
                } else {
                    data_word |= val;
                    offset = 0;
                    col_start = data_word;
                    st = STATE_COLSET2;
                }
                break;
            case STATE_COLSET2:
                if (offset == 0) {
                    data_word = val << 8;
                    offset = 1;
                } else {
                    data_word |= val;
                    offset = 0;
                    col_end = data_word;
                    st = STATE_IGNORING;
                    //logfmt("col_start: %d, col_end: %d",col_start,col_end);
                }
                break;
            case STATE_ROWSET1:
                if (offset == 0) {
                    data_word = val << 8;
                    offset = 1;
                } else {
                    data_word |= val;
                    offset = 0;
                    row_start = data_word;
                    st = STATE_ROWSET2;
                }
                break;
            case STATE_ROWSET2:
                if (offset == 0) {
                    data_word = val << 8;
                    offset = 1;
                } else {
                    data_word |= val;
                    offset = 0;
                    row_end = data_word;
                    st = STATE_IGNORING;
                    //logfmt("row_start: %d, row_end: %d",row_start,row_end);
                }
                break;
        }
    } else {
        DWORD wr;
        bytes_written = 0;
        bytes_read = 0;
        cmd = val;
        if (cmd == colset) {
            logfmt("cmd: COLSET");
        } else if (cmd == rowset) {
            logfmt("cmd: ROWSET");
        } else if (cmd == write) {
            logfmt("cmd: RAMWR");
        } else {
            logfmt("cmd: %02X", cmd);
        }
        if (cmd == colset) {
            if (in_pixel_transfer) {
                ReleaseMutex(render_mutex);
            }
            in_pixel_transfer = false;
            data_word = 0;
            offset = 0;
            st = STATE_COLSET1;
        } else if (cmd == rowset) {
            if (in_pixel_transfer) {
                ReleaseMutex(render_mutex);
            }
            in_pixel_transfer = false;
            data_word = 0;
            offset = 0;
            st = STATE_ROWSET1;
        } else if (cmd == write) {
            if (in_pixel_transfer) {
                ReleaseMutex(render_mutex);
            }
            offset = 0;
            data_word = 0;
            column = col_start;
            row = row_start;
            in_pixel_transfer = false;
            wr = WaitForSingleObject(screen_ready, INFINITE);
            if (WAIT_OBJECT_0 == wr) {  // no time-out interval)
                wr = WaitForSingleObject(
                    render_mutex,  // handle to mutex
                    INFINITE);
                if (WAIT_OBJECT_0 == wr) {  // no time-out interval)
                    logfmt("Ready to write");
                    in_pixel_transfer = true;
                    st = STATE_WRITE;
                } else {
                    in_pixel_transfer = false;
                }
            } else {
                logfmt("Error writing pixels (unable to wait screen ready): %x", GetLastError());
            }
        } else if (cmd == read) {
            if (in_pixel_transfer) {
                ReleaseMutex(render_mutex);
            }
            offset = 0;
            data_word = 0;
            column = col_start;
            row = row_start;
            in_pixel_transfer = false;
            wr = WaitForSingleObject(screen_ready, INFINITE);
            if (WAIT_OBJECT_0 == wr) {  // no time-out interval)
                wr = WaitForSingleObject(
                    render_mutex,  // handle to mutex
                    INFINITE);
                if (WAIT_OBJECT_0 == wr) {  // no time-out interval)
                    in_pixel_transfer = true;
                    st = STATE_READ;
                } else {
                    in_pixel_transfer = false;
                }
            }
        } else {
            if (in_pixel_transfer) {
                ReleaseMutex(render_mutex);
            }
            in_pixel_transfer = false;
            st = STATE_IGNORING;
        }
    }
    return val;
}
static DWORD render_thread_proc(void* state) {
    bool quit = false;
    while (!quit) {
        
        if (0!=render_changed && frame_buffer && render_target && render_bitmap) {
            if (WAIT_OBJECT_0 == WaitForSingleObject(
                                     render_mutex,  // handle to mutex
                                     INFINITE)) {   // no time-out interval)
                render_bitmap->CopyFromMemory(NULL, frame_buffer, 4 * screen_size.width);
                render_target->BeginDraw();
                D2D1_RECT_F rect_dest = {
                    0,
                    0,
                    (float)screen_size.width,
                    (float)screen_size.height};
                render_target->DrawBitmap(render_bitmap,
                                          rect_dest, 1.0f, D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR, NULL);
                render_target->EndDraw();
                ReleaseMutex(render_mutex);
                InterlockedExchange(&render_changed,0);
            }
        }
        if (WAIT_OBJECT_0 == WaitForSingleObject(quit_event, 0)) {
            quit = true;
        }
    }
    return 0;
}
static void on_rst_changed(void* state) {
    if (reset_state == 2 && !rst.value()) {
        reset_state = 0;
        if (quit_event != NULL) {
            SetEvent(quit_event);
        }
    }
    if (rst.value()) {
        ++reset_state;
    }
    if (reset_state == 2) {
        HRESULT hr;
        if (screen_ready == NULL) {
            screen_ready = CreateEventW(
                NULL,           // default security attributes
                TRUE,           // manual-reset event
                FALSE,          // initial state is nonsignaled
                L"ScreenReady"  // object name
            );
            if (screen_ready == NULL) {
                logfmt("unable to create screen_ready handle");
            }
        }
        if (quit_event == NULL) {
            CreateThread(NULL, 4000, window_thread_proc, NULL, 0, NULL);
        }
    }
}
int CALL Configure(int prop, void* data, size_t size) {
    if (!can_configure) {
        return 2;
    }
    switch (prop) {
        case SPI_SCREEN_PROP_RESOLUTION:
            if (size == sizeof(screen_size)) {
                memcpy(&screen_size, data, size);
                return 0;
            }
            break;
        case SPI_SCREEN_PROP_BKL_LOW:
            if (size = sizeof(int)) {
                int v = *((int*)data);
                bkl_low = (v != 0);
                return 0;
            }
            break;
        case SPI_SCREEN_PROP_COLSET:
            if (size == sizeof(uint8_t)) {
                colset = *((uint8_t*)data);
                return 0;
            }
            break;
        case SPI_SCREEN_PROP_ROWSET:
            if (size == sizeof(uint8_t)) {
                rowset = *((uint8_t*)data);
                return 0;
            }
            break;
        case SPI_SCREEN_PROP_WRITE:
            if (size == sizeof(uint8_t)) {
                write = *((uint8_t*)data);
                return 0;
            }
            break;
        case SPI_SCREEN_PROP_READ:
            if (size == sizeof(uint8_t)) {
                read = *((uint8_t*)data);
                return 0;
            }
            break;
        case SPI_SCREEN_PROP_OFFSETS:
            if (size == sizeof(screen_offsets)) {
                memcpy(&screen_offsets, data, size);
                return 0;
            }
            break;
        default:
            break;
    }
    return 1;
}
int CALL Connect(uint8_t pin, gpio_get_callback getter, gpio_set_callback setter, void* state) {
    switch (pin) {
        case SPI_SCREEN_PIN_CS:
            if (getter == nullptr) {
                return 1;
            }
            cs.read = getter;
            cs.read_state = state;
            break;
        case SPI_SCREEN_PIN_DC:
            if (getter == nullptr) {
                return 1;
            }
            dc.read = getter;
            dc.read_state = state;
            break;
        case SPI_SCREEN_PIN_RST:
            if (getter == nullptr) {
                return 1;
            }
            rst.read = getter;
            rst.read_state = state;
            rst.on_change_callback = on_rst_changed;
            break;
        case SPI_SCREEN_PIN_BKL:
            if (getter == nullptr) {
                return 1;
            }
            bkl.read = getter;
            bkl.read_state = state;
            break;
        default:
            return 1;
    }
    can_configure = false;
    return 0;
}
// int CALL Update() {
//     return 0;
// }
int CALL PinChange(uint8_t pin, uint32_t value) {
    switch (pin) {
        case SPI_SCREEN_PIN_CS:
            cs.on_changed(value);
            break;
        case SPI_SCREEN_PIN_DC:
            dc.on_changed(value);
            break;
        case SPI_SCREEN_PIN_RST:
            rst.on_changed(value);
            break;
        case SPI_SCREEN_PIN_BKL:
            bkl.on_changed(value);
            break;
        default:
            return 1;
    }
    return 0;
}
int CALL AttachLog(log_callback logger) {
    ::logger = logger;
    return logger == NULL;
}
int CALL TransferBitsSPI(uint8_t* data, size_t size_bits) {
    if (cs.value()) {
        return 0;
    }
    size_t full_bytes = size_bits / 8;
    while (full_bytes--) {
        *data = process_byte(*data);
        data++;
    }
    return 0;
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {
    switch (fdwReason) {
        case DLL_PROCESS_ATTACH:
            hInstance = hinstDLL;
            break;

        case DLL_PROCESS_DETACH:
            if (quit_event != NULL) {
                SetEvent(quit_event);
            }
            break;

        case DLL_THREAD_ATTACH:
            break;

        case DLL_THREAD_DETACH:
            break;
    }

    return TRUE;
}