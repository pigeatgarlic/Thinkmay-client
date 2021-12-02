/**
 * @file remote-app-message.h
 * @author {Do Huy Hoang} ({huyhoangdo0205@gmail.com})
 * @brief 
 * @version 1.0
 * @date 2021-12-02
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#ifndef __REMOTE_APP_MESSAGE_H__
#define __REMOTE_APP_MESSAGE_H__
#include <remote-app-type.h>
#include <remote-app.h>

#include <module-code.h>
#include <message-form.h>



/**
 * @brief 
 * perform send message to worker node
 * @param self 
 * @param message 
 */
void				send_message			(RemoteApp* self,
											 JsonObject* message);
	
/**
 * @brief 
 * handle message on remote app
 * @param core 
 * @param data 
 */
void				remote_app_on_message	(RemoteApp* core,
											gchar* data);

#endif