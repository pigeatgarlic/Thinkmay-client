#ifndef __REMOTE_APP_INPUT_H__
#define __REMOTE_APP_INPUT_H__
#include <gst/gst.h>
#include <remote-app-type.h>





#ifdef G_OS_WIN32
#include <windows.h>

/**
 * @brief 
 * 
 * @param hwnd 
 * @param message 
 * @param wParam 
 * @param lParam 
 */
void                handle_message_window_proc      (HWND hwnd, 
                                                    UINT message, 
                                                    WPARAM wParam, 
                                                    LPARAM lParam);
#else

/**
 * @brief 
 * handle navigation event from gstreamer 
 * @param event 
 * @param core 
 * @return gboolean 
 */
gboolean            handle_navigator                (GstEvent *event,
                                                     RemoteApp* core);

#endif
#endif