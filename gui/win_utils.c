/*
    GiiBiiAdvance - GBA/GB  emulator
    Copyright (C) 2011-2014 Antonio Niño Díaz (AntonioND)

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include <SDL2/SDL.h>

#include "win_utils.h"
#include "win_utils_draw.h"

#include "../font_utils.h"
#include "../general_utils.h"

//-------------------------------------------------------------------------------------------

void GUI_ConsoleReset(_gui_console * con, int screen_width, int screen_height) // in pixels
{
    GUI_ConsoleClear(con);
    con->__console_chars_w = screen_width/FONT_WIDTH;
    con->__console_chars_h = screen_height/FONT_HEIGHT;
    if(con->__console_chars_w > GUI_CONSOLE_MAX_WIDTH)
        con->__console_chars_w = GUI_CONSOLE_MAX_WIDTH;
    if(con->__console_chars_h > GUI_CONSOLE_MAX_HEIGHT)
        con->__console_chars_h = GUI_CONSOLE_MAX_HEIGHT;
}

void GUI_ConsoleClear(_gui_console * con)
{
    memset(con->__console_buffer,0,sizeof(con->__console_buffer));
    memset(con->__console_buffer_color,0xFF,sizeof(con->__console_buffer_color));
}

int GUI_ConsoleModePrintf(_gui_console * con, int x, int y, const char * txt, ...)
{
    char txtbuffer[1000];
    va_list args;
    va_start(args,txt);
    vsnprintf(txtbuffer, sizeof(txtbuffer), txt, args);
    va_end(args);
    txtbuffer[sizeof(txtbuffer)-1] = '\0';

    if(x > con->__console_chars_w) return 0;
    if(y > con->__console_chars_h) return 0;

    int i = 0;
    while(1)
    {
        unsigned char c = txtbuffer[i++];
        if(c == '\0')
        {
            break;
        }
        else if(c == '\n')
        {
            x = 0;
            y++;
            if(y >= con->__console_chars_h) break;
        }
        else
        {
            con->__console_buffer[y*con->__console_chars_w+x] = c;
            x++;
        }

        if(x >= con->__console_chars_w)
        {
            x = 0;
            y++;
            if(y >= con->__console_chars_h) break;
        }
    }

    return i;
}

int GUI_ConsoleColorizeLine(_gui_console * con, int y, int color)
{
    if(y > con->__console_chars_h) return 0;
    int x;
    for(x = 0; x < con->__console_chars_w; x++)
    {
        con->__console_buffer_color[y*con->__console_chars_w+x] = color;
        if(con->__console_buffer[y*con->__console_chars_w+x] == 0)
            con->__console_buffer[y*con->__console_chars_w+x] = ' ';
    }
    return 1;
}

void GUI_ConsoleDraw(_gui_console * con, char * buffer, int buf_w, int buf_h) // buffer is 24 bit per pixel
{
    int x,y;
    for(y = 0; y < con->__console_chars_h; y++) for(x = 0; x < con->__console_chars_w; x++)
    {
        unsigned char c = con->__console_buffer[y*con->__console_chars_w+x];
        int color = con->__console_buffer_color[y*con->__console_chars_w+x];
        if(c) FU_PrintChar(buffer,buf_w,buf_h,x*FONT_WIDTH,y*FONT_HEIGHT,c,color);
    }
}

void GUI_ConsoleDrawAt(_gui_console * con, char * buffer, int buf_w, int buf_h, int scrx, int scry, int scrw, int scrh)
{
    int x,y;
    for(y = 0; y < con->__console_chars_h; y++) for(x = 0; x < con->__console_chars_w; x++)
    {
        unsigned char c = con->__console_buffer[y*con->__console_chars_w+x];
        int color = con->__console_buffer_color[y*con->__console_chars_w+x];
        if(c)
        {
            FU_PrintChar(buffer,buf_w,buf_h,scrx+x*FONT_WIDTH,scry+y*FONT_HEIGHT,c,color);
        }
    }
}

//-------------------------------------------------------------------------------------------

void GUI_InputWindowOpen(_gui_inputwindow * win, char * caption, _gui_void_arg_pchar_int_fn callback)
{
    if(win == NULL) return;

    win->enabled = 1;
    win->input_text[0] = '\0';
    win->callback = callback;
    s_strncpy(win->window_caption,caption,sizeof(win->window_caption));
}

void GUI_InputWindowClose(_gui_inputwindow * win)
{
    if(win == NULL) return;

    win->enabled = 0;
    if(win->callback)
        win->callback(win->input_text,0);
}

int GUI_InputWindowIsEnabled(_gui_inputwindow * win)
{
    if(win == NULL) return 0;
    return win->enabled;
}

//---------------------------------------------------------------------------------

void GUI_SetTextBox(_gui_element * e, _gui_console * con, int x, int y, int w, int h,
                    _gui_void_arg_int_int_fn callback)
{
    e->element_type = GUI_TYPE_TEXTBOX;

    e->info.textbox.con = con;
    e->x = x;
    e->y = y;
    e->w = w;
    e->h = h;
    e->info.textbox.mouse_press_callback = callback;

    GUI_ConsoleReset(con, w, h);
}

void GUI_SetButton(_gui_element * e, int x, int y, int w, int h, const char * label, _gui_void_arg_void_fn callback)
{
    e->element_type = GUI_TYPE_BUTTON;

    e->x = x;
    e->y = y;
    e->w = w;
    e->h = h;
    e->info.button.callback = callback;
    strcpy(e->info.button.name,label);
}

void GUI_SetRadioButton(_gui_element * e, int x, int y, int w, int h, const char * label, int group_id, int btn_id,
                        int start_pressed, _gui_void_arg_int_fn callback)
{
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
    strcpy(e->info.radiobutton.name,label);
}

void GUI_SetLabel(_gui_element * e, int x, int y, int w, int h, const char * label)
{
    e->element_type = GUI_TYPE_LABEL;

    e->x = x;
    e->y = y;
    e->w = w;
    e->h = h;
    strcpy(e->info.label.text,label);
}

void GUI_SetBitmap(_gui_element * e, int x, int y, int w, int h, char * bitmap, _gui_int_arg_int_int_fn callback) // 24 bit
{
    e->element_type = GUI_TYPE_BITMAP;

    e->x = x;
    e->y = y;
    e->w = w;
    e->h = h;
    e->info.bitmap.bitmap = bitmap;
    e->info.bitmap.callback = callback;
}

void GUI_SetWindow(_gui_element * e, int x, int y, int w, int h, void * gui, const char * caption)
{
    e->element_type = GUI_TYPE_WINDOW;

    e->x = x;
    e->y = y;
    e->w = w;
    e->h = h;
    e->info.window.gui = gui;
    e->info.window.enabled = 0;
    strcpy(e->info.window.caption,caption);

    _gui * relative_gui = gui;

    if(relative_gui == NULL)
        return;

    _gui_element ** gui_elements = relative_gui->elements;

    if(gui_elements == NULL) return;

    while(*gui_elements != NULL)
    {
        (*gui_elements)->x += x;
        (*gui_elements)->y += y + FONT_HEIGHT + 2;
        gui_elements++;
    }
}

void GUI_SetMessageBox(_gui_element * e, _gui_console * con, int x, int y, int w, int h, const char * caption)
{
    e->element_type = GUI_TYPE_MESSAGEBOX;

    e->info.messagebox.enabled = 0;
    e->info.messagebox.con = con;
    e->x = x;
    e->y = y;
    e->w = w;
    e->h = h;
    s_strncpy(e->info.messagebox.caption,caption,sizeof(e->info.messagebox.caption));

    GUI_ConsoleReset(con, w, h-FONT_WIDTH-1);
}

int _gui_word_fits(const char * text, int x_start, int x_end)
{
    while(1)
    {
        char c = *text++;
        if(c == '\0') break;
        else if(c == '\n')  break;
        else if(c == ' ')  break;
        else if(c == '\t')  break;
        else x_start ++;
    }

    return (x_start <= x_end);
}

void GUI_SetScrollableTextWindow(_gui_element * e, int x, int y, int w, int h, const char * text, const char * caption)
{
    e->element_type = GUI_TYPE_SCROLLABLETEXTWINDOW;

    w = FONT_WIDTH*(w/FONT_WIDTH);
    h = ( FONT_HEIGHT*( (h-(FONT_HEIGHT+2)) /FONT_HEIGHT) ) + (FONT_HEIGHT+2);

    e->x = x;
    e->y = y;
    e->w = w;
    e->h = h;
    e->info.scrollabletextwindow.text = text;
    e->info.scrollabletextwindow.enabled = 0;
    strcpy(e->info.scrollabletextwindow.caption,caption);
    e->info.scrollabletextwindow.numlines = 0;
    e->info.scrollabletextwindow.currentline = 0;

    e->info.scrollabletextwindow.max_drawn_lines = (e->h - (FONT_HEIGHT+1)) / FONT_HEIGHT;

    //count number of text lines

    int textwidth = (e->w / FONT_WIDTH) - 1;

    int skipspaces = 0;

    int i = 0;
    int curx = 0;
    while(1)
    {
        char c = text[i++];
        if(skipspaces)
        {
            while(1)
            {
                if( ! ( (c == ' ') || (c == '\0') ) )
                   break;
                c = text[i++];
            }
            skipspaces = 0;
        }

        if(c == '\0')
        {
            break;
        }
        else if(c == '\n')
        {
            curx = 0;
            e->info.scrollabletextwindow.numlines ++;
        }
        else
        {
            if(_gui_word_fits(&(text[i-1]),curx,textwidth) == 0)
            {
                curx = 0;
                e->info.scrollabletextwindow.numlines ++;
                i--;
                skipspaces = 1;
            }
            else
            {
                curx ++;
                if(curx == textwidth)
                {
                    if(text[i] == '\n') i++;
                    else skipspaces = 1;
                    e->info.scrollabletextwindow.numlines ++;
                    curx = 0;
                }
            }
        }
    }

    e->info.scrollabletextwindow.numlines ++;
}

void GUI_SetScrollBar(_gui_element * e, int x, int y, int w, int h, int min_value, int max_value,
                      int start_value, _gui_void_arg_int_fn callback)
{
    e->element_type = GUI_TYPE_SCROLLBAR;

    e->x = x;
    e->y = y;
    e->w = w;
    e->h = h;

    if(w > h) e->info.scrollbar.is_vertical = 0;
    else e->info.scrollbar.is_vertical = 1;

    e->info.scrollbar.value_min = min_value;
    e->info.scrollbar.value_max = max_value;
    e->info.scrollbar.value = start_value;
    e->info.scrollbar.callback = callback;
}

//----------------------------------------------------------------------------------------------

void GUI_SetLabelCaption(_gui_element * e, const char * label)
{
    if(e == NULL)
        return;

    if(e->element_type != GUI_TYPE_LABEL)
        return;

    strcpy(e->info.label.text,label);
}

void _gui_clear_radiobuttons(_gui_element ** element_list, int group_id)
{
    while(*element_list != NULL)
    {
        _gui_element * e = *element_list;

        if(e->element_type == GUI_TYPE_RADIOBUTTON)
        {
            if(e->info.radiobutton.group_id == group_id)
                e->info.radiobutton.is_pressed = 0;
        }

        element_list++;
    }
}

void GUI_RadioButtonSetEnabled(_gui_element * e, int enabled)
{
    if(e == NULL) return;

    if(e->element_type != GUI_TYPE_RADIOBUTTON) return;

    e->info.radiobutton.is_enabled = enabled;
}

void GUI_RadioButtonSetPressed(void * complete_gui, _gui_element * e)
{
    if(e == NULL) return;

    if(e->element_type != GUI_TYPE_RADIOBUTTON) return;

    _gui * g = complete_gui;

    _gui_clear_radiobuttons(g->elements,e->info.radiobutton.group_id);

    e->info.radiobutton.is_pressed = 1;
}

void GUI_WindowSetEnabled(_gui_element * e, int enabled)
{
    if(e == NULL)
        return;

    if(e->element_type != GUI_TYPE_WINDOW)
        return;

    e->info.window.enabled = enabled;

    _gui * wingui = e->info.window.gui;

    if(wingui == NULL)
        return;

    _gui_element ** gui_element = wingui->elements;

    while(*gui_element != NULL)
    {
        if((*gui_element)->element_type == GUI_TYPE_BUTTON)
        {
            (*gui_element)->info.button.is_pressed = 0;
        }

        gui_element++;
    }
}

int GUI_WindowGetEnabled(_gui_element * e)
{
    if(e == NULL)
        return 0;

    if(e->element_type == GUI_TYPE_WINDOW)
        return e->info.window.enabled;

    return 0;
}

void GUI_MessageBoxSetEnabled(_gui_element * e, int enabled)
{
    if(e == NULL) return;

    if(e->element_type != GUI_TYPE_MESSAGEBOX) return;

    e->info.messagebox.enabled = enabled;
}

int GUI_MessageBoxGetEnabled(_gui_element * e)
{
    if(e == NULL)
        return 0;

    if(e->element_type == GUI_TYPE_MESSAGEBOX)
        return e->info.messagebox.enabled;

    return 0;
}

void GUI_ScrollableTextWindowSetEnabled(_gui_element * e, int enabled)
{
    if(e == NULL) return;

    if(e->element_type != GUI_TYPE_SCROLLABLETEXTWINDOW) return;

    e->info.scrollabletextwindow.enabled = enabled;
}

int GUI_ScrollableTextWindowGetEnabled(_gui_element * e)
{
    if(e == NULL)
        return 0;

    if(e->element_type == GUI_TYPE_SCROLLABLETEXTWINDOW)
        return e->info.scrollabletextwindow.enabled;

    return 0;
}

//-------------------------------------------------------------------------------------------
