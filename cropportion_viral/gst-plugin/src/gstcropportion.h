/*
 * GStreamer
 * Copyright (C) 2005 Thomas Vander Stichele <thomas@apestaart.org>
 * Copyright (C) 2005 Ronald S. Bultje <rbultje@ronald.bitfreak.net>
 * Copyright (C) 2020 Niels De Graef <niels.degraef@gmail.com>
 * Copyright (C) 2024 Viral Patel <<viral19patel.vp@gmail.com>>
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Alternatively, the contents of this file may be used under the
 * GNU Lesser General Public License Version 2.1 (the "LGPL"), in
 * which case the following provisions apply instead of the ones
 * mentioned above:
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef __GST_CROPPORTION_H__
#define __GST_CROPPORTION_H__

#include <gst/gst.h>
#ifdef HAVE_CONFIG_H
# include <config.h>
  #endif
  #include <math.h>
#include <gst/gst.h>
#include <stdio.h>
#include <glib.h>
#include <gst/video/video.h>
#include <gst/video/gstvideometa.h>


G_BEGIN_DECLS

#define GST_TYPE_CROPPORTION (gst_crop_portion_get_type())
G_DECLARE_FINAL_TYPE (GstCropPortion, gst_crop_portion,
    GST, CROPPORTION, GstElement)

	//Define the enum for the shapes
	typedef enum {
	    SQUARE,
	    RECTANGLE,
	    TRIANGLE,
	    CIRCLE
	} GstDiffShapeType;


typedef struct _GstCropPortion GstCropPortion;
struct _GstCropPortion
{
  GstElement element;
  GstDiffShapeType Shape_Type;

  gchar *fillcolour;
  gboolean strict;
  gint Size;
  gint frame_width;
  gint frame_height;
  gint coordinate[2] ;
  gint scalesize[2];
  gint shape_width, shape_height;
  gint start_row; 
  gint start_col;
 
  GstPad *sinkpad, *srcpad;
	gboolean cord_default; 
	gboolean default_shape;
	gboolean check_Property_validation; 
	gboolean cord_negative;
	gboolean scale_negative;
	gboolean size_default;
	gboolean scale_default;
	gboolean cord_exceed;
	gboolean shape_default;
	gint circle_radius;
	gint circle_center_x;
	gint circle_center_y;
};

G_END_DECLS

#endif /* __GST_CROPPORTION_H__ */
