/**
 * @file remote-app-input.c
 * @author {Do Huy Hoang} ({huyhoangdo0205@gmail.com})
 * @brief 
 * @version 1.0
 * @date 2021-12-02
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#include <remote-app-input.h>
#include <gst/gst.h>
#include <remote-app-type.h>
#include <remote-app-data-channel.h>


#include <gst/video/navigation.h>

#include <human-interface-opcode.h>
#include <message-form.h>


#ifdef G_OS_WIN32

#pragma comment(lib, "XInput.lib")
#include <windows.h>
#include <Xinput.h>
#include <Windows.h>

#define WIN32_HID_CAPTURE




struct _HIDHandler
{
    GThread *gamepad_thread;
    HANDLE event_handle;
    gboolean closing;
    GMutex lock;
};

#else


#endif



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

/**
 * @brief 
 * 
 * @param data 
 * @return gpointer 
 */
static gpointer 
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



/**
 * @brief 
 * detect if a key is pressed
 * @param key 
 * @return gboolean 
 */
gboolean
_keydown(int *key)
{
    return (GetAsyncKeyState(key) & 0x8000) != 0;
}




void 
handle_message_window_proc(HWND hwnd, 
                            UINT message, 
                            WPARAM wParam, 
                            LPARAM lParam)
{
    switch (message)
    {
        case WM_CHAR:
          if (_keydown(0x11) && _keydown(0xA0) && _keydown(0x46)) {
            switch_fullscreen_mode(hwnd);
          } if (_keydown(0x11) && _keydown(0xA0) && _keydown(0x50)) {
            // hidden mouse setting func
          } break;
        case WM_MOUSEWHEEL:
          if (GET_WHEEL_DELTA_WPARAM(wParam) < 0) {
            // negative indicates a rotation towards the user (down)
          } else if (GET_WHEEL_DELTA_WPARAM(wParam) > 0) {
            // a positive indicate the wheel is rotated away from the user (up)
          } break;
        case WM_LBUTTONDOWN:
          break;
        case WM_LBUTTONUP:
          break;
        case WM_RBUTTONDOWN:
          break;
        case WM_RBUTTONUP:
          break;
        case WM_MBUTTONDOWN:
          break;
        case WM_MBUTTONUP: // awaiting for test
          break;
        default:
          break;
    }
}

/**
 * @brief 
 * 
 * @param hid 
 */
void
remote_app_input_setup_gamepad(HIDHandler* hid)
{
    hid->gamepad_thread = g_thread_new("gamepad thread", 
        (GThreadFunc)gamepad_thread_func, NULL);
}



#endif