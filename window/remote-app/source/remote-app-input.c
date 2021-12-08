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
#include <remote-app-type.h>
#include <remote-app-data-channel.h>
#include <remote-app-gui.h>


#include <gst/video/navigation.h>
#include <gst/gst.h>

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
    /**
     * @brief 
     * reference to remote app
     */
    RemoteApp* app;
    /**
     * @brief 
     * handle gamepad event
     */
    GThread *gamepad_thread;

    /**
     * @brief 
     * 
     */
    HANDLE event_handle;


    /**
     * @brief 
     * 
     */
    gboolean capturing;


    /**
     * @brief 
     * 
     */
    GMutex lock;
};

static HIDHandler HID_handler = {0}; 

HIDHandler*
init_input_capture_system(void)
{
    return &HID_handler;
}

/**
 * @brief 
 * 
 * @param data 
 * @return gpointer 
 */
gpointer            gamepad_thread_func             (gpointer data);






void
trigger_capture_input_event(RemoteApp* app)
{
    HIDHandler* handler = remote_app_get_hid_handler(app);

    handler->capturing = TRUE;
    handler->gamepad_thread = g_thread_new("gamepad thread", 
        (GThreadFunc)gamepad_thread_func, app);
}


#else


#endif



struct _HidInput
{
    gdouble x_pos;
    gdouble y_pos;

    gdouble delta_x;
    gdouble delta_y;

    gint button_code;

    gint keyboard_code;
    gchar keyboard_string[10];

    gint wheel_dY;
    gint wheel_dX;

    HidOpcode opcode;
    gboolean relative;

    JsonObject* json;
};

static void
send_mouse_move_signal(HidInput* input,
                  RemoteApp* core)
{
    JsonObject* object = json_object_new();
    json_object_set_int_member(object,"Opcode",(gint)input->opcode);
    if(input->relative)
    {
        json_object_set_int_member(object,"dX",(gint)input->delta_x);
        json_object_set_int_member(object,"dY",(gint)input->delta_y);
    }
    else 
    {
        json_object_set_int_member(object,"dX",(gint)input->x_pos);
        json_object_set_int_member(object,"dY",(gint)input->y_pos);
    }
    hid_data_channel_send(get_string_from_json_object(object),core);
}

static void
send_mouse_button_signal(HidInput* input,
                         RemoteApp* core)
{
    JsonObject* object = json_object_new();
    json_object_set_int_member(object,"Opcode",(gint)input->opcode);
    json_object_set_int_member(object,"button",input->button_code);
    if(input->relative)
    {
        json_object_set_int_member(object,"dX",(gint)input->delta_x);
        json_object_set_int_member(object,"dY",(gint)input->delta_y);
    }
    else 
    {
        json_object_set_int_member(object,"dX",(gint)input->x_pos);
        json_object_set_int_member(object,"dY",(gint)input->y_pos);
    }
    hid_data_channel_send(get_string_from_json_object(object),core);
}

static void
send_mouse_wheel_signal(HidInput* input,
                         RemoteApp* core)
{
    JsonObject* object = json_object_new();
    json_object_set_int_member(object,"Opcode",(gint)input->opcode);
    json_object_set_int_member(object,"dY",(gint)input->wheel_dY);
    hid_data_channel_send(get_string_from_json_object(object),core);
}

static void
send_gamepad_signal(HARDWAREINPUT input,
                    RemoteApp* core)
{
    JsonObject* object = json_object_new();
    json_object_set_int_member(object,"Opcode",GAMEPAD_IN);
    json_object_set_int_member(object,"uMsg",(gint)input.uMsg);
    json_object_set_int_member(object,"wParamH",(gint)input.wParamH);
    json_object_set_int_member(object,"wParamL",(gint)input.wParamL);
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



/**
 * @brief 
 * parse human interface event and convert to thinkmay standard
 * @param input Hid structure to parse
 * @param core remote app
 */
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
        case MOUSE_RAW:
            send_mouse_button_signal(input,core);
            break;

        case MOUSE_WHEEL:
            send_mouse_wheel_signal(input,core);
            break;

        case KEYUP:
            send_key_event(input,core);
            break;
        case KEYDOWN:
            send_key_event(input,core);
            break;
        case KEYRAW:
            send_key_event(input,core);
            break;
        default:
            return;
    }
}


#ifndef WIN32_HID_CAPTURE
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
    RemoteApp* app = (RemoteApp*)data;

    XINPUT_STATE state, prevstate;
    memset(&state, 0, sizeof(XINPUT_STATE));
    memset(&prevstate, 0, sizeof(XINPUT_STATE));
    DWORD dwResult = XInputGetState(0, &prevstate);

    if (dwResult == ERROR_SUCCESS)
    {
        // Controller is connected
        while (XInputGetState(0, &state) == ERROR_SUCCESS)
        {
            //dwpacketnumber diff?
            if (state.dwPacketNumber != prevstate.dwPacketNumber)
            {
                // handle event
            }
            memcpy(&prevstate, &state, sizeof(XINPUT_STATE));
            Sleep(10);
        }
    }
    else
    {
        g_printerr("Cannot detect gamepad");
    }
}


void
send_gamepad_vibration(XINPUT_VIBRATION vibration)
{
    XInputSetState(0, &vibration);
}



static void
handle_window_mouse(RAWMOUSE input,     
                    HidInput* navigation)
{
    // https://docs.microsoft.com/en-us/windows/win32/api/winuser/ns-winuser-rawmouse
    // wheel handling 

    if ((input.usButtonFlags & RI_MOUSE_WHEEL) == RI_MOUSE_WHEEL ||
        (input.usButtonFlags & RI_MOUSE_HWHEEL) == RI_MOUSE_HWHEEL)
    {
        static const unsigned long defaultScrollLinesPerWheelDelta = 3;
        static const unsigned long defaultScrollCharsPerWheelDelta = 1;

        gint wheelDelta = (float)(short)input.usButtonData;
        gint numTicks = wheelDelta / WHEEL_DELTA;

        gboolean isHorizontalScroll = (input.usButtonFlags & RI_MOUSE_HWHEEL) == RI_MOUSE_HWHEEL;
        gboolean isScrollByPage = FALSE;
        float scrollDelta = numTicks;

        if (isHorizontalScroll)
        {
            unsigned long scrollChars = defaultScrollCharsPerWheelDelta;
            SystemParametersInfo(SPI_GETWHEELSCROLLCHARS, 0, &scrollChars, 0);
            scrollDelta *= scrollChars;
        }
        else
        {
            unsigned long scrollLines = defaultScrollLinesPerWheelDelta;
            SystemParametersInfo(SPI_GETWHEELSCROLLLINES, 0, &scrollLines, 0);
            if (scrollLines == WHEEL_PAGESCROLL)
                isScrollByPage = TRUE;
            else
                scrollDelta *= scrollLines;
        }


        navigation->opcode = MOUSE_WHEEL;
        navigation->wheel_dY = scrollDelta;
        return;
    }

    navigation->opcode = MOUSE_RAW;
    navigation->button_code = input.usButtonFlags;

    // mouse handling
    if ((input.usFlags & MOUSE_MOVE_ABSOLUTE) == MOUSE_MOVE_ABSOLUTE)
    {
        gboolean isVirtualDesktop = (input.usFlags & MOUSE_VIRTUAL_DESKTOP) == MOUSE_VIRTUAL_DESKTOP;

        gint width = GetSystemMetrics(isVirtualDesktop ? SM_CXVIRTUALSCREEN : SM_CXSCREEN);
        gint height = GetSystemMetrics(isVirtualDesktop ? SM_CYVIRTUALSCREEN : SM_CYSCREEN);

        navigation->x_pos = ((input.lLastX / 65535.0f) * width);
        navigation->y_pos = ((input.lLastY / 65535.0f) * height);
        navigation->relative = FALSE;
    }
    else if (input.lLastX != 0 || input.lLastY != 0)
    {
        navigation->delta_x = input.lLastX;
        navigation->delta_y = input.lLastY;
        navigation->relative = TRUE;
    }
}

static void
handle_window_keyboard(RAWKEYBOARD input,
                        HidInput* navigation)
{
    // https://docs.microsoft.com/en-us/windows/win32/api/winuser/ns-winuser-rawkeyboard
    gint VKey  = input.VKey;
    gint Message= input.Message;

    navigation->opcode = KEYRAW;
    navigation->keyboard_code = VKey;
}

static void
handle_window_hid(RAWHID input,
                HidInput* navigation)
{
    gchar* buffer = input.bRawData;

    for(gint i = 1; i < input.dwCount + 1; i++)
    {
        GBytes* bytes = g_bytes_new(buffer+((i-1)*input.dwSizeHid),input.dwSizeHid);
        // handle byte here
    }
}

void 
handle_message_window_proc(HWND hwnd, 
                            UINT message, 
                            WPARAM wParam, 
                            LPARAM lParam)
{
    if(!HID_handler.capturing)
        return;

    gint KeyIndex;
    HidInput* navigation = malloc(sizeof(HidInput));

    guint dwSize;
    GetRawInputData((HRAWINPUT)lParam, RID_INPUT, NULL, &dwSize, sizeof(RAWINPUTHEADER));
    gpointer buffer = malloc(dwSize);
    if (GetRawInputData((HRAWINPUT)lParam, RID_INPUT, buffer, &dwSize, sizeof(RAWINPUTHEADER)) != dwSize) 
        return;

    RAWINPUT* raw_input = (RAWINPUT*) buffer;
    if (raw_input->header.dwType == RIM_TYPEKEYBOARD) 
    {
        handle_window_keyboard(raw_input->data.keyboard,navigation);
    }
    else if (raw_input->header.dwType == RIM_TYPEMOUSE) 
    {
        handle_window_mouse(raw_input->data.mouse,navigation);
    }
    else if (raw_input->header.dwType == RIM_TYPEHID)
    {
        handle_window_hid(raw_input->data.hid,navigation);
    }
    

    parse_hid_event(navigation,HID_handler.app);
    free(navigation);
}




#endif