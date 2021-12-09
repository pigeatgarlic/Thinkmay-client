/**
 * @file remote-app-pipeline.c
 * @author {Do Huy Hoang} ({huyhoangdo0205@gmail.com})
 * @brief 
 * @version 1.0
 * @date 2021-12-08
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#include <remote-app-pipeline.h>
#include <remote-app-type.h>
#include <remote-app-data-channel.h>
#include <remote-app-signalling.h>
#include <remote-app-remote-config.h>
#include <remote-app-pipeline.h>
#include <remote-app-gui.h>
#include <remote-app-input.h>

#include <qoe.h>
#include <platform-selection.h>

#include <gst/gst.h>
#include <glib-2.0/glib.h>
#include <gst/webrtc/webrtc.h>




/**
 * @brief 
 * naming of gstelement
 */
enum
{
    /*screen capture source*/
    VIDEO_SINK,

    VIDEO_CONVERT,
    /*video encoder*/
    VIDEO_DECODER,

    /*payload packetize*/
    VIDEO_DEPAYLOAD,

    VIDEO_ELEMENT_LAST
};

/**
 * @brief 
 * naming of gstelement
 */
enum
{
    /*audio capture source*/
    PULSE_SINK,
    WASAPI_SINK,

    AUDIO_CONVERT,
    AUDIO_RESAMPLE,

    /*audio encoder*/
    OPUS_DECODER,
    AAC_DECODER,

    /*rtp packetize and queue*/
    AUDIO_DEPAYLOAD,
    AUDIO_SINK,

    AUDIO_ELEMENT_LAST
};


struct _Pipeline
{
    GstElement* pipeline;
    GstElement* webrtcbin;

    GstElement* video_element[VIDEO_ELEMENT_LAST];
    GstElement* audio_element[AUDIO_ELEMENT_LAST];

    GstCaps* video_caps[VIDEO_ELEMENT_LAST];
    GstCaps* audio_caps[AUDIO_ELEMENT_LAST];
};


void
setup_video_sink_navigator(RemoteApp* core);



Pipeline*
pipeline_initialize(RemoteApp* core)
{
    Pipeline* pipeline = malloc(sizeof(Pipeline));
    memset(pipeline,0,sizeof(Pipeline));
    return pipeline;
}

static gboolean
start_pipeline(RemoteApp* core)
{
    GstStateChangeReturn ret;
    Pipeline* pipe = remote_app_get_pipeline(core);

    ret = GST_IS_ELEMENT(pipe->pipeline);    

    ret = gst_element_set_state(GST_ELEMENT(pipe->pipeline), GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE)
    {
        GError error;
        error.message = "Fail to start pipeline, this may due to pipeline setup failure";
        remote_app_finalize(core, &error);
    }
    return TRUE;
}



static void
handle_audio_stream (GstPad * pad, 
                     RemoteApp* core)
{
    Pipeline* pipeline = remote_app_get_pipeline(core);

    GstElement* queue =                      gst_element_factory_make ("queue", NULL);
    pipeline->audio_element[AUDIO_CONVERT] = gst_element_factory_make ("audioconvert", NULL);
    pipeline->audio_element[AUDIO_RESAMPLE]= gst_element_factory_make ("audioresample", NULL);
    pipeline->audio_element[AUDIO_SINK] =    gst_element_factory_make ("autoaudiosink", NULL);

    /* Might also need to resample, so add it just in case.
    * Will be a no-op if it's not required. */
    gst_bin_add_many (GST_BIN (pipeline->pipeline), queue, 
        pipeline->audio_element[AUDIO_CONVERT], 
        pipeline->audio_element[AUDIO_RESAMPLE], 
        pipeline->audio_element[AUDIO_SINK], NULL);

    gst_element_sync_state_with_parent (queue);
    gst_element_sync_state_with_parent (pipeline->audio_element[AUDIO_CONVERT]);
    gst_element_sync_state_with_parent (pipeline->audio_element[AUDIO_RESAMPLE]);
    gst_element_sync_state_with_parent (pipeline->audio_element[AUDIO_SINK]);

    gst_element_link_many (queue, 
        pipeline->audio_element[AUDIO_CONVERT], 
        pipeline->audio_element[AUDIO_RESAMPLE],
        pipeline->audio_element[AUDIO_SINK], NULL);

    GstPad* queue_pad = gst_element_get_static_pad (queue, "sink");
    GstPadLinkReturn ret = gst_pad_link (pad, queue_pad);

    g_assert_cmphex (ret, ==, GST_PAD_LINK_OK);
}

static void
handle_video_stream (GstPad * pad, 
                     RemoteApp* core)
{
    Pipeline* pipeline = remote_app_get_pipeline(core);

    GstElement* queue = gst_element_factory_make ("queue", NULL);
    pipeline->video_element[VIDEO_CONVERT] = gst_element_factory_make ("videoconvert", NULL);
    pipeline->video_element[VIDEO_SINK] = gst_element_factory_make (DEFAULT_VIDEO_SINK, NULL);

    gst_bin_add_many (GST_BIN (pipeline->pipeline), queue, 
        pipeline->video_element[VIDEO_CONVERT], 
        pipeline->video_element[VIDEO_SINK], NULL);

    gst_element_sync_state_with_parent (queue);
    gst_element_sync_state_with_parent (pipeline->video_element[VIDEO_CONVERT]);
    gst_element_sync_state_with_parent (pipeline->video_element[VIDEO_SINK]);

    gst_element_link_many (queue, 
        pipeline->video_element[VIDEO_CONVERT], 
        pipeline->video_element[VIDEO_SINK], NULL);

#ifndef G_OS_WIN32
    setup_video_sink_navigator(core);
#endif
    
    GstPad* queue_pad = gst_element_get_static_pad (queue, "sink");
    GstPadLinkReturn ret = gst_pad_link (pad, queue_pad);
    g_assert_cmphex (ret, ==, GST_PAD_LINK_OK);

    trigger_capture_input_event(core);
    setup_video_overlay(pipeline->video_element[VIDEO_SINK],core);
}



/**
 * @brief 
 * 
 * @param decodebin 
 * @param pad 
 * @param data 
 */
static void
on_incoming_decodebin_stream (GstElement * decodebin, 
                              GstPad * pad,
                              gpointer data)
{
    RemoteApp* core = (RemoteApp*)data;
    Pipeline* pipeline = remote_app_get_pipeline(core);

    if (!gst_pad_has_current_caps (pad)) 
    {
        g_printerr ("Pad '%s' has no caps, can't do anything, ignoring\n",
            GST_PAD_NAME (pad));
        return;
    }

    GstCaps* caps = gst_pad_get_current_caps (pad);
    gchar*   name = gst_structure_get_name (gst_caps_get_structure (caps, 0));

    if (g_str_has_prefix (name, "video")) 
    {
        handle_video_stream(pad, core);
    } 
    else if (g_str_has_prefix (name, "audio")) 
    {
        handle_audio_stream(pad, core);
    } 
    else 
    {
      g_printerr ("Unknown pad %s, ignoring", GST_PAD_NAME (pad));
    }
}



/**
 * @brief 
 * 
 * @param webrtc 
 * @param webrtcbin_pad 
 * @param data 
 */
static void
on_incoming_stream (GstElement * webrtc, 
                    GstPad * webrtcbin_pad, 
                    gpointer data)
{
    RemoteApp* core = (RemoteApp*)data;
    if (GST_PAD_DIRECTION (webrtcbin_pad) != GST_PAD_SRC)
      return;

    Pipeline* pipeline = remote_app_get_pipeline(core);
    GstElement* pipe = pipeline->pipeline;
    
    GstCaps* caps = gst_pad_get_current_caps(webrtcbin_pad);
    gchar* encoding = gst_structure_get_string(gst_caps_get_structure(caps, 0), "encoding-name");
    gchar* name = gst_structure_get_name(gst_caps_get_structure(caps, 0));

    g_print("handling media type %s with encoding %s\n",name,encoding);

    pipeline->video_element[VIDEO_DECODER] = gst_element_factory_make ("decodebin", NULL);
    g_signal_connect (pipeline->video_element[VIDEO_DECODER], "pad-added",
        G_CALLBACK (on_incoming_decodebin_stream), core);
    gst_bin_add (GST_BIN (pipe), pipeline->video_element[VIDEO_DECODER]);

    gst_element_sync_state_with_parent (pipeline->video_element[VIDEO_DECODER]);

    pipeline->video_caps[VIDEO_DECODER] = gst_element_get_static_pad (pipeline->video_element[VIDEO_DECODER], "sink");
    gst_pad_link (webrtcbin_pad, pipeline->video_caps[VIDEO_DECODER]);
    gst_object_unref (pipeline->video_caps[VIDEO_DECODER]);
}











#ifndef G_OS_WIN32

static gboolean
handle_event(GstPad* pad, 
            GstObject* parent, 
            GstEvent* event)
{
    switch (GST_EVENT_TYPE (event)) {
      case GST_EVENT_NAVIGATION:
        handle_navigator(event,pipeline_singleton.core);
        break;
      default:
        gst_pad_event_default(pad, parent,event);
        break;
    }
}

/**
 * @brief Set the up video sink navigator object
 * 
 * @param core 
 */
void
setup_video_sink_navigator(RemoteApp* core)
{
    Pipeline* pipeline = remote_app_get_pipeline(core);
    GstPad* pad = gst_element_get_static_pad(pipeline->video_element[VIDEO_CONVERT],"src");

    gst_pad_set_event_function_full(pad,handle_event,core,NULL);
}
#endif

 



gpointer
setup_pipeline(RemoteApp* core)
{
    GstCaps *video_caps;
    GstWebRTCRTPTransceiver *trans = NULL;
    SignallingHub* signalling = remote_app_get_signalling_hub(core);
    Pipeline* pipe = remote_app_get_pipeline(core);

    GError* error = NULL;

    // pipe->pipeline = gst_pipeline_new("wecrtc client");
    // pipe->webrtcbin = gst_element_factory_make("webrtcbin","webrtcbin");
    // gst_bin_add(GST_BIN (pipe->pipeline),pipe->webrtcbin);
    // gst_element_sync_state_with_parent(pipe->webrtcbin);
    pipe->pipeline = gst_parse_launch("webrtcbin name=webrtcbin  bundle-policy=max-bundle audiotestsrc is-live=true wave=red-noise ! audioconvert ! audioresample ! queue ! opusenc ! rtpopuspay ! queue ! application/x-rtp,media=audio,payload=96,encoding-name=97 ! webrtcbin",&error);
    pipe->webrtcbin =  gst_bin_get_by_name(GST_BIN(pipe->pipeline),"webrtcbin");
    g_object_set(pipe->webrtcbin, "latency", 0, NULL);


    /* Incoming streams will be exposed via this signal */
    g_signal_connect(pipe->webrtcbin, "pad-added",
        G_CALLBACK (on_incoming_stream),core);

    gst_element_change_state(pipe->pipeline, GST_STATE_READY);
    connect_signalling_handler(core);
    connect_data_channel_signals(core);
    start_pipeline(core);
}







GstElement*
pipeline_get_webrtc_bin(Pipeline* pipe)
{
    return pipe->webrtcbin;
}

GstElement*
pipeline_get_pipline(Pipeline* pipe)
{
    return pipe->pipeline;
}



GstElement*         
pipeline_get_pipeline_element(Pipeline* pipeline)
{
    return pipeline->pipeline;
}