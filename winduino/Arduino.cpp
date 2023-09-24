#define UNICODE
#if defined(UNICODE) && !defined(_UNICODE)
    #define _UNICODE
#elif defined(_UNICODE) && !defined(UNICODE)
    #define UNICODE
#endif

/////////////////////////////////////////////////////
#include <assert.h>
#include <conio.h>
#include <d2d1.h>
#include <stdio.h>
#include <stdint.h>
#include <windows.h>

/////////////////////////////////////////////////////
#pragma comment(lib, "d2d1.lib")
/////////////////////////////////////////////////////

#include "Arduino.h"


typedef __cdecl void(*gpio_set_callback)(uint32_t value, void* state);
typedef __cdecl uint8_t(*gpio_get_callback)(void* state);
typedef __cdecl int(*hardware_configure_fn)(int prop, void* data, size_t size);
typedef __cdecl int(*hardware_connect_fn)(uint8_t pin, gpio_get_callback getter, gpio_set_callback setter, void* state);
typedef __cdecl int(*hardware_update_fn)(void);
typedef __cdecl int(*hardware_pin_change_fn)(uint8_t pin, uint32_t value);
typedef __cdecl int(*hardware_transfer_bits_spi_fn)(uint8_t* data, size_t size_bits);
typedef __cdecl int(*hardware_attach_log_fn)(hardware_log_callback);
typedef struct hardware_handle {
    HMODULE hmodule;
    hardware_configure_fn configure;
    hardware_connect_fn connect;
    hardware_update_fn update;
    hardware_pin_change_fn pin_change;
    hardware_transfer_bits_spi_fn transfer_bits_spi;
    hardware_attach_log_fn attach_log;
    hardware_handle* next;
} hardware_handle_t;
typedef struct hardware_connection {
    hardware_handle_t* hardware;
    uint8_t pin;
    hardware_connection* next;
} hardware_connection_t;
static bool is_isr = false;
typedef struct gpio {
    uint8_t id;
    HWND hwnd_text;
    int interrupt_mode;
    void (*interrupt_cb)(void);
    uint8_t mode;
    uint32_t value() const {
        return m_value;
    }
    void value(uint32_t value) {
        if(interrupt_mode==LOW) {
            if(m_value!=value) {
                m_value = value;
                notify_changed();
            }
            if(!value && interrupt_cb!=nullptr) {
                is_isr = true;
                interrupt_cb();
                is_isr = false;
            }
        } else if(value!=m_value) {
            switch(interrupt_mode) {
                case FALLING:
                if(!value && interrupt_cb!=nullptr) {
                    is_isr = true;
                    interrupt_cb();
                    is_isr = false;
                }
                break;
                case RISING:
                if(value && interrupt_cb!=nullptr) {
                    is_isr = true;
                    interrupt_cb();
                    is_isr = false;
                }
                break;
                case CHANGE:
                if(interrupt_cb!=nullptr) {
                    is_isr = true;
                    interrupt_cb();
                    is_isr = false;
                }
                break;
            }
            m_value = value;
            notify_changed();
        }
    }
    bool is_input() const {
        switch(mode) {
            case INPUT:
            case INPUT_PULLUP:
            case INPUT_PULLDOWN:
                return true;
            default:
                return false;
        }
    }
    bool is_output() const {
        switch(mode) {
            case OUTPUT:
            case OUTPUT_OPEN_DRAIN:
                return true;
            default:
                return false;
        }
    }
    bool connect(hardware_handle_t* hw,uint8_t pin) {
        if(hw==nullptr) {
            return false;
        }
        hardware_connection_t* con = new hardware_connection_t();
        if(con==nullptr) {
            return false;
        }
        if(0!=hw->connect(pin,get_pin,set_pin,this)) {
            delete con;
            return false;
        }
        
        con->next = nullptr;
        con->hardware = hw;
        con->pin = pin;
        if(m_connect_list==nullptr) {
            m_connect_list = con;
        } else {
            hardware_connection_t* p = m_connect_list;
            while(p!=nullptr) {
                if(p->next==nullptr) {
                    p->next = con;
                    break;
                }
                p=p->next;
            }
        }
        return true;
    }
    
private:
    uint32_t m_value;
    hardware_connection_t* m_connect_list;
    
    static uint8_t get_pin(void* state) {
        gpio* st = (gpio*)state;
        return st->value();
    }
    static void set_pin(uint32_t value, void* state) {
        gpio* st = (gpio*)state;
        st->value(value);
    }
    void notify_changed() {
        
        hardware_connection_t* p = m_connect_list;
        while(p!=nullptr) {
            if(p->hardware->pin_change!=nullptr) {
                p->hardware->pin_change(p->pin,m_value);
            }
            p=p->next;
        }
    }
} gpio_t;
static hardware_handle_t* hardware_head=nullptr;
static gpio_t gpios[256];


// so we can implement millis(), delay()
static LARGE_INTEGER start_time;
// frame counter
static volatile DWORD frames = 0;
// handles for windows
static HANDLE quit_event = NULL;
static HANDLE app_thread = NULL;
static HANDLE app_mutex = NULL;
static HWND hwnd_log;
static HWND hwnd_main;
static bool updating_gpios = false;

HMENU menu;
HMENU gpio_menu;
// flag to indicate quitting
static bool should_quit = false;
// directX stuff
static ID2D1HwndRenderTarget* render_target = nullptr;
static ID2D1Factory* d2d_factory = nullptr;
static ID2D1Bitmap* render_bitmap = nullptr;
// mouse mess
static struct { int x; int y; } mouse_loc;
static int mouse_state = 0;  // 0 = released, 1 = pressed
static int old_mouse_state = 0;
static int mouse_req = 0;

// updates the window title with the FPS and any mouse info
static void update_title(HWND hwnd) {
    wchar_t wsztitle[64];
    wcscpy(wsztitle, L"Winduino - ");
    DWORD f = frames;
    _itow((int)f, wsztitle + wcslen(wsztitle), 10);
    wcscat(wsztitle, L" FPS");

    if (mouse_state) {
        wcscat(wsztitle, L" (");
        f = mouse_loc.x;
        _itow((int)f, wsztitle + wcslen(wsztitle), 10);
        wcscat(wsztitle, L", ");
        f = mouse_loc.y;
        _itow((int)f, wsztitle + wcslen(wsztitle), 10);
        wcscat(wsztitle, L")");
    }
    SetWindowTextW(hwnd, wsztitle);
}
const char * pathToFileName(const char * path)
{
    size_t i = 0;
    size_t pos = 0;
    char * p = (char *)path;
    while(*p){
        i++;
        if(*p == '/' || *p == '\\'){
            pos = i;
        }
        p++;
    }
    return path+pos;
}
// this handles our main application loop
// plus rendering
static DWORD render_thread_proc(void* state) {
    
    bool quit = false;
    while (!quit) {
        hardware_handle_t* hw = hardware_head;
        while(hw!=nullptr) {
            if(hw->update!=nullptr) {
                hw->update();
            }
            hw=hw->next;
        }
        loop();
        if (render_target && render_bitmap) {
            if (WAIT_OBJECT_0 == WaitForSingleObject(
                                     app_mutex,  // handle to mutex
                                     INFINITE)) {   // no time-out interval)

                render_target->BeginDraw();
                D2D1_RECT_F rect_dest = {
                    0,
                    0,
                    (float)winduino_screen_size.width,
                    (float)winduino_screen_size.height};
                render_target->DrawBitmap(render_bitmap,
                                          rect_dest, 1.0f, D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR, NULL);
                render_target->EndDraw();
                ReleaseMutex(app_mutex);
                InterlockedIncrement(&frames);
            }
        }
        if (WAIT_OBJECT_0 == WaitForSingleObject(quit_event, 0)) {
            quit = true;
        }
    }
    return 0;
}
// the only message we care about for the
// main window is WM_CLOSE
static LRESULT CALLBACK WindowProcMain(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    if (uMsg == WM_CLOSE) {
        // inform all threads we're quitting
        SetEvent(quit_event);
        should_quit = true;
    }
   
    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}
static LRESULT CALLBACK WindowProcGpio(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    if(uMsg==WM_CREATE) {
        gpio_t& g = *(gpio_t*)((LPCREATESTRUCT) lParam)->lpCreateParams;
        SetWindowLongPtrW(hWnd,GWLP_USERDATA,(LONG_PTR)g.id);
        HWND hwnd_st_u, hwnd_ed_u;
        int x, w, y, h;
        y = 10; h = 20;
        x = 10; w = 50;
        hwnd_st_u = CreateWindowW(L"static", L"ST_U",
                              WS_CHILD | WS_VISIBLE | WS_TABSTOP,
                              x, y, w, h,
                              hWnd, (HMENU)(501),
                              (HINSTANCE) GetModuleHandle(NULL), NULL);
        SetWindowTextW(hwnd_st_u, L"Value:");

        x += w; w = 60;
        hwnd_ed_u = CreateWindowW(L"edit", L"",
                                    WS_CHILD | WS_VISIBLE | WS_TABSTOP | (WS_DISABLED*(!g.is_input()))
                                    | ES_LEFT | WS_BORDER,
                                    x, y, w, h,
                                    hWnd, (HMENU)(502),
                                    (HINSTANCE) GetModuleHandle(NULL), NULL);
        wchar_t val[64];
        if(g.value()==HIGH) {
            wcscpy(val,L"HIGH");
        } else if(g.value()==LOW) {
            wcscpy(val,L"LOW");
        } else {
            _itow(g.value(),val,10);
        }
        SetWindowTextW(hwnd_ed_u, val);
        g.hwnd_text = hwnd_ed_u;
        
    }
    if(uMsg==WM_COMMAND) {
        if(HIWORD (wParam) == EN_CHANGE) {
            uint8_t gpio = GetWindowLongPtrW(hWnd,GWLP_USERDATA);
            wchar_t sz[1024];
            gpio_t& g = gpios[gpio];
            GetWindowTextW((HWND)lParam,sz,sizeof(sz)/sizeof(wchar_t));
            sz[63]=0;
            if(0==wcsicmp(sz,L"LOW")) {
                g.value(LOW);
            } else if(0==wcsicmp(sz,L"HIGH")) {
                g.value(HIGH);
            } else {
                size_t l = wcslen(sz);
                bool isnum = true;
                for(int i = 0;i<l;++i) {
                    if(!iswdigit(sz[i])) {
                        isnum = false;
                        break;
                    }
                }
                if(isnum) {
                    int v = _wtoi(sz);
                    g.value(v);
                    
                }
            }
        }
        return 0;
    }
    if (uMsg == WM_CLOSE) {
        gpios[GetWindowLongPtrW(hWnd,GWLP_USERDATA)].hwnd_text = NULL;
    }
   
    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}
static LRESULT CALLBACK WindowProcDX(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    // shouldn't get this, but handle anyway
    if (uMsg == WM_SIZE) {
        if (render_target) {
            D2D1_SIZE_U size = D2D1::SizeU(LOWORD(lParam), HIWORD(lParam));
            render_target->Resize(size);
        }
    }
    // in case we receive the close event
    if (uMsg == WM_CLOSE) {
        SetEvent(quit_event);
        should_quit = true;
    }
    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

bool read_mouse(int* out_x,int *out_y) {
    if (WAIT_OBJECT_0 == WaitForSingleObject(
                             app_mutex,  // handle to mutex
                             INFINITE)) {   // no time-out interval)

        if (mouse_state) {
            *out_x = mouse_loc.x;
            *out_y = mouse_loc.y;
        } 
        mouse_req = 0;
        ReleaseMutex(app_mutex);
        return mouse_state;
    
    }
    return false;
}
void flush_bitmap(int x1, int y1, int w, int h, const void* bmp) {
    if (render_bitmap != NULL) {
        D2D1_RECT_U b;
        b.top = y1;
        b.left = x1;
        b.bottom = y1+h;
        b.right = x1+w;
        render_bitmap->CopyFromMemory(&b, bmp, w * 4);
    }
}
uint32_t millis() {
    LARGE_INTEGER counter_freq;
    LARGE_INTEGER end_time;
    QueryPerformanceFrequency(&counter_freq);
    QueryPerformanceCounter(&end_time);
    return uint32_t(((double)(end_time.QuadPart - start_time.QuadPart) / counter_freq.QuadPart) * 1000);
}
uint32_t micros() {
    LARGE_INTEGER counter_freq;
    LARGE_INTEGER end_time;
    QueryPerformanceFrequency(&counter_freq);
    QueryPerformanceCounter(&end_time);
    return uint32_t(((double)(end_time.QuadPart - start_time.QuadPart) / counter_freq.QuadPart)*1000000.0);
}
void delay(uint32_t ms) {
    if(is_isr) return;
    uint32_t end = ms + millis();
    while(millis()<end);
}
void delayMicroseconds(uint32_t us) {
    uint32_t end = us + micros();
    while(micros()<end);
}

// write to the log window
void append_log_window(const char* text) {
   int index = GetWindowTextLength (hwnd_log);
   //SetFocus (hwnd_log); // set focus
   SendMessageA(hwnd_log, EM_SETSEL, (WPARAM)index, (LPARAM)index); // set selection - end of text
   SendMessageA(hwnd_log, EM_REPLACESEL, 0, (LPARAM)text); // append!
}
static void update_gpios() {
    updating_gpios = true;
    while(GetMenuItemCount(gpio_menu)) {
        RemoveMenu(gpio_menu,0,MF_BYPOSITION); 
    }
    wchar_t name[256];
    for(size_t i = 0; i < 256; ++i) {
        gpio_t& g = gpios[i];
        if(g.mode!=0) {
            wcscpy(name,L"GPIO ");
            _itow((int)i,name+wcslen(name),10);
            switch(g.mode) {
                case INPUT:
                case INPUT_PULLDOWN:
                case INPUT_PULLUP:
                    wcscat(name,L" <");
                    break;
                case OUTPUT:
                case OUTPUT_OPEN_DRAIN:
                    wcscat(name,L" >");
            }
            MENUITEMINFOW mif;
            mif.cbSize = sizeof(MENUITEMINFOW);
            mif.cch = wcslen(name);
            mif.dwTypeData = name;
            mif.fMask = MIIM_STRING | MIIM_ID | MIIM_STATE;
            mif.wID = (~i);
            mif.fState = g.value()==0?MFS_UNCHECKED:MFS_CHECKED;
            InsertMenuItemW(gpio_menu,GetMenuItemCount(gpio_menu),TRUE,&mif);
        }
        if(g.hwnd_text!=NULL) {
                // update the visible text box
                wchar_t val[64];
                if(!g.is_input()) {
                if(g.value()==HIGH) {
                    wcscpy(val,L"HIGH");
                } else if(g.value()==LOW) {
                    wcscpy(val,L"LOW");
                } else {
                    _itow(g.value(),val,10);
                }
                if(GetFocus()!=g.hwnd_text) {
                    SetWindowTextW(g.hwnd_text, val);
                }
            }
            EnableWindow(g.hwnd_text,g.is_input()?TRUE:FALSE);
        }
    }
    updating_gpios = false;   
}
void ensure_gpio_window(size_t gpio) {
    if(gpio>255) {
        return;
    }
    if(gpios[gpio].hwnd_text) {
        return;
    }
    wchar_t name[64];
    wcscpy(name,L"GPIO ");
    _itow(gpio,name+wcslen(name),10);
    HWND hwnd = CreateWindowExW(
        WS_EX_TOOLWINDOW, L"Winduino_GPIO", name,
        WS_CAPTION | WS_SYSMENU,
        CW_USEDEFAULT, CW_USEDEFAULT,
        200,
        100,
        NULL, NULL, GetModuleHandle(NULL), &gpios[gpio]);
    if(hwnd==NULL) {
        return;
    }
    ShowWindow(hwnd,SW_SHOW);
    UpdateWindow(hwnd);
    SetForegroundWindow(hwnd);
    SetActiveWindow(hwnd);
}
// entry point
int main(int argc, char* argv[]) {
    // Initialize COM
    CoInitialize(0);
    HRESULT hr = S_OK;
    // get our uptime start
    QueryPerformanceCounter(&start_time);
    // init GPIOs
    for(size_t i = 0; i < 256;++i) {
        gpios[i].id = i;
        gpios[i].mode = 0; // not set yet
        gpios[i].interrupt_mode = -1;
        gpios[i].interrupt_cb = nullptr;
        gpios[i].hwnd_text = nullptr;
        gpios[i].value(0);
    }
    // register the window classes
    // we'll need:
    WNDCLASSW wc;
    HINSTANCE hInstance = GetModuleHandle(NULL);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WindowProcMain;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = hInstance;
    wc.hbrBackground = NULL;
    wc.lpszMenuName = NULL;
    wc.hIcon = NULL;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.lpszClassName = L"Winduino";

    RegisterClassW(&wc);
    wc.lpfnWndProc = WindowProcDX;
    wc.lpszClassName = L"Winduino_DX";
    RegisterClassW(&wc);
    wc.lpfnWndProc = WindowProcGpio;
    wc.lpszClassName = L"Winduino_GPIO";
    RegisterClassW(&wc);
    HWND hwnd_dx;
    RECT r = {0, 0, winduino_screen_size.width*2, winduino_screen_size.height - 1};
    // adjust the size of the window so
    // the above is our client rect
    AdjustWindowRectEx(&r, WS_CAPTION | WS_SYSMENU | WS_BORDER, FALSE, WS_EX_APPWINDOW);
    r.bottom+=GetSystemMetrics(SM_CYMENU);
    menu = CreateMenu();
    if(menu==NULL) {
        goto exit;
    }
    // MENUINFO mi;
    // GetMenuInfo(menu,&mi);
    // mi.fMask = MIM_STYLE | MIM_APPLYTOSUBMENUS;
    // mi.dwStyle|=MNS_NOTIFYBYPOS;
    // SetMenuInfo(menu,&mi);

    gpio_menu = CreateMenu();
    if(gpio_menu==NULL) {
        goto exit;
    }
    
    MENUITEMINFOW mif;
    wchar_t wcsmenu[256];
    wcscpy(wcsmenu,L"&GPIO");
    mif.cbSize = sizeof(MENUITEMINFOW);
    mif.cch = wcslen(wcsmenu)+1;
    mif.fMask = MIIM_STRING | MIIM_SUBMENU;
    mif.dwTypeData = wcsmenu;
    mif.hSubMenu = gpio_menu;
    InsertMenuItemW(menu,0,TRUE,&mif);
    // create the main window
    hwnd_main = CreateWindowExW(
        WS_EX_APPWINDOW, L"Winduino", L"Winduino",
        WS_CAPTION | WS_SYSMENU,
        CW_USEDEFAULT, CW_USEDEFAULT,
        r.right - r.left + 1,
        r.bottom - r.top + 1,
        NULL, menu, hInstance, NULL);
    hwnd_dx=NULL; 
    if (!IsWindow(hwnd_main)) goto exit;
    // create the DirectX window
    hwnd_dx = CreateWindowW(L"Winduino_DX",L"",
                            WS_CHILDWINDOW | WS_VISIBLE,
                            0,0,winduino_screen_size.width,winduino_screen_size.height,
                            hwnd_main,NULL,
                            hInstance,NULL);
    if (!IsWindow(hwnd_dx)) goto exit;
    // create the log textbox
    hwnd_log=CreateWindowExW(WS_EX_CLIENTEDGE, L"edit", L"",
                              WS_CHILD | WS_VISIBLE |WS_HSCROLL | WS_VSCROLL | WS_TABSTOP | WS_BORDER | ES_LEFT| ES_MULTILINE ,
                              winduino_screen_size.width+1, 0, (r.right+r.left)/2, winduino_screen_size.height,
                              hwnd_main, (HMENU)(1),
                              hInstance, NULL);
    // for signalling when to exit
    quit_event = CreateEvent(
        NULL,              // default security attributes
        TRUE,              // manual-reset event
        FALSE,             // initial state is nonsignaled
        TEXT("QuitEvent")  // object name
    );
    if (quit_event == NULL) {
        goto exit;
    }
    // for handling our render
    app_mutex = CreateMutex(NULL, FALSE, NULL);
    if (app_mutex == NULL) {
        goto exit;
    }
    // start DirectX
    hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &d2d_factory);
    assert(hr == S_OK);
    if (hr != S_OK) goto exit;
    // set up our direct2d surface
    {
        RECT rc;
        GetClientRect(hwnd_dx, &rc);
        D2D1_SIZE_U size = D2D1::SizeU(
            (rc.right - rc.left),
            rc.bottom - rc.top);

        hr = d2d_factory->CreateHwndRenderTarget(
            D2D1::RenderTargetProperties(),
            D2D1::HwndRenderTargetProperties(hwnd_dx, size),
            &render_target);
        assert(hr == S_OK);
        if (hr != S_OK) goto exit;
    }
    // initialize the render bitmap
    {
        
        D2D1_SIZE_U size = {0};
        D2D1_BITMAP_PROPERTIES props;
        render_target->GetDpi(&props.dpiX, &props.dpiY);
        D2D1_PIXEL_FORMAT pixelFormat = D2D1::PixelFormat(
        #ifdef USE_RGB
            DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
        #else
            DXGI_FORMAT_B8G8R8A8_UNORM,
        #endif
            D2D1_ALPHA_MODE_IGNORE);
        props.pixelFormat = pixelFormat;
        size.width = winduino_screen_size.width;
        size.height = winduino_screen_size.height;

        hr = render_target->CreateBitmap(size,
                                         props,
                                         &render_bitmap);
        assert(hr == S_OK);
        if (hr != S_OK) goto exit;
    }
    // show the main window
    ShowWindow(hwnd_main, SW_SHOWNORMAL);
    UpdateWindow(hwnd_main);
    // for the frame counter
    SetTimer(hwnd_main, 0, 1000, NULL);
    // run setup() to initialize user code
    setup();
    
    // this is the thread where the actual rendering 
    // takes place and where loop() is run
    app_thread = CreateThread(NULL, 8000 * 4, render_thread_proc, NULL, 0, NULL);
    if (app_thread == NULL) {
        goto exit;
    }
    // main message pump
    while (!should_quit) {
        DWORD result = 0;
        MSG msg = {0};
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) {
                should_quit = true;
                break;
            }
            if (msg.message == WM_TIMER) {
                update_title(hwnd_main);
                InterlockedExchange(&frames, 0);
            }
            // handle our out of band messages
            if (msg.message == WM_LBUTTONDOWN && msg.hwnd==hwnd_dx) {
                if(LOWORD(msg.lParam)<winduino_screen_size.width && 
                    HIWORD(msg.lParam)<winduino_screen_size.height) {
                    SetCapture(hwnd_dx);
                    
                    if (WAIT_OBJECT_0 == WaitForSingleObject(
                                            app_mutex,  // handle to mutex
                                            INFINITE)) {   // no time-out interval)
                        old_mouse_state = mouse_state;
                        mouse_state = 1;
                        mouse_loc.x = LOWORD(msg.lParam);
                        mouse_loc.y = HIWORD(msg.lParam);
                        mouse_req = 1;
                        ReleaseMutex(app_mutex);
                    }
                    update_title(hwnd_main);
                }
            }
            if (msg.message == WM_MOUSEMOVE && 
                msg.hwnd==hwnd_dx) {
                if (WAIT_OBJECT_0 == WaitForSingleObject(
                                         app_mutex,  // handle to mutex
                                         INFINITE)) {   // no time-out interval)
                    if (mouse_state == 1 && MK_LBUTTON == msg.wParam) {
                        mouse_req = 1;
                        mouse_loc.x = (int16_t)LOWORD(msg.lParam);
                        mouse_loc.y = (int16_t)HIWORD(msg.lParam);
                    }
                    ReleaseMutex(app_mutex);
                }
                update_title(hwnd_main);
            }
            if (msg.message == WM_LBUTTONUP &&
                msg.hwnd==hwnd_dx) {
                ReleaseCapture();
                if (WAIT_OBJECT_0 == WaitForSingleObject(
                                         app_mutex,  // handle to mutex
                                         INFINITE)) {   // no time-out interval)

                    old_mouse_state = mouse_state;
                    mouse_req = 1;
                    mouse_state = 0;
                    mouse_loc.x = (int16_t)LOWORD(msg.lParam);
                    mouse_loc.y = (int16_t)HIWORD(msg.lParam);
                    ReleaseMutex(app_mutex);
                }
                update_title(hwnd_main);
            }
             if(msg.message == WM_COMMAND && msg.hwnd==hwnd_main) {
                ensure_gpio_window((uint8_t)~msg.wParam);
                //Serial.printf("selected %d\r\n",~msg.wParam);
            }
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        if (WAIT_OBJECT_0 == WaitForSingleObject(quit_event, 0)) {
            should_quit = true;
        }
    }
exit:
    if (IsWindow(hwnd_dx)) {
        DestroyWindow(hwnd_dx);
    }
    if (IsWindow(hwnd_main)) {
        DestroyWindow(hwnd_main);
    }
    if (app_thread != NULL) {
        CloseHandle(app_thread);
    }
    if (quit_event != NULL) {
        CloseHandle(quit_event);
    }
    if (app_mutex != NULL) {
        CloseHandle(app_mutex);
    }
    render_target->Release();
    render_bitmap->Release();
    d2d_factory->Release();
    CoUninitialize();
}

void pinMode(uint8_t pin, uint8_t mode) {
    gpios[pin].mode = mode;
    update_gpios();
}
void digitalWrite(uint8_t pin, uint8_t val) {
    gpio_t& g = gpios[pin];
    if(g.mode == OUTPUT || g.mode == OUTPUT_OPEN_DRAIN) {
        g.value(val==LOW?LOW:HIGH);
        update_gpios();
    }
}
int digitalRead(uint8_t pin) {
    gpio_t& g = gpios[pin];
    if(g.mode == INPUT || g.mode == INPUT_PULLUP || g.mode == INPUT_PULLDOWN) {
        return g.value() == LOW?LOW:HIGH;
    }
    return LOW;
}
void analogWrite(uint8_t pin, int value) {
    if(value<0) { value = 0; }
    else if(value>255) {value = 255;}
    gpio_t& g = gpios[pin];
    if(g.mode == OUTPUT || g.mode == OUTPUT_OPEN_DRAIN) {
        g.value((uint32_t)value);
        update_gpios();
    }
}
uint16_t analogRead(uint8_t pin) {
    gpio_t& g = gpios[pin];
    if(g.mode == INPUT || g.mode == INPUT_PULLUP || g.mode == INPUT_PULLDOWN) {
        return (uint16_t)g.value();
    }
    return 0;
}
void yield() {
    
}
void attachInterrupt(uint8_t pin, void (*cb)(void), int mode) {
    if(!gpios[pin].is_input()) {
        pinMode(pin,INPUT);
    }
    gpios[pin].interrupt_mode = mode;
    gpios[pin].interrupt_cb = cb;
}
void detachInterrupt(uint8_t pin) {
    gpios[pin].mode = 0;
    gpios[pin].interrupt_mode = -1;
    gpios[pin].interrupt_cb = nullptr;
    update_gpios();
}
// note that this effective "leaks" since there's no way to free
// it doesn't matter, because hardware cannot be reloaded or 
// unloaded for the life of the process
void* hardware_load(const char* name) {
    HMODULE h = LoadLibraryA(name);
    if(h==NULL) {
        return nullptr;
    }
    hardware_handle_t* result = new hardware_handle_t();
    if(result==nullptr) {
        return nullptr;
    }
    result->next = nullptr;
    result->hmodule = h;
    result->configure = (hardware_configure_fn)GetProcAddress(h,"Configure");
    result->connect = (hardware_connect_fn)GetProcAddress(h,"Connect");
    result->update = (hardware_update_fn)GetProcAddress(h,"Update");
    result->pin_change = (hardware_pin_change_fn)GetProcAddress(h,"PinChange");
    result->transfer_bits_spi = (hardware_transfer_bits_spi_fn)GetProcAddress(h,"TransferBitsSPI");
    result->attach_log = (hardware_attach_log_fn)GetProcAddress(h,"AttachLog");
    if(result->connect == nullptr) {
        delete result;
        return nullptr;
    }
    if(hardware_head == nullptr) {
        hardware_head = result;
    } else {
        hardware_handle_t* p = hardware_head;
        while(p!=nullptr) {
            if(p->next==nullptr) {
                p->next = result;
                break;
            }
            p=p->next;
        }
    }
    return result;
}
bool hardware_set_pin(void* hw, uint8_t mcu_pin, uint8_t hw_pin) {
    if(hw==nullptr) {
        return false;
    }
    return gpios[mcu_pin].connect((hardware_handle_t*)hw,hw_pin);
} 

bool hardware_configure(void* hw, int prop, void* data, size_t size) {
    if(hw==nullptr) {
        return false;
    }
    hardware_handle_t* h = (hardware_handle_t*)hw;
    if(h->configure==nullptr) return false;
    return 0==h->configure(prop,data,size);
}
bool hardware_transfer_bits_spi(uint8_t* data, size_t size_bits) {
    hardware_handle_t* h = hardware_head;
    while(h!=nullptr) {
        if(h->transfer_bits_spi!=nullptr) {
            if(0!=h->transfer_bits_spi(data,size_bits)) {
                return false;
            }
        }
        h=h->next;
    }
    return true;
}
static void logger_log(const char* text) {
    Serial.println(text);
}
bool hardware_attach_log(void* hw) {
    if(hw==nullptr) {
        return false;
    }
    hardware_handle_t* h = (hardware_handle_t*)hw;
    if(h->attach_log!=nullptr) {
        h->attach_log(logger_log);
    }
    return true;    
}