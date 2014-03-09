
#include <string.h>

#include "../font_utils.h"

#include "win_utils_draw.h"
#include "win_utils.h"

//-------------------------------------------------------------------------------------------

int _gui_word_fits(const char * text, int x_start, int x_end); // in win_utils.h

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

//-------------------------------------------------------------------------------------------

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

//---------------------------------------------------------------------------------

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
