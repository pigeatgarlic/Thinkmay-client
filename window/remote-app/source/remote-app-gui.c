
/// <summary>
/// @file remote-app-gui.c
/// @author {Do Huy Hoang} ({huyhoangdo0205@gmail.com})
/// </summary>
/// @version 1.0
/// @date 2021-11-30
/// 
/// @copyright Copyright (c) 2021
/// 

#include <remote-app-gui.h>
#include <remote-app-type.h>
#include <remote-app.h>

#include <glib.h>




struct _GUI
{
    RemoteApp *app;
#ifdef G_OS_WIN32
    HWND window;
    LONG prev_style;
    RECT prev_rect;
    RECT wr;
#endif
    GstElement* sink_element;
    gboolean fullscreen;
};

static GUI _gui;





GUI*
init_remote_app_gui(RemoteApp *app)
{
    _gui.app = app;
    _gui.wr.bottom = 1080;
    _gui.wr.top = 0;
    _gui.wr.left = 0;
    _gui.wr.right = 1920;

    return &_gui;
}

/// <summary>
/// detect if a key is pressed
/// </summary>
/// <param name="key"></param>
/// <returns></returns>
gboolean
_keydown(int *key)
{
    return (GetAsyncKeyState(key) & 0x8000) != 0;
}


void 
adjust_window(GUI* gui)
{
    AdjustWindowRect(&(gui->wr), WS_OVERLAPPEDWINDOW, FALSE);
}


///////////////////////////////////////////////////////////////////////////
void switch_fullscreen_mode(GUI* gui)
{
    long _prev_style;

    gui->fullscreen = !gui->fullscreen;

    if (!gui->fullscreen)
    {
        /* Restore the window's attributes and size */
        SetWindowLong(gui->window, GWL_STYLE, gui->prev_style);

        SetWindowPos(gui->window, HWND_NOTOPMOST,
                    gui->prev_rect.left,
                    gui->prev_rect.top,
                    1556 - gui->prev_rect.left,
                    884 - gui->prev_rect.top, SWP_FRAMECHANGED | SWP_NOACTIVATE);

        ShowWindow(gui->window, SW_NORMAL);
    }
    else
    {
        RECT fullscreen_rect;

        /* show window before change style */
        ShowWindow(gui->window, SW_SHOW);

        /* Save the old window rect so we can restore it when exiting
        * fullscreen mode */
        GetWindowRect(gui->window, &(gui->prev_rect));
        gui->prev_style = GetWindowLong(gui->window, GWL_STYLE);

        if (!get_monitor_size(&fullscreen_rect, gui->window))
        {
          g_warning("Couldn't get monitor size");

          gui->fullscreen = !gui->fullscreen;
          return;
        }

        /* Make the window borderless so that the client area can fill the screen */
        _prev_style = gui->prev_style;
        SetWindowLong(gui->window, GWL_STYLE,
                      _prev_style &
                          ~(WS_CAPTION | WS_MAXIMIZEBOX | WS_MINIMIZEBOX | WS_SYSMENU |
                            WS_THICKFRAME));
        gui->prev_style = _prev_style;
        SetWindowPos(gui->window, HWND_NOTOPMOST,
                    fullscreen_rect.left,
                    fullscreen_rect.top,
                    fullscreen_rect.right,
                    fullscreen_rect.bottom, SWP_FRAMECHANGED | SWP_NOACTIVATE);

        ShowWindow(gui->window, SW_MAXIMIZE);
    }
}
///////////////////////////////////////////////////////////////

static gboolean
get_monitor_size(RECT *rect, HWND *hwnd)
{
    HMONITOR monitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
    MONITORINFOEX monitor_info;
    DEVMODE dev_mode;

    monitor_info.cbSize = sizeof(monitor_info);
    if (!GetMonitorInfo(monitor, (LPMONITORINFO)&monitor_info))
    {
        return FALSE;
    }

    dev_mode.dmSize = sizeof(dev_mode);
    dev_mode.dmDriverExtra = sizeof(POINTL);
    dev_mode.dmFields = DM_POSITION;
    if (!EnumDisplaySettings(monitor_info.szDevice, ENUM_CURRENT_SETTINGS, &dev_mode))
    {
        return FALSE;
    }

    SetRect(rect, 0, 0, dev_mode.dmPelsWidth, dev_mode.dmPelsHeight);

    return TRUE;
}
////////////////////////////////////////////////////////////////////////

static gboolean
adjust_video_position(RemoteApp* app, gint x, gint y, gint width, gint height)
{
    GUI* gui = remote_app_get_gui(app);
    gst_video_overlay_set_render_rectangle(GST_VIDEO_OVERLAY(gui->sink_element), x, y, width, height);
}







static LRESULT CALLBACK
window_proc(HWND hWnd, UINT message, 
            WPARAM wParam, 
            LPARAM lParam)
{
    if (message == WM_DESTROY) {
        remote_app_finalize(_gui.app,0,NULL);
    } else {
        handle_message_window_proc(hWnd, message, wParam, lParam );
    }

    return DefWindowProc(hWnd, message, wParam, lParam);
}

HWND 
set_up_window(GUI* gui)
{
    HINSTANCE hinstance = GetModuleHandle(NULL);
    WNDCLASSEX wc = { 0, };
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW; //// ????
    wc.lpfnWndProc = (WNDPROC)window_proc;
    wc.hInstance = hinstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.lpszClassName = "GstWIN32VideoOverlay";
    RegisterClassEx(&wc);

    return CreateWindowEx(0, wc.lpszClassName,
                          "Thinkmay remote control",
                          WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_OVERLAPPEDWINDOW,
                          CW_USEDEFAULT, CW_USEDEFAULT,
                          gui->wr.right  - gui->wr.left, 
                          gui->wr.bottom - gui->wr.top, 
                          (HWND)NULL, (HMENU)NULL,
                          hinstance, NULL);
}

void 
handle_message_window_proc(HWND hwnd, 
                            UINT message, 
                            WPARAM wParam, 
                            LPARAM lParam)
{
    switch (message)
    {
        case WM_CHAR:
          if (_keydown(0x11) && _keydown(0xA0) && _keydown(0x46)) {
            switch_fullscreen_mode(hwnd);
          } if (_keydown(0x11) && _keydown(0xA0) && _keydown(0x50)) {
            // hidden mouse setting func
          } break;
        case WM_MOUSEWHEEL:
          if (GET_WHEEL_DELTA_WPARAM(wParam) < 0) {
            // negative indicates a rotation towards the user (down)
          } else if (GET_WHEEL_DELTA_WPARAM(wParam) > 0) {
            // a positive indicate the wheel is rotated away from the user (up)
          } break;
        case WM_LBUTTONDOWN:
          break;
        case WM_LBUTTONUP:
          break;
        case WM_RBUTTONDOWN:
          break;
        case WM_RBUTTONUP:
          break;
        case WM_MBUTTONDOWN:
          break;
        case WM_MBUTTONUP: // awaiting for test
          break;
        default:
          break;
    }
}



void
remote_app_redirect_gui(RemoteApp* app, GstElement* sink)
{

    GUI* gui = remote_app_get_gui(app);
    adjust_window(gui);
    gui->window = set_up_window(gui);
    gst_video_overlay_set_window_handle(GST_VIDEO_OVERLAY(sink),(guintptr)gui->window);
}
///////////////////////////////////////////////////////////////////////
/// get monitor size of entire screen instead of only the window






void
gui_terminate(GUI* gui)
{
    DestroyWindow(gui->window);
}
