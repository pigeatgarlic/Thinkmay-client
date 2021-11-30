/// <summary>
/// @file main.c
/// @author {Do Huy Hoang} ({huyhoangdo0205@gmail.com})
/// </summary>
/// @version 1.0
/// @date 2021-11-30
/// 
/// @copyright Copyright (c) 2021
/// 

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

}