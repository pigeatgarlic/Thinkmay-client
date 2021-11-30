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

typedef struct
{
    GThread *thread;
    HANDLE event_handle;
    HANDLE console_handle;
    gboolean closing;
    GMutex lock;
} Win32KeyHandler;

static Win32KeyHandler *win32_key_handler = NULL;

void adjust_window();
gpointer gamepad_thread_func(gpointer data);
gboolean _keydown(int *key);
void switch_fullscreen_mode(HWND *hwnd);
HWND set_up_window(WNDCLASSEX wc, gchar *title, HINSTANCE hinstance);
void handle_message_window_proc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam, GstElement *sink);
gchar *select_sink_element();
gpointer win32_kb_thread(gpointer user_data);