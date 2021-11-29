#include <remote-app-gui.h>
#include <remote-app-type.h>
#include <remote-app.h>
struct _GUI
{
  RemoteApp *app;
};
static GUI gui_declare;
static gboolean fullscreen = FALSE;
static LONG prev_style = 0;
static RECT prev_rect = {
    0,
};

void init_remote_app_gui(RemoteApp *app)
{
  gui_declare.app = app;
}

gboolean
_keydown(int *key)
{
  return (GetAsyncKeyState(key) & 0x8000) != 0;
}

RECT wr = {0, 0, 1920, 1080};

void adjust_window()
{
  AdjustWindowRect(&wr, WS_OVERLAPPEDWINDOW, FALSE);
}

gpointer gamepad_thread_func(gpointer data)
{
  DWORD dwResult;

  XINPUT_STATE state, prevstate;
  SecureZeroMemory(&state, sizeof(XINPUT_STATE));
  SecureZeroMemory(&prevstate, sizeof(XINPUT_STATE));
  dwResult = XInputGetState(0, &prevstate);

  DWORD wButtonPressing = 0;
  DWORD Lstick = 64;

  //key down R key and key up R key
  INPUT inputs[3];
  ZeroMemory(inputs, sizeof(inputs));
  //key down
  inputs[0].type = INPUT_KEYBOARD;
  inputs[0].ki.wVk = 0x52;

  //key up (same as inputs[0] object but with extra attr)r
  inputs[1].ki.dwFlags = KEYEVENTF_KEYUP;

  if (dwResult == ERROR_SUCCESS)
  {
    // Controller is connected
    while (XInputGetState(0, &state) == ERROR_SUCCESS)
    {
      //dwpacketnumber diff?
      if (state.dwPacketNumber != prevstate.dwPacketNumber)
      {
        //different input
        wButtonPressing = state.Gamepad.wButtons;
      }
      if (wButtonPressing & Lstick)
      {
        //std::cout << "hold" << std::endl;
        XINPUT_VIBRATION vibration;
        vibration.wLeftMotorSpeed = 65535;
        vibration.wRightMotorSpeed = 65535;
        XInputSetState(0, &vibration);
        SendInput(1, &inputs[0], sizeof(INPUT));
      }
      if (prevstate.Gamepad.wButtons & Lstick && (wButtonPressing & Lstick) == 0)
      {
        SendInput(1, &inputs[1], sizeof(INPUT));
      }
      MoveMemory(&prevstate, &state, sizeof(XINPUT_STATE));

      Sleep(10);
    }
  }
}

////////////////////////////////////
// get debug infor from the gstreamer pipeline
gboolean
bus_msg(GstBus *bus, GstMessage *msg, gpointer user_data, HWND hwnd, gboolean visible, gboolean test_reuse, GMainLoop *loop)
{
  GstElement *pipeline = GST_ELEMENT(user_data);
  switch (GST_MESSAGE_TYPE(msg))
  {
  case GST_MESSAGE_ASYNC_DONE:
    /* make window visible when we have something to show */
    if (!visible && hwnd)
    {
      ShowWindow(hwnd, SW_SHOW);
      visible = TRUE;
    }

    gst_element_set_state(pipeline, GST_STATE_PLAYING);
    break;
  case GST_MESSAGE_ERROR:
  {
    GError *err;
    gchar *dbg;

    gst_message_parse_error(msg, &err, &dbg);
    g_printerr("ERROR %s \n", err->message);
    if (dbg != NULL)
      g_printerr("ERROR debug information: %s\n", dbg);
    g_clear_error(&err);
    g_free(dbg);
    test_reuse = FALSE;

    g_main_loop_quit(loop);
    break;
  }
  default:
    break;
  }

  return TRUE;
}

//////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////
void switch_fullscreen_mode(HWND *hwnd)
{
  long _prev_style;
  if (!hwnd)
    return;

  fullscreen = !fullscreen;

  gst_print("Full screen %s\n", fullscreen ? "on" : "off");

  if (!fullscreen)
  {
    /* Restore the window's attributes and size */
    SetWindowLong(hwnd, GWL_STYLE, prev_style);

    SetWindowPos(hwnd, HWND_NOTOPMOST,
                 prev_rect.left,
                 prev_rect.top,
                 1556 - prev_rect.left,
                 884 - prev_rect.top, SWP_FRAMECHANGED | SWP_NOACTIVATE);

    ShowWindow(hwnd, SW_NORMAL);
  }
  else
  {
    RECT fullscreen_rect;

    /* show window before change style */
    ShowWindow(hwnd, SW_SHOW);

    /* Save the old window rect so we can restore it when exiting
     * fullscreen mode */
    GetWindowRect(hwnd, &prev_rect);
    prev_style = GetWindowLong(hwnd, GWL_STYLE);

    if (!get_monitor_size(&fullscreen_rect, hwnd))
    {
      g_warning("Couldn't get monitor size");

      fullscreen = !fullscreen;
      return;
    }

    /* Make the window borderless so that the client area can fill the screen */
    _prev_style = prev_style;
    SetWindowLong(hwnd, GWL_STYLE,
                  _prev_style &
                      ~(WS_CAPTION | WS_MAXIMIZEBOX | WS_MINIMIZEBOX | WS_SYSMENU |
                        WS_THICKFRAME));
    prev_style = _prev_style;
    SetWindowPos(hwnd, HWND_NOTOPMOST,
                 fullscreen_rect.left,
                 fullscreen_rect.top,
                 fullscreen_rect.right,
                 fullscreen_rect.bottom, SWP_FRAMECHANGED | SWP_NOACTIVATE);

    ShowWindow(hwnd, SW_MAXIMIZE);
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

HWND set_up_window(WNDCLASSEX wc, gchar *title,  HINSTANCE hinstance )
{
  return CreateWindowEx(0, wc.lpszClassName,
                        title,
                        WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_OVERLAPPEDWINDOW,
                        CW_USEDEFAULT, CW_USEDEFAULT,
                        wr.right - wr.left, wr.bottom - wr.top, (HWND)NULL, (HMENU)NULL,
                        hinstance, NULL);
}