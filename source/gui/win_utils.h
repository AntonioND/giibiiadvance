// SPDX-License-Identifier: GPL-2.0-or-later
//
// Copyright (c) 2011-2015, 2019, Antonio Niño Díaz
//
// GiiBiiAdvance - GBA/GB emulator

#ifndef WIN_UTILS__
#define WIN_UTILS__

#include <SDL.h>

#define GUI_CONSOLE_MAX_WIDTH 150
#define GUI_CONSOLE_MAX_HEIGHT 80

typedef struct
{
    int __console_chars_w;
    int __console_chars_h;
    char __console_buffer[GUI_CONSOLE_MAX_WIDTH * GUI_CONSOLE_MAX_HEIGHT];
    int __console_buffer_color[GUI_CONSOLE_MAX_WIDTH * GUI_CONSOLE_MAX_HEIGHT];
} _gui_console;

// Sizes in pixels
void GUI_ConsoleReset(_gui_console *con, int screen_width, int screen_height);
void GUI_ConsoleClear(_gui_console *con);
int GUI_ConsoleModePrintf(_gui_console *con, int x, int y, const char *txt, ...);
int GUI_ConsoleColorizeLine(_gui_console *con, int y, int color);
// The buffer is 24 bit per pixel
void GUI_ConsoleDraw(_gui_console *con, char *buffer, int buf_w, int buf_h);
void GUI_ConsoleDrawAt(_gui_console *con, char *buffer, int buf_w, int buf_h,
                       int scrx, int scry, int scrw, int scrh);

//------------------------------------------------------------------------------

#define GUI_TYPE_NONE                  0
#define GUI_TYPE_TEXTBOX               1
#define GUI_TYPE_BUTTON                2
#define GUI_TYPE_RADIOBUTTON           3
#define GUI_TYPE_LABEL                 4
#define GUI_TYPE_BITMAP                5
#define GUI_TYPE_WINDOW                6
#define GUI_TYPE_MESSAGEBOX            7
#define GUI_TYPE_SCROLLABLETEXTWINDOW  8
#define GUI_TYPE_SCROLLBAR             9
#define GUI_TYPE_GROUPBOX             10
#define GUI_TYPE_CHECKBOX             11
// Helper to get callbacks when pressing keyboard or joystick buttons
#define GUI_TYPE_INPUTGET             12

typedef void (*_gui_void_arg_void_fn)(void);
typedef void (*_gui_void_arg_int_fn)(int);
typedef void (*_gui_void_arg_int_int_fn)(int, int);
typedef int (*_gui_int_arg_int_int_fn)(int, int);
typedef void (*_gui_void_arg_pchar_int_fn)(char *, int);
typedef int (*_gui_int_arg_sdl_event_fn)(SDL_Event *);

typedef struct
{
    int element_type;
    int x, y, w, h;
    union {
        struct
        {
            _gui_console *con;
            _gui_void_arg_int_int_fn mouse_press_callback; // Args -> x, y
        } textbox;
        struct
        {
            char name[60];
            _gui_void_arg_void_fn callback;
            int is_pressed;
        } button;
        struct
        {
            char name[60];
            _gui_void_arg_int_fn callback; // Arg -> btn_id of clicked button
            int is_pressed;
            int group_id;
            int btn_id; // Button ID inside group
            int is_enabled;
        } radiobutton;
        struct
        {
            char text[100];
        } label;
        struct
        {
            char *bitmap;
            _gui_int_arg_int_int_fn callback;
        } bitmap;
        struct
        {
            int enabled;
            void *gui; // Pointer to _gui
            char caption[100];
        } window;
        struct
        {
            int enabled;
            _gui_console *con;
            char caption[100];
        } messagebox;
        struct
        {
            int enabled;
            const char *text;
            char caption[100];
            int currentline; // Top line of the window
            int numlines;    // Counted number of lines
            int max_drawn_lines;
        } scrollabletextwindow;
        struct
        {
            int is_vertical;
            int value;
            int value_min;
            int value_max;
            _gui_void_arg_int_fn callback; // Arg -> newvalue
        } scrollbar;
        struct
        {
            char label[100];
        } groupbox;
        struct
        {
            int checked;
            char label[100];
            _gui_void_arg_int_fn callback; // Arg -> 1 = is checked
        } checkbox;
        struct
        {
            // Return 1 if you want to redraw the GUI
            _gui_int_arg_sdl_event_fn callback;
        } inputget;
    } info;
} _gui_element;

//------------------------------------------------------------------------------

void GUI_SetTextBox(_gui_element *e, _gui_console *con, int x, int y, int w,
                    int h, _gui_void_arg_int_int_fn callback);

void GUI_SetButton(_gui_element *e, int x, int y, int w, int h,
                   const char *label, _gui_void_arg_void_fn callback);

void GUI_SetRadioButton(_gui_element *e, int x, int y, int w, int h,
                        const char *label, int group_id, int btn_id,
                        int start_pressed, _gui_void_arg_int_fn callback);

// if (w <= 0) w = strlen(label) * FONT_WIDTH;
void GUI_SetLabel(_gui_element *e, int x, int y, int w, int h,
                  const char *label);

// Bitmap is 24 bit. return 1 from callback to redraw GUI
void GUI_SetBitmap(_gui_element *e, int x, int y, int w, int h, char *bitmap,
                   _gui_int_arg_int_int_fn callback);

// gui is a _gui pointer
void GUI_SetWindow(_gui_element *e, int x, int y, int w, int h, void *gui,
                   const char *caption);

void GUI_SetMessageBox(_gui_element *e, _gui_console *con, int x, int y, int w,
                       int h, const char *caption);

void GUI_SetScrollableTextWindow(_gui_element *e, int x, int y, int w, int h,
                                 const char *text, const char *caption);

void GUI_SetScrollBar(_gui_element *e, int x, int y, int w, int h,
                      int min_value, int max_value, int start_value,
                      _gui_void_arg_int_fn callback);

void GUI_SetGroupBox(_gui_element *e, int x, int y, int w, int h,
                     const char *label);

// w is ignored, left for consistency
void GUI_SetCheckBox(_gui_element *e, int x, int y, int w, int h,
                     const char *label, int start_pressed,
                     _gui_void_arg_int_fn callback);

// Return 1 from callback if you want to redraw the GUI
void GUI_SetInputGet(_gui_element *e, _gui_int_arg_sdl_event_fn callback);

//------------------------------------------------------------------------------

void GUI_SetButtonText(_gui_element *e, const char *text);

void GUI_SetLabelCaption(_gui_element *e, const char *label);

// The value is clamped to the configured range
void GUI_ScrollBarSetValue(_gui_element *e, int value);

void GUI_RadioButtonSetEnabled(_gui_element *e, int enabled);
// complete_gui is a _gui pointer
void GUI_RadioButtonSetPressed(void *complete_gui, _gui_element *e);

void GUI_WindowSetEnabled(_gui_element *e, int enabled);
int GUI_WindowGetEnabled(_gui_element *e);

void GUI_MessageBoxSetEnabled(_gui_element *e, int enabled);
int GUI_MessageBoxGetEnabled(_gui_element *e);

void GUI_ScrollableTextWindowSetEnabled(_gui_element *e, int enabled);
int GUI_ScrollableTextWindowGetEnabled(_gui_element *e);

//------------------------------------------------------------------------------

//------------------------------------------------------------------------------

#define GUI_INPUTWINDOW_MAX_LEN 30
#define GUI_INPUTWINDOW_WIDTH   ((GUI_INPUTWINDOW_MAX_LEN + 2) * FONT_WIDTH)
#define GUI_INPUTWINDOW_HEIGHT  50

typedef struct
{
    int enabled;
    char window_caption[GUI_INPUTWINDOW_MAX_LEN + 1];
    char input_text[GUI_INPUTWINDOW_MAX_LEN + 1];
    _gui_void_arg_pchar_int_fn callback;
    //int outputtype
} _gui_inputwindow;

//------------------------------------------------------------------------------

// callback returns (char * text, int is_valid_text)
void GUI_InputWindowOpen(_gui_inputwindow *win, char *caption,
                         _gui_void_arg_pchar_int_fn callback);
void GUI_InputWindowClose(_gui_inputwindow *win);
int GUI_InputWindowIsEnabled(_gui_inputwindow *win);
int GUI_InputWindowSendEvent(_gui_inputwindow *win, SDL_Event *e);

//------------------------------------------------------------------------------

//------------------------------------------------------------------------------

typedef struct
{
    char *text;
    _gui_void_arg_void_fn callback;
    int enabled; // 1 -> enabled
} _gui_menu_entry;

typedef struct
{
    char *title;
    _gui_menu_entry **entry;
} _gui_menu_list;

typedef struct
{
    int element_opened;
    _gui_menu_list **list_entry;
} _gui_menu;

//------------------------------------------------------------------------------

//------------------------------------------------------------------------------

//------------------------------------------------------------------------------

typedef struct
{
    _gui_element **elements;
    _gui_inputwindow *inputwindow;
    _gui_menu *menu;
} _gui;

//------------------------------------------------------------------------------

#include "win_utils_draw.h"

#include "win_utils_events.h"

#endif // WIN_UTILS__
