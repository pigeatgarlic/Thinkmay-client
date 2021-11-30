#include <windows.h>
#include <WinUser.h>
#include <remote-app.h>
#include <libloaderapi.h>
#include <gst/gst.h>
#include <gst/video/videooverlay.h>
#include <gst/video/gstvideosink.h>
#include <Xinput.h>
#include <glib-2.0/glib.h>
#include <gst/video/videooverlay.h>
#include <gst/video/gstvideosink.h>

void adjust_window();
gpointer gamepad_thread_func(gpointer data);
gboolean _keydown(int *key);
void switch_fullscreen_mode(HWND *hwnd);
HWND set_up_window(WNDCLASSEX wc, gchar *title, HINSTANCE hinstance);
void handle_message_window_proc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
gchar* select_sink_element();