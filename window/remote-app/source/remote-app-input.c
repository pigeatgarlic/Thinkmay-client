#include <remote-app-input.h>
#include <gst/gst.h>
#include <remote-app-type.h>
#include <remote-app-data-channel.h>


#include <gst/video/navigation.h>

#include <human-interface-opcode.h>
#include <message-form.h>


#ifdef G_OS_WIN32
#define WIN32_HID_CAPTURE

#else


#endif

typedef struct
{
  GThread *thread;
  HANDLE event_handle;
  HANDLE console_handle;
  gboolean closing;
  GMutex lock;
} Win32KeyHandler;


struct _HidInput
{
    gdouble x_pos;
    gdouble y_pos;

    gdouble delta_x;
    gdouble delta_y;

    gint button_code;
    gchar* keyboard_code;

    HidOpcode opcode;




    HandleIntputFunction function;






    JsonObject* json;
};

static void
send_mouse_move_signal(HidInput* input,
                  RemoteApp* core)
{
    JsonObject* object = json_object_new();
    json_object_set_int_member(object,"Opcode",(gint)input->opcode);
    json_object_set_int_member(object,"dX",(gint)input->x_pos);
    json_object_set_int_member(object,"dY",(gint)input->y_pos);
    hid_data_channel_send(get_string_from_json_object(object),core);

    input->function(input->delta_y);
}

static void
send_mouse_button_signal(HidInput* input,
                         RemoteApp* core)
{
    JsonObject* object = json_object_new();
    json_object_set_int_member(object,"Opcode",(gint)input->opcode);
    json_object_set_int_member(object,"button",input->button_code);
    json_object_set_int_member(object,"dX",(gint)input->x_pos);
    json_object_set_int_member(object,"dY",(gint)input->y_pos);
    hid_data_channel_send(get_string_from_json_object(object),core);
}


static void
send_key_event(HidInput* input,
            RemoteApp* core)
{
    JsonObject* object = json_object_new();
    json_object_set_int_member(object,"Opcode",(gint)input->opcode);
    json_object_set_string_member(object,"wVk",input->keyboard_code);
    hid_data_channel_send(get_string_from_json_object(object),core);
}


#ifndef WIN32_HID_CAPTURE

static void
parse_hid_event(HidInput* input, 
                RemoteApp* core)
{
    switch((gint)input->opcode)
    {
        case MOUSE_MOVE:
            send_mouse_move_signal(input,core);
            break;
        case MOUSE_UP:
            send_mouse_button_signal(input,core);
            break;
        case MOUSE_DOWN:
            send_mouse_button_signal(input,core);
            break;
        case MOUSE_WHEEL:
            break;
        case KEYUP:
            send_key_event(input,core);
            break;
        case KEYDOWN:
            send_key_event(input,core);
            break;
        default:
            return;
    }
}


gboolean      
handle_navigator(GstEvent *event, 
                RemoteApp* core)
{
    HidInput* navigation = malloc(sizeof(HidInput));
    gint eventcode = gst_navigation_event_get_type(event);\
    
    switch (eventcode)
    {
        case GST_NAVIGATION_EVENT_KEY_PRESS: 
            gst_navigation_event_parse_key_event(event,&(navigation->keyboard_code));
            navigation->opcode = KEYDOWN;
            break; 
        case GST_NAVIGATION_EVENT_KEY_RELEASE: 
            gst_navigation_event_parse_key_event(event,&(navigation->keyboard_code));
            navigation->opcode = KEYUP;
            break;
        case GST_NAVIGATION_EVENT_MOUSE_MOVE: 
            gst_navigation_event_parse_mouse_move_event(event,&(navigation->x_pos),&(navigation->y_pos));
            navigation->opcode = MOUSE_MOVE;
            break; 
        case GST_NAVIGATION_EVENT_MOUSE_SCROLL: 
            gst_navigation_event_parse_mouse_scroll_event(event,&(navigation->x_pos),&(navigation->y_pos),&(navigation->delta_x),&(navigation->delta_y));
            navigation->opcode = MOUSE_WHEEL;
            break; 
        case GST_NAVIGATION_EVENT_MOUSE_BUTTON_PRESS: 
            gst_navigation_event_parse_mouse_button_event(event,&(navigation->button_code),&(navigation->x_pos),&(navigation->y_pos));
            navigation->opcode = MOUSE_DOWN;
            break; 
        case GST_NAVIGATION_EVENT_MOUSE_BUTTON_RELEASE: 
            gst_navigation_event_parse_mouse_button_event(event,&(navigation->button_code),&(navigation->x_pos),&(navigation->y_pos));
            navigation->opcode = MOUSE_UP;
            break; 
        default:
            break;
    }
    parse_hid_event(navigation,core);
    free(navigation);
}
#else
#pragma comment(lib, "XInput.lib")
#include <windows.h>
#include <Xinput.h>

gpointer 
gamepad_thread_func(gpointer data)
{
  DWORD dwResult;

  XINPUT_STATE state, prevstate;
  SecureZeroMemory(&state, sizeof(XINPUT_STATE));
  SecureZeroMemory(&prevstate, sizeof(XINPUT_STATE));
  dwResult = XInputGetState(0, &prevstate);

  DWORD wButtonPressing = 0;
  DWORD Lstick = 64;

  //key down R key and key up R key
  INPUT inputs[3];
  ZeroMemory(inputs, sizeof(inputs));
  //key down
  inputs[0].type = INPUT_KEYBOARD;
  inputs[0].ki.wVk = 0x52;

  //key up (same as inputs[0] object but with extra attr)r
  inputs[1].ki.dwFlags = KEYEVENTF_KEYUP;

  if (dwResult == ERROR_SUCCESS)
  {
    // Controller is connected
    while (XInputGetState(0, &state) == ERROR_SUCCESS)
    {
      //dwpacketnumber diff?
      if (state.dwPacketNumber != prevstate.dwPacketNumber)
      {
        //different input
        wButtonPressing = state.Gamepad.wButtons;
      }
      if (wButtonPressing & Lstick)
      {
        //std::cout << "hold" << std::endl;
        XINPUT_VIBRATION vibration;
        vibration.wLeftMotorSpeed = 65535;
        vibration.wRightMotorSpeed = 65535;
        XInputSetState(0, &vibration);
        SendInput(1, &inputs[0], sizeof(INPUT));
      }
      if (prevstate.Gamepad.wButtons & Lstick && (wButtonPressing & Lstick) == 0)
      {
        SendInput(1, &inputs[1], sizeof(INPUT));
      }
      MoveMemory(&prevstate, &state, sizeof(XINPUT_STATE));

      Sleep(10);
    }
  }
}


//////// get input character from std input
static gboolean
win32_kb_source_cb(Win32KeyHandler *handler)
{
  HANDLE h_input = handler->console_handle;
  INPUT_RECORD buffer;
  DWORD n;

  if (PeekConsoleInput(h_input, &buffer, 1, &n) && n == 1)
  {
    ReadConsoleInput(h_input, &buffer, 1, &n);

    if (buffer.EventType == KEY_EVENT && buffer.Event.KeyEvent.bKeyDown)
    {
      gchar key_val[2] = {0};

      switch (buffer.Event.KeyEvent.wVirtualKeyCode)
      {
      case VK_RIGHT:
        gst_println("Move xpos to %d", x++);
        gst_video_overlay_set_render_rectangle(GST_VIDEO_OVERLAY(sink), //// buildin function from gstreamer
                                               x, y, width, height);
        break;
      case VK_LEFT:
        gst_println("Move xpos to %d", x--);
        gst_video_overlay_set_render_rectangle(GST_VIDEO_OVERLAY(sink),
                                               x, y, width, height);
        break;
      case VK_UP:
        gst_println("Move ypos to %d", y--);
        gst_video_overlay_set_render_rectangle(GST_VIDEO_OVERLAY(sink),
                                               x, y, width, height);
        break;
      case VK_DOWN:
        gst_println("Move ypos to %d", y++);
        gst_video_overlay_set_render_rectangle(GST_VIDEO_OVERLAY(sink),
                                               x, y, width, height);
        break;
      default:
        key_val[0] = buffer.Event.KeyEvent.uChar.AsciiChar;
        switch (key_val[0])
        {
        case '<':
          gst_println("Decrease width to %d", width--);
          gst_video_overlay_set_render_rectangle(GST_VIDEO_OVERLAY(sink),
                                                 x, y, width, height);
          break;
        case '>':
          gst_println("Increase width to %d", width++);
          gst_video_overlay_set_render_rectangle(GST_VIDEO_OVERLAY(sink),
                                                 x, y, width, height);
          break;
        case '+':
          gst_println("Increase height to %d", height++);
          gst_video_overlay_set_render_rectangle(GST_VIDEO_OVERLAY(sink),
                                                 x, y, width, height);
          break;
        case '-':
          gst_println("Decrease height to %d", height--);
          gst_video_overlay_set_render_rectangle(GST_VIDEO_OVERLAY(sink),
                                                 x, y, width, height);
          break;
        case 'r':
          gst_println("Reset render rectangle by setting -1 width/height");
          gst_video_overlay_set_render_rectangle(GST_VIDEO_OVERLAY(sink),
                                                 x, y, -1, -1);
          break;
        case 'e':
          gst_println("Expose overlay");
          gst_video_overlay_expose(GST_VIDEO_OVERLAY(sink));
          break;
        case 'k':
          print_keyboard_help();
          break;
        default:
          break;
        }
        break;
      }
    }
  }

  return G_SOURCE_REMOVE;
}
//xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx




#endif