/*
 * GStreamer
 * Copyright (C) 2019 Seungha Yang <seungha.yang@navercorp.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <gst/gst.h>
#include <gst/video/videooverlay.h>
#include <gst/video/gstvideosink.h>
#include <windows.h>
#include <glib-2.0/glib.h>
#include <string.h>
#include <json-glib/json-glib.h>
#include <human-interface-opcode.h>
#include <message-form.h>
#include <remote-app-gui.h>

#pragma comment(lib, "XInput.lib")
#include <windows.h>
#include <Xinput.h>

XINPUT_STATE state;
static GMainLoop *loop = NULL;
static GMainLoop *pipeline_loop = NULL;
static gboolean visible = FALSE;
static gboolean test_reuse = FALSE;
static HWND hwnd = NULL;
static gboolean test_fullscreen = FALSE;
static gchar *video_sink = NULL;
static GstElement *sink = NULL;
static gboolean run_thread = FALSE;


static gint x = 0;
static gint y = 0;
static gint width = 1920;
static gint height = 1080;

typedef struct
{
  GThread *thread;
  HANDLE event_handle;
  HANDLE console_handle;
  gboolean closing;
  GMutex lock;
} Win32KeyHandler;

static Win32KeyHandler *win32_key_handler = NULL;

#define DEFAULT_VIDEO_SINK "d3d11videosink"

///////////////////////////////////////////////////////////////////////
/// get monitor size of entire screen instead of only the window

static LRESULT CALLBACK
window_proc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  XInputGetState(0, &state);
  if ((state.Gamepad.wButtons & XINPUT_GAMEPAD_A) != 0)
  {
    int eqew = 1;
  }
  switch (message)
  {
  case WM_DESTROY:
    if (loop)
      g_main_loop_quit(loop);
    if (pipeline_loop)
      g_main_loop_quit(pipeline_loop);
    return 0;
    break;
  case WM_CHAR:
    if (_keydown(0x11) && _keydown(0xA0) && _keydown(0x46))
    {
      switch_fullscreen_mode(hwnd);
    }
    if (_keydown(0x11) && _keydown(0xA0) && _keydown(0x50))
    {
      // hidden mouse setting func
    }
  case WM_MOUSEWHEEL:
    if (GET_WHEEL_DELTA_WPARAM(wParam) < 0)
    {
      // negative indicates a rotation towards the user (down)
    }
    else if (GET_WHEEL_DELTA_WPARAM(wParam) > 0)
    {
      // a positive indicate the wheel is rotated away from the user (up)
    }
    break;
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
    /*
      - Use the following code to obtain the information in the wParam parameter.
        int xPos = LOWORD(lParam);
        int yPos = HIWORD(lParam);
        zDelta = GET_WHEEL_DELTA_WPARAM(wParam);
        fwKeys = GET_KEYSTATE_WPARAM(wParam);
    */
  default:
    // SetCapture(hWnd);
    if (message != 127)
      printf("%d \n", message);
    break;
  }

  return DefWindowProc(hWnd, message, wParam, lParam);
}

////////////////////////////////////
// get debug infor from the gstreamer pipeline
static gboolean
bus_msg(GstBus *bus, GstMessage *msg, gpointer user_data)
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
static gboolean
msg_cb(GIOChannel *source, GIOCondition condition, gpointer data)
{
  MSG msg;

  if (!PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
    return G_SOURCE_CONTINUE;

  TranslateMessage(&msg);
  DispatchMessage(&msg);

  return G_SOURCE_CONTINUE;
}

void example_create_json_object()
{
  JsonObject *json = json_object_new();
  json_object_set_int_member(json, "Opcode", MOUSE_DOWN);

  char *string = get_string_from_json_object(json);
  g_print(string);
}

//////////////////////////////////
/// quit program if the pipeline is unable to start
static gboolean
timeout_cb(gpointer user_data)
{
  g_main_loop_quit((GMainLoop *)user_data);

  return G_SOURCE_REMOVE;
}

static gpointer
pipeline_runner_func(gpointer user_data)
{
  GstElement *pipeline, *src;
  GstStateChangeReturn sret;
  gint num_repeat = 0;
  GMainContext *context = NULL;
  GMainLoop *this_loop;

  if (run_thread)
  {
    /* We are in runner thread, create our loop */
    context = g_main_context_new();
    pipeline_loop = g_main_loop_new(context, FALSE);

    g_main_context_push_thread_default(context);

    this_loop = pipeline_loop;
  }
  else
  {
    this_loop = loop;
  }

  /* prepare the pipeline */
  pipeline = gst_pipeline_new("win32-overlay");
  src = gst_element_factory_make("videotestsrc", NULL);
  sink = gst_element_factory_make(video_sink, NULL);

  if (!sink)
  {
    g_printerr("%s element is not available\n", video_sink);
    exit(1);
  }

  gst_object_ref_sink(sink);

  gst_bin_add_many(GST_BIN(pipeline), src, sink, NULL);
  gst_element_link(src, sink);

  gst_bus_add_watch(GST_ELEMENT_BUS(pipeline), bus_msg, pipeline);

  do
  {
    gst_print("Running loop %d\n", num_repeat++);

    gst_video_overlay_set_window_handle(GST_VIDEO_OVERLAY(sink),
                                        (guintptr)hwnd);

    sret = gst_element_set_state(pipeline, GST_STATE_PAUSED);
    if (sret == GST_STATE_CHANGE_FAILURE)
    {
      g_printerr("Pipeline doesn't want to pause\n");
      break;
    }
    else
    {
      /* add timer to repeat and reuse pipeline  */
      if (test_reuse)
      {
        GSource *timeout_source = g_timeout_source_new_seconds(3);

        g_source_set_callback(timeout_source,
                              (GSourceFunc)timeout_cb, this_loop, NULL);
        g_source_attach(timeout_source, NULL);
        g_source_unref(timeout_source);
      }

      g_main_loop_run(this_loop);
    }
    gst_element_set_state(pipeline, GST_STATE_NULL);

    visible = FALSE;
  } while (test_reuse);

  gst_bus_remove_watch(GST_ELEMENT_BUS(pipeline));
  gst_object_unref(pipeline);

  if (run_thread)
  {
    g_main_context_pop_thread_default(context);
    g_main_context_unref(context);

    g_main_loop_quit(loop);
    g_main_loop_unref(pipeline_loop);
  }

  return NULL;
}

static void
print_keyboard_help(void)
{
  /* *INDENT-OFF* */
  static struct
  {
    const gchar *key_desc;
    const gchar *key_help;
  } key_controls[] = {
      {"\342\206\222", "move overlay to right-hand side"},
      {"\342\206\220", "move overlay to left-hand side"},
      {"\342\206\221", "move overlay to upward"},
      {"\342\206\223", "move overlay to downward"},
      {">", "increase overlay width"},
      {"<", "decrease overlay width"},
      {"+", "increase overlay height"},
      {"-", "decrease overlay height"},
      {"r", "reset render rectangle"},
      {"e", "expose overlay"},
      {"k", "show keyboard shortcuts"},
  };
  /* *INDENT-ON* */

  guint i, chars_to_pad, desc_len, max_desc_len = 0;

  gst_print("\n\n%s\n\n", "Interactive mode - keyboard controls:");

  for (i = 0; i < G_N_ELEMENTS(key_controls); ++i)
  {
    desc_len = g_utf8_strlen(key_controls[i].key_desc, -1);
    max_desc_len = MAX(max_desc_len, desc_len);
  }
  ++max_desc_len;

  for (i = 0; i < G_N_ELEMENTS(key_controls); ++i)
  {
    chars_to_pad = max_desc_len - g_utf8_strlen(key_controls[i].key_desc, -1);
    gst_print("\t%s", key_controls[i].key_desc);
    gst_print("%-*s: ", chars_to_pad, "");
    gst_print("%s\n", key_controls[i].key_help);
  }
  gst_print("\n");
}

//////// get input character from std input
static gboolean
win32_kb_source_cb(Win32KeyHandler *handler)
{
  HANDLE h_input = handler->console_handle;
  INPUT_RECORD buffer;
  DWORD n;

  if (PeekConsoleInput(h_input, &buffer, 1, &n) && n == 1)
  {
    ReadConsoleInput(h_input, &buffer, 1, &n);

    if (buffer.EventType == KEY_EVENT && buffer.Event.KeyEvent.bKeyDown)
    {
      gchar key_val[2] = {0};

      switch (buffer.Event.KeyEvent.wVirtualKeyCode)
      {
      case VK_RIGHT:
        gst_println("Move xpos to %d", x++);
        gst_video_overlay_set_render_rectangle(GST_VIDEO_OVERLAY(sink), //// buildin function from gstreamer
                                               x, y, width, height);
        break;
      case VK_LEFT:
        gst_println("Move xpos to %d", x--);
        gst_video_overlay_set_render_rectangle(GST_VIDEO_OVERLAY(sink),
                                               x, y, width, height);
        break;
      case VK_UP:
        gst_println("Move ypos to %d", y--);
        gst_video_overlay_set_render_rectangle(GST_VIDEO_OVERLAY(sink),
                                               x, y, width, height);
        break;
      case VK_DOWN:
        gst_println("Move ypos to %d", y++);
        gst_video_overlay_set_render_rectangle(GST_VIDEO_OVERLAY(sink),
                                               x, y, width, height);
        break;
      default:
        key_val[0] = buffer.Event.KeyEvent.uChar.AsciiChar;
        switch (key_val[0])
        {
        case '<':
          gst_println("Decrease width to %d", width--);
          gst_video_overlay_set_render_rectangle(GST_VIDEO_OVERLAY(sink),
                                                 x, y, width, height);
          break;
        case '>':
          gst_println("Increase width to %d", width++);
          gst_video_overlay_set_render_rectangle(GST_VIDEO_OVERLAY(sink),
                                                 x, y, width, height);
          break;
        case '+':
          gst_println("Increase height to %d", height++);
          gst_video_overlay_set_render_rectangle(GST_VIDEO_OVERLAY(sink),
                                                 x, y, width, height);
          break;
        case '-':
          gst_println("Decrease height to %d", height--);
          gst_video_overlay_set_render_rectangle(GST_VIDEO_OVERLAY(sink),
                                                 x, y, width, height);
          break;
        case 'r':
          gst_println("Reset render rectangle by setting -1 width/height");
          gst_video_overlay_set_render_rectangle(GST_VIDEO_OVERLAY(sink),
                                                 x, y, -1, -1);
          break;
        case 'e':
          gst_println("Expose overlay");
          gst_video_overlay_expose(GST_VIDEO_OVERLAY(sink));
          break;
        case 'k':
          print_keyboard_help();
          break;
        default:
          break;
        }
        break;
      }
    }
  }

  return G_SOURCE_REMOVE;
}

//////////////////////////////////////////////////////
/// get stdin character
static gpointer
win32_kb_thread(gpointer user_data)
{
  Win32KeyHandler *handler = (Win32KeyHandler *)user_data;
  HANDLE handles[2];

  handles[0] = handler->event_handle;
  handles[1] = handler->console_handle;

  while (TRUE)
  {
    DWORD ret = WaitForMultipleObjects(2, handles, FALSE, INFINITE);
    static guint i = 0;

    if (ret == WAIT_FAILED)
    {
      g_warning("WaitForMultipleObject Failed");
      return NULL;
    }

    g_mutex_lock(&handler->lock);
    if (handler->closing)
    {
      g_mutex_unlock(&handler->lock);

      return NULL;
    }
    g_mutex_unlock(&handler->lock);

    g_idle_add((GSourceFunc)win32_kb_source_cb, handler);
  }

  return NULL;
}

/////////////////////////////////////////////
gint main(gint argc, gchar **argv)
{
  WNDCLASSEX wc = {
      0,
  };
  HINSTANCE hinstance = GetModuleHandle(NULL);
  GIOChannel *msg_io_channel;
  GOptionContext *option_ctx;
  GError *error = NULL;
  gchar *title = NULL;
  RECT wr = {0, 0, width, height};
  gint exitcode = 0;
  gboolean ret;
  GThread *thread = NULL;
  GOptionEntry options[] = {
      {"videosink", 0, 0, G_OPTION_ARG_STRING, &video_sink,
       "Video sink to use (default is glimagesink)", NULL},
      {"repeat", 0, 0, G_OPTION_ARG_NONE, &test_reuse,
       "Test reuse video sink element", NULL},
      {"fullscreen", 0, 0, G_OPTION_ARG_NONE, &test_fullscreen,
       "Test full screen (borderless topmost) mode switching via "
       "\"SPACE\" key or \"right mouse button\" click",
       NULL},
      {"run-thread", 0, 0, G_OPTION_ARG_NONE, &run_thread,
       "Run pipeline from non-window thread", NULL},
      {NULL}};

  option_ctx = g_option_context_new("WIN32 video overlay example");
  g_option_context_add_main_entries(option_ctx, options, NULL);
  g_option_context_add_group(option_ctx, gst_init_get_option_group());
  ret = g_option_context_parse(option_ctx, &argc, &argv, &error);
  g_option_context_free(option_ctx);

  if (!ret)
  {
    g_printerr("option parsing failed: %s\n", error->message);
    g_clear_error(&error);
    exit(1);
  }
  ////////////////////////////////////////////////////////////////////////////
  // END get input option

  GThread *game_pad_thread = g_thread_new("gamepad thread", gamepad_thread_func, NULL);

  /* prepare window */
  ///////////////////////////////////
  wc.cbSize = sizeof(WNDCLASSEX);
  wc.style = CS_HREDRAW | CS_VREDRAW; //// ????
  wc.lpfnWndProc = (WNDPROC)window_proc;
  wc.hInstance = hinstance;
  wc.hCursor = LoadCursor(NULL, IDC_ARROW);
  wc.lpszClassName = "GstWIN32VideoOverlay";
  RegisterClassEx(&wc);
  //////////////////////////////////////

  if (!video_sink)
    video_sink = g_strdup(DEFAULT_VIDEO_SINK);
  ///////////////// select sink element

  title = g_strdup_printf("%s - Win32-VideoOverlay", video_sink);
  AdjustWindows();
  hwnd = CreateWindowEx(0, wc.lpszClassName,
                        title,
                        WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_OVERLAPPEDWINDOW,
                        CW_USEDEFAULT, CW_USEDEFAULT,
                        wr.right - wr.left, wr.bottom - wr.top, (HWND)NULL, (HMENU)NULL,
                        hinstance, NULL);
  ////////////////////////////////////////////////////////// setup window (resolution etc etc )

  ////////////////////////////////////////// create gmain loop
  loop = g_main_loop_new(NULL, FALSE);
  msg_io_channel = g_io_channel_win32_new_messages(0);
  g_io_add_watch(msg_io_channel, G_IO_IN, msg_cb, NULL);

  {
    SECURITY_ATTRIBUTES attrs;

    attrs.nLength = sizeof(SECURITY_ATTRIBUTES);
    attrs.lpSecurityDescriptor = NULL;
    attrs.bInheritHandle = FALSE;

    //// Win32 Keyhandler is a self defined structure with input std handle + thread +
    win32_key_handler = g_new0(Win32KeyHandler, 1);

    /* create cancellable event handle */
    win32_key_handler->event_handle = CreateEvent(&attrs, TRUE, FALSE, NULL);

    win32_key_handler->console_handle = GetStdHandle(STD_INPUT_HANDLE);
    g_mutex_init(&win32_key_handler->lock);
    win32_key_handler->thread =
        g_thread_new("key-handler", win32_kb_thread, win32_key_handler);
  }
  ///////////////////////////////////////////////////////////////////////

  gst_println("Press 'k' to see a list of keyboard shortcuts");

  if (run_thread)
  {
    thread = g_thread_new("pipeline-thread",
                          (GThreadFunc)pipeline_runner_func, NULL);
    g_main_loop_run(loop);
  }
  else
  {
    pipeline_runner_func(NULL);
  }

terminate:
  if (hwnd)
    DestroyWindow(hwnd);

  if (win32_key_handler)
  {
    g_mutex_lock(&win32_key_handler->lock);
    win32_key_handler->closing = TRUE;
    g_mutex_unlock(&win32_key_handler->lock);

    SetEvent(win32_key_handler->event_handle);
    g_thread_join(win32_key_handler->thread);
    CloseHandle(win32_key_handler->event_handle);

    g_mutex_clear(&win32_key_handler->lock);
    g_free(win32_key_handler);
  }

  gst_clear_object(&sink);
  g_io_channel_unref(msg_io_channel);
  g_main_loop_unref(loop);
  g_free(title);
  g_free(video_sink);

  return exitcode;
}