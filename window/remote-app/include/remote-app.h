/**
 * @file remote-app.h
 * @author {Do Huy Hoang} ({huyhoangdo0205@gmail.com})
 * @brief 
 * @version 1.0
 * @date 2021-12-02
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#ifndef __REMOTE_APP_H__
#define __REMOTE_APP_H__

#include "remote-app-type.h"
#include <message-form.h>






	

/**
 * @brief 
 * 
 * @param session_id 
 * @param signalling_url 
 * @param turn 
 * @param audio_codec 
 * @param video_codec 
 * @return RemoteApp* 
 */
RemoteApp*		remote_app_initialize				(gint session_id,
													gchar* signalling_url,
													gchar* turn,
													gchar* audio_codec,
													gchar* video_codec);


/**
 * @brief 
 * finalize remote app and end the remote connection
 * @param self 
 * @param exit_code 
 * @param error 
 */
void			remote_app_finalize					(RemoteApp* self,
														gint exit_code,
														GError* error);

/**
 * @brief 
 * get pipeline from remote app
 * @param self 
 * @return Pipeline* 
 */
Pipeline*		remote_app_get_pipeline				(RemoteApp* self);

/**
 * @brief 
 * get webrtc hub from remote app
 * @param self 
 * @return WebRTCHub* 
 */
WebRTCHub*		remote_app_get_rtc_hub				(RemoteApp* self);

/**
 * @brief 
 * get qoe object from remtoe app
 * @param self 
 * @return QoE* 
 */
QoE*			remote_app_get_qoe					(RemoteApp* self);



/**
 * @brief 
 * get signalling hub from remote app
 * @param core 
 * @return SignallingHub* signalling hub
 */
SignallingHub*	remote_app_get_signalling_hub			(RemoteApp* core);


/**
 * @brief 
 * remote report app error to host
 * @param self 
 * @param error message
 */
void			report_remote_app_error				(RemoteApp* self,
													gchar* error);


#endif 