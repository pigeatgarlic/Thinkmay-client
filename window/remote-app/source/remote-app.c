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
remote_app_setup_session(RemoteApp* self, 
						gint session_id, 
						gchar* signalling_url,
						gchar* turn, 
						gchar* audio_codec, 
						gchar* video_codec)
{
	Codec audio = 0,video = 0;

	GError* error = NULL;
	if(!g_strcmp0(audio_codec,"aac"))	
		audio = AAC_ENC;
	else if(!g_strcmp0(audio_codec,"opus"))
		audio = OPUS_ENC;


	if(!g_strcmp0(video_codec,"h264"))
		video = CODEC_H264;
	if(!g_strcmp0(video_codec,"h265"))
		video = CODEC_H265;
	if(!g_strcmp0(video_codec,"vp9"))
		video = CODEC_VP9;
	
	signalling_hub_setup(self->signalling,turn,signalling_url,session_id);
	qoe_setup(self->qoe,audio,video);


}




RemoteApp*
remote_app_initialize(gint session_id,
					  gchar* signalling_url,
					  gchar* turn,
					  gchar* audio_codec,
					  gchar* video_codec)
{
	RemoteApp* app= malloc(sizeof(RemoteApp));
	app->hub =				webrtchub_initialize();
	app->signalling =		signalling_hub_initialize(app);

	app->qoe =				qoe_initialize();
	app->pipe =				pipeline_initialize(app);
	app->loop =				g_main_loop_new(NULL, FALSE);
	 
	remote_app_setup_session(app, 
							session_id, 
							signalling_url, 
							turn, 
							audio_codec, 
							video_codec);


	setup_pipeline(app);

	connect_to_websocket_signalling_server_async(app);
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
	// gui_terminate(self->gui);
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