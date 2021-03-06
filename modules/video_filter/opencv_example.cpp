/*****************************************************************************
 * opencv_example.cpp : Example OpenCV internal video filter
 * (performs face identification).  Mostly taken from the facedetect.c
 * OpenCV sample.
 *****************************************************************************
 * Copyright (C) 2006 the VideoLAN team
 *
 * Authors: Dugal Harris <dugalh@protoclea.co.za>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/

/*****************************************************************************
 * Preamble
 *****************************************************************************/
#include <cxcore.h>
#include <cv.h>
#include <highgui.h>


#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <vlc_common.h>
#include <vlc_plugin.h>
#include <vlc_filter.h>
#include <vlc_vout.h>
#include "filter_common.h"
#include <vlc_image.h>
#include "filter_event_info.h"

/*****************************************************************************
 * filter_sys_t : filter descriptor
 *****************************************************************************/
struct filter_sys_t
{
    CvMemStorage* p_storage;
    CvHaarClassifierCascade* p_cascade;
    video_filter_event_info_t event_info;
    int i_id;
};

/****************************************************************************
 * Local prototypes
 ****************************************************************************/
static int  OpenFilter ( vlc_object_t * );
static void CloseFilter( vlc_object_t * );

static picture_t *Filter( filter_t *, picture_t * );

/*****************************************************************************
 * Module descriptor
 *****************************************************************************/
vlc_module_begin ()
    set_description( N_("OpenCV face detection example filter") )
    set_shortname( N_( "OpenCV example" ))
    set_capability( "opencv example", 1 )
    add_shortcut( "opencv_example" )

    set_category( CAT_VIDEO )
    set_subcategory( SUBCAT_VIDEO_VFILTER2 )
    set_callbacks( OpenFilter, CloseFilter )

    add_string( "opencv-haarcascade-file", "c:\\haarcascade_frontalface_alt.xml", NULL,
                          N_("Haar cascade filename"),
                          N_("Name of XML file containing Haar cascade description"), false);
vlc_module_end ()

/*****************************************************************************
 * OpenFilter: probe the filter and return score
 *****************************************************************************/
static int OpenFilter( vlc_object_t *p_this )
{
    filter_t *p_filter = (filter_t*)p_this;
    filter_sys_t *p_sys;

    /* Allocate the memory needed to store the decoder's structure */
    if( ( p_filter->p_sys = p_sys =
          (filter_sys_t *)malloc(sizeof(filter_sys_t)) ) == NULL )
    {
        return VLC_ENOMEM;
    }

    //init the video_filter_event_info_t struct
    p_sys->event_info.i_region_size = 0;
    p_sys->event_info.p_region = NULL;
    p_sys->i_id = 0;

    p_filter->pf_video_filter = Filter;

    //create the VIDEO_FILTER_EVENT_VARIABLE
    vlc_value_t val;
    if (var_Create( p_filter->p_libvlc, VIDEO_FILTER_EVENT_VARIABLE, VLC_VAR_ADDRESS | VLC_VAR_DOINHERIT ) != VLC_SUCCESS)
        msg_Err( p_filter, "Could not create %s", VIDEO_FILTER_EVENT_VARIABLE);

    val.p_address = &(p_sys->event_info);
    if (var_Set( p_filter->p_libvlc, VIDEO_FILTER_EVENT_VARIABLE, val )!=VLC_SUCCESS)
        msg_Err( p_filter, "Could not set %s", VIDEO_FILTER_EVENT_VARIABLE);

    //OpenCV init specific to this example
    char* filename = config_GetPsz( p_filter, "opencv-haarcascade-file" );
    p_filter->p_sys->p_cascade = (CvHaarClassifierCascade*)cvLoad( filename, 0, 0, 0 );
    p_filter->p_sys->p_storage = cvCreateMemStorage(0);
    free( filename );

    return VLC_SUCCESS;
}

/*****************************************************************************
 * CloseFilter: clean up the filter
 *****************************************************************************/
static void CloseFilter( vlc_object_t *p_this )
{
    filter_t *p_filter = (filter_t*)p_this;
    filter_sys_t *p_sys = p_filter->p_sys;

    if( p_filter->p_sys->p_cascade )
        cvReleaseHaarClassifierCascade( &p_filter->p_sys->p_cascade );

    if( p_filter->p_sys->p_storage )
        cvReleaseMemStorage( &p_filter->p_sys->p_storage );

    free( p_filter->p_sys->event_info.p_region );
    free( p_sys );

    var_Destroy( p_filter->p_libvlc, VIDEO_FILTER_EVENT_VARIABLE);
}

/****************************************************************************
 * Filter: Check for faces and raises an event when one is found.
 ****************************************************************************
 * p_pic: A picture_t with its p_data_orig member set to an array of
 * IplImages (one image for each picture_t plane).
 ****************************************************************************/
static picture_t *Filter( filter_t *p_filter, picture_t *p_pic )
{
    IplImage** p_img = NULL;
    int i_planes = 0;
    CvPoint pt1, pt2;
    int i, scale = 1;
 
    if ((!p_pic) )
    {
        msg_Err( p_filter, "no image array" );
        return NULL;
    }
    if (!(p_pic->p_data_orig))
    {
        msg_Err( p_filter, "no image array" );
        return NULL;
    }
    //(hack) cast the picture_t to array of IplImage*
    p_img = (IplImage**) p_pic->p_data_orig;
    i_planes = p_pic->i_planes;

    //check the image array for validity
    if ((!p_img[0]))    //1st plane is 'I' i.e. greyscale
    {
        msg_Err( p_filter, "no image" );
        return NULL;
    }
    if ((p_pic->format.i_chroma != VLC_CODEC_I420))
    {
        msg_Err( p_filter, "wrong chroma - use I420" );
        return NULL;
    }
    if (i_planes<1)
    {
        msg_Err( p_filter, "no image planes" );
        return NULL;
    }

    //perform face detection
    cvClearMemStorage(p_filter->p_sys->p_storage);
    CvSeq* faces = NULL;
    if( p_filter->p_sys->p_cascade )
    {
        //we should make some of these params config variables
        faces = cvHaarDetectObjects( p_img[0], p_filter->p_sys->p_cascade,
            p_filter->p_sys->p_storage, 1.15, 5, CV_HAAR_DO_CANNY_PRUNING,
                                            cvSize(20, 20) );
        //create the video_filter_region_info_t struct
        CvRect* r;
        if (faces && (faces->total > 0))
        {
            //msg_Dbg( p_filter, "Found %d face(s)", faces->total );
            free( p_filter->p_sys->event_info.p_region );
            p_filter->p_sys->event_info.p_region = NULL;
            if( NULL == ( p_filter->p_sys->event_info.p_region =
                  (video_filter_region_info_t *)malloc(faces->total*sizeof(video_filter_region_info_t))))
            {
                return NULL;
            }
            memset(p_filter->p_sys->event_info.p_region, 0, faces->total*sizeof(video_filter_region_info_t));
            p_filter->p_sys->event_info.i_region_size = faces->total;
        }

        //populate the video_filter_region_info_t struct
        for( i = 0; i < (faces ? faces->total : 0); i++ )
        {
            r = (CvRect*)cvGetSeqElem( faces, i );
            pt1.x = r->x*scale;
            pt2.x = (r->x+r->width)*scale;
            pt1.y = r->y*scale;
            pt2.y = (r->y+r->height)*scale;
            cvRectangle( p_img[0], pt1, pt2, CV_RGB(0,0,0), 3, 8, 0 );

            *(CvRect*)(&(p_filter->p_sys->event_info.p_region[i])) = *r;
            p_filter->p_sys->event_info.p_region[i].i_id = p_filter->p_sys->i_id++;
            p_filter->p_sys->event_info.p_region[i].p_description = "Face Detected";
        }

        if (faces && (faces->total > 0))    //raise the video filter event
            var_TriggerCallback( p_filter->p_libvlc, VIDEO_FILTER_EVENT_VARIABLE );
    }
    else
        msg_Err( p_filter, "No cascade - is opencv-haarcascade-file valid?" );

    return p_pic;
}

