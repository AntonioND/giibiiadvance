
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

int GUI_ConsoleModePrintf(_gui_console * con, int x, int y, char * txt, ...)
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
        unsigned char c = txtbuffer[i];
        if(c == '\0')
            break;
        if(c == '\n')
        {
            x = 0;
            y++;
            if(y > con->__console_chars_h) break;
        }
        con->__console_buffer[y*con->__console_chars_w+x] = c;
        i++;
        x++;
        if(x > con->__console_chars_w)
        {
            x = 0;
            y++;
            if(y > con->__console_chars_h) break;
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

void GUI_SetButton(_gui_element * e, int x, int y, int w, int h, char * label, _gui_void_arg_void_fn callback)
{
    e->element_type = GUI_TYPE_BUTTON;

    e->x = x;
    e->y = y;
    e->w = w;
    e->h = h;
    e->info.button.callback = callback;
    strcpy(e->info.button.name,label);
}

void GUI_SetRadioButton(_gui_element * e, int x, int y, int w, int h, char * label, int group_id, int btn_id,
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

void GUI_SetLabel(_gui_element * e, int x, int y, int w, int h, char * label)
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

void GUI_SetWindow(_gui_element * e, int x, int y, int w, int h, void * gui, char * caption)
{
    e->element_type = GUI_TYPE_WINDOW;

    e->x = x;
    e->y = y;
    e->w = w;
    e->h = h;
    e->info.window.gui = gui;
    e->info.window.enabled = 0;
    strcpy(e->info.window.text,caption);

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

//----------------------------------------------------------------------------------------------

void GUI_WindowSetEnabled(_gui_element * e, int enabled)
{
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

        int text_width = strlen(e->info.window.text) * FONT_12_WIDTH;
        int x_off = ( e->w - text_width ) / 2;

        FU_Print12Color(buffer,w,h,e->x+x_off,e->y,0xFFE0E0E0,e->info.window.text);

        if(e->info.window.gui)
            GUI_Draw(e->info.window.gui,buffer,w,h,0);
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

    return 0;
}

int GUI_SendEvent(_gui * gui, SDL_Event * e)
{
    int has_to_redraw = 0;

    if(GUI_InputWindowIsEnabled(gui->inputwindow))
    {
        has_to_redraw = _gui_inputwindow_send_event(gui->inputwindow,e);
        return has_to_redraw;
    }

    if(_gui_menu_send_event(gui->menu,e))
        return 1;

    _gui_element ** elements = gui->elements;

    if(elements == NULL)
        return has_to_redraw;

    int window_enabled = _gui_get_first_window_enabled(gui);
    if(window_enabled >= 0)
    {
        return __gui_send_event_element(gui->elements,elements[window_enabled],e);
    }

    while(*elements != NULL)
    {
        if(__gui_send_event_element(gui->elements,*elements++,e))
            return 1;
    }

    return has_to_redraw;
}

//-------------------------------------------------------------------------------------------
