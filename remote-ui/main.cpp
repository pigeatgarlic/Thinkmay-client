// #include "../remote-app/include/remote-app.h"
// #include <remote-app-type.h>
#include "remoteui.h"
#include <QApplication>

#include <QMediaPlayer>
#include <QVideoWidget>
#include <QHBoxLayout>
#include <QMediaPlaylist>
//#include <QtMultimediaWidgets/QVideoWidget>
//#include <QtMultimedia/QMediaPlayer>
//#include <QtMultimedia/QMediaPlaylist>

// #include <gst/gst.h>
// #include <glib-2.0/glib.h>

// #include <string-spliter.h>
// #include <general-constant.h>

// #ifndef GST_USE_UNSTABLE_API
// #define GST_USE_UNSTABLE_API
// #endif

// static gchar signalling_url[50] = "wss://host.thinkmay.net/Handshake";
// static gchar turn[100] = "turn://thinkmaycoturn:thinkmaycoturn_password@turn:stun.thinkmay.net:3478";
// static gint  session_id = 0;
// static gchar video_codec[50] = {0};
// static gchar audio_codec[50] = {0}; 
// static gchar connection_string[200] = {0};


// #define GST_DEBUG               4

// static GOptionEntry entries[] = {
//   {"sessionid", 0, 0, G_OPTION_ARG_INT, &session_id,
//       "String ID of the peer to connect to", "ID"},
//   {"signalling", 0, 0, G_OPTION_ARG_STRING, &signalling_url,
//       "Signalling server to connect to", "URL"},
//   {"turn", 0, 0, G_OPTION_ARG_STRING, &turn,
//       "Request that the peer generate the offer and we'll answer", "URL"},
//   {"audiocodec", 0, 0, G_OPTION_ARG_STRING, &audio_codec,
//       "audio codec use for decode bin", "codec"},
//   {"videocodec", 0, 0, G_OPTION_ARG_STRING, &video_codec,
//       "video codec use for decode bin", "codec"},
//   {"connection", 0, 0, G_OPTION_ARG_STRING, &connection_string,
//       "connection ", "codec"},
//   {NULL},
// };


// char**
// split(char *string, 
//       const char delimiter) 
// {
//     int length = 0, count = 0, i = 0, j = 0;
//     while(*(string++)) {
//         if (*string == delimiter) count++;
//         length++;
//     }
//     string -= (length + 1); // string was incremented one more than length
//     char **array = (char **)malloc(sizeof(char *) * (length + 1));
//     char ** base = array;
//     for(i = 0; i < (count + 1); i++) {
//         j = 0;
//         while(string[j] != delimiter) j++;
//         j++;
//         *array = (char *)malloc(sizeof(char) * j);
//         memcpy(*array, string, (j-1));
//         (*array)[j-1] = '\0';
//         string += j;
//         array++;
//     }
//     *array = '\0';
//     return base;  
// }

// void
// string_split_free(gchar** base)
// {
//     gint i = 0;
//     while(base[i]) {
//         free(base[i]);
//         i++;
//     }
//     free(base);
//     base = NULL;
// }


int main(int argc, char* argv[])
{
    // thinkmay_init(argv[0],19);

    // GOptionContext *context;
    // GError *error = NULL;

    // context = g_option_context_new ("- thinkmay gstreamer client");
    // g_option_context_add_main_entries (context, entries, NULL);
    // g_option_context_add_group (context, gst_init_get_option_group ());
    // if (!g_option_context_parse (context, &argc, &argv, &error)) {
    //     g_printerr ("Error initializing: %s\n", error->message);
    //     return -1;
    // }

    // if(argc == 2)
    // {
    //     gchar** array = split(argv[1],'/');

    //     if(g_strcmp0(array[0],"thinkmay:"))
    //     {
    //         g_print("%s :",array[0]);
    //         g_printerr("wrong uri, remote app exiting");
    //         return;
    //     }


    //     gchar** array_param = split(array[2],'.');
    //     do
    //     {
    //         if(*(array_param))
    //         {
    //             gchar** parameter = split(*(array_param),'=');
    //             if(!g_strcmp0(*(parameter ),"sessionid"))
    //             {
    //                 gint id = strtol(*(parameter +1),NULL,10);
    //                 session_id = id;
    //             }
    //             else if(!g_strcmp0(*(parameter ),"signalling"))
    //             {
    //                 memcpy(signalling_url,*(parameter +1),strlen(*(parameter +1)));
    //             }
    //             else if(!g_strcmp0(*(parameter ),"turn"))
    //             {
    //                 memcpy(turn,*(parameter +1),strlen(*(parameter +1)));
    //             }
    //             else if(!g_strcmp0(*(parameter ),"videocodec"))
    //             {
    //                 memcpy(video_codec,*(parameter +1),strlen(*(parameter +1)));
    //             }
    //         }
    //     }
    //     while(*(array_param++));
    // }

    // g_print("Starting connnection with session client id %d, videocodec %s , signalling server url %s\n",session_id,video_codec,signalling_url);


    QApplication a(argc, argv);
    // RemoteUI w;
    // w.show();
    
    QMediaPlayer* player = new QMediaPlayer;
    QVideoWidget* vw = new QVideoWidget;

    player->setVideoOutput(vw);

//    For dev without gst:
        player->setMedia(QUrl("https://www.youtube.com/watch?v=Vdm6i1m4tDE&ab_channel=V%C5%A9Official"));
// in this method, output returnDirectShowPlayerService::doRender: Unresolved error code 0x80040218 (), but it not important
//  cause is that the application cannot decode media feed because it lacks decoding components,
//        such as codec for this video feed is not installed/available.

//    For launch: fill your gstreamer in this url
//    player->setMedia("gst-pipeline: videotestsrc ! qtvideosink");

    vw->setWindowTitle("ThinkMay Remote App");

    vw->setFullScreen(true);
    vw->show();

    player->play();


    return a.exec();

    // remote_app_initialize(session_id,
    //                 signalling_url, 
    //                 turn, 
    //                 audio_codec, 
    //                 video_codec);
    return 0;
}
