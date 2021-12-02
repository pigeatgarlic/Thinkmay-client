#include <remote-app-data-channel.h>
#include <remote-app-message.h>
#include <remote-app-type.h>
#include <remote-app.h>

#include <module-code.h>

#include <glib.h>
#include <glib-object.h>
#include <gst/webrtc/webrtc.h>
#include <gst/webrtc/webrtc_fwd.h>
#include <json-glib/json-glib.h>



#include <stdio.h>
#include <module-code.h>







void
remote_app_on_message(RemoteApp* core,
						 gchar* data)
{
	QoE* qoe = remote_app_get_qoe(core);


    GError* error = NULL;
	JsonParser* parser = json_parser_new();
    JsonObject* object = get_json_object_from_string(data,&error,parser);
	if(!error == NULL || object == NULL) {return;}

	gint		from =		json_object_get_int_member(object, "From");
	gint 	to =			json_object_get_int_member(object, "To");
	gint     opcode =		json_object_get_int_member(object, "Opcode");
	gchar* data_string =	json_object_get_string_member(object, "Data");
	g_object_unref(parser);
}




void
send_message(RemoteApp* self,
			 JsonObject* message)
{
	gint to = json_object_get_int_member(message, "To");

	gchar* string_data = get_string_from_json_object(message);

	switch(to)
	{
	}
}