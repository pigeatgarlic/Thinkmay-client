/**
 * @file remote-app-gui.h
 * @author {Do Huy Hoang} ({huyhoangdo0205@gmail.com})
 * @brief 
 * @version 1.0
 * @date 2021-12-01
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#include <remote-app.h>

#ifdef G_OS_WIN32
#include <windows.h>
#include <WinUser.h>
#include <libloaderapi.h>
#include <Xinput.h>
#endif

#include <gst/gst.h>
#include <gst/video/videooverlay.h>
#include <gst/video/gstvideosink.h>
#include <glib-2.0/glib.h>
#include <gst/video/videooverlay.h>
#include <gst/video/gstvideosink.h>




void                adjust_window                           ();

gpointer            gamepad_thread_func                     (gpointer data);

gboolean            _keydown                                (int *key);

void                switch_fullscreen_mode                  (HWND *hwnd);

HWND                set_up_window                           (WNDCLASSEX wc, 
                                                            gchar *title, 
                                                            HINSTANCE hinstance);

void                handle_message_window_proc              (HWND hwnd, 
                                                            UINT message, 
                                                            WPARAM wParam, 
                                                            LPARAM lParam);

gchar               *select_sink_element                    ();

gpointer            win32_kb_thread                         (gpointer user_data);