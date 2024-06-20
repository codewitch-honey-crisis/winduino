
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

static HMODULE hInstance = NULL;
void spi_screen::initialize() {
    screen_size= {320, 240};
    screen_offsets= {0,0};
    screen_st = SCREEN_STATE_INITIAL;
    touch_address = 0x38;
    touch_factory_mode = false;
    touch_current_reg = -1;
    touch_threshhold = 0x80;
    bkl_low = false;
    colset= TFT_CASET;
    rowset= TFT_PASET;
    write= TFT_RAMWR;
    read = TFT_RAMRD;
    col_start=0;
    col_end=0;
    row_start=0;
    row_end=0;
    int cmd= -1;
    uint8_t in_byte = 0;
    int bit_count = 0;
    frame_buffer=NULL;
    data_word = 0;
    bytes_written=0;
    bytes_read=0;
    column=0;
    row=0;
    offset=0;
    can_configure = true;
    render_mutex = NULL;
    render_thread= NULL;
    quit_event= NULL;
    reset_state = false;
    screen_ready = NULL;
    touch_mutex = NULL;
    logger = NULL;
    in_pixel_transfer = false;
    render_changed = 1;
}
void spi_screen::logfmt(uint8_t level,const char* format, ...) {
    if (logger == nullptr) {
        return;
    }
    if(level>log_level) {
        return;
    }
    char loc_buf[1024];
    char* temp = loc_buf;
    if(log_prefix!=nullptr) {
        strcpy(temp,log_prefix);
        strcpy(temp+strlen(temp),": ");
    } else {
        *temp=0;
    }
    va_list arg;
    va_list copy;
    va_start(arg, format);
    va_copy(copy, arg);
    int len = vsnprintf(temp+strlen(temp), sizeof(loc_buf), format, copy);
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
// updates the window title with the FPS and any mouse info
void spi_screen::update_title(HWND hwnd) {
    wchar_t wsztitle[64];
    uint16_t f;
    wcscpy(wsztitle, L"SPI Screen");
    if(WAIT_OBJECT_0==WaitForSingleObject(touch_mutex,INFINITE)) {
        if (mouse_state) {
            wcscat(wsztitle, L" (");
            f = mouse_loc.x;
            _itow((int)f, wsztitle + wcslen(wsztitle), 10);
            wcscat(wsztitle, L", ");
            f = mouse_loc.y;
            _itow((int)f, wsztitle + wcslen(wsztitle), 10);
            wcscat(wsztitle, L")");
        }
        ReleaseMutex(touch_mutex);
        SetWindowTextW(hwnd, wsztitle);
    }
    
}
LRESULT spi_screen::WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    spi_screen* st =  (spi_screen*)GetWindowLongPtrW(hWnd,GWLP_USERDATA);
    if(st!=NULL) {
        if (uMsg == WM_CLOSE) {
            SetEvent(st->quit_event);
        }
    }
    return DefWindowProcW(hWnd, uMsg, wParam, lParam);
}

DWORD spi_screen::window_thread_proc(void* state) {
    spi_screen* st = (spi_screen*)state;
    HRESULT hr;
    ResetEvent(st->screen_ready);

    if (st->frame_buffer == nullptr) {
        st->frame_buffer = (uint8_t*)malloc(4 * st->screen_size.width * st->screen_size.height);
        if (st->frame_buffer == nullptr) {
            return 1;
        }
    }
    if(st->touch_mutex==nullptr) {
        st->touch_mutex = CreateMutex(NULL, FALSE, NULL);
        if(st->touch_mutex==NULL) {
            return 1;
        }
    }
    if (st->render_mutex == nullptr) {
        if (!st->created_wndcls) {
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
            st->created_wndcls = true;
        }
        if (st->quit_event == NULL) {
            // for signalling when to exit
            st->quit_event = CreateEventW(
                NULL,         // default security attributes
                TRUE,         // manual-reset event
                FALSE,        // initial state is nonsignaled
                L"QuitEvent"  // object name
            );
            if (st->quit_event == NULL) {
                return 1;
            }
        }
        if (st->d2d_factory == nullptr) {
            // start DirectX
            hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &st->d2d_factory);
            if (!SUCCEEDED(hr)) {
                return 1;
            }
        }
        RECT r = {0, 0, st->screen_size.width, st->screen_size.height};
        AdjustWindowRectEx(&r, WS_CAPTION | WS_BORDER, FALSE, WS_EX_TOOLWINDOW);
        st->hwnd_screen = CreateWindowExW(WS_EX_TOOLWINDOW | WS_EX_APPWINDOW, L"spi_screen", L"SPI Screen", WS_CAPTION | WS_BORDER, CW_USEDEFAULT, CW_USEDEFAULT, r.right - r.left, r.bottom - r.top, NULL, NULL, /* GetModuleHandleW(NULL)*/ hInstance, NULL);
        if (st->hwnd_screen == nullptr) {
            return 1;
        }
        SetWindowLongPtrW(st->hwnd_screen,GWLP_USERDATA,(LONG_PTR)st);
        if (st->render_target == nullptr) {
            RECT rc;
            GetClientRect(st->hwnd_screen, &rc);
            D2D1_SIZE_U size = D2D1::SizeU(
                (rc.right - rc.left),
                rc.bottom - rc.top);

            hr = st->d2d_factory->CreateHwndRenderTarget(
                D2D1::RenderTargetProperties(),
                D2D1::HwndRenderTargetProperties(st->hwnd_screen, size),
                &st->render_target);

            if (!SUCCEEDED(hr)) {
                return 1;
            }
        }
        if (st->render_bitmap == nullptr) {
            // initialize the render bitmap
            D2D1_SIZE_U size = {0};
            D2D1_BITMAP_PROPERTIES props;
            st->render_target->GetDpi(&props.dpiX, &props.dpiY);
            D2D1_PIXEL_FORMAT pixelFormat = D2D1::PixelFormat(
                DXGI_FORMAT_B8G8R8A8_UNORM,
                D2D1_ALPHA_MODE_IGNORE);
            props.pixelFormat = pixelFormat;
            size.width = st->screen_size.width;
            size.height = st->screen_size.height;

            hr = st->render_target->CreateBitmap(size,
                                             props,
                                             &st->render_bitmap);
            if (!SUCCEEDED(hr)) {
                return 1;
            }
        }
        if (st->render_mutex == NULL) {
            st->render_mutex = CreateMutexW(NULL, FALSE, NULL);
            if (st->render_mutex == NULL) {
                return 1;
            }
        }
        if (st->render_thread == NULL) {
            st->render_thread = CreateThread(NULL, 4000, render_thread_proc,st, 0, NULL);
            if (st->render_thread == NULL) {
                return 1;
            }
        }
        ShowWindow(st->hwnd_screen, SW_SHOW);
        UpdateWindow(st->hwnd_screen);
    }
    SetEvent(st->screen_ready);
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
            if (msg.message == WM_LBUTTONDOWN) {
                if(LOWORD(msg.lParam)<st->screen_size.width && 
                    HIWORD(msg.lParam)<st->screen_size.height) {
                    SetCapture(msg.hwnd);
                    
                    if (WAIT_OBJECT_0 == WaitForSingleObject(
                                            st->touch_mutex,  // handle to mutex
                                            INFINITE)) {   // no time-out interval)
                        st->old_mouse_state = st->mouse_state;
                        st->mouse_state = 1;
                        st->mouse_loc.x = LOWORD(msg.lParam);
                        if(st->mouse_loc.x<0 || (st->mouse_loc.x & 0x8000)) st->mouse_loc.x=0;
                        if(st->mouse_loc.x>=st->screen_size.width) st->mouse_loc.x = st->screen_size.width-1;
                        st->mouse_loc.y = HIWORD(msg.lParam);
                        if(st->mouse_loc.y<0 || (st->mouse_loc.y & 0x8000)) st->mouse_loc.y=0;
                        if(st->mouse_loc.y>=st->screen_size.height) st->mouse_loc.y = st->screen_size.height-1;
                        st->mouse_req = 1;
                        ReleaseMutex(st->touch_mutex);
                    }
                    st->update_title(msg.hwnd);
                }
            }
            if (msg.message == WM_MOUSEMOVE) {
                if (WAIT_OBJECT_0 == WaitForSingleObject(
                                         st->touch_mutex,  // handle to mutex
                                         INFINITE)) {   // no time-out interval)
                    if (st->mouse_state == 1 && MK_LBUTTON == msg.wParam) {
                        st->mouse_req = 1;
                        st->mouse_loc.x = LOWORD(msg.lParam);
                        if(st->mouse_loc.x<0 || (st->mouse_loc.x & 0x8000)) st->mouse_loc.x=0;
                        if(st->mouse_loc.x>=st->screen_size.width) st->mouse_loc.x = st->screen_size.width-1;
                        st->mouse_loc.y = HIWORD(msg.lParam);
                        if(st->mouse_loc.y<0 || (st->mouse_loc.y & 0x8000)) st->mouse_loc.y=0;
                        if(st->mouse_loc.y>=st->screen_size.height) st->mouse_loc.y = st->screen_size.height-1;
                    }
                    ReleaseMutex(st->touch_mutex);
                }
                st->update_title(msg.hwnd);
            }
            if (msg.message == WM_LBUTTONUP) {
                ReleaseCapture();
                if (WAIT_OBJECT_0 == WaitForSingleObject(
                                         st->touch_mutex,  // handle to mutex
                                         INFINITE)) {   // no time-out interval)

                    st->old_mouse_state = st->mouse_state;
                    st->mouse_req = 1;
                    st->mouse_state = 0;
                    st->mouse_loc.x = LOWORD(msg.lParam);
                    if(st->mouse_loc.x<0 || (st->mouse_loc.x & 0x8000)) st->mouse_loc.x=0;
                    if(st->mouse_loc.x>=st->screen_size.width) st->mouse_loc.x = st->screen_size.width-1;
                    st->mouse_loc.y = HIWORD(msg.lParam);
                    if(st->mouse_loc.y<0 || (st->mouse_loc.y & 0x8000)) st->mouse_loc.y=0;
                    if(st->mouse_loc.y>=st->screen_size.height) st->mouse_loc.y = st->screen_size.height-1;
                    ReleaseMutex(st->touch_mutex);
                }
                st->update_title(msg.hwnd);
            }
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        if (WAIT_OBJECT_0 == WaitForSingleObject(st->quit_event, 0)) {
            quit = true;
        }
    }
    ResetEvent(st->screen_ready);
    if (st->quit_event != NULL) {
        SetEvent(st->quit_event);
    }
    if (st->render_thread != NULL) {
        CloseHandle(st->render_thread);
    }
    if (st->render_bitmap != nullptr) {
        st->render_bitmap->Release();
        st->render_bitmap = nullptr;
    }
    if (st->render_target != nullptr) {
        st->render_target->Release();
        st->render_target = nullptr;
    }
    if (st->hwnd_screen != NULL) {
        DestroyWindow(st->hwnd_screen);
        st->hwnd_screen = NULL;
    }
    if (st->render_mutex != NULL) {
        CloseHandle(st->render_mutex);
        st->render_mutex = NULL;
    }
    if (st->quit_event != NULL) {
        CloseHandle(st->quit_event);
        st->quit_event = NULL;
    }
    return 0;
}
uint8_t spi_screen::process_byte_spi(uint8_t val) {
    if (can_configure) return val;
    if (dc.value()) {
        if (screen_st == SCREEN_STATE_INITIAL) {
            return 0;
        }
        int x, y;
        uint8_t* p;
        switch (screen_st) {
            case SCREEN_STATE_WRITE:
                switch (offset) {
                    case 0:
                        data_word = val << 8;
                        offset = 1;
                        ++bytes_written;
                        break;
                    case 1:

                        data_word |= val;
                        x = column - screen_offsets.x;
                        y = row - screen_offsets.y;
                        if (in_pixel_transfer && x >= 0 && x < screen_size.width && y >= 0 && y < screen_size.height) {
                            uint8_t r = ((float)((data_word >> 11) & 31) / 31.0f) * 255;
                            uint8_t g = ((float)((data_word >> 5) & 63) / 63.0f) * 255;
                            uint8_t b = ((float)((data_word >> 0) & 31) / 31.0f) * 255;

                            uint8_t* p = frame_buffer + ((x + screen_size.width * y) * 4);
                            *p++ = b;
                            *p++ = g;
                            *p++ = r;
                            *p = 0xFF;
                            InterlockedExchange(&render_changed, 1);
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
                                screen_st = SCREEN_STATE_IGNORING;
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
                            screen_st = SCREEN_STATE_IGNORING;
                        }

                        break;
                }

                break;
            case SCREEN_STATE_READ1:
                // toss this byte. quirk of the hardware
                screen_st = SCREEN_STATE_READ2;
                bytes_read = 0;
                break;
            case SCREEN_STATE_READ2:
                ++bytes_read;
                val = 0;
                x = column - screen_offsets.x;
                y = row - screen_offsets.y;

                if (in_pixel_transfer && x >= 0 && x < screen_size.width && y >= 0 && y < screen_size.height) {
                    switch (offset) {
                        case 0:
                            p = frame_buffer + ((x + screen_size.width * y) * 4) + 2;
                            val = (*p) & 0xFC;
                            offset = 1;

                            break;
                        case 1:
                            p = frame_buffer + ((x + screen_size.width * y) * 4) + 1;
                            val = (*p) & 0xFC;
                            offset = 2;
                            break;
                        case 2:
                            p = frame_buffer + ((x + screen_size.width * y) * 4) + 0;
                            val = (*p) & 0xFC;
                            offset = 0;
                            break;
                        default:
                            break;
                    }
                }
                if (bytes_read == sizeof(uint16_t) * (row_end - row_start + 1) * (col_start - col_end + 1)) {
                    if (in_pixel_transfer) {
                        ReleaseMutex(render_mutex);
                    }
                    in_pixel_transfer = false;
                    screen_st = SCREEN_STATE_IGNORING;
                }
                break;
            case SCREEN_STATE_COLSET1:
                if (offset == 0) {
                    data_word = val << 8;
                    offset = 1;
                } else {
                    data_word |= val;
                    offset = 0;
                    col_start = data_word;
                    screen_st = SCREEN_STATE_COLSET2;
                }
                break;
            case SCREEN_STATE_COLSET2:
                if (offset == 0) {
                    data_word = val << 8;
                    offset = 1;
                } else {
                    data_word |= val;
                    offset = 0;
                    col_end = data_word;
                    screen_st = SCREEN_STATE_IGNORING;
                    // logfmt("col_start: %d, col_end: %d",col_start,col_end);
                }
                break;
            case SCREEN_STATE_ROWSET1:
                if (offset == 0) {
                    data_word = val << 8;
                    offset = 1;
                } else {
                    data_word |= val;
                    offset = 0;
                    row_start = data_word;
                    screen_st = SCREEN_STATE_ROWSET2;
                }
                break;
            case SCREEN_STATE_ROWSET2:
                if (offset == 0) {
                    data_word = val << 8;
                    offset = 1;
                } else {
                    data_word |= val;
                    offset = 0;
                    row_end = data_word;
                    screen_st = SCREEN_STATE_IGNORING;
                    // logfmt("row_start: %d, row_end: %d",row_start,row_end);
                }
                break;
        }
    } else {
        DWORD wr;
        bytes_written = 0;
        bytes_read = 0;
        cmd = val;
        if (cmd == colset) {
            if (in_pixel_transfer) {
                ReleaseMutex(render_mutex);
            }
            in_pixel_transfer = false;
            data_word = 0;
            offset = 0;
            screen_st = SCREEN_STATE_COLSET1;
        } else if (cmd == rowset) {
            if (in_pixel_transfer) {
                ReleaseMutex(render_mutex);
            }
            in_pixel_transfer = false;
            data_word = 0;
            offset = 0;
            screen_st = SCREEN_STATE_ROWSET1;
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
                    in_pixel_transfer = true;
                    screen_st = SCREEN_STATE_WRITE;
                } else {
                    in_pixel_transfer = false;
                }
            } else {
                logfmt(1,"Error writing pixels (unable to wait screen ready): %x", GetLastError());
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
                    screen_st = SCREEN_STATE_READ1;
                } else {
                    in_pixel_transfer = false;
                }
            }
        } else {
            if (in_pixel_transfer) {
                ReleaseMutex(render_mutex);
            }
            in_pixel_transfer = false;
            screen_st = SCREEN_STATE_IGNORING;
        }
    }
    return val;
}
unsigned long __stdcall spi_screen::render_thread_proc(void* state) {
    spi_screen* st = (spi_screen*)state;
    bool quit = false;
    while (!quit) {
        if (0 != st->render_changed && st->frame_buffer && st->render_target && st->render_bitmap) {
            if (WAIT_OBJECT_0 == WaitForSingleObject(
                                     st->render_mutex,  // handle to mutex
                                     INFINITE)) {   // no time-out interval)
                st->render_bitmap->CopyFromMemory(NULL, st->frame_buffer, 4 * st->screen_size.width);
                st->render_target->BeginDraw();
                D2D1_RECT_F rect_dest = {
                    0,
                    0,
                    (float)st->screen_size.width,
                    (float)st->screen_size.height};
                st->render_target->DrawBitmap(st->render_bitmap,
                                          rect_dest, 1.0f, D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR, NULL);
                st->render_target->EndDraw();
                ReleaseMutex(st->render_mutex);
                InterlockedExchange(&st->render_changed, 0);
            }
        }
        if (WAIT_OBJECT_0 == WaitForSingleObject(st->quit_event, 0)) {
            quit = true;
        }
    }
    return 0;
}
void spi_screen::on_rst_changed(void* state) {
    spi_screen* st = (spi_screen*)state;
    if (st->reset_state == 2 && !st->rst.value()) {
        st->reset_state = 0;
        if (st->quit_event != NULL) {
            SetEvent(st->quit_event);
        }
    }
    if (st->rst.value()) {
        ++st->reset_state;
    }
    if (st->reset_state == 2) {
        HRESULT hr;
        if (st->screen_ready == NULL) {
            st->screen_ready = CreateEventW(
                NULL,           // default security attributes
                TRUE,           // manual-reset event
                FALSE,          // initial state is nonsignaled
                NULL  // object name
            );
            if (st->screen_ready == NULL) {
                st->logfmt(1,"Unable to create screen_ready handle");
            }
        }
        if (st->quit_event == NULL) {
            CreateThread(NULL, 4000, window_thread_proc, st, 0, NULL);
        }
    }
}
int CALL spi_screen::CanConfigure() {
    return 1;
}
int CALL spi_screen::Configure(int prop, void* data, size_t size) {
    if (!can_configure) {
        logfmt(1,"Invalid state for configuration");
        return 2;
    }
    switch (prop) {
        case SPI_SCREEN_PROP_RESOLUTION:
            if (size == sizeof(screen_size)) {
                memcpy(&screen_size, data, size);
                logfmt(3,"Resolution set to %dx%d",screen_size.width,screen_size.height);
                return 0;
            } 
            break;
        case SPI_SCREEN_PROP_BKL_LOW:
            if (size = sizeof(int)) {
                int v = *((int*)data);
                bkl_low = (v != 0);
                logfmt(3,"Backlight set on = %s",bkl_low?"low":"high");
                return 0;
            }
            break;
        case SPI_SCREEN_PROP_COLSET:
            if (size == sizeof(uint8_t)) {
                colset = *((uint8_t*)data);
                logfmt(3,"COLSET set to = %02X",colset);
                return 0;
            }
            break;
        case SPI_SCREEN_PROP_ROWSET:
            if (size == sizeof(uint8_t)) {
                rowset = *((uint8_t*)data);
                logfmt(3,"ROWSET set to = %02X",rowset);
                return 0;
            }
            break;
        case SPI_SCREEN_PROP_WRITE:
            if (size == sizeof(uint8_t)) {
                write = *((uint8_t*)data);
                logfmt(3,"RAMWR set to = %02X",write);
                return 0;
            }
            break;
        case SPI_SCREEN_PROP_READ:
            if (size == sizeof(uint8_t)) {
                read = *((uint8_t*)data);
                logfmt(3,"RAMRD set to = %02X",read);
                return 0;
            }
            break;
        case SPI_SCREEN_PROP_OFFSETS:
            if (size == sizeof(screen_offsets)) {
                memcpy(&screen_offsets, data, size);
                logfmt(3,"Offsets set to {%d,%d}",screen_offsets.x,screen_offsets.y);
                return 0;
            }
            break;
        default:
            break;
    }
    logfmt(1,"Configure: Unknown property or bad size");
    return 1;
}
int CALL spi_screen::CanConnect() {
    return 1;
}
int CALL spi_screen::Connect(uint8_t pin, gpio_get_callback getter, gpio_set_callback setter, void* state) {
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
            rst.on_change_callback_state = this;
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
int CALL spi_screen::CanUpdate() {
    return 0;
}
int CALL Update() {
    return 0;
}
int CALL spi_screen::CanPinChange() {
    return 1;
}

int CALL spi_screen::PinChange(uint8_t pin, uint32_t value) {
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
int CALL spi_screen::CanAttachLog() {
    return 1;
}

int CALL spi_screen::AttachLog(log_callback logger, const char* prefix, uint8_t level) {
    this->logger = logger;
    this->log_prefix = prefix;
    this->log_level = level;
    return logger == NULL;
}
int CALL spi_screen::CanTransferBitsSPI() {
    return 1;
}

int CALL spi_screen::TransferBitsSPI(uint8_t* data, size_t size_bits) {
    if (cs.value()) {
        return 0;
    }
    size_t full_bytes = size_bits / 8;
    while (full_bytes--) {
        *data = process_byte_spi(*data);
        data++;
    }
    return 0;
}
int CALL spi_screen::CanTransferBytesI2C() {
    return 1;
}

int CALL spi_screen::TransferBytesI2C(const uint8_t* in, size_t in_size, uint8_t* out, size_t* in_out_out_size) {
    
    //logfmt("addr: %X, in_size: %d, out_size: %d%s",*in&0x7f,in_size,*in_out_out_size,*in&0x80?" (read)":" (write)");

    if (in == nullptr) {
        return 1;
    }
    int val;
    size_t out_size = 0;
    touch_states_t touch_st = TOUCH_STATE_INITIAL;
    while(in_size) {
        switch (touch_st) {
            case TOUCH_STATE_INITIAL:
                touch_st = TOUCH_STATE_ADDRESS;
                // fall through
            case TOUCH_STATE_ADDRESS:
                if ((*in & 0x7F) != touch_address) {
                    //logfmt("ignoring data not addressed to FT6236 - address %02X",(*in & 0x7F));
                    return 0;  // not our message
                }
                if ((*in & 0x80)) {
                    touch_st = TOUCH_STATE_READ;
                } else {
                    touch_st = TOUCH_STATE_WRITE;
                }
                ++in;
                --in_size;
                if(touch_st!=TOUCH_STATE_READ) {
                    break;
                }
            case TOUCH_STATE_READ:
                if(touch_current_reg<0) {
                    // our I2C register was not set
                    return 2;
                }
                switch(touch_current_reg) {
                    case TOUCH_REG_XL:
                        if(in_out_out_size==nullptr || out==nullptr || *in_out_out_size<1) {
                            return 1;
                        }
                        if(WAIT_OBJECT_0==WaitForSingleObject(touch_mutex,INFINITE)) {
                            *out = mouse_loc.x & 0xFF;
                            ReleaseMutex(touch_mutex);
                        } else {
                            return 3;
                        }
                        ++out_size;
                        touch_st = TOUCH_STATE_ADDRESS;
                    break;
                    case TOUCH_REG_XH:
                        if(in_out_out_size==nullptr || out==nullptr || *in_out_out_size<1) {
                            return 1;
                        }
                        if(WAIT_OBJECT_0==WaitForSingleObject(touch_mutex,INFINITE)) {
                            *out = (mouse_loc.x & 0xFF00)>>8;
                            ReleaseMutex(touch_mutex);
                        } else {
                            return 3;
                        }
                        ++out_size;
                        touch_st = TOUCH_STATE_ADDRESS;
                    break;
                    case TOUCH_REG_YL:
                        if(in_out_out_size==nullptr || out==nullptr || *in_out_out_size<1) {
                            return 1;
                        }
                        if(WAIT_OBJECT_0==WaitForSingleObject(touch_mutex,INFINITE)) {
                            *out = mouse_loc.y & 0xFF;
                            ReleaseMutex(touch_mutex);
                        } else {
                            return 3;
                        }
                        ++out_size;
                        touch_st = TOUCH_STATE_ADDRESS;
                    break;
                    case TOUCH_REG_YH:
                        if(in_out_out_size==nullptr || out==nullptr || *in_out_out_size<1) {
                            return 1;
                        }
                        ++out_size;
                        
                        if(WAIT_OBJECT_0==WaitForSingleObject(touch_mutex,INFINITE)) {
                            *out = (mouse_loc.y & 0xFF00)>>8;
                            ReleaseMutex(touch_mutex);
                        } else {
                            return 3;
                        }
                        touch_st = TOUCH_STATE_ADDRESS;
                    break;
                    case TOUCH_REG_NUMTOUCHES:
                        if(in_out_out_size==nullptr || out==nullptr || *in_out_out_size<1) {
                            return 1;
                        }
                        ++out_size;
                        if(WAIT_OBJECT_0==WaitForSingleObject(touch_mutex,INFINITE)) {
                            *out = !!mouse_state;
                            ReleaseMutex(touch_mutex);
                        } else {
                            return 3;
                        }
                        touch_st = TOUCH_STATE_ADDRESS;
                    break;
                    case TOUCH_REG_THRESHHOLD:
                        if(in_out_out_size==nullptr || out==nullptr || *in_out_out_size<1) {
                            return 1;
                        }
                        ++out_size;
                        *out = touch_threshhold;
                        touch_st = TOUCH_STATE_ADDRESS;
                    break;
                    case TOUCH_REG_VENDID:
                        if(in_out_out_size==nullptr || out==nullptr || *in_out_out_size<1) {
                            return 1;
                        }
                        //logfmt("touch VENDID read");
                        *out  = FT6236_VENDID;
                        ++out_size;
                        touch_st = TOUCH_STATE_ADDRESS;
                    break;
                    case TOUCH_REG_CHIPID:
                        if(in_out_out_size==nullptr || out==nullptr || *in_out_out_size<1) {
                            return 1;
                        }
                        *out = FT6236_CHIPID;
                        ++out_size;
                        touch_st = TOUCH_STATE_ADDRESS;
                    break;
                    case TOUCH_REG_MODE:
                        if(in_out_out_size==nullptr || out==nullptr || *in_out_out_size<16) {
                            return 1; // invalid args
                        }
                        *out++ = 0; // no idea what this is supposed to be?
                        *out++ = 0; // ?
                        if(WAIT_OBJECT_0==WaitForSingleObject(touch_mutex,INFINITE)) {
                            *out++ = !!mouse_state;
                            *out++ = (mouse_loc.x & 0xFF00)>>8;
                            *out++ = mouse_loc.x & 0x00FF;
                            *out++ = (mouse_loc.y & 0xFF00)>>8;
                            *out++ = mouse_loc.y & 0x00FF;
                            // garbage for the second touch
                            // we don't support it
                            out_size+=16;
                            touch_st = TOUCH_STATE_ADDRESS;
                            ReleaseMutex(touch_mutex);
                        } else {
                            return 3;
                        }
                        
                    break;
                }
                break;
            case TOUCH_STATE_WRITE:
                //logfmt("touch state write");
                if(in_size<1) {
                    return 1;
                }
                //logfmt("touch set current register %02X",*in);
                touch_current_reg = *in++;
                --in_size;
                if(in_size) {
                    switch(touch_current_reg) {
                        case TOUCH_REG_THRESHHOLD:
                            if(in_size<1) {
                                return 1;
                            }
                            //logfmt("reg write threshold");
                            touch_threshhold = *in++;
                            --in_size;
                            break;
                        default:
                            touch_threshhold = *in++;
                            --in_size;
                            break;
                    }
                }
                touch_st = TOUCH_STATE_ADDRESS;
                break;
        }
        //logfmt("loop tail - in_size: %d",in_size);
    }
    if(out_size!=0) {
        *in_out_out_size = out_size;
    }
    return 0;    
}
int spi_screen::Update() {
    return 0;
}
int spi_screen::Destroy() {
    delete this;
    return 0;
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {
    switch (fdwReason) {
        case DLL_PROCESS_ATTACH:
            hInstance = hinstDLL;
            break;

        case DLL_PROCESS_DETACH:
           
            break;

        case DLL_THREAD_ATTACH:
            break;

        case DLL_THREAD_DETACH:
            break;
    }

    return TRUE;
}

int CALL CreateHardware(hardware_interface** out_hardware) {
    if(out_hardware==NULL) {
        return 1;
    }
    spi_screen* scr = new spi_screen();
    scr->initialize();
    if(scr==nullptr) {
        return 3;
    }
    *out_hardware = static_cast<hardware_interface*>(scr);
    return 0;
}