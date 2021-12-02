#ifndef __REMOTE_APP_INPUT_H__
#define __REMOTE_APP_INPUT_H__
#include <gst/gst.h>
#include <remote-app-type.h>



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