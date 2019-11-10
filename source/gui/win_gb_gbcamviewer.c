// SPDX-License-Identifier: GPL-2.0-or-later
//
// Copyright (c) 2011-2015, 2019, Antonio Niño Díaz
//
// GiiBiiAdvance - GBA/GB emulator

#include <SDL.h>

#include <string.h>

#include "../debug_utils.h"
#include "../file_utils.h"
#include "../font_utils.h"
#include "../general_utils.h"
#include "../png_utils.h"
#include "../window_handler.h"

#include "win_gb_debugger.h"
#include "win_main.h"
#include "win_utils.h"

#include "../gb_core/gameboy.h"
#include "../gb_core/debug_video.h"
#include "../gb_core/camera.h"

//------------------------------------------------------------------------------------------------

extern _GB_CONTEXT_ GameBoy;

//------------------------------------------------------------------------------------------------

static int WinIDGBCameraViewer;

#define WIN_GB_CAMERAVIEWER_WIDTH  700
#define WIN_GB_CAMERAVIEWER_HEIGHT 500

static int GBCameraViewerCreated = 0;

//-----------------------------------------------------------------------------------

static u32 gb_cameraview_selected_photo_index = 0;

#define GB_CAMERA_ALLPHOTOS_BUFFER_WIDTH  (248)
#define GB_CAMERA_ALLPHOTOS_BUFFER_HEIGHT (208)

static char gb_allphotos_buffer[GB_CAMERA_ALLPHOTOS_BUFFER_WIDTH*GB_CAMERA_ALLPHOTOS_BUFFER_HEIGHT*3];

static char gb_photo_zoomed_buffer[128*112*3];

static char gb_camera_webcam_output_buffer[128*112*3];
static char gb_camera_retina_processed_buffer[128*112*3];
static char gb_camera_scratch_photo_buffer[128*112*3];

//-----------------------------------------------------------------------------------

static _gui_element gb_camview_photos_groupbox;
static _gui_console gb_camview_con;
static _gui_element gb_camview_textbox;
static _gui_element gb_camview_photos_all_bmp;
static _gui_element gb_camview_photos_zoomed_bmp;
static _gui_element gb_camview_dumpbtn; // add "dump all", "dump this" and "dump all mini" buttons

static _gui_element gb_camview_cam_output_groupbox;
static _gui_element gb_camview_webcam_output_bmp;
static _gui_element gb_camview_retina_processed_bmp;
static _gui_element gb_camview_scratch_photo_bmp;

static _gui_element * gb_camviwer_window_gui_elements[] = {
    &gb_camview_photos_groupbox,
    &gb_camview_photos_all_bmp,
    &gb_camview_photos_zoomed_bmp,
    &gb_camview_textbox,
    &gb_camview_dumpbtn,

    &gb_camview_cam_output_groupbox,
    &gb_camview_webcam_output_bmp,
    &gb_camview_retina_processed_bmp,
    &gb_camview_scratch_photo_bmp,

    NULL
};

static _gui gb_camviewer_window_gui = {
    gb_camviwer_window_gui_elements,
    NULL,
    NULL
};

//----------------------------------------------------------------

static int _win_gb_camviewer_allphotos_bmp_callback(int x, int y)
{
    if(x >= ((32+8)*6)) x = ((32+8)*6)-1;
    if(y >= ((32+8)*5)) y = ((32+8)*5)-1;
    int x_ = (x/(32+8));
    int y_ = (y/(32+8));

    gb_cameraview_selected_photo_index = x_+y_*6;

    return 1;
}

//----------------------------------------------------------------

void Win_GB_GBCameraViewerUpdate(void)
{
    if(GBCameraViewerCreated == 0) return;

    if(Win_MainRunningGB() == 0) return;

    if(GB_MapperIsGBCamera() == 0) return;

    GUI_ConsoleClear(&gb_camview_con);

    //_GB_MEMORY_ * mem = &GameBoy.Memory;

    if(GameBoy.Emulator.RAM_Banks == 16) // if this is official GB Camera software
    {
        GB_Debug_GBCameraMiniPhotoPrintAll(gb_allphotos_buffer);

        GUI_Draw_SetDrawingColor(255,0,0);
        int l = (gb_cameraview_selected_photo_index%6)*(32+8)+8-1; //left
        int t = (gb_cameraview_selected_photo_index/6)*(32+8)+8-1; // top
        int r = l + 32+1; // right
        int b = t + 32+1; // bottom
        GUI_Draw_Rect(gb_allphotos_buffer,GB_CAMERA_ALLPHOTOS_BUFFER_WIDTH,GB_CAMERA_ALLPHOTOS_BUFFER_HEIGHT,l,r,t,b);
        l--; t--; r++; b++;
        GUI_Draw_Rect(gb_allphotos_buffer,GB_CAMERA_ALLPHOTOS_BUFFER_WIDTH,GB_CAMERA_ALLPHOTOS_BUFFER_HEIGHT,l,r,t,b);

        GB_Debug_GBCameraPhotoPrint(gb_photo_zoomed_buffer,128,114,gb_cameraview_selected_photo_index);
    }

    //---------------------------

    GB_Debug_GBCameraWebcamOutputPrint(gb_camera_webcam_output_buffer, 128,112);
    GB_Debug_GBCameraRetinaProcessedPrint(gb_camera_retina_processed_buffer, 128,112);
    GB_Debug_GBCameraPhotoPrint(gb_camera_scratch_photo_buffer, 128,112, -1);

}

//----------------------------------------------------------------

static void _win_gb_camera_viewer_render(void)
{
    if(GBCameraViewerCreated == 0) return;

    char buffer[WIN_GB_CAMERAVIEWER_WIDTH*WIN_GB_CAMERAVIEWER_HEIGHT*3];
    GUI_Draw(&gb_camviewer_window_gui,buffer,WIN_GB_CAMERAVIEWER_WIDTH,WIN_GB_CAMERAVIEWER_HEIGHT,1);

    WH_Render(WinIDGBCameraViewer, buffer);
}

static int _win_gb_map_viewer_callback(SDL_Event * e)
{
    if(GBCameraViewerCreated == 0) return 1;

    if(GB_MapperIsGBCamera() == 0) return 0;

    int redraw = GUI_SendEvent(&gb_camviewer_window_gui,e);

    int close_this = 0;

    if(e->type == SDL_WINDOWEVENT)
    {
        if(e->window.event == SDL_WINDOWEVENT_FOCUS_GAINED)
        {
            redraw = 1;
        }
        else if(e->window.event == SDL_WINDOWEVENT_EXPOSED)
        {
            redraw = 1;
        }
        else if(e->window.event == SDL_WINDOWEVENT_CLOSE)
        {
            close_this = 1;
        }
    }
    else if( e->type == SDL_KEYDOWN)
    {
        if( e->key.keysym.sym == SDLK_ESCAPE )
        {
            close_this = 1;
        }
    }

    if(close_this)
    {
        GBCameraViewerCreated = 0;
        WH_Close(WinIDGBCameraViewer);
        return 1;
    }

    if(redraw)
    {
        Win_GB_GBCameraViewerUpdate();
        _win_gb_camera_viewer_render();
        return 1;
    }

    return 0;
}

//----------------------------------------------------------------

static void _win_gb_mapviewer_dump_btn_callback(void)
{
    if(Win_MainRunningGB() == 0) return;

    if(GB_MapperIsGBCamera() == 0) return;

/*
    if(gb_map_zoomed_tile_is_pal == 0)
        GB_Debug_MapPrintBW(gb_map_buffer,GB_MAP_BUFFER_WIDTH,GB_MAP_BUFFER_HEIGHT,gb_mapview_selected_mapbase,
                          gb_mapview_selected_tilebase);
    else
        GB_Debug_MapPrint(gb_map_buffer,GB_MAP_BUFFER_WIDTH,GB_MAP_BUFFER_HEIGHT,gb_mapview_selected_mapbase,
                          gb_mapview_selected_tilebase);

    char buffer_temp[GB_MAP_BUFFER_WIDTH*GB_MAP_BUFFER_HEIGHT*4];

    char * src = gb_map_buffer;
    char * dst = buffer_temp;
    int i;
    for(i = 0; i < GB_MAP_BUFFER_WIDTH*GB_MAP_BUFFER_HEIGHT; i++)
    {
        *dst++ = *src++;
        *dst++ = *src++;
        *dst++ = *src++;
        *dst++ = 0xFF;
    }

    char * name = FU_GetNewTimestampFilename("gb_map");
    Save_PNG(name,GB_MAP_BUFFER_WIDTH,GB_MAP_BUFFER_HEIGHT,buffer_temp,0);
*/
    Win_GB_GBCameraViewerUpdate();
}

//----------------------------------------------------------------

int Win_GB_GBCameraViewerCreate(void)
{
    if(Win_MainRunningGB() == 0) return 0;

    if(GB_MapperIsGBCamera() == 0) return 0;

    if(GBCameraViewerCreated == 1)
    {
        WH_Focus(WinIDGBCameraViewer);
        return 0;
    }

    //GUI_SetTextBox(&gb_camview_textbox,&gb_camview_con,
    //               6,108, 19*FONT_WIDTH,6*FONT_HEIGHT, NULL);

    //-------------------------------------------------------------------

    GUI_SetGroupBox(&gb_camview_photos_groupbox,6,6,6+GB_CAMERA_ALLPHOTOS_BUFFER_WIDTH+6+128+6,300,"Photos");

    GUI_SetBitmap(&gb_camview_photos_all_bmp,12,18,GB_CAMERA_ALLPHOTOS_BUFFER_WIDTH,GB_CAMERA_ALLPHOTOS_BUFFER_HEIGHT,
                  gb_allphotos_buffer, _win_gb_camviewer_allphotos_bmp_callback);

    GUI_SetBitmap(&gb_camview_photos_zoomed_bmp,12+GB_CAMERA_ALLPHOTOS_BUFFER_WIDTH+6,18, 128,112,
                  gb_photo_zoomed_buffer, NULL);

    GUI_SetButton(&gb_camview_dumpbtn,12+GB_CAMERA_ALLPHOTOS_BUFFER_WIDTH+6,238,FONT_WIDTH*6,FONT_HEIGHT*2,"Dump",
                  _win_gb_mapviewer_dump_btn_callback);

    //-------------------------------------------------------------------

    GUI_SetGroupBox(&gb_camview_cam_output_groupbox,406,6,
                    6+128+6,366,"Webcam|Retina|RAM");

    GUI_SetBitmap(&gb_camview_webcam_output_bmp,412,18, 128,112, gb_camera_webcam_output_buffer, NULL);

    GUI_SetBitmap(&gb_camview_retina_processed_bmp,412,18+112+6, 128,112, gb_camera_retina_processed_buffer, NULL);

    GUI_SetBitmap(&gb_camview_scratch_photo_bmp,412,18+224+12, 128,112, gb_camera_scratch_photo_buffer, NULL);

    //-------------------------------------------------------------------

    GBCameraViewerCreated = 1;

    gb_cameraview_selected_photo_index = 0;

    WinIDGBCameraViewer = WH_Create(WIN_GB_CAMERAVIEWER_WIDTH,WIN_GB_CAMERAVIEWER_HEIGHT, 0,0, 0);
    WH_SetCaption(WinIDGBCameraViewer,"GB Camera Viewer");

    WH_SetEventCallback(WinIDGBCameraViewer,_win_gb_map_viewer_callback);

    Win_GB_GBCameraViewerUpdate();
    _win_gb_camera_viewer_render();

    return 1;
}

void Win_GB_GBCameraViewerClose(void)
{
    if(GBCameraViewerCreated == 0)
        return;

    GBCameraViewerCreated = 0;
    WH_Close(WinIDGBCameraViewer);
}

//----------------------------------------------------------------
