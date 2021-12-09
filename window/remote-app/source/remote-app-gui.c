/**
 * @file remote-app-gui.c
 * @author {Do Huy Hoang} ({huyhoangdo0205@gmail.com})
 * @brief 
 * @version 1.0
 * @date 2021-12-08
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#include <remote-app-gui.h>
#include <remote-app-type.h>
#include <remote-app-pipeline.h>
#include <remote-app-input.h>
#include <remote-app.h>

#include <glib.h>

#include <gst/video/videooverlay.h>
#include <gst/video/gstvideosink.h>
#include <glib-2.0/glib.h>
#include <gst/video/videooverlay.h>
#include <gst/video/gstvideosink.h>


#ifdef G_OS_WIN32
#include <windows.h>
#include <WinUser.h>
#include <libloaderapi.h>
#include <Xinput.h>
#endif


struct _GUI
{
    /**
     * @brief 
     * reference to remote app
     */
    RemoteApp *app;

#ifdef G_OS_WIN32
    /**
     * @brief 
     * win32 window for display video
     */
    HWND window;

    /**
     * @brief 
     * revious window style
     */
    LONG prev_style;

    /**
     * @brief 
     * previous rectangle size, use for setup fullscreen mode
     */
    RECT prev_rect;

    /**
     * @brief 
     * window retangle, size and position of window
     */
    RECT wr;

    /**
     * @brief 
     * IO channel for remote app
     */
    GIOChannel *msg_io_channel;
#endif

    /**
     * @brief 
     * gst video sink element
     */
    GstElement* sink_element;

    /**
     * @brief 
     * fullscreen mode on or off
     */
    gboolean fullscreen;
};

static GUI _gui = {0};


/**
 * @brief 
 * window procedure function
 * @param hWnd 
 * @param message 
 * @param wParam 
 * @param lParam 
 * @return LRESULT 
 */
static LRESULT CALLBACK       window_proc             (HWND hWnd, 
                                                      UINT message, 
                                                      WPARAM wParam, 
                                                      LPARAM lParam);

/**
 * @brief 
 * 
 * @param source 
 * @param condition 
 * @param data 
 * @return gboolean 
 */
static gboolean
msg_cb (GIOChannel * source, 
        GIOCondition condition, 
        gpointer data)
{
  MSG msg;

  if (!PeekMessage (&msg, NULL, 0, 0, PM_REMOVE))
    return G_SOURCE_CONTINUE;

  TranslateMessage (&msg);
  DispatchMessage (&msg);

  return G_SOURCE_CONTINUE;
}

/**
 * @brief Set the up window object
 * setup window 
 * @param gui 
 * @return HWND 
 */
static void
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

    AdjustWindowRect (&(gui->wr), WS_OVERLAPPEDWINDOW, FALSE);
    gui->window = CreateWindowEx(0, wc.lpszClassName,
                          "Thinkmay remote control",
                          WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_OVERLAPPEDWINDOW,
                          CW_USEDEFAULT, CW_USEDEFAULT,
                          gui->wr.right  - gui->wr.left, 
                          gui->wr.bottom - gui->wr.top, 
                          (HWND)NULL, (HMENU)NULL,
                          hinstance, NULL);
    gui->msg_io_channel = g_io_channel_win32_new_messages (0);
    g_io_add_watch (gui->msg_io_channel, G_IO_IN, msg_cb, NULL);
    ShowWindow (_gui.window, SW_SHOW);
}


/**
 * @brief 
 * handle message from gbus of gstreamer pipeline
 * @param bus 
 * @param msg 
 * @param user_data 
 * @return gboolean 
 */
static gboolean
bus_msg (GstBus * bus, 
        GstMessage * msg, 
        gpointer user_data)
{
  GstElement *pipeline = GST_ELEMENT (user_data);
  switch (GST_MESSAGE_TYPE (msg)) {
    case GST_MESSAGE_ASYNC_DONE:
      gst_element_set_state (
        GST_BIN(pipeline_get_pipeline_element(pipeline)), 
        GST_STATE_PLAYING);
      break;
  }

  return TRUE;
}

gpointer
setup_video_overlay(GstElement* videosink, 
                    RemoteApp* app)
{
    GUI* gui = remote_app_get_gui(app);
    Pipeline* pipeline = remote_app_get_pipeline(app);
    GstElement* pipe_element = pipeline_get_pipeline_element(pipeline);

    set_up_window(gui);

    /* prepare the pipeline */
    gst_object_ref_sink (videosink);

    gst_bus_add_watch (GST_ELEMENT_BUS (pipe_element), bus_msg, pipeline);

    gst_video_overlay_set_window_handle (GST_VIDEO_OVERLAY (videosink),
        (guintptr) _gui.window);

    GstStateChangeReturn sret = 
      gst_element_set_state (GST_BIN(pipe_element), GST_STATE_PAUSED);
    if (sret == GST_STATE_CHANGE_FAILURE) 
        g_printerr ("Pipeline doesn't want to pause\n");

    return NULL;
}

GUI*
init_remote_app_gui(RemoteApp *app)
{
    memset(&_gui,0,sizeof(GUI));


    _gui.app = app;
    _gui.wr.bottom = 1080;
    _gui.wr.top = 0;
    _gui.wr.left = 0;
    _gui.wr.right = 1920;

    return &_gui;
}




/**
 * @brief Get the monitor size object
 * 
 * @param rect 
 * @param hwnd 
 * @return gboolean 
 */
static gboolean
get_monitor_size(RECT *rect, 
                 HWND *hwnd)
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








void 
switch_fullscreen_mode(GUI* gui)
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



/**
 * @brief 
 * 
 * @param app 
 * @param x 
 * @param y 
 * @param width 
 * @param height 
 * @return gboolean 
 */
static gboolean
adjust_video_position(RemoteApp* app, 
                      gint x, 
                      gint y, 
                      gint width, 
                      gint height)
{
    GUI* gui = remote_app_get_gui(app);
    gst_video_overlay_set_render_rectangle(GST_VIDEO_OVERLAY(gui->sink_element), 
                                            x, 
                                            y, 
                                            width, 
                                            height);
}





/**
 * @brief 
 * detect if a key is pressed
 * @param key 
 * @return gboolean 
 */
static gboolean
_keydown(int *key)
{
    return (GetAsyncKeyState(key) & 0x8000) != 0;
}


/**
 * @brief 
 * window proc responsible for handling message from window HWND
 * @param hWnd 
 * @param message 
 * @param wParam 
 * @param lParam 
 * @return LRESULT 
 */
static LRESULT CALLBACK
window_proc(HWND hWnd, 
            UINT message, 
            WPARAM wParam, 
            LPARAM lParam)
{
    if (message == WM_DESTROY) 
    {
        remote_app_finalize(_gui.app,0,NULL);
    } 
    else if (message == WM_INPUT)
    {
        handle_message_window_proc(hWnd, message, wParam, lParam );
    }
    else if (message = WM_CHAR)
    {
        if (_keydown(0x11) && _keydown(0xA0) && _keydown(0x46)) 
        {
            switch_fullscreen_mode(_gui.window);
        } 
        if (_keydown(0x11) && _keydown(0xA0) && _keydown(0x50)) 
        {

        } 
    }

    return DefWindowProc(hWnd, message, wParam, lParam);
}



/**
 * @brief 
 * 
 * @param gui 
 */
static void 
adjust_window(GUI* gui)
{
    AdjustWindowRect(&(gui->wr), WS_OVERLAPPEDWINDOW, FALSE);
}







void
gui_terminate(GUI* gui)
{
    DestroyWindow(gui->window);
}
