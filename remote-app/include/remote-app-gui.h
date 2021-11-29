#include <windows.h>
#include <WinUser.h>
#include <remote-app.h>
#include <libloaderapi.h>
#include <gst/gst.h>
#include <gst/video/videooverlay.h>
#include <gst/video/gstvideosink.h>
#include <Xinput.h> 
void AdjustWindows();
gpointer gamepad_thread_func(gpointer data);
gboolean _keydown(int *key);
gboolean bus_msg(GstBus *bus, GstMessage *msg, gpointer user_data);
void switch_fullscreen_mode(HWND hwnd);