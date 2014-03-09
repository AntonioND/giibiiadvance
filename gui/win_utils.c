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

#include "../font_utils.h"
#include "../general_utils.h"

//-------------------------------------------------------------------------------------------

static int __gui_coord_is_inside_rect(int xm, int ym, int x, int w, int y, int h)
{
    return (xm >= x) && (xm < (x+w)) && (ym >= y) && (ym < (y+h));
}

//-------------------------------------------------------------------------------------------

void GUI_ConsoleReset(_gui_console * con, int screen_width, int screen_height) // in pixels
{
    GUI_ConsoleClear(con);
    con->__console_chars_w = screen_width/FONT_12_WIDTH;
    con->__console_chars_h = screen_height/FONT_12_HEIGHT;
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
        if(c) FU_PrintChar12(buffer,buf_w,buf_h,x*FONT_12_WIDTH,y*FONT_12_HEIGHT,c,color);
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
            FU_PrintChar12(buffer,buf_w,buf_h,scrx+x*FONT_12_WIDTH,scry+y*FONT_12_HEIGHT,c,color);
        }
    }
}

//-------------------------------------------------------------------------------------------

static int _gui_r, _gui_g, _gui_b;

static void _gui_set_drawing_color(int r, int g, int b)
{
    _gui_r = r;
    _gui_g = g;
    _gui_b = b;
}

static void _gui_draw_horizontal_line(char * buffer, int w, int h, int x1, int x2, int y)
{
    if(y > h) return;
    if(y < 0) return;

    if(x1 < 0) x1 = 0;
    if(x2 >= w) x2 = w-1;

    char * dst = &(buffer[(y*w+x1)*3]);

    while(x1 <= x2)
    {
        *dst++ = _gui_r;
        *dst++ = _gui_g;
        *dst++ = _gui_b;

        x1++;
    }
}

static void _gui_draw_vertical_line(char * buffer, int w, int h, int x, int y1, int y2)
{
    if(x > w) return;
    if(x < 0) return;

    if(y1 < 0) y1 = 0;
    if(y2 >= h) y2 = h-1;

    char * dst = &(buffer[(y1*w+x)*3]);

    while(y1 <= y2)
    {
        *dst++ = _gui_r;
        *dst++ = _gui_g;
        *dst++ = _gui_b;

        dst += (w-1) * 3;
        y1++;
    }
}

static void _gui_draw_rect(char * buffer, int w, int h, int x1, int x2, int y1, int y2)
{
    _gui_draw_horizontal_line(buffer,w,h, x1,x2, y1);
    _gui_draw_vertical_line(buffer,w,h, x1, y1,y2);
    _gui_draw_horizontal_line(buffer,w,h, x1,x2, y2);
    _gui_draw_vertical_line(buffer,w,h, x2, y1,y2);
}

static void _gui_draw_fill_rect(char * buffer, int w, int h, int x1, int x2, int y1, int y2)
{
    int y;
    for(y = y1; y <= y2; y++)
        _gui_draw_horizontal_line(buffer,w,h, x1,x2, y);
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

static int _gui_inputwindow_send_event(_gui_inputwindow * win, SDL_Event * e)
{
    if(win == NULL) return 0;

    if(e->type == SDL_KEYDOWN)
    {
        int key = e->key.keysym.sym;
        if( ((key >= SDLK_a) && (key <= SDLK_f)) || ((key >= SDLK_0) && (key <= SDLK_9)) )
        {
            int l = strlen(win->input_text);
            if( l < (sizeof(win->input_text)-1) )
            {
                win->input_text[l++] = toupper(key);
                win->input_text[l] = '\0';
            }
        }
        else if(key == SDLK_BACKSPACE)
        {
            int l = strlen(win->input_text);
            if(l == 0)
            {
                GUI_InputWindowClose(win);
                return 1;
            }
            win->input_text[l-1] = '\0';
        }
        else if(key == SDLK_RETURN)
        {
            int l = strlen(win->input_text);
            if(win->callback)
                win->callback(win->input_text, (l>0)?1:0);
            win->enabled = 0;
        }
    }
    return 1;
}

int GUI_InputWindowIsEnabled(_gui_inputwindow * win)
{
    if(win == NULL) return 0;
    return win->enabled;
}

static void _gui_draw_inputwindow(_gui_inputwindow * win, char * buffer, int w, int h)
{
    if(win == NULL) return;

    if(win->enabled == 0)
        return;

    int x = (w - GUI_INPUTWINDOW_WIDTH) / 2;
    int y = (h - GUI_INPUTWINDOW_HEIGHT) / 2;

     _gui_set_drawing_color(224,224,224);
    _gui_draw_fill_rect(buffer,w,h,
                      x,x+GUI_INPUTWINDOW_WIDTH-1,
                      y,y+FONT_12_HEIGHT+1);

    _gui_set_drawing_color(0,0,0);
    _gui_draw_rect(buffer,w,h,
                      x-1,x+GUI_INPUTWINDOW_WIDTH-1+1,
                      y-1,y+GUI_INPUTWINDOW_HEIGHT-1+1);
    _gui_draw_horizontal_line(buffer,w,h, x-1,x+GUI_INPUTWINDOW_WIDTH-1+1, y+FONT_12_HEIGHT+1);

    _gui_set_drawing_color(255, 255,255);
    _gui_draw_fill_rect(buffer,w,h,
                      x,x+GUI_INPUTWINDOW_WIDTH-1,
                      y+FONT_12_HEIGHT+2,y+GUI_INPUTWINDOW_HEIGHT-1);

    int text_width = strlen(win->window_caption) * FONT_12_WIDTH;
    int x_off = ( GUI_INPUTWINDOW_WIDTH - text_width ) / 2;

    FU_Print12Color(buffer,w,h,x+x_off,y,0xFFE0E0E0,win->window_caption);

    FU_Print12(buffer,w,h,x+FONT_12_WIDTH,y+FONT_12_HEIGHT+4,win->input_text);

}

//-------------------------------------------------------------------------------------------

static void _gui_draw_menu(_gui_menu * menu, char * buffer, int w, int h)
{
    if(menu == NULL) return;

    int x = 0;

    int selected_element_x = 0;

    int i = 0;
    while(1)
    {
        _gui_menu_list * list = menu->list_entry[i];

        if(list == NULL) break;

        int l = strlen(list->title);

        FU_Print12Color(buffer,w,h,x,0,0xFFE0E0E0," ");
        x += FONT_12_WIDTH;
        if(menu->element_opened == i)
        {
            selected_element_x = x;
            FU_Print12Color(buffer,w,h,x,0,0xFFC0C0C0,list->title);
        }
        else
        {
            FU_Print12Color(buffer,w,h,x,0,0xFFE0E0E0,list->title);
        }
        x += l*FONT_12_WIDTH;
        FU_Print12Color(buffer,w,h,x,0,0xFFE0E0E0," ");
        x += FONT_12_WIDTH;

        i++;
    }

    _gui_set_drawing_color(224,224,224);
    _gui_draw_fill_rect(buffer,w,h,
                        0,x,
                        FONT_12_HEIGHT, FONT_12_HEIGHT + (FONT_12_HEIGHT/2) - 1);

    _gui_set_drawing_color(0,0,0);
    _gui_draw_horizontal_line(buffer,w,h,
                              0,x,
                              FONT_12_HEIGHT+FONT_12_HEIGHT/2);
    _gui_draw_vertical_line(buffer,w,h,
                              x,
                              0,FONT_12_HEIGHT+FONT_12_HEIGHT/2);

    // draw the menu bar, but not any menu
    if(menu->element_opened < 0)
        return;

    if(menu->element_opened >= i)
        return;

    _gui_menu_list * selected_list = menu->list_entry[menu->element_opened];

    if(selected_list == NULL) return;

    _gui_menu_entry ** entries = selected_list->entry;

    //get longer string for this list
    i = 0;
    int longest_string = 0;
    while(1)
    {
        if(entries[i] == NULL)
            break;

        int l = strlen(entries[i]->text);
        if(longest_string < l)
            longest_string = l;

        i++;
    }

    _gui_set_drawing_color(224,224,224);
    _gui_draw_fill_rect(buffer,w,h,
                        selected_element_x - FONT_12_WIDTH, selected_element_x + FONT_12_WIDTH*(longest_string+1),
                        FONT_12_HEIGHT+FONT_12_HEIGHT/2, FONT_12_HEIGHT+FONT_12_HEIGHT/2 + (i+1)*FONT_12_HEIGHT);

    _gui_set_drawing_color(0,0,0);
    _gui_draw_rect(buffer,w,h,
                    selected_element_x - FONT_12_WIDTH - 1, selected_element_x + FONT_12_WIDTH*(longest_string+1) + 1,
                    FONT_12_HEIGHT + FONT_12_HEIGHT/2 - 1, FONT_12_HEIGHT+FONT_12_HEIGHT/2 + (i+1)*FONT_12_HEIGHT + 1);

    //print list
    int y = 2*FONT_12_HEIGHT;
    i = 0;
    while(1)
    {
        if(entries[i] == NULL)
            break;

        FU_Print12Color(buffer,w,h,selected_element_x,y,0xFFE0E0E0,entries[i]->text);

        y += FONT_12_HEIGHT;
        i++;
    }
}

static int _gui_menu_send_event(_gui_menu * menu, SDL_Event * e)
{
    if(menu == NULL) return 0;

    if(e->type != SDL_MOUSEBUTTONDOWN)
        return 0;

    if(e->button.button != SDL_BUTTON_LEFT)
        return 0;

    int x = 0;

    int selected_element_x = 0;

    int i = 0;
    while(1)
    {
        _gui_menu_list * list = menu->list_entry[i];

        if(list == NULL) break;

        int l = strlen(list->title);

        x += FONT_12_WIDTH;
        if(menu->element_opened == i) selected_element_x = x;

        if(__gui_coord_is_inside_rect(e->button.x,e->button.y,
            x-FONT_12_WIDTH,(l+1)*FONT_12_WIDTH,   0,FONT_12_HEIGHT+FONT_12_HEIGHT/2))
        {
            menu->element_opened = i;
            return 1;
        }

        x += l*FONT_12_WIDTH;
        x += FONT_12_WIDTH;

        i++;
    }

    // check only the bar
    if(menu->element_opened < 0)
        return 0;

    if(menu->element_opened >= i)
        return 0;

    _gui_menu_list * selected_list = menu->list_entry[menu->element_opened];

    if(selected_list == NULL) return 0;

    _gui_menu_entry ** entries = selected_list->entry;

    //get longer string for this list
    i = 0;
    int longest_string = 0;
    while(1)
    {
        if(entries[i] == NULL)
            break;

        int l = strlen(entries[i]->text);
        if(longest_string < l)
            longest_string = l;

        i++;
    }

    //print list
    int y = 2*FONT_12_HEIGHT;
    i = 0;
    while(1)
    {
        if(entries[i] == NULL)
            break;

        if(__gui_coord_is_inside_rect(e->button.x,e->button.y,
            selected_element_x - FONT_12_WIDTH - 1, FONT_12_WIDTH*(longest_string+1+1) + 1,
            y, FONT_12_HEIGHT)) // clicked in an element
        {
            if(entries[i]->callback)
            {
                entries[i]->callback();
                menu->element_opened = -1; // close the menu
            }
            return 1;
        }

        y += FONT_12_HEIGHT;
        i++;
    }

    if(__gui_coord_is_inside_rect(e->button.x,e->button.y,
            selected_element_x - FONT_12_WIDTH - 1, FONT_12_WIDTH*(longest_string+2) + 2,
            FONT_12_HEIGHT + FONT_12_HEIGHT/2 - 1, (i+1)*FONT_12_HEIGHT + 2))
    {
        return 1; // clicked inside the menu box, but not in a element
    }

    //if execution got here, clicked outside the menu
    menu->element_opened = -1; // close the menu
    return 1;
}

//-------------------------------------------------------------------------------------------

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

void GUI_SetBitmap(_gui_element * e, int x, int y, int w, int h, char * bitmap, _gui_void_arg_void_fn callback) // 24 bit
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
        (*gui_elements)->y += y + FONT_12_HEIGHT + 2;
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

    GUI_ConsoleReset(con, w, h-FONT_12_WIDTH-1);
}

static int _gui_word_fits(const char * text, int x_start, int x_end)
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

    w = FONT_12_WIDTH*(w/FONT_12_WIDTH);
    h = ( FONT_12_HEIGHT*( (h-(FONT_12_HEIGHT+2)) /FONT_12_HEIGHT) ) + (FONT_12_HEIGHT+2);

    e->x = x;
    e->y = y;
    e->w = w;
    e->h = h;
    e->info.scrollabletextwindow.text = text;
    e->info.scrollabletextwindow.enabled = 0;
    strcpy(e->info.scrollabletextwindow.caption,caption);
    e->info.scrollabletextwindow.numlines = 0;
    e->info.scrollabletextwindow.currentline = 0;

    e->info.scrollabletextwindow.max_drawn_lines = (e->h - (FONT_12_HEIGHT+1)) / FONT_12_HEIGHT;

    //count number of text lines

    int textwidth = (e->w / FONT_12_WIDTH) - 1;

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

//----------------------------------------------------------------------------------------------

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

static int _gui_get_first_window_enabled(_gui * gui)
{
    if(gui == NULL)
        return -1;

    _gui_element ** gui_elements = gui->elements;

    if(gui_elements == NULL) return -1;

    int i = 0;
    while(*gui_elements != NULL)
    {
        if( (*gui_elements)->element_type == GUI_TYPE_WINDOW)
            if( (*gui_elements)->info.window.enabled ) return i;
        if( (*gui_elements)->element_type == GUI_TYPE_SCROLLABLETEXTWINDOW)
            if( (*gui_elements)->info.scrollabletextwindow.enabled ) return i;
        i++;
        gui_elements++;
    }

    return -1;
}

static int _gui_get_first_messagebox_enabled(_gui * gui)
{
    if(gui == NULL)
        return -1;

    _gui_element ** gui_elements = gui->elements;

    if(gui_elements == NULL) return -1;

    int i = 0;
    while(*gui_elements != NULL)
    {
        if( (*gui_elements)->element_type == GUI_TYPE_MESSAGEBOX)
            if( (*gui_elements)->info.messagebox.enabled ) return i;
        i++;
        gui_elements++;
    }

    return -1;
}

//-------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------

static void __gui_draw_init_buffer(char * buffer, int w, int h)
{
    _gui_set_drawing_color(192,192,192);
    _gui_draw_fill_rect(buffer,w,h,  0,w-1,  0,h-1);
}

static void __gui_draw_element(_gui_element * e, char * buffer, int w, int h)
{
    if(e->element_type == GUI_TYPE_TEXTBOX)
    {
        _gui_set_drawing_color(255,255,255);

        _gui_draw_fill_rect(buffer,w,h,
                      e->x,e->x+e->w-1,
                      e->y,e->y+e->h-1);
        int i;
        for(i = 1; i < 2; i++)
        {
            _gui_set_drawing_color(i*(256/4),i*(256/4),i*(256/4));
            _gui_draw_rect(buffer,w,h,
                      e->x-i,e->x+e->w+i-1,
                      e->y-i,e->y+e->h+i-1);
        }

        GUI_ConsoleDrawAt(e->info.textbox.con,
                           buffer,w,h,
                           e->x,e->y,
                           e->w,e->h);
    }
    else if(e->element_type == GUI_TYPE_BUTTON)
    {
        if(e->info.button.is_pressed)
            _gui_set_drawing_color(255,255,255);
        else
            _gui_set_drawing_color(224,224,224);

        _gui_draw_fill_rect(buffer,w,h,
                      e->x,e->x+e->w-1,
                      e->y,e->y+e->h-1);

        int i;
        for(i = 1; i < 4; i++)
        {
            _gui_set_drawing_color(224-i*(224/4),224-i*(224/4),224-i*(224/4));
            _gui_draw_rect(buffer,w,h,
                      e->x-i,e->x+e->w+i-1,
                      e->y-i,e->y+e->h+i-1);
        }

        int namewidth = FONT_12_WIDTH*strlen(e->info.button.name);
        int xoff = (e->w - namewidth) / 2;
        int yoff = (e->h - FONT_12_HEIGHT) / 2;

        if(e->info.button.is_pressed)
            FU_Print12Color(buffer,w,h,e->x+xoff,e->y+yoff,0xFFFFFFFF,e->info.button.name);
        else
            FU_Print12Color(buffer,w,h,e->x+xoff,e->y+yoff,0xFFE0E0E0,e->info.button.name);

        e->info.button.is_pressed = 0;
    }
    else if(e->element_type == GUI_TYPE_RADIOBUTTON)
    {
        if(e->info.radiobutton.is_pressed)
            _gui_set_drawing_color(255,255,255);
        else
            _gui_set_drawing_color(224,224,224);

        _gui_draw_fill_rect(buffer,w,h,
                      e->x,e->x+e->w-1,
                      e->y,e->y+e->h-1);
/*
        int i;
        for(i = 1; i < 4; i++)
        {
            _gui_set_drawing_color(224-i*(224/4),224-i*(224/4),224-i*(224/4));
            _gui_draw_rect(buffer,w,h,
                      e->x-i,e->x+e->w+i-1,
                      e->y-i,e->y+e->h+i-1);
        }
*/
        int namewidth = FONT_12_WIDTH*strlen(e->info.radiobutton.name);
        int xoff = (e->w - namewidth) / 2;
        int yoff = (e->h - FONT_12_HEIGHT) / 2;

        if(e->info.radiobutton.is_pressed)
            FU_Print12Color(buffer,w,h,e->x+xoff,e->y+yoff,0xFFFFFFFF,e->info.radiobutton.name);
        else
            FU_Print12Color(buffer,w,h,e->x+xoff,e->y+yoff,0xFFE0E0E0,e->info.radiobutton.name);
    }
    else if(e->element_type == GUI_TYPE_LABEL)
    {
        int namewidth = FONT_12_WIDTH*strlen(e->info.label.text);
        int xoff = (e->w - namewidth) / 2;
        int yoff = (e->h - FONT_12_HEIGHT) / 2;

        FU_Print12Color(buffer,w,h,e->x+xoff,e->y+yoff,0xFFC0C0C0,e->info.label.text);
    }
    else if(e->element_type == GUI_TYPE_BITMAP)
    {
        int x,y;
        for(y = 0; y < e->h; y++) for(x = 0; x < e->w; x++)
        {
            char * src = &(e->info.bitmap.bitmap[ ( y*e->w + x ) * 3 ]);
            char * dst = &(buffer[ ( (e->y+y)*w + (e->x+x) ) * 3 ]);
            *dst++ = *src++;
            *dst++ = *src++;
            *dst++ = *src++;
        }
    }
    else if(e->element_type == GUI_TYPE_WINDOW)
    {
        if(e->info.window.enabled == 0)
            return;

        _gui_set_drawing_color(224,224,224);
        _gui_draw_fill_rect(buffer,w,h,
                          e->x,e->x+e->w-1,
                          e->y,e->y+FONT_12_HEIGHT+1);

        _gui_set_drawing_color(0,0,0);
        _gui_draw_rect(buffer,w,h,
                          e->x-1,e->x+e->w-1+1,
                          e->y-1,e->y+e->h-1+1);
        _gui_draw_horizontal_line(buffer,w,h, e->x-1,e->x+e->w-1+1,
                                  e->y+FONT_12_HEIGHT+1);

        _gui_set_drawing_color(255, 255,255);
        _gui_draw_fill_rect(buffer,w,h,
                          e->x,e->x+e->w-1,
                          e->y+FONT_12_HEIGHT+2,e->y+e->h-1);

        int text_width = strlen(e->info.window.caption) * FONT_12_WIDTH;
        int x_off = ( e->w - text_width ) / 2;

        FU_Print12Color(buffer,w,h,e->x+x_off,e->y,0xFFE0E0E0,e->info.window.caption);

        if(e->info.window.gui)
            GUI_Draw(e->info.window.gui,buffer,w,h,0);
    }
    else if(e->element_type == GUI_TYPE_MESSAGEBOX)
    {
        if(e->info.messagebox.enabled == 0)
            return;

        _gui_set_drawing_color(224,224,224);
        _gui_draw_fill_rect(buffer,w,h,
                          e->x,e->x+e->w-1,
                          e->y,e->y+FONT_12_HEIGHT+1);

        _gui_set_drawing_color(0,0,0);
        _gui_draw_rect(buffer,w,h,
                          e->x-1,e->x+e->w-1+1,
                          e->y-1,e->y+e->h-1+1);
        _gui_draw_horizontal_line(buffer,w,h, e->x-1,e->x+e->w-1+1,
                                  e->y+FONT_12_HEIGHT+1);

        _gui_set_drawing_color(255, 255,255);
        _gui_draw_fill_rect(buffer,w,h,
                          e->x,e->x+e->w-1,
                          e->y+FONT_12_HEIGHT+2,e->y+e->h-1);

        int text_width = strlen(e->info.messagebox.caption) * FONT_12_WIDTH;
        int x_off = ( e->w - text_width ) / 2;

        FU_Print12Color(buffer,w,h,e->x+x_off,e->y,0xFFE0E0E0,e->info.messagebox.caption);

        GUI_ConsoleDrawAt(e->info.messagebox.con,
                            buffer,w,h,
                            e->x,e->y+FONT_12_HEIGHT+2,
                            e->x+e->w-1,e->y+e->h-1);
    }
    else if(e->element_type == GUI_TYPE_SCROLLABLETEXTWINDOW)
    {
        if(e->info.scrollabletextwindow.enabled == 0)
            return;

        _gui_set_drawing_color(224,224,224);
        _gui_draw_fill_rect(buffer,w,h,
                          e->x,e->x+e->w-1,
                          e->y,e->y+FONT_12_HEIGHT+1);

        _gui_set_drawing_color(0,0,0);
        _gui_draw_rect(buffer,w,h,
                          e->x-1,e->x+e->w-1+1,
                          e->y-1,e->y+e->h-1+1);
        _gui_draw_horizontal_line(buffer,w,h, e->x-1,e->x+e->w-1+1,
                                  e->y+FONT_12_HEIGHT+1);

        _gui_set_drawing_color(255, 255,255);
        _gui_draw_fill_rect(buffer,w,h,
                          e->x,e->x+e->w-1,
                          e->y+FONT_12_HEIGHT+2,e->y+e->h-1);

        int caption_width = strlen(e->info.scrollabletextwindow.caption) * FONT_12_WIDTH;
        int x_off = ( e->w - caption_width ) / 2;

        int skipspaces = 0;

        FU_Print12Color(buffer,w,h,e->x+x_off,e->y,0xFFE0E0E0,e->info.scrollabletextwindow.caption);

        if(e->info.scrollabletextwindow.text)
        {
            int countlines = 0;

            int textwidth = (e->w / FONT_12_WIDTH) - 1;

            int i = 0;
            int curx = 0;
            while(1) // search start of printed text
            {
                if(countlines == e->info.scrollabletextwindow.currentline)
                    break;

                char c = e->info.scrollabletextwindow.text[i++];
                if(skipspaces)
                {
                    while(1)
                    {
                        if( ! ( (c == ' ') || (c == '\0') ) )
                           break;
                        c = e->info.scrollabletextwindow.text[i++];
                    }
                    skipspaces = 0;
                }

                if(c == '\0')
                {
                    return;
                }
                else if(c == '\n')
                {
                    curx = 0;
                    countlines ++;
                }
                else
                {
                    if(_gui_word_fits(&(e->info.scrollabletextwindow.text[i-1]),curx,textwidth) == 0)
                    {
                        curx = 0;
                        countlines ++;
                        i--;
                        skipspaces = 1;
                    }
                    else
                    {
                        curx ++;
                        if(curx == textwidth)
                        {
                            if(e->info.scrollabletextwindow.text[i] == '\n') i++;
                            countlines ++;
                            curx = 0;
                            skipspaces = 1;
                        }
                    }
                }
            }

            int cury = 0;

            int basexcoord = e->x;
            int baseycoord = e->y+FONT_12_HEIGHT+2;

            countlines = 0;

            skipspaces = 0;

            while(1) // print text
            {
                char c = e->info.scrollabletextwindow.text[i++];
                if(skipspaces)
                {
                    while(1)
                    {
                        if( ! ( (c == ' ') || (c == '\0') ) )
                           break;
                        c = e->info.scrollabletextwindow.text[i++];
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
                    cury ++;
                    countlines ++;
                }
                else
                {
                    if(_gui_word_fits(&(e->info.scrollabletextwindow.text[i-1]),curx,textwidth) == 0)
                    {
                        curx = 0;
                        cury ++;
                        countlines ++;
                        i--;
                        skipspaces = 1;
                    }
                    else
                    {
                        FU_PrintChar12(buffer,w,h, basexcoord+curx*FONT_12_WIDTH, baseycoord+cury*FONT_12_HEIGHT,
                                   c, 0xFFFFFFFF);
                        curx ++;
                        if(curx == textwidth)
                        {
                            if(e->info.scrollabletextwindow.text[i] == '\n') i++;
                            else skipspaces = 1;
                            cury ++;
                            countlines++;
                            curx = 0;
                        }
                    }

                }

                if(e->info.scrollabletextwindow.max_drawn_lines == countlines)
                    break;
            }

            for(i = 0; i < e->info.scrollabletextwindow.max_drawn_lines; i++)
                FU_PrintChar12(buffer,w,h, basexcoord+textwidth*FONT_12_WIDTH, baseycoord+i*FONT_12_HEIGHT,
                    ' ', 0xFFE0E0E0);

            int percent =
                (e->info.scrollabletextwindow.currentline * 100) /
                (e->info.scrollabletextwindow.numlines - e->info.scrollabletextwindow.max_drawn_lines);

            int bar_position = 1 + (percent * (e->info.scrollabletextwindow.max_drawn_lines-5) / 100);

            FU_PrintChar12(buffer,w,h, basexcoord+textwidth*FONT_12_WIDTH, baseycoord+bar_position*FONT_12_HEIGHT,
                    CHR_SHADED_DARK, 0xFFE0E0E0);
            FU_PrintChar12(buffer,w,h, basexcoord+textwidth*FONT_12_WIDTH, baseycoord+(bar_position+1)*FONT_12_HEIGHT,
                    CHR_SHADED_DARK, 0xFFE0E0E0);
            FU_PrintChar12(buffer,w,h, basexcoord+textwidth*FONT_12_WIDTH, baseycoord+(bar_position+2)*FONT_12_HEIGHT,
                    CHR_SHADED_DARK, 0xFFE0E0E0);

            //if(e->info.scrollabletextwindow.currentline > 0)
                FU_PrintChar12(buffer,w,h, basexcoord+textwidth*FONT_12_WIDTH, baseycoord,
                    CHR_ARROW_UP, 0xFFE0E0E0);
            //if(e->info.scrollabletextwindow.currentline <
            //        e->info.scrollabletextwindow.numlines - e->info.scrollabletextwindow.max_drawn_lines)
                FU_PrintChar12(buffer,w,h, basexcoord+textwidth*FONT_12_WIDTH,
                               baseycoord+(e->info.scrollabletextwindow.max_drawn_lines-1)*FONT_12_HEIGHT,
                               CHR_ARROW_DOWN, 0xFFE0E0E0);
        }
    }
}

void GUI_Draw(_gui * gui, char * buffer, int w, int h, int clean)
{
    if(gui == NULL) return;

    if(clean)
        __gui_draw_init_buffer(buffer,w,h);

    _gui_element ** e = gui->elements;

    if(e == NULL)
        return;

    while(*e != NULL)
        __gui_draw_element(*e++,buffer,w,h);

    _gui_draw_inputwindow(gui->inputwindow,buffer,w,h);
    _gui_draw_menu(gui->menu,buffer,w,h);
}

//-------------------------------------------------------------------------------------------

static void _gui_clear_radiobuttons(_gui_element ** complete_gui, int group_id)
{
    while(*complete_gui != NULL)
    {
        _gui_element * e = *complete_gui;

        if(e->element_type == GUI_TYPE_RADIOBUTTON)
        {
            e->info.radiobutton.is_pressed = 0;
        }

        complete_gui++;
    }
}

//-------------------------------------------------------------------------------------------

int __gui_send_event_element(_gui_element ** complete_gui, _gui_element * gui, SDL_Event * e)
{
    if(gui->element_type == GUI_TYPE_TEXTBOX)
    {
        if(e->type == SDL_MOUSEBUTTONDOWN)
        {
            if(e->button.button == SDL_BUTTON_LEFT)
            {
                if(__gui_coord_is_inside_rect(e->button.x,e->button.y,
                                              gui->x,gui->w,
                                              gui->y,gui->h))
                {
                    if(gui->info.textbox.mouse_press_callback)
                            gui->info.textbox.mouse_press_callback(e->button.x - gui->x,
                                                                   e->button.y - gui->y);
                    return 1;
                }
            }
        }
    }
    else if(gui->element_type == GUI_TYPE_BUTTON)
    {
        if(e->type == SDL_MOUSEBUTTONDOWN)
        {
            if(e->button.button == SDL_BUTTON_LEFT)
            {
                if(__gui_coord_is_inside_rect(e->button.x,e->button.y,gui->x,gui->w,
                                              gui->y,gui->h))
                {
                    gui->info.button.is_pressed = 1;

                    if(gui->info.button.callback)
                            gui->info.button.callback();

                    return 1;
                }
            }
        }
        else if(e->type == SDL_MOUSEBUTTONUP)
        {
            if(e->button.button == SDL_BUTTON_LEFT)
            {
                gui->info.button.is_pressed = 0;
                return 1;
            }
        }
        else if(e->type == SDL_MOUSEMOTION)
        {
            if(__gui_coord_is_inside_rect(e->motion.x,e->motion.y,gui->x,gui->w,
                                              gui->y,gui->h))
            {
                gui->info.button.is_pressed = 0;
                return 1;
            }
        }
    }
    else if(gui->element_type == GUI_TYPE_RADIOBUTTON)
    {
        if(e->type == SDL_MOUSEBUTTONDOWN)
        {
            if(e->button.button == SDL_BUTTON_LEFT)
            {
                if(__gui_coord_is_inside_rect(e->button.x,e->button.y,
                                              gui->x,gui->w,
                                              gui->y,gui->h))
                {
                    _gui_clear_radiobuttons(complete_gui,gui->info.radiobutton.group_id);

                    gui->info.radiobutton.is_pressed = 1;

                    if(gui->info.radiobutton.callback)
                            gui->info.radiobutton.callback(gui->info.radiobutton.btn_id);

                    return 1;
                }
            }
        }
    }
    else if(gui->element_type == GUI_TYPE_BITMAP)
    {
        if(e->type == SDL_MOUSEBUTTONDOWN)
        {
            if(e->button.button == SDL_BUTTON_LEFT)
            {
                if(__gui_coord_is_inside_rect(e->button.x,e->button.y,gui->x,gui->w,
                                              gui->y,gui->h))
                {
                    if(gui->info.bitmap.callback)
                            gui->info.bitmap.callback();

                    return 0;
                }
            }
        }
    }
    else if(gui->element_type == GUI_TYPE_WINDOW)
    {
        if(gui->info.window.enabled == 0)
            return 0;

        _gui * relative_gui = gui->info.window.gui;
        return GUI_SendEvent(relative_gui,e);
    }
    else if(gui->element_type == GUI_TYPE_MESSAGEBOX)
    {
        if(e->type == SDL_MOUSEBUTTONDOWN)
        {
            if(e->button.button == SDL_BUTTON_LEFT)
            {
                if(__gui_coord_is_inside_rect(e->button.x,e->button.y,
                                              gui->x,gui->w,
                                              gui->y,gui->h))
                {
                    gui->info.messagebox.enabled = 0;
                    return 1;
                }
            }
        }
    }
    else if(gui->element_type == GUI_TYPE_SCROLLABLETEXTWINDOW)
    {
        if(gui->info.scrollabletextwindow.enabled == 0)
            return 0;

        if(e->type == SDL_MOUSEBUTTONDOWN)
        {
            if(e->button.button == SDL_BUTTON_LEFT)
            {
                if(__gui_coord_is_inside_rect(e->button.x,e->button.y,gui->x,gui->w,
                                              gui->y,gui->h) == 0)
                {
                    gui->info.scrollabletextwindow.enabled = 0;
                    return 1;
                }

                int basexcoord = gui->x;
                int baseycoord = gui->y+FONT_12_HEIGHT+2;
                int textwidth = (gui->w / FONT_12_WIDTH) - 1;

                if(__gui_coord_is_inside_rect(e->button.x,e->button.y,
                                              basexcoord+textwidth*FONT_12_WIDTH,FONT_12_WIDTH,
                                              baseycoord,FONT_12_HEIGHT))
                {
                    gui->info.scrollabletextwindow.currentline --;
                    if(gui->info.scrollabletextwindow.currentline < 0)
                            gui->info.scrollabletextwindow.currentline = 0;
                    return 1;
                }

                if(__gui_coord_is_inside_rect(e->button.x,e->button.y,
                                basexcoord+textwidth*FONT_12_WIDTH,FONT_12_WIDTH,
                                baseycoord+(gui->info.scrollabletextwindow.max_drawn_lines-1)*FONT_12_HEIGHT,FONT_12_HEIGHT))
                {
                    gui->info.scrollabletextwindow.currentline ++;
                    if(gui->info.scrollabletextwindow.currentline >=
                            gui->info.scrollabletextwindow.numlines - gui->info.scrollabletextwindow.max_drawn_lines)
                        gui->info.scrollabletextwindow.currentline =
                                gui->info.scrollabletextwindow.numlines - gui->info.scrollabletextwindow.max_drawn_lines;
                    if(gui->info.scrollabletextwindow.currentline < 0)
                        gui->info.scrollabletextwindow.currentline = 0;
                    return 1;
                }

                if(__gui_coord_is_inside_rect(e->button.x,e->button.y,
                                basexcoord+textwidth*FONT_12_WIDTH,FONT_12_WIDTH,
                                baseycoord+FONT_12_HEIGHT,(gui->info.scrollabletextwindow.max_drawn_lines-1)*FONT_12_HEIGHT))
                {
                    int percent = ( ( e->button.y-(baseycoord+FONT_12_HEIGHT) ) * 100 ) /
                                            ( (gui->info.scrollabletextwindow.max_drawn_lines-4) * FONT_12_HEIGHT );

                    int line =
                        ( (gui->info.scrollabletextwindow.numlines - gui->info.scrollabletextwindow.max_drawn_lines)
                         * percent ) / 100;

                    gui->info.scrollabletextwindow.currentline = line;

                    if(gui->info.scrollabletextwindow.currentline >=
                            gui->info.scrollabletextwindow.numlines - gui->info.scrollabletextwindow.max_drawn_lines)
                        gui->info.scrollabletextwindow.currentline =
                                gui->info.scrollabletextwindow.numlines - gui->info.scrollabletextwindow.max_drawn_lines;
                    if(gui->info.scrollabletextwindow.currentline < 0)
                        gui->info.scrollabletextwindow.currentline = 0;
                }
            }
        }
        else if(e->type == SDL_MOUSEWHEEL)
        {
            gui->info.scrollabletextwindow.currentline -= e->wheel.y*3;

            if(gui->info.scrollabletextwindow.currentline >=
                    gui->info.scrollabletextwindow.numlines - gui->info.scrollabletextwindow.max_drawn_lines)
                gui->info.scrollabletextwindow.currentline =
                        gui->info.scrollabletextwindow.numlines - gui->info.scrollabletextwindow.max_drawn_lines;
            if(gui->info.scrollabletextwindow.currentline < 0)
                gui->info.scrollabletextwindow.currentline = 0;
            return 1;
        }
        else if( e->type == SDL_KEYDOWN )
        {
            if(e->key.keysym.sym == SDLK_UP)
            {
                gui->info.scrollabletextwindow.currentline --;
                if(gui->info.scrollabletextwindow.currentline < 0)
                    gui->info.scrollabletextwindow.currentline = 0;
                return 1;
            }
            else if(e->key.keysym.sym == SDLK_DOWN)
            {
                gui->info.scrollabletextwindow.currentline ++;
                if(gui->info.scrollabletextwindow.currentline >=
                        gui->info.scrollabletextwindow.numlines - gui->info.scrollabletextwindow.max_drawn_lines)
                    gui->info.scrollabletextwindow.currentline =
                        gui->info.scrollabletextwindow.numlines - gui->info.scrollabletextwindow.max_drawn_lines;
                if(gui->info.scrollabletextwindow.currentline < 0)
                    gui->info.scrollabletextwindow.currentline = 0;
                return 1;
            }
            else if(e->key.keysym.sym == SDLK_RETURN)
            {
                gui->info.scrollabletextwindow.enabled = 0;
            }
            else if(e->key.keysym.sym == SDLK_BACKSPACE)
            {
                gui->info.scrollabletextwindow.enabled = 0;
            }
        }
    }

    return 0;
}

int GUI_SendEvent(_gui * gui, SDL_Event * e)
{
    //highest priority = input window
    if(GUI_InputWindowIsEnabled(gui->inputwindow))
    {
        return _gui_inputwindow_send_event(gui->inputwindow,e);
    }

    //higher priority = menu
    if(_gui_menu_send_event(gui->menu,e))
        return 1;

    _gui_element ** elements = gui->elements;
    if(elements == NULL)
        return 0;

    //high priority = message box
    int messagebox_enabled = _gui_get_first_messagebox_enabled(gui); //
    if(messagebox_enabled >= 0)
    {
        return __gui_send_event_element(gui->elements,elements[messagebox_enabled],e);
    }

    //normal priority = other windows
    int window_enabled = _gui_get_first_window_enabled(gui);
    if(window_enabled >= 0)
    {
        return __gui_send_event_element(gui->elements,elements[window_enabled],e);
    }

    //low priority = the rest
    while(*elements != NULL)
    {
        if(__gui_send_event_element(gui->elements,*elements++,e))
            return 1;
    }

    return 0;
}

//-------------------------------------------------------------------------------------------
