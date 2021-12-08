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
#include <remote-app-type.h>


#include <gst/gst.h>
#include <glib-2.0/glib.h>







/**
 * @brief 
 * switch to fullscreen mode
 * 
 * @param hwnd 
 */
void                switch_fullscreen_mode                  (GUI* gui);

/**
 * @brief Set the up window object
 * 
 * @param gui 
 */
GUI*                init_remote_app_gui                     (RemoteApp *app);


/**
 * @brief 
 * terminate gui after use, call when remote app is finalized 
 * @param gui 
 */
void                gui_terminate                           (GUI* gui);

