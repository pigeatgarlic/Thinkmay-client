#include <glib.h>
#include <logging.h>
#include <gio/gio.h>
#include <string.h>

#include <message-form.h>



void
write_to_log_file(gchar* file_name,
                  gchar* text)
{
    g_print(text);
}                  