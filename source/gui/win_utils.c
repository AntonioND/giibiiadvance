// SPDX-License-Identifier: GPL-2.0-or-later
//
// Copyright (c) 2011-2015, 2019-2020, Antonio Niño Díaz
//
// GiiBiiAdvance - GBA/GB emulator

#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include <SDL2/SDL.h>

#include "win_utils.h"
#include "win_utils_draw.h"

#include "../font_utils.h"
#include "../general_utils.h"

//------------------------------------------------------------------------------

// Values in pixels
void GUI_ConsoleReset(_gui_console *con, int screen_width, int screen_height)
{
    GUI_ConsoleClear(con);
    con->__console_chars_w = screen_width / FONT_WIDTH;
    con->__console_chars_h = screen_height / FONT_HEIGHT;

    if (con->__console_chars_w > GUI_CONSOLE_MAX_WIDTH)
        con->__console_chars_w = GUI_CONSOLE_MAX_WIDTH;
    if (con->__console_chars_h > GUI_CONSOLE_MAX_HEIGHT)
        con->__console_chars_h = GUI_CONSOLE_MAX_HEIGHT;
}

void GUI_ConsoleClear(_gui_console *con)
{
    memset(con->__console_buffer, 0, sizeof(con->__console_buffer));
    memset(con->__console_buffer_color, 0xFF,
           sizeof(con->__console_buffer_color));
}

int GUI_ConsoleModePrintf(_gui_console *con, int x, int y, const char *txt, ...)
{
    char txtbuffer[1000];

    va_list args;
    va_start(args, txt);
    vsnprintf(txtbuffer, sizeof(txtbuffer), txt, args);
    va_end(args);
    txtbuffer[sizeof(txtbuffer) - 1] = '\0';

    if (x > con->__console_chars_w)
        return 0;
    if (y > con->__console_chars_h)
        return 0;

    int i = 0;
    while (1)
    {
        unsigned char c = txtbuffer[i++];
        if (c == '\0')
        {
            break;
        }
        else if (c == '\n')
        {
            x = 0;
            y++;
            if (y >= con->__console_chars_h)
                break;
        }
        else
        {
            con->__console_buffer[y * con->__console_chars_w + x] = c;
            x++;
        }

        if (x >= con->__console_chars_w)
        {
            x = 0;
            y++;
            if (y >= con->__console_chars_h)
                break;
        }
    }

    return i;
}

int GUI_ConsoleColorizeLine(_gui_console *con, int y, int color)
{
    if (y > con->__console_chars_h)
        return 0;

    for (int x = 0; x < con->__console_chars_w; x++)
    {
        con->__console_buffer_color[y * con->__console_chars_w + x] = color;
        if (con->__console_buffer[y * con->__console_chars_w + x] == 0)
            con->__console_buffer[y * con->__console_chars_w + x] = ' ';
    }
    return 1;
}

// buffer is 24 bit per pixel
void GUI_ConsoleDraw(_gui_console *con, char *buffer, int buf_w, int buf_h)
{
    for (int y = 0; y < con->__console_chars_h; y++)
    {
        for (int x = 0; x < con->__console_chars_w; x++)
        {
            unsigned char c;
            int color;

            c = con->__console_buffer[y * con->__console_chars_w + x];
            color = con->__console_buffer_color[y * con->__console_chars_w + x];

            if (c)
            {
                FU_PrintChar(buffer, buf_w, buf_h,
                             x * FONT_WIDTH, y * FONT_HEIGHT, c, color);
            }
        }
    }
}

void GUI_ConsoleDrawAt(_gui_console *con, char *buffer, int buf_w, int buf_h,
                       int scrx, int scry, unused__ int scrw, unused__ int scrh)
{
    for (int y = 0; y < con->__console_chars_h; y++)
    {
        for (int x = 0; x < con->__console_chars_w; x++)
        {
            unsigned char c;
            int color;

            c = con->__console_buffer[y * con->__console_chars_w + x];
            color = con->__console_buffer_color[y * con->__console_chars_w + x];

            if (c)
            {
                FU_PrintChar(buffer, buf_w, buf_h, scrx + x * FONT_WIDTH,
                             scry + y * FONT_HEIGHT, c, color);
            }
        }
    }
}

//------------------------------------------------------------------------------

void GUI_InputWindowOpen(_gui_inputwindow *win, char *caption,
                         _gui_void_arg_pchar_int_fn callback)
{
    if (win == NULL)
        return;

    win->enabled = 1;
    win->input_text[0] = '\0';
    win->callback = callback;
    s_strncpy(win->window_caption, caption, sizeof(win->window_caption));
}

void GUI_InputWindowClose(_gui_inputwindow *win)
{
    if (win == NULL)
        return;

    win->enabled = 0;
    if (win->callback)
        win->callback(win->input_text, 0);
}

int GUI_InputWindowIsEnabled(_gui_inputwindow *win)
{
    if (win == NULL)
        return 0;

    return win->enabled;
}

//------------------------------------------------------------------------------

void GUI_SetTextBox(_gui_element *e, _gui_console *con, int x, int y, int w,
                    int h, _gui_void_arg_int_int_fn callback)
{
    if (e == NULL)
        return;

    e->element_type = GUI_TYPE_TEXTBOX;

    e->info.textbox.con = con;
    e->x = x;
    e->y = y;
    e->w = w;
    e->h = h;
    e->info.textbox.mouse_press_callback = callback;

    GUI_ConsoleReset(con, w, h);
}

void GUI_SetButton(_gui_element *e, int x, int y, int w, int h,
                   const char *label, _gui_void_arg_void_fn callback)
{
    if (e == NULL)
        return;

    e->element_type = GUI_TYPE_BUTTON;

    e->x = x;
    e->y = y;
    e->w = w;
    e->h = h;
    e->info.button.callback = callback;
    s_strncpy(e->info.button.name, label, sizeof(e->info.button.name));
}

void GUI_SetRadioButton(_gui_element *e, int x, int y, int w, int h,
                        const char *label, int group_id, int btn_id,
                        int start_pressed, _gui_void_arg_int_fn callback)
{
    if (e == NULL)
        return;

    e->element_type = GUI_TYPE_RADIOBUTTON;

    e->x = x;
    e->y = y;
    e->w = w;
    e->h = h;
    e->info.radiobutton.callback = callback;
    e->info.radiobutton.group_id = group_id;
    e->info.radiobutton.btn_id = btn_id;
    e->info.radiobutton.is_pressed = start_pressed;
    e->info.radiobutton.is_enabled = 1;
    s_strncpy(e->info.radiobutton.name, label,
              sizeof(e->info.radiobutton.name));
}

void GUI_SetLabel(_gui_element *e, int x, int y, int w, int h,
                  const char *label)
{
    if (e == NULL)
        return;

    e->element_type = GUI_TYPE_LABEL;

    e->x = x;
    e->y = y;
    if (w <= 0)
        w = strlen(label) * FONT_WIDTH;
    e->w = w;
    e->h = h;
    s_strncpy(e->info.label.text, label, sizeof(e->info.label.text));
}

// 24-bit buffer
void GUI_SetBitmap(_gui_element *e, int x, int y, int w, int h, char *bitmap,
                   _gui_int_arg_int_int_fn callback)
{
    if (e == NULL)
        return;

    e->element_type = GUI_TYPE_BITMAP;

    e->x = x;
    e->y = y;
    e->w = w;
    e->h = h;
    e->info.bitmap.bitmap = bitmap;
    e->info.bitmap.callback = callback;
}

void GUI_SetWindow(_gui_element *e, int x, int y, int w, int h, void *gui,
                   const char *caption)
{
    if (e == NULL)
        return;

    e->element_type = GUI_TYPE_WINDOW;

    e->x = x;
    e->y = y;
    e->w = w;
    e->h = h;
    e->info.window.gui = gui;
    e->info.window.enabled = 0;
    s_strncpy(e->info.window.caption, caption, sizeof(e->info.window.caption));

    _gui *relative_gui = gui;

    if (relative_gui == NULL)
        return;

    _gui_element **gui_elements = relative_gui->elements;

    if (gui_elements == NULL)
        return;

    while (*gui_elements != NULL)
    {
        (*gui_elements)->x += x;
        (*gui_elements)->y += y + FONT_HEIGHT + 2;
        gui_elements++;
    }
}

void GUI_SetMessageBox(_gui_element *e, _gui_console *con, int x, int y, int w,
                       int h, const char *caption)
{
    if (e == NULL)
        return;

    e->element_type = GUI_TYPE_MESSAGEBOX;

    e->info.messagebox.enabled = 0;
    e->info.messagebox.con = con;
    e->x = x;
    e->y = y;
    e->w = w;
    e->h = h;
    s_strncpy(e->info.messagebox.caption, caption,
              sizeof(e->info.messagebox.caption));

    GUI_ConsoleReset(con, w, h - FONT_WIDTH - 1);
}

int _gui_word_fits(const char *text, int x_start, int x_end)
{
    while (1)
    {
        char c = *text++;
        if (c == '\0')
            break;
        else if (c == '\n')
            break;
        else if (c == ' ')
            break;
        else if (c == '\t')
            break;
        else
            x_start++;
    }

    return (x_start <= x_end);
}

void GUI_SetScrollableTextWindow(_gui_element *e, int x, int y, int w, int h,
                                 const char *text, const char *caption)
{
    if (e == NULL)
        return;

    e->element_type = GUI_TYPE_SCROLLABLETEXTWINDOW;

    w = FONT_WIDTH * (w / FONT_WIDTH);
    h = (FONT_HEIGHT * ((h - (FONT_HEIGHT + 2)) / FONT_HEIGHT))
        + (FONT_HEIGHT + 2);

    e->x = x;
    e->y = y;
    e->w = w;
    e->h = h;
    e->info.scrollabletextwindow.text = text;
    e->info.scrollabletextwindow.enabled = 0;
    s_strncpy(e->info.scrollabletextwindow.caption, caption,
              sizeof(e->info.scrollabletextwindow.caption));
    e->info.scrollabletextwindow.numlines = 0;
    e->info.scrollabletextwindow.currentline = 0;

    e->info.scrollabletextwindow.max_drawn_lines =
            (e->h - (FONT_HEIGHT + 1)) / FONT_HEIGHT;

    // Count number of text lines

    int textwidth = (e->w / FONT_WIDTH) - 1;

    int skipspaces = 0;

    int i = 0;
    int curx = 0;
    while (1)
    {
        char c = text[i++];
        if (skipspaces)
        {
            while (1)
            {
                if (!((c == ' ') || (c == '\0')))
                    break;
                c = text[i++];
            }
            skipspaces = 0;
        }

        if (c == '\0')
        {
            break;
        }
        else if (c == '\n')
        {
            curx = 0;
            e->info.scrollabletextwindow.numlines++;
        }
        else
        {
            if (_gui_word_fits(&(text[i - 1]), curx, textwidth) == 0)
            {
                curx = 0;
                e->info.scrollabletextwindow.numlines++;
                i--;
                skipspaces = 1;
            }
            else
            {
                curx++;
                if (curx == textwidth)
                {
                    if (text[i] == '\n')
                        i++;
                    else
                        skipspaces = 1;
                    e->info.scrollabletextwindow.numlines++;
                    curx = 0;
                }
            }
        }
    }

    e->info.scrollabletextwindow.numlines++;
}

void GUI_SetScrollBar(_gui_element *e, int x, int y, int w, int h,
                      int min_value, int max_value, int start_value,
                      _gui_void_arg_int_fn callback)
{
    if (e == NULL)
        return;

    e->element_type = GUI_TYPE_SCROLLBAR;

    e->x = x;
    e->y = y;
    e->w = w;
    e->h = h;

    if (w > h)
        e->info.scrollbar.is_vertical = 0;
    else
        e->info.scrollbar.is_vertical = 1;

    e->info.scrollbar.value_min = min_value;
    e->info.scrollbar.value_max = max_value;
    e->info.scrollbar.value = start_value;
    e->info.scrollbar.callback = callback;
}

void GUI_SetGroupBox(_gui_element *e, int x, int y, int w, int h,
                     const char *label)
{
    if (e == NULL)
        return;

    e->element_type = GUI_TYPE_GROUPBOX;

    e->x = x;
    e->y = y;
    e->w = w;
    e->h = h;
    s_strncpy(e->info.groupbox.label, label, sizeof(e->info.groupbox.label));
}

void GUI_SetCheckBox(_gui_element *e, int x, int y, int w, int h,
                     const char *label, int start_pressed,
                     _gui_void_arg_int_fn callback)
{
    if (e == NULL)
        return;

    e->element_type = GUI_TYPE_CHECKBOX;

    e->x = x;
    e->y = y;
    e->w = w;
    e->h = h;
    s_strncpy(e->info.checkbox.label, label, sizeof(e->info.checkbox.label));
    e->info.checkbox.checked = start_pressed;
    e->info.checkbox.callback = callback;
}

void GUI_SetInputGet(_gui_element *e, _gui_int_arg_sdl_event_fn callback)
{
    if (e == NULL)
        return;

    e->element_type = GUI_TYPE_INPUTGET;

    e->info.inputget.callback = callback;
}

//------------------------------------------------------------------------------

void GUI_SetButtonText(_gui_element *e, const char *text)
{
    if (e == NULL)
        return;

    if (e->element_type != GUI_TYPE_BUTTON)
        return;

    s_strncpy(e->info.button.name, text, sizeof(e->info.button.name));
}

// The value is clamped to configured range
void GUI_ScrollBarSetValue(_gui_element *e, int value)
{
    if (e == NULL)
        return;

    if (e->element_type != GUI_TYPE_SCROLLBAR)
        return;

    e->info.scrollbar.value = value;

    if (e->info.scrollbar.value < e->info.scrollbar.value_min)
        e->info.scrollbar.value = e->info.scrollbar.value_min;
    else if (e->info.scrollbar.value > e->info.scrollbar.value_max)
        e->info.scrollbar.value = e->info.scrollbar.value_max;
}

void GUI_SetLabelCaption(_gui_element *e, const char *label)
{
    if (e == NULL)
        return;

    if (e->element_type != GUI_TYPE_LABEL)
        return;

    s_strncpy(e->info.label.text, label, sizeof(e->info.label.text));
}

void _gui_clear_radiobuttons(_gui_element **element_list, int group_id)
{
    while (*element_list != NULL)
    {
        _gui_element *e = *element_list;

        if (e->element_type == GUI_TYPE_RADIOBUTTON)
        {
            if (e->info.radiobutton.group_id == group_id)
                e->info.radiobutton.is_pressed = 0;
        }

        element_list++;
    }
}

void GUI_RadioButtonSetEnabled(_gui_element *e, int enabled)
{
    if (e == NULL)
        return;

    if (e->element_type != GUI_TYPE_RADIOBUTTON)
        return;

    e->info.radiobutton.is_enabled = enabled;
}

void GUI_RadioButtonSetPressed(void *complete_gui, _gui_element *e)
{
    if (e == NULL)
        return;

    if (e->element_type != GUI_TYPE_RADIOBUTTON)
        return;

    _gui *g = complete_gui;

    _gui_clear_radiobuttons(g->elements, e->info.radiobutton.group_id);

    e->info.radiobutton.is_pressed = 1;
}

void GUI_WindowSetEnabled(_gui_element *e, int enabled)
{
    if (e == NULL)
        return;

    if (e->element_type != GUI_TYPE_WINDOW)
        return;

    e->info.window.enabled = enabled;

    _gui *wingui = e->info.window.gui;

    if (wingui == NULL)
        return;

    _gui_element **gui_element = wingui->elements;

    while (*gui_element != NULL)
    {
        if ((*gui_element)->element_type == GUI_TYPE_BUTTON)
        {
            (*gui_element)->info.button.is_pressed = 0;
        }

        gui_element++;
    }
}

int GUI_WindowGetEnabled(_gui_element *e)
{
    if (e == NULL)
        return 0;

    if (e->element_type == GUI_TYPE_WINDOW)
        return e->info.window.enabled;

    return 0;
}

void GUI_MessageBoxSetEnabled(_gui_element *e, int enabled)
{
    if (e == NULL)
        return;

    if (e->element_type != GUI_TYPE_MESSAGEBOX)
        return;

    e->info.messagebox.enabled = enabled;
}

int GUI_MessageBoxGetEnabled(_gui_element *e)
{
    if (e == NULL)
        return 0;

    if (e->element_type == GUI_TYPE_MESSAGEBOX)
        return e->info.messagebox.enabled;

    return 0;
}

void GUI_ScrollableTextWindowSetEnabled(_gui_element *e, int enabled)
{
    if (e == NULL)
        return;

    if (e->element_type != GUI_TYPE_SCROLLABLETEXTWINDOW)
        return;

    e->info.scrollabletextwindow.enabled = enabled;
}

int GUI_ScrollableTextWindowGetEnabled(_gui_element *e)
{
    if (e == NULL)
        return 0;

    if (e->element_type == GUI_TYPE_SCROLLABLETEXTWINDOW)
        return e->info.scrollabletextwindow.enabled;

    return 0;
}
