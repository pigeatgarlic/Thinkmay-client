/**
 * @file remote-app.c
 * @author {Do Huy Hoang} ({huyhoangdo0205@gmail.com})
 * @brief 
 * @version 1.0
 * @date 2021-12-02
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#include <remote-app-signalling.h>
#include <remote-app-remote-config.h>
#include <remote-app-pipeline.h>
#include <remote-app-data-channel.h>
#include <remote-app.h>
#include <remote-app-message.h>
#include <remote-app-type.h>

#include <exit-code.h>
#include <module-code.h>


#include <glib.h>
#include <logging.h>
#include <message-form.h>
#include <module-code.h>
#include <gst/base/gstbasesink.h>
#include <libsoup/soup.h>

#define SESSION_INFOR_URL "https://host.thinkmay.net/Session/Setting"

struct _RemoteApp
{
	Pipeline* pipe;

	WebRTCHub* hub;

	GMainLoop* loop;

	SignallingHub* signalling;

	QoE* qoe;

	GUI* gui;
};



/**
 * @brief 
 * setup session 
 * @param self 
 * @param session_id 
 * @param signalling_url 
 * @param turn 
 * @param audio_codec 
 * @param video_codec 
 */
static void
remote_app_setup_session(RemoteApp* self, gchar* remote_token)
{    
	const char* http_aliases[] = { "https", NULL };
	SoupSession* session = soup_session_new_with_options(
			SOUP_SESSION_SSL_STRICT, TRUE,
			SOUP_SESSION_SSL_USE_SYSTEM_CA_FILE, TRUE,
			SOUP_SESSION_HTTPS_ALIASES, http_aliases, NULL);
	Codec audio_codec = 0,video_codec = 0;
	gchar* signalling_url;
	JsonArray* STUNlist;
	GString* turn = g_string_new("turn://");
	
	SoupMessage* infor_message = soup_message_new(SOUP_METHOD_GET,SESSION_INFOR_URL);
	soup_message_headers_append(infor_message->request_headers,"Authentication",remote_token);

	soup_session_send_message(session,infor_message);


	if(infor_message->status_code == SOUP_STATUS_OK)
	{
		GError* error = NULL;
		JsonParser* parser = json_parser_new();
		JsonObject* json_infor = get_json_object_from_string(infor_message->response_body->data,error,parser);
		if (error)
			remote_app_finalize(self,0,NULL);
		
		
		
		audio_codec = json_object_get_int_member(json_infor,"AudioCodec");
		video_codec = json_object_get_int_member(json_infor,"VideoCodec");
		gchar* turnIP = json_object_get_int_member(json_infor,"turnIP");
		gchar* turnUser = json_object_get_int_member(json_infor,"turnUser");
		gchar* turnPassword = json_object_get_int_member(json_infor,"turnPassword");
		signalling_url = json_object_get_string_member(json_infor,"SignallingUrl");

		g_string_append(turn,turnUser);
		g_string_append(turn,":");
		g_string_append(turn,turnPassword);
		g_string_append(turn,"@");
		g_string_append(turn,turnIP);
		g_string_append(turn,":3478");


		STUNlist = json_object_get_array_member(json_infor,"STUNlist");
	}
	else
	{
		remote_app_finalize(self,0,NULL);
	}
	


	signalling_hub_setup(self->signalling, g_string_free(turn,FALSE),signalling_url);
	qoe_setup(self->qoe,audio_codec,video_codec);
}




RemoteApp*
remote_app_initialize(gchar* remote_token)
{
	RemoteApp* app= malloc(sizeof(RemoteApp));
	app->hub =				webrtchub_initialize();
	app->signalling =		signalling_hub_initialize(app);

	app->qoe =				qoe_initialize();
	app->pipe =				pipeline_initialize(app);
	app->loop =				g_main_loop_new(NULL, FALSE);
	 
	remote_app_setup_session(app, remote_token);


	setup_pipeline(app);

	signalling_connect(app);
	g_main_loop_run(app->loop);
	return app;	
}





















void
remote_app_finalize(RemoteApp* self, 
					  gint exit_code, 
					  GError* error)
{
	if(error)
		g_print(error->message);

	signalling_close(self->signalling);
	ExitProcess(exit_code);
}









Pipeline*
remote_app_get_pipeline(RemoteApp* self)
{
	return self->pipe;
}

WebRTCHub*
remote_app_get_rtc_hub(RemoteApp* self)
{
	return self->hub;
}


QoE*
remote_app_get_qoe(RemoteApp* self)
{
	return self->qoe;
}

GUI*
remote_app_get_gui(RemoteApp* core)
{
	return core->gui;
}

SignallingHub*
remote_app_get_signalling_hub(RemoteApp* core)
{
	return core->signalling;
}

GMainContext*
remote_app_get_main_context(RemoteApp* core)
{
	return g_main_loop_get_context(core->loop);
}