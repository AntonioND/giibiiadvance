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

#include <string.h>
#include <ctype.h>

#include "../font_utils.h"

#include "win_utils_events.h"
#include "win_utils.h"

//-------------------------------------------------------------------------------------------

static int _gui_pos_is_inside_rect(int xm, int ym, int x, int w, int y, int h)
{
    return (xm >= x) && (xm < (x+w)) && (ym >= y) && (ym < (y+h));
}

//-------------------------------------------------------------------------------------------

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

//-------------------------------------------------------------------------------------------

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

        x += FONT_WIDTH;
        if(menu->element_opened == i) selected_element_x = x;

        if(_gui_pos_is_inside_rect(e->button.x,e->button.y,
            x-FONT_WIDTH,(l+1)*FONT_WIDTH,   0,FONT_HEIGHT+FONT_HEIGHT/2))
        {
            menu->element_opened = i;
            return 1;
        }

        x += l*FONT_WIDTH;
        x += FONT_WIDTH;

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
    int y = 2*FONT_HEIGHT;
    i = 0;
    while(1)
    {
        if(entries[i] == NULL)
            break;

        if(_gui_pos_is_inside_rect(e->button.x,e->button.y,
            selected_element_x - FONT_WIDTH - 1, FONT_WIDTH*(longest_string+1+1) + 1,
            y, FONT_HEIGHT)) // clicked in an element
        {
            if( (entries[i]->enabled) && (entries[i]->callback) )
            {
                entries[i]->callback();
                menu->element_opened = -1; // close the menu
            }
            return 1;
        }

        y += FONT_HEIGHT;
        i++;
    }

    if(_gui_pos_is_inside_rect(e->button.x,e->button.y,
            selected_element_x - FONT_WIDTH - 1, FONT_WIDTH*(longest_string+2) + 2,
            FONT_HEIGHT + FONT_HEIGHT/2 - 1, (i+1)*FONT_HEIGHT + 2))
    {
        return 1; // clicked inside the menu box, but not in a element
    }

    //if execution got here, clicked outside the menu
    menu->element_opened = -1; // close the menu
    return 1;
}

//-------------------------------------------------------------------------------------------

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

void _gui_clear_radiobuttons(_gui_element ** element_list, int group_id);  // in win_utils.c

//-------------------------------------------------------------------------------------------

int __gui_send_event_element(_gui_element ** complete_gui, _gui_element * gui, SDL_Event * e)
{
    if(gui->element_type == GUI_TYPE_TEXTBOX)
    {
        if(e->type == SDL_MOUSEBUTTONDOWN)
        {
            if(e->button.button == SDL_BUTTON_LEFT)
            {
                if(_gui_pos_is_inside_rect(e->button.x,e->button.y,
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
                if(_gui_pos_is_inside_rect(e->button.x,e->button.y,gui->x,gui->w,
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
            if(_gui_pos_is_inside_rect(e->motion.x,e->motion.y,gui->x,gui->w,
                                              gui->y,gui->h))
            {
                gui->info.button.is_pressed = 0;
                return 1;
            }
        }
    }
    else if(gui->element_type == GUI_TYPE_RADIOBUTTON)
    {
        if(gui->info.radiobutton.is_enabled)
        {
            if(e->type == SDL_MOUSEBUTTONDOWN)
            {
                if(e->button.button == SDL_BUTTON_LEFT)
                {
                    if(_gui_pos_is_inside_rect(e->button.x,e->button.y,
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
        else
        {
            gui->info.radiobutton.is_pressed = 0;
        }
    }
    else if(gui->element_type == GUI_TYPE_LABEL)
    {
        //nothing
        return 0;
    }
    else if(gui->element_type == GUI_TYPE_BITMAP)
    {
        if(e->type == SDL_MOUSEBUTTONDOWN)
        {
            if(e->button.button == SDL_BUTTON_LEFT)
            {
                if(_gui_pos_is_inside_rect(e->button.x,e->button.y,gui->x,gui->w,
                                              gui->y,gui->h))
                {
                    if(gui->info.bitmap.callback)
                            return gui->info.bitmap.callback(e->button.x-gui->x,e->button.y-gui->y);

                    return 0;
                }
            }
        }
    }
    else if(gui->element_type == GUI_TYPE_WINDOW)
    {
        if(gui->info.window.enabled == 0)
            return 0;

        if(_gui_pos_is_inside_rect(e->button.x,e->button.y, gui->x,gui->w, gui->y,gui->h))
        {
            _gui * relative_gui = gui->info.window.gui;
            return GUI_SendEvent(relative_gui,e);
        }
        return 0;
    }
    else if(gui->element_type == GUI_TYPE_MESSAGEBOX)
    {
        if(e->type == SDL_MOUSEBUTTONDOWN)
        {
            if(e->button.button == SDL_BUTTON_LEFT)
            {
                if(_gui_pos_is_inside_rect(e->button.x,e->button.y,
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
                if(_gui_pos_is_inside_rect(e->button.x,e->button.y,gui->x,gui->w,
                                              gui->y,gui->h) == 0)
                {
                    gui->info.scrollabletextwindow.enabled = 0;
                    return 1;
                }

                int basexcoord = gui->x;
                int baseycoord = gui->y+FONT_HEIGHT+2;
                int textwidth = (gui->w / FONT_WIDTH) - 1;

                if(_gui_pos_is_inside_rect(e->button.x,e->button.y,
                                              basexcoord+textwidth*FONT_WIDTH,FONT_WIDTH,
                                              baseycoord,FONT_HEIGHT))
                {
                    gui->info.scrollabletextwindow.currentline --;
                    if(gui->info.scrollabletextwindow.currentline < 0)
                            gui->info.scrollabletextwindow.currentline = 0;
                    return 1;
                }

                if(_gui_pos_is_inside_rect(e->button.x,e->button.y,
                                basexcoord+textwidth*FONT_WIDTH,FONT_WIDTH,
                                baseycoord+(gui->info.scrollabletextwindow.max_drawn_lines-1)*FONT_HEIGHT,FONT_HEIGHT))
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

                if(_gui_pos_is_inside_rect(e->button.x,e->button.y,
                                basexcoord+textwidth*FONT_WIDTH,FONT_WIDTH,
                                baseycoord+FONT_HEIGHT,(gui->info.scrollabletextwindow.max_drawn_lines-1)*FONT_HEIGHT))
                {
                    int percent = ( ( e->button.y-(baseycoord+FONT_HEIGHT) ) * 100 ) /
                                            ( (gui->info.scrollabletextwindow.max_drawn_lines-4) * FONT_HEIGHT );

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
        else if(e->type == SDL_MOUSEMOTION)
        {
            if(e->motion.state & SDL_BUTTON_LMASK)
            {
                int basexcoord = gui->x;
                int baseycoord = gui->y+FONT_HEIGHT+2;
                int textwidth = (gui->w / FONT_WIDTH) - 1;

                if(_gui_pos_is_inside_rect(e->button.x,e->button.y,
                                basexcoord+textwidth*FONT_WIDTH,FONT_WIDTH,
                                baseycoord+FONT_HEIGHT,(gui->info.scrollabletextwindow.max_drawn_lines-1)*FONT_HEIGHT))
                {
                    int percent = ( ( e->button.y-(baseycoord+FONT_HEIGHT) ) * 100 ) /
                                            ( (gui->info.scrollabletextwindow.max_drawn_lines-4) * FONT_HEIGHT );

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

                    return 1;
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
    else if(gui->element_type == GUI_TYPE_SCROLLBAR)
    {
        if(e->type == SDL_MOUSEBUTTONDOWN)
        {
            if(e->button.button == SDL_BUTTON_LEFT)
            {
                //is inside the bar?
                if(_gui_pos_is_inside_rect(e->button.x,e->button.y, gui->x,gui->w, gui->y,gui->h))
                {
                    int rel_x = e->button.x - gui->x;
                    int rel_y = e->button.y - gui->y;

                    if(gui->info.scrollbar.is_vertical)
                    {
                        if(rel_y < gui->w)
                        {
                            //up button
                            gui->info.scrollbar.value --;
                        }
                        else if(rel_y > (gui->h-gui->w) )
                        {
                            //down button
                            gui->info.scrollbar.value ++;
                        }
                        else
                        {
                            //bar
                            int barsize = gui->h - (2 * (gui->w+1) ); // total length - side buttons
                            barsize -= 2*gui->w; // total length - side buttons - scrollable button
                            if(barsize < 0) return 1; // bad size...

                            int range = gui->info.scrollbar.value_max - gui->info.scrollbar.value_min;

                            int minus = (gui->w+1) + gui->w; // one side button plus half scroll button
                            gui->info.scrollbar.value = ( (rel_y - minus) * range ) / barsize;
                            gui->info.scrollbar.value += gui->info.scrollbar.value_min;
                        }
                    }
                    else
                    {
                        if(rel_x < gui->h)
                        {
                            //up button
                            gui->info.scrollbar.value --;
                        }
                        else if(rel_x > (gui->w-gui->h) )
                        {
                            //down button
                            gui->info.scrollbar.value ++;
                        }
                        else
                        {
                            //bar
                            int barsize = gui->w - (2 * (gui->h+1) ); // total length - side buttons
                            barsize -= 2*gui->h; // total length - side buttons - scrollable button
                            if(barsize < 0) return 1; // bad size...

                            int range = gui->info.scrollbar.value_max - gui->info.scrollbar.value_min;

                            int minus = (gui->h+1) + gui->h; // one side button plus half scroll button
                            gui->info.scrollbar.value = ( (rel_x - minus) * range ) / barsize;
                            gui->info.scrollbar.value += gui->info.scrollbar.value_min;
                        }
                    }

                    if(gui->info.scrollbar.value < gui->info.scrollbar.value_min)
                        gui->info.scrollbar.value = gui->info.scrollbar.value_min;
                    else if(gui->info.scrollbar.value > gui->info.scrollbar.value_max)
                        gui->info.scrollbar.value = gui->info.scrollbar.value_max;

                    if(gui->info.scrollbar.callback)
                        gui->info.scrollbar.callback(gui->info.scrollbar.value);

                    return 1;
                }
            }
        }
        else if(e->type == SDL_MOUSEMOTION)
        {
            if(e->motion.state & SDL_BUTTON_LMASK)
            {
                //is inside the bar?
                if(_gui_pos_is_inside_rect(e->button.x,e->button.y, gui->x,gui->w, gui->y,gui->h))
                {
                    int rel_x = e->button.x - gui->x;
                    int rel_y = e->button.y - gui->y;

                    if(gui->info.scrollbar.is_vertical)
                    {
                        if(rel_y < gui->w)
                        {
                            //up button
                        }
                        else if(rel_y > (gui->h-gui->w) )
                        {
                            //down button
                        }
                        else
                        {
                            //bar
                            int barsize = gui->h - (2 * (gui->w+1) ); // total length - side buttons
                            barsize -= 2*gui->w; // total length - side buttons - scrollable button
                            if(barsize < 0) return 1; // bad size...

                            int range = gui->info.scrollbar.value_max - gui->info.scrollbar.value_min;

                            int minus = (gui->w+1) + gui->w; // one side button plus half scroll button
                            gui->info.scrollbar.value = ( (rel_y - minus) * range ) / barsize;
                            gui->info.scrollbar.value += gui->info.scrollbar.value_min;
                        }
                    }
                    else
                    {
                        if(rel_x < gui->h)
                        {
                            //up button
                        }
                        else if(rel_x > (gui->w-gui->h) )
                        {
                            //down button
                        }
                        else
                        {
                            //bar
                            int barsize = gui->w - (2 * (gui->h+1) ); // total length - side buttons
                            barsize -= 2*gui->h; // total length - side buttons - scrollable button
                            if(barsize < 0) return 1; // bad size...

                            int range = gui->info.scrollbar.value_max - gui->info.scrollbar.value_min;

                            int minus = (gui->h+1) + gui->h; // one side button plus half scroll button
                            gui->info.scrollbar.value = ( (rel_x - minus) * range ) / barsize;
                            gui->info.scrollbar.value += gui->info.scrollbar.value_min;
                        }
                    }

                    if(gui->info.scrollbar.value < gui->info.scrollbar.value_min)
                        gui->info.scrollbar.value = gui->info.scrollbar.value_min;
                    else if(gui->info.scrollbar.value > gui->info.scrollbar.value_max)
                        gui->info.scrollbar.value = gui->info.scrollbar.value_max;

                    if(gui->info.scrollbar.callback)
                        gui->info.scrollbar.callback(gui->info.scrollbar.value);

                    return 1;
                }
            }
        }
    }
    else if(gui->element_type == GUI_TYPE_GROUPBOX)
    {
        //nothing
        return 0;
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
