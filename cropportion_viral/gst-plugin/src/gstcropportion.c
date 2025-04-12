/*
 * GStreamer
 * Copyright (C) 2005 Thomas Vander Stichele <thomas@apestaart.org>
 * Copyright (C) 2005 Ronald S. Bultje <rbultje@ronald.bitfreak.net>
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

/**
 * SECTION:element-cropportion
 *
 * FIXME:Describe cropportion here.
 *
 * <refsect2>
 * <title>Example launch line</title>
 * |[
 * gst-launch -v -m fakesrc ! cropportion ! fakesink silent=TRUE
 * ]|
 * </refsect2>
 */

#include "gstcropportion.h"

GST_DEBUG_CATEGORY_STATIC(gst_crop_portion_debug);
#define GST_CAT_DEFAULT gst_crop_portion_debug

#define CORD_DEFAULT_X 0
#define CORD_DEFAULT_Y 0
#define SCALE_DEFAULT_X 0
#define SCALE_DEFAULT_Y 0
#define SHAPEWIDTH_DEFAULT 0
#define SHAPEHEIGHT_DEFAULT 0
#define COLOUR_DEFAULT "white"
#define STRICT_DEFAULT FALSE

/* Filter signals and args */
enum {
  LAST_SIGNAL
};

enum {
  PROP_0,
  PROP_CORD,
  PROP_SHAPE,
  PROP_SHAPE_SIZE,
  PROP_SCALE_SIZE,
  PROP_FILL_COLOUR,
  PROP_STRICT_MODE
};

// the capabilities of the inputs and outputs.
static GstStaticPadTemplate sink_factory = GST_STATIC_PAD_TEMPLATE("sink",
  GST_PAD_SINK,
  GST_PAD_ALWAYS,
  GST_STATIC_CAPS("video/x-raw, format=NV12,"
									"format=(string)NV12,"
									"width=(int)[1,1920],"
									"height=(int)[1,1080],"
									"framerate=(fraction)[1,60]")
);

static GstStaticPadTemplate src_factory = GST_STATIC_PAD_TEMPLATE("src",
  GST_PAD_SRC,
  GST_PAD_ALWAYS,
  GST_STATIC_CAPS("video/x-raw,"
									"format=(string)NV12,"
									"width=(int)[1,1920],"
									"height=(int)[1,1080],"
									"framerate=(fraction)[1,60]"
		)
);

#define gst_crop_portion_parent_class parent_class
G_DEFINE_TYPE(GstCropPortion, gst_crop_portion, GST_TYPE_ELEMENT);

GST_ELEMENT_REGISTER_DEFINE(crop_portion, "cropportion", GST_RANK_NONE,
  GST_TYPE_CROPPORTION); //Register the cropportion ,second field is plugin name which is given in pipeline

#define DEFAULT_SHAPE_TYPE SQUARE

static void gst_crop_portion_set_property(GObject * object,
  guint prop_id,
  const GValue * value, GParamSpec * pspec);
static void gst_crop_portion_get_property(GObject * object,
  guint prop_id, GValue * value, GParamSpec * pspec);

static gboolean gst_crop_portion_sink_event(GstPad * pad,
  GstObject * parent, GstEvent * event);
static GstFlowReturn gst_crop_portion_chain(GstPad * pad,
  GstObject * parent, GstBuffer * buf);
void gst_crop_portion_dimensions (GstCropPortion *filter, int *width, int *height);

#define GST_TYPE_DIFF_SHAPE_TYPE gst_diff_shape_type_get_type()
static GType
gst_diff_shape_type_get_type(void) {
  static GType diff_shape_type = 0;
  static
  const GEnumValue Shape_Type[] = {
    {
      SQUARE,
      "SQUARE SHAPE TO CROP",
      "square"
    },
    {
      RECTANGLE,
      "RECTANGLE SHAPE TO CROP",
      "rectangle"
    },
    {
      TRIANGLE,
      "TRIANGLE SHAPE TO CROP",
      "triangle"
    },
    {
      CIRCLE,
      "CIRCLE SHAPE TO CROP",
      "circle"
    },
    {
      0,
      NULL,
      NULL
    },
  };

  if (!diff_shape_type) {
    diff_shape_type = g_enum_register_static("GstDiffShapeType", Shape_Type);
  }

  return diff_shape_type;
}

/* initialize the cropportion's class */
static void
gst_crop_portion_class_init(GstCropPortionClass * klass) {

  GObjectClass * gobject_class;
  GstElementClass * gstelement_class;

  gobject_class = (GObjectClass * ) klass;
  gstelement_class = (GstElementClass * ) klass;

  gobject_class -> set_property = gst_crop_portion_set_property; /* define virtual function pointers */
  gobject_class -> get_property = gst_crop_portion_get_property;

  //install property for coordinate
  g_object_class_install_property(gobject_class, PROP_CORD,
    gst_param_spec_array("crop-coordinate", "Coordinate", "It will crop the shape from center of coordinate given by user in formate '<x,y>'. The default value are center of resolution",
      g_param_spec_int("element", "Element", "element of an array", G_MININT, G_MAXINT, 0, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS), G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  //install prop for shape
  g_object_class_install_property(gobject_class, PROP_SHAPE,
    g_param_spec_enum("shape", "Different Shape",
      "To crop different shapes like square(0),rectangle(1),triangle(2),circle(3) and default shape is square.",
      GST_TYPE_DIFF_SHAPE_TYPE, DEFAULT_SHAPE_TYPE,
      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  //install property for shape-size
g_object_class_install_property(gobject_class, PROP_SHAPE_SIZE,
    g_param_spec_int("shape-size", "Shape-size", "Size of cropped shape", 2, INT_MAX, 2, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));


  //install property for scale-size
  g_object_class_install_property(gobject_class, PROP_SCALE_SIZE,
    gst_param_spec_array("scale-size", "scale-size", "To scale the cropped part in any resolution and maximum resolution is 4k in formate '<x,y>'",
      g_param_spec_int("element", "Element", "element of an array", G_MININT, G_MAXINT, 0, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS), G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  //install property for fill-colour		      
  g_object_class_install_property(gobject_class, PROP_FILL_COLOUR,
    g_param_spec_string("fill-color", "fill-colour", "To fill the colour outside the remaining portion of circle and triangle shapes. The fill colour are black,white,red,green,blue and the default fill is white", COLOUR_DEFAULT, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  //install property for STRICT
  g_object_class_install_property(gobject_class, PROP_STRICT_MODE,
    g_param_spec_boolean("strict-mode", "Strict", " In strict mode all warning are consider to be error if it is TRUE otherwise it take default values",
      STRICT_DEFAULT, G_PARAM_READWRITE));

  gst_element_class_set_details_simple(gstelement_class, //element meta provide extra information of plugin
    "Crop Portion", //name of plugin
    "crop the shape ", //name
    "crop ths shape like square,rectangle,triangle and circle. It can also scale the cropped part upto 4k resolution", "Viral Patel <<user@hostname.org>>"); //discription, author

  gst_element_class_add_pad_template(gstelement_class,
    gst_static_pad_template_get( & src_factory)); //registered the src pad
  gst_element_class_add_pad_template(gstelement_class,
    gst_static_pad_template_get( & sink_factory)); //registered the sink pad
}

/* 
 * initialize the new element
 * instantiate pads and add them to element
 * set pad callback functions
 * initialize instance structure
 */
static void
gst_crop_portion_init(GstCropPortion * filter) {

  filter -> sinkpad = gst_pad_new_from_static_template( & sink_factory, "sink"); //created sink pad which is registed in class class_init() 

  gst_pad_set_event_function(filter -> sinkpad, // configure event function on the pad before adding the pad to the element
    GST_DEBUG_FUNCPTR(gst_crop_portion_sink_event));

  gst_pad_set_chain_function(filter -> sinkpad, //configure chain function on the pad before adding  the pad to the element
    GST_DEBUG_FUNCPTR(gst_crop_portion_chain));

  GST_PAD_SET_PROXY_CAPS(filter -> sinkpad);

  gst_element_add_pad(GST_ELEMENT(filter), filter -> sinkpad);

  filter -> srcpad = gst_pad_new_from_static_template( & src_factory, "src"); //created src pad which is registed in class class_init() 
  GST_PAD_SET_PROXY_CAPS(filter -> srcpad);
  gst_element_add_pad(GST_ELEMENT(filter), filter -> srcpad);

  //set default coordinate
  filter -> coordinate[0] = CORD_DEFAULT_X;
  filter -> coordinate[1] = CORD_DEFAULT_Y;
  filter -> scalesize[0] = SCALE_DEFAULT_X;
  filter -> scalesize[1] = SCALE_DEFAULT_Y;
  filter -> shape_width = SHAPEWIDTH_DEFAULT;
  filter -> shape_height = SHAPEHEIGHT_DEFAULT;
  filter -> Shape_Type = DEFAULT_SHAPE_TYPE;
  filter -> fillcolour = COLOUR_DEFAULT;
  filter -> strict = STRICT_DEFAULT;
  filter -> cord_default = FALSE;  // set the coordinate to center of resolution if false 
  filter -> scale_default = FALSE;
  filter -> cord_exceed = FALSE; 
  filter -> size_default = FALSE;
  
};

static void
gst_crop_portion_set_property(GObject * object, guint prop_id,
  const GValue * value, GParamSpec * pspec) {

  GstCropPortion * filter = GST_CROPPORTION(object);

  switch (prop_id) {

  case PROP_CORD: {
    const GValue * v;
    gint i;

    // Check if the array size is correct
    if (gst_value_array_get_size(value) != 2) { //check for 2 argument <1,2>
      goto wrong_format;
    }

    // Iterate through the array to get the coordinates
    for (i = 0; i < 2; i++) {
      v = gst_value_array_get_value(value, i); //value at respective index will be stored eg. v[0]=1,v[1]=2
      if (!G_VALUE_HOLDS_INT(v)) //check the is g_type_int ? if false then wrong formate will implemented  
        goto wrong_format;
      filter -> coordinate[i] = g_value_get_int(v);
		  }
		  filter -> circle_center_x = filter -> coordinate[0];
		  filter -> circle_center_y = filter -> coordinate[1];
		  filter -> cord_default = TRUE;
		  filter -> cord_exceed = TRUE;
  	}
  break;
  wrong_format:
    g_warning("Invalid size of coordinate or integer number\n");
  break;

  case PROP_SHAPE:
    filter -> Shape_Type = g_value_get_enum(value);
    break;

  case PROP_SHAPE_SIZE:
    filter -> Size = g_value_get_int(value);
    filter -> circle_radius = filter -> Size;
    filter -> size_default = TRUE;
    break;

  case PROP_SCALE_SIZE: {
    const GValue * v;
    gint i;

    if (gst_value_array_get_size(value) != 2) { //check for 2 argument <1,2>
      goto invalid;
    }

    for (i = 0; i < 2; i++) {
      v = gst_value_array_get_value(value, i); //value at respective index will be stored eg. v[0]=1,v[1]=2
      if (!G_VALUE_HOLDS_INT(v)) //check it is g_type_int ? if false then wrong formate will implemented  
        goto invalid;
      filter -> scalesize[i] = g_value_get_int(v);
    }
    filter -> scale_default = TRUE;
  }
  break;
  invalid:
    g_warning("Invalid size of Resize value or integer number\n");
  break;

  case PROP_FILL_COLOUR:
    filter -> fillcolour = strdup(g_value_get_string(value));
    break;

  case PROP_STRICT_MODE:
    filter -> strict = g_value_get_boolean(value);
    break;

  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
    break;
  }
}

static void
gst_crop_portion_get_property(GObject * object, guint prop_id,
  GValue * value, GParamSpec * pspec) {
  GstCropPortion * filter = GST_CROPPORTION(object);

  switch (prop_id) {

  case PROP_CORD:

    gint i;
    for (i = 0; i < 2; i++) {
      GValue v = G_VALUE_INIT;
      g_value_init( & v, G_TYPE_INT);
      g_value_set_int( & v, filter -> coordinate[i]);
      gst_value_array_append_and_take_value(value, & v);
      g_value_unset( & v);
    }
    break;

  case PROP_SHAPE:
    g_value_set_enum(value, filter -> Shape_Type);
    break;

  case PROP_SHAPE_SIZE:
    g_value_set_int(value, filter -> Size);
    break;

  case PROP_SCALE_SIZE:
    gint j;
    for (j = 0; j < 2; j++) {
      GValue v = G_VALUE_INIT;
      g_value_init( & v, G_TYPE_INT);
      g_value_set_int( & v, filter -> scalesize[j]);
      gst_value_array_append_and_take_value(value, & v);
      g_value_unset( & v);
    }
    break;

  case PROP_FILL_COLOUR:
    g_value_set_string(value, filter -> fillcolour);
    break;

  case PROP_STRICT_MODE:
    g_value_set_boolean(value, filter -> strict);
    break;

  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
    break;
  }
}

/* this function handles sink events */
static gboolean
gst_crop_portion_sink_event(GstPad * pad, GstObject * parent,
  GstEvent * event) {

  GstCropPortion * filter;
  gboolean ret;
  filter = GST_CROPPORTION(parent);

  GST_LOG_OBJECT(filter, "Received %s event: %"
    GST_PTR_FORMAT,
    GST_EVENT_TYPE_NAME(event), event);

  switch (GST_EVENT_TYPE(event)) {

  case GST_EVENT_CAPS: {
    GstCaps * caps;
    gchar * caps_str;
    gst_event_parse_caps(event, & caps);
    caps_str = gst_caps_to_string(caps);
    /* do something with the caps */
    g_print("Received caps in sink: %s\n", caps_str);
    g_free(caps_str);

    /* and forward */
    ret = gst_pad_event_default(pad, parent, event);
    gint width, height;
    GstStructure * structure = gst_caps_get_structure(caps, 0); // Assuming single structure
    gst_structure_get_int(structure, "width", & width);
    gst_structure_get_int(structure, "height", & height);
    filter -> frame_width = width;
    filter -> frame_height = height;

    if (filter -> Size == 0 || filter -> Size < 0) {
      g_warning("shape size is mandatory and positive value\n");
      exit(1);
    }
    
    if (filter->Size < 2) {
					g_warning ("shape-size cannot be %d.\n", filter->Size);
					exit (1);
		}
		
		if(filter->Size == 2 || filter->Size == 3){
			filter->Size = 4;
		}
		
		if ( (filter->Size % 2) != 0) {
			filter->Size -= 1;
		}

    break;
  }

  default:
    ret = gst_pad_event_default(pad, parent, event);
    break;
  }

  return ret;
}

static GstFlowReturn
gst_crop_portion_chain(GstPad * pad, GstObject * parent, GstBuffer * buf) {

  GstCropPortion * filter;
  filter = GST_CROPPORTION(parent);
  
  // adjust the crop coordinates if not even.
	if ( (filter -> coordinate[0] % 2) != 0 ) 
		filter -> coordinate[0] -= 1;
	if ( (filter -> coordinate[1] % 2) != 0)
		filter -> coordinate[1] -= 1;

  if (!filter -> cord_default) {
    filter -> coordinate[0] = filter -> frame_width / 2;
    filter -> coordinate[1] = filter -> frame_height / 2;
    filter -> circle_center_x = filter -> coordinate[0];
    filter -> circle_center_y = filter -> coordinate[1];
    g_print("cordinate set to : <%d, %d> \n", filter -> coordinate[0], filter -> coordinate[1]);
    filter -> cord_default = TRUE;
  }

  if (filter -> cord_default) {
    if (filter -> coordinate[0] >= filter -> frame_width || filter -> coordinate[1] >= filter -> frame_height || filter -> coordinate[0] < 0 || filter -> coordinate[1] < 0) {
      if (filter -> strict) {
        GST_ELEMENT_ERROR(filter, RESOURCE, NOT_FOUND, ("Invalid value for coordinate property."),
          ("Give coordinates within the boundary."));
        exit(1);
      } else {
        g_warning("Invalid value negative or height or width greater than incoming video '<%d, %d>' for coordinate property. Valid values are positive integer numbers or inside incoming video resolution.", filter -> coordinate[0], filter -> coordinate[1]);

        filter -> coordinate[0] = filter -> frame_width / 2;
        filter -> coordinate[1] = filter -> frame_height / 2;
        filter -> circle_center_x = filter -> coordinate[0];
        filter -> circle_center_y = filter -> coordinate[1];
        g_print("cordinate set to : <%d, %d> \n", filter -> coordinate[0], filter -> coordinate[1]);
        filter -> cord_default = FALSE;
      }
    }
  }

  gint incoming_width, incoming_height; //Incoming height & width
  GstCaps * caps;
  gint size;
  gint new_buf_width;
  gint new_buf_height;

  GstBuffer * buffer_new = NULL, * scalebuf = NULL;
  GstVideoInfo video_info_new_buffer;
  GstVideoFrame vframe_new_buffer;

  GstCaps * incomingcaps = gst_caps_copy(gst_pad_get_current_caps(pad));
  GstStructure * structure = gst_caps_get_structure(incomingcaps, 0);

  gst_structure_get(structure, "width", G_TYPE_INT, & incoming_width, NULL);
  gst_structure_get(structure, "height", G_TYPE_INT, & incoming_height, NULL);

  switch (filter -> Shape_Type) {
  
  case SQUARE: {
    
    gst_crop_portion_dimensions (filter, &new_buf_width, &new_buf_height); //calculate dimension
         
    if (filter -> scalesize[0] == 0 && filter -> scalesize[1] == 0) { //if user didn't give scale then new buffer is sent as it is otherwise set scale size
      filter -> scalesize[0] = new_buf_width;
      filter -> scalesize[1] = new_buf_height;
    }
    
    if (filter -> scale_default) {
      if (filter -> scalesize[0] > 3840 || filter -> scalesize[1] > 2160 || filter -> scalesize[0] < 0 || filter -> scalesize[1] < 0) {
        if (filter -> strict) {
          GST_ELEMENT_ERROR(filter, RESOURCE, NOT_FOUND, ("Invalid value for scale-size property."),
            ("Maximum resolution is 4k."));
          exit(1);
        } else {
          g_warning("Invalid value (negative or resolution higher than 4k) '<%d, %d>' for scale-size property. Valid values are positive integer numbers or inside 4k video resolution.", filter -> scalesize[0], filter -> scalesize[1]);

          filter -> scalesize[0] = new_buf_width;
          filter -> scalesize[1] = new_buf_height;				
          g_print("scale-size set to : <%d, %d> \n", filter -> scalesize[0], filter -> scalesize[1]);
          filter -> scale_default = FALSE;
        }
      }
    }
    gst_structure_set(structure, "width", G_TYPE_INT, filter -> scalesize[0], "height", G_TYPE_INT, filter -> scalesize[1], "format", G_TYPE_STRING, "NV12", NULL);

    caps = gst_caps_new_simple("video/x-raw",
      "format", G_TYPE_STRING, "NV12",
      "width", G_TYPE_INT, new_buf_width,
      "height", G_TYPE_INT, new_buf_height,
      "bpp", G_TYPE_INT, 12,
      NULL);
    
    size = new_buf_width * new_buf_height * (1.5);
    buffer_new = gst_buffer_new_allocate(NULL, size, NULL);

    if (!gst_buffer_make_writable(buffer_new)) {
      GST_ELEMENT_ERROR(filter, RESOURCE, NOT_FOUND, ("Failed to make the buffer writable"),
        ("Failed to make the  new buffer writable"));
      exit(1);
    }
		
    if (!gst_video_info_from_caps( & video_info_new_buffer, caps)) {
      GST_ELEMENT_ERROR(filter, RESOURCE, NOT_FOUND, ("fail to get videoinfo"),
        ("fail to get videoinfo for new buffer"));
      exit(1);
    }

    if (!gst_video_frame_map( & vframe_new_buffer, & video_info_new_buffer, buffer_new, GST_MAP_WRITE)) {
      GST_ELEMENT_ERROR(filter, RESOURCE, NOT_FOUND, ("failed to map videoframe"),
        ("failed to map videoframe of new buffer"));
     	exit(1);
    }

    guint8 * y_pixels_N = GST_VIDEO_FRAME_PLANE_DATA( & vframe_new_buffer, 0); // Y plane-Starting address of pixel
    guint8 * uv_pixels_N = GST_VIDEO_FRAME_PLANE_DATA( & vframe_new_buffer, 1); // UV plane

    guint y_stride_N = GST_VIDEO_FRAME_PLANE_STRIDE( & vframe_new_buffer, 0); // Y plane stride-Xaxis
    guint uv_stride_N = GST_VIDEO_FRAME_PLANE_STRIDE( & vframe_new_buffer, 1); // UV plane stride

    guint y_N_pixel_stride = GST_VIDEO_FRAME_COMP_PSTRIDE( & vframe_new_buffer, 0);
    guint uv_N_pixel_stride = GST_VIDEO_FRAME_COMP_PSTRIDE( & vframe_new_buffer, 1);

    GstVideoInfo video_info_org_buffer;
    GstVideoFrame vframe_org_buffer;

    if (!gst_video_info_from_caps( & video_info_org_buffer, gst_pad_get_current_caps(pad))) {
      GST_ELEMENT_ERROR(filter, RESOURCE, NOT_FOUND, ("fail to get videoinfo"),
        ("fail to get videoinfo for original buffer"));
      exit(1);
    }

    if (!gst_video_frame_map( & vframe_org_buffer, & video_info_org_buffer, buf, GST_MAP_READ)) {
      GST_ELEMENT_ERROR(filter, RESOURCE, NOT_FOUND, ("failed to map videoframe"),
        ("failed to map videoframe for original buffer"));
      exit(1);
    }

    guint8 * y_pixels_O = GST_VIDEO_FRAME_PLANE_DATA( & vframe_org_buffer, 0); // Y plane->starting of pixel for original frame
    guint8 * uv_pixels_O = GST_VIDEO_FRAME_PLANE_DATA( & vframe_org_buffer, 1); // UV plane

    guint y_stride_O = GST_VIDEO_FRAME_PLANE_STRIDE( & vframe_org_buffer, 0); // Y plane stride
    guint uv_stride_O = GST_VIDEO_FRAME_PLANE_STRIDE( & vframe_org_buffer, 1); // UV plane stride

    guint y_O_pixel_stride = GST_VIDEO_FRAME_COMP_PSTRIDE( & vframe_org_buffer, 0);
    guint uv_O_pixel_stride = GST_VIDEO_FRAME_COMP_PSTRIDE( & vframe_org_buffer, 1);

    gint width1, height1, start_x, start_y, bottom, right;
    width1 = new_buf_width;
    height1 = new_buf_height;
    start_x = filter->start_col;    
    start_y = filter->start_row;
    bottom = start_y + height1;
    right = start_x + width1;

    guint8 * y_new, * uv_new, * y_org, * uv_org;
    gint start = 0, end = 0, start1 = 0, end1 = 0;

    for (start = 0, start1 = start_y; start < height1 && start1 < bottom; start++, start1++) {
      for (end = 0, end1 = start_x; end < width1 && end1 < right; end++, end1++) {

        y_new = y_pixels_N + start * y_stride_N + end * y_N_pixel_stride; //starting pixel of new buffer
        uv_new = uv_pixels_N + start / 2 * uv_stride_N + (end / 2) * uv_N_pixel_stride;

        y_org = y_pixels_O + start1 * y_stride_O + end1 * y_O_pixel_stride; //starting pixel of original buffer
        uv_org = uv_pixels_O + start1 / 2 * uv_stride_O + (end1 / 2) * uv_O_pixel_stride;

        if (end1 >= start_x && end1 < right && start1 >= start_y && start1 < bottom) { //data transfer
          y_new[0] = y_org[0];
          uv_new[0] = uv_org[0];
          uv_new[1] = uv_org[1];
        }
      }
    }

    GstVideoInfo video_info_scale;
    GstVideoFrame vframe_scale;

    gsize s_size = (filter -> scalesize[0]) * (filter -> scalesize[1]) * (1.5);

    /* creating scaling buffer*/
    scalebuf = gst_buffer_new_allocate(NULL, s_size, NULL);

    if (!gst_buffer_make_writable(scalebuf)) {
      GST_ELEMENT_ERROR(filter, RESOURCE, NOT_FOUND, ("Failed to make the buffer writable"),
        ("Failed to make the  scale buffer writable"));
      exit(1);
    }

    if (!gst_video_info_from_caps( & video_info_scale, incomingcaps)) {
      GST_ELEMENT_ERROR(filter, RESOURCE, NOT_FOUND, ("fail to get videoinfo"),
        ("fail to get videoinfo for scale buffer"));
      exit(1);
    }

    if (!gst_video_frame_map( & vframe_scale, & video_info_scale, scalebuf, GST_MAP_WRITE)) {
      GST_ELEMENT_ERROR(filter, RESOURCE, NOT_FOUND, ("failed to map videoframe"),
        ("failed to map videoframe for scaling buffer"));
      exit(1);

    }

    guint8 * y_pixels_scale = GST_VIDEO_FRAME_PLANE_DATA( & vframe_scale, 0); // Y plane
    guint8 * uv_pixels_scale = GST_VIDEO_FRAME_PLANE_DATA( & vframe_scale, 1); // UV plane (interleaved)

    guint y_stride_scale = GST_VIDEO_FRAME_PLANE_STRIDE( & vframe_scale, 0); // Y plane stride
    guint uv_stride_scale = GST_VIDEO_FRAME_PLANE_STRIDE( & vframe_scale, 1); // UV plane stride

    guint y_pixel_stride_scale = GST_VIDEO_FRAME_COMP_PSTRIDE( & vframe_scale, 0);
    guint uv_pixel_stride_scale = GST_VIDEO_FRAME_COMP_PSTRIDE( & vframe_scale, 1);

    for (gint start_h = 0; start_h < filter -> scalesize[1]; start_h++) {
      for (gint start_w = 0; start_w < filter -> scalesize[0]; start_w++) {
        //this is nearest neighbour algorithm
        gint new_x = (start_w * width1) / filter -> scalesize[0];
        gint new_y = (start_h * height1) / filter -> scalesize[1];
	
        guint8 * y_new = y_pixels_N + new_y * y_stride_N + new_x * y_N_pixel_stride;
        guint8 * uv_new = uv_pixels_N + (new_y / 2) * uv_stride_N + (new_x / 2) * uv_N_pixel_stride;

        guint8 * y_org = y_pixels_scale + start_h * y_stride_scale + start_w * y_pixel_stride_scale;
        guint8 * uv_org = uv_pixels_scale + (start_h / 2) * uv_stride_scale + (start_w / 2) * uv_pixel_stride_scale;

        y_org[0] = y_new[0];
        uv_org[0] = uv_new[0];
        uv_org[1] = uv_new[1];
      }
    }

    gst_pad_set_caps(filter -> srcpad, incomingcaps);
    gst_video_frame_unmap( & vframe_new_buffer);
    gst_video_frame_unmap( & vframe_scale);
    break;
  }

  case RECTANGLE: {
    
    gst_crop_portion_dimensions (filter, &new_buf_width, &new_buf_height);
          
    if (filter -> scalesize[0] == 0 && filter -> scalesize[1] == 0) { //if user didn't give scale then new buffer is sent as it is otherwise set scale size
      filter -> scalesize[0] = new_buf_width;
      filter -> scalesize[1] = new_buf_height;
    }
    
    if (filter -> scale_default) {
      if (filter -> scalesize[0] > 3840 || filter -> scalesize[1] > 2160 || filter -> scalesize[0] < 0 || filter -> scalesize[1] < 0) {
        if (filter -> strict) {
          GST_ELEMENT_ERROR(filter, RESOURCE, NOT_FOUND, ("Invalid value for scale-size property."),
            ("Maximum resolution is 4k."));
          exit(1);
        } else {
          g_warning("Invalid value (negative or resolution higher than 4k) '<%d, %d>' for scale-size property. Valid values are positive integer numbers or inside 4k video resolution.", filter -> scalesize[0], filter -> scalesize[1]);

          filter -> scalesize[0] = new_buf_width;
          filter -> scalesize[1] = new_buf_height;				
          g_print("scale-size set to : <%d, %d> \n", filter -> scalesize[0], filter -> scalesize[1]);
          filter -> scale_default = FALSE;
        }
      }
    }
    gst_structure_set(structure, "width", G_TYPE_INT, filter -> scalesize[0], "height", G_TYPE_INT, filter -> scalesize[1], "format", G_TYPE_STRING, "NV12", NULL);

    caps = gst_caps_new_simple("video/x-raw",
      "format", G_TYPE_STRING, "NV12",
      "width", G_TYPE_INT, new_buf_width,
      "height", G_TYPE_INT, new_buf_height,
      "bpp", G_TYPE_INT, 12,
      NULL);
    
    size = new_buf_width * new_buf_height * (1.5);
    buffer_new = gst_buffer_new_allocate(NULL, size, NULL);

    if (!gst_buffer_make_writable(buffer_new)) {
      GST_ELEMENT_ERROR(filter, RESOURCE, NOT_FOUND, ("Failed to make the buffer writable"),
        ("Failed to make the  new buffer writable"));
      exit(1);
    }

    if (!gst_video_info_from_caps( & video_info_new_buffer, caps)) {
      GST_ELEMENT_ERROR(filter, RESOURCE, NOT_FOUND, ("fail to get videoinfo"),
        ("fail to get videoinfo for new buffer"));
      exit(1);
    }

    if (!gst_video_frame_map( & vframe_new_buffer, & video_info_new_buffer, buffer_new, GST_MAP_WRITE)) {
      GST_ELEMENT_ERROR(filter, RESOURCE, NOT_FOUND, ("failed to map videoframe"),
        ("failed to map videoframe of new buffer"));
     	exit(1);
    }

    guint8 * y_pixels_N = GST_VIDEO_FRAME_PLANE_DATA( & vframe_new_buffer, 0); // Y plane-Starting address of pixel
    guint8 * uv_pixels_N = GST_VIDEO_FRAME_PLANE_DATA( & vframe_new_buffer, 1); // UV plane

    guint y_stride_N = GST_VIDEO_FRAME_PLANE_STRIDE( & vframe_new_buffer, 0); // Y plane stride-Xaxis
    guint uv_stride_N = GST_VIDEO_FRAME_PLANE_STRIDE( & vframe_new_buffer, 1); // UV plane stride

    guint y_N_pixel_stride = GST_VIDEO_FRAME_COMP_PSTRIDE( & vframe_new_buffer, 0);
    guint uv_N_pixel_stride = GST_VIDEO_FRAME_COMP_PSTRIDE( & vframe_new_buffer, 1);

    GstVideoInfo video_info_org_buffer;
    GstVideoFrame vframe_org_buffer;

    if (!gst_video_info_from_caps( & video_info_org_buffer, gst_pad_get_current_caps(pad))) {
      GST_ELEMENT_ERROR(filter, RESOURCE, NOT_FOUND, ("fail to get videoinfo"),
        ("fail to get videoinfo for original buffer"));
      exit(1);
    }

    if (!gst_video_frame_map( & vframe_org_buffer, & video_info_org_buffer, buf, GST_MAP_READ)) {
      GST_ELEMENT_ERROR(filter, RESOURCE, NOT_FOUND, ("failed to map videoframe"),
        ("failed to map videoframe for original buffer"));
      exit(1);
    }

    guint8 * y_pixels_O = GST_VIDEO_FRAME_PLANE_DATA( & vframe_org_buffer, 0); // Y plane->starting of pixel for original frame
    guint8 * uv_pixels_O = GST_VIDEO_FRAME_PLANE_DATA( & vframe_org_buffer, 1); // UV plane

    guint y_stride_O = GST_VIDEO_FRAME_PLANE_STRIDE( & vframe_org_buffer, 0); // Y plane stride
    guint uv_stride_O = GST_VIDEO_FRAME_PLANE_STRIDE( & vframe_org_buffer, 1); // UV plane stride

    guint y_O_pixel_stride = GST_VIDEO_FRAME_COMP_PSTRIDE( & vframe_org_buffer, 0);
    guint uv_O_pixel_stride = GST_VIDEO_FRAME_COMP_PSTRIDE( & vframe_org_buffer, 1);

    gint width1, height1, start_x, start_y, bottom, right;
    width1 = new_buf_width;
    height1 = new_buf_height;
    start_x = filter->start_col;    
    start_y = filter->start_row;
    bottom = start_y + height1;
    right = start_x + width1;

    guint8 * y_new, * uv_new, * y_org, * uv_org;
    gint start = 0, end = 0, start1 = 0, end1 = 0;

    for (start = 0, start1 = start_y; start < height1 && start1 < bottom; start++, start1++) {
      for (end = 0, end1 = start_x; end < width1 && end1 < right; end++, end1++) {

        y_new = y_pixels_N + start * y_stride_N + end * y_N_pixel_stride; //starting pixel of new buffer
        uv_new = uv_pixels_N + start / 2 * uv_stride_N + (end / 2) * uv_N_pixel_stride;

        y_org = y_pixels_O + start1 * y_stride_O + end1 * y_O_pixel_stride; //starting pixel of original buffer
        uv_org = uv_pixels_O + start1 / 2 * uv_stride_O + (end1 / 2) * uv_O_pixel_stride;

        if (end1 >= start_x && end1 < right && start1 >= start_y && start1 < bottom) { //data transfer
          y_new[0] = y_org[0];
          uv_new[0] = uv_org[0];
          uv_new[1] = uv_org[1];
        }
      }
    }

    GstVideoInfo video_info_scale;
    GstVideoFrame vframe_scale;

    gsize s_size = (filter -> scalesize[0]) * (filter -> scalesize[1]) * (1.5);

    /* creating scaling buffer*/
    scalebuf = gst_buffer_new_allocate(NULL, s_size, NULL);

    if (!gst_buffer_make_writable(scalebuf)) {
      GST_ELEMENT_ERROR(filter, RESOURCE, NOT_FOUND, ("Failed to make the buffer writable"),
        ("Failed to make the  scale buffer writable"));
      exit(1);
    }

    if (!gst_video_info_from_caps( & video_info_scale, incomingcaps)) {
      GST_ELEMENT_ERROR(filter, RESOURCE, NOT_FOUND, ("fail to get videoinfo"),
        ("fail to get videoinfo for scale buffer"));
      exit(1);
    }

    if (!gst_video_frame_map( & vframe_scale, & video_info_scale, scalebuf, GST_MAP_WRITE)) {
      GST_ELEMENT_ERROR(filter, RESOURCE, NOT_FOUND, ("failed to map videoframe"),
        ("failed to map videoframe for scaling buffer"));
      exit(1);
    }

    guint8 * y_pixels_scale = GST_VIDEO_FRAME_PLANE_DATA( & vframe_scale, 0); // Y plane
    guint8 * uv_pixels_scale = GST_VIDEO_FRAME_PLANE_DATA( & vframe_scale, 1); // UV plane (interleaved)

    guint y_stride_scale = GST_VIDEO_FRAME_PLANE_STRIDE( & vframe_scale, 0); // Y plane stride
    guint uv_stride_scale = GST_VIDEO_FRAME_PLANE_STRIDE( & vframe_scale, 1); // UV plane stride

    guint y_pixel_stride_scale = GST_VIDEO_FRAME_COMP_PSTRIDE( & vframe_scale, 0);
    guint uv_pixel_stride_scale = GST_VIDEO_FRAME_COMP_PSTRIDE( & vframe_scale, 1);

    for (gint start_h = 0; start_h < filter -> scalesize[1]; start_h++) {
      for (gint start_w = 0; start_w < filter -> scalesize[0]; start_w++) {
        //this is nearest neighbour algorithm
        gint new_x = (start_w * width1) / filter -> scalesize[0];
        gint new_y = (start_h * height1) / filter -> scalesize[1];
	
        guint8 * y_new = y_pixels_N + new_y * y_stride_N + new_x * y_N_pixel_stride;
        guint8 * uv_new = uv_pixels_N + (new_y / 2) * uv_stride_N + (new_x / 2) * uv_N_pixel_stride;

        guint8 * y_org = y_pixels_scale + start_h * y_stride_scale + start_w * y_pixel_stride_scale;
        guint8 * uv_org = uv_pixels_scale + (start_h / 2) * uv_stride_scale + (start_w / 2) * uv_pixel_stride_scale;

        y_org[0] = y_new[0];
        uv_org[0] = uv_new[0];
        uv_org[1] = uv_new[1];
      }
    }

    gst_pad_set_caps(filter -> srcpad, incomingcaps);
    gst_video_frame_unmap( & vframe_new_buffer);
    gst_video_frame_unmap( & vframe_scale);
    
    break;
  }

  case TRIANGLE: {
  
   gst_crop_portion_dimensions (filter, &new_buf_width, &new_buf_height);
         
    if (filter -> scalesize[0] == 0 && filter -> scalesize[1] == 0) { //if user didn't give scale then new buffer is sent as it is otherwise set scale size
      filter -> scalesize[0] = new_buf_width;
      filter -> scalesize[1] = new_buf_height;
    }
    
    if (filter -> scale_default) {
      if (filter -> scalesize[0] > 3840 || filter -> scalesize[1] > 2160 || filter -> scalesize[0] < 0 || filter -> scalesize[1] < 0) {
        if (filter -> strict) {
          GST_ELEMENT_ERROR(filter, RESOURCE, NOT_FOUND, ("Invalid value for scale-size property."),
            ("Maximum resolution is 4k."));
          exit(1);
        } else {
          g_warning("Invalid value (negative or resolution higher than 4k) '<%d, %d>' for scale-size property. Valid values are positive integer numbers or inside 4k video resolution.", filter -> scalesize[0], filter -> scalesize[1]);

          filter -> scalesize[0] = new_buf_width;
          filter -> scalesize[1] = new_buf_height;				
          g_print("scale-size set to : <%d, %d> \n", filter -> scalesize[0], filter -> scalesize[1]);
          filter -> scale_default = FALSE;
        }
      }
    }
    gst_structure_set(structure, "width", G_TYPE_INT, filter -> scalesize[0], "height", G_TYPE_INT, filter -> scalesize[1], "format", G_TYPE_STRING, "NV12", NULL);

    caps = gst_caps_new_simple("video/x-raw",
      "format", G_TYPE_STRING, "NV12",
      "width", G_TYPE_INT, new_buf_width,
      "height", G_TYPE_INT, new_buf_height,
      "bpp", G_TYPE_INT, 12,
      NULL);
    
    size = new_buf_width * new_buf_height * (1.5);
    buffer_new = gst_buffer_new_allocate(NULL, size, NULL);

    if (!gst_buffer_make_writable(buffer_new)) {
      GST_ELEMENT_ERROR(filter, RESOURCE, NOT_FOUND, ("Failed to make the buffer writable"),
        ("Failed to make the  new buffer writable"));
      exit(1);
    }

    if (!gst_video_info_from_caps( & video_info_new_buffer, caps)) {
      GST_ELEMENT_ERROR(filter, RESOURCE, NOT_FOUND, ("fail to get videoinfo"),
        ("fail to get videoinfo for new buffer"));
      exit(1);
    }

    if (!gst_video_frame_map( & vframe_new_buffer, & video_info_new_buffer, buffer_new, GST_MAP_WRITE)) {
      GST_ELEMENT_ERROR(filter, RESOURCE, NOT_FOUND, ("failed to map videoframe"),
        ("failed to map videoframe of new buffer"));
     	exit(1);
    }

    guint8 * y_pixels_N = GST_VIDEO_FRAME_PLANE_DATA( & vframe_new_buffer, 0); // Y plane-Starting address of pixel
    guint8 * uv_pixels_N = GST_VIDEO_FRAME_PLANE_DATA( & vframe_new_buffer, 1); // UV plane

    guint y_stride_N = GST_VIDEO_FRAME_PLANE_STRIDE( & vframe_new_buffer, 0); // Y plane stride-Xaxis
    guint uv_stride_N = GST_VIDEO_FRAME_PLANE_STRIDE( & vframe_new_buffer, 1); // UV plane stride

    guint y_N_pixel_stride = GST_VIDEO_FRAME_COMP_PSTRIDE( & vframe_new_buffer, 0);
    guint uv_N_pixel_stride = GST_VIDEO_FRAME_COMP_PSTRIDE( & vframe_new_buffer, 1);

    GstVideoInfo video_info_org_buffer;
    GstVideoFrame vframe_org_buffer;

    if (!gst_video_info_from_caps( & video_info_org_buffer, gst_pad_get_current_caps(pad))) {
      GST_ELEMENT_ERROR(filter, RESOURCE, NOT_FOUND, ("fail to get videoinfo"),
        ("fail to get videoinfo for original buffer"));
      exit(1);
    }

    if (!gst_video_frame_map( & vframe_org_buffer, & video_info_org_buffer, buf, GST_MAP_READ)) {
      GST_ELEMENT_ERROR(filter, RESOURCE, NOT_FOUND, ("failed to map videoframe"),
        ("failed to map videoframe for original buffer"));
      exit(1);
    }

    guint8 * y_pixels_O = GST_VIDEO_FRAME_PLANE_DATA( & vframe_org_buffer, 0); // Y plane->starting of pixel for original frame
    guint8 * uv_pixels_O = GST_VIDEO_FRAME_PLANE_DATA( & vframe_org_buffer, 1); // UV plane

    guint y_stride_O = GST_VIDEO_FRAME_PLANE_STRIDE( & vframe_org_buffer, 0); // Y plane stride
    guint uv_stride_O = GST_VIDEO_FRAME_PLANE_STRIDE( & vframe_org_buffer, 1); // UV plane stride

    guint y_O_pixel_stride = GST_VIDEO_FRAME_COMP_PSTRIDE( & vframe_org_buffer, 0);
    guint uv_O_pixel_stride = GST_VIDEO_FRAME_COMP_PSTRIDE( & vframe_org_buffer, 1);
    
    gint width1, height1, start_x, start_y, bottom, right;
    width1 = new_buf_width ;
    height1 = new_buf_height;
    start_x = filter->start_col;    
    start_y =  filter->start_row;

    bottom = start_y + (height1);
    right = start_x + (width1);

    guint8 * y_new, * uv_new, * y_org, * uv_org;
    gint start = 0, end = 0, start1 = 0, end1 = 0;

    double m1, m2, m3, c1, c2, c3, line_AB, line_BC, line_AC;
	   m1 = tan (M_PI / 3);
	   m2 = tan ((2 * M_PI) / 3);
	   m3 = tan (0);
	   
	   c1 = (filter->coordinate[1] - filter->Size) - (m1 * filter->coordinate[0] );
	   c2 = (filter->coordinate[1] - filter->Size) - (m2 * filter->coordinate[0] );
	   c3 = filter->coordinate[1]  + (filter->Size / 2);
    
   for (start = 0, start1 = start_y; start < height1 && start1 < bottom; start++, start1++) {
      for (end = 0, end1 = start_x; end < width1 && end1 < right; end++, end1++) {
      
        line_AB = (m1 * end1) + c1;
				line_AC = (m2 * end1) + c2;
				line_BC = (m3 * end1) + c3;

        y_new = y_pixels_N + start * y_stride_N + end * y_N_pixel_stride; //starting pixel of new buffer
        uv_new = uv_pixels_N + start / 2 * uv_stride_N + (end / 2) * uv_N_pixel_stride;

        y_org = y_pixels_O + start1 * y_stride_O + end1 * y_O_pixel_stride; //starting pixel of original buffer
        uv_org = uv_pixels_O + start1 / 2 * uv_stride_O + (end1 / 2) * uv_O_pixel_stride;
                
        if ((start1 >= line_AB && start1 >= line_AC && start1 <= line_BC)) { //data transfer
          y_new[0] = y_org[0];
          uv_new[0] = uv_org[0];
          uv_new[1] = uv_org[1];
        }
        else {
		      if (strcmp(filter -> fillcolour, "red") == 0) { //fill colour outside the shape
		      
		          y_new[0] = 76;
		          uv_new[0] = 84;
		          uv_new[1] = 255;
		        }
		        /* pixel values are set to green. */
		        else if (strcmp(filter -> fillcolour, "green") == 0) {
		        
		          y_new[0] = 149;
		          uv_new[0] = 43;
		          uv_new[1] = 21;
		        }
		        /* pixel values are set to blue. */
		        else if (strcmp(filter -> fillcolour, "blue") == 0) {
		        
		          y_new[0] = 29;
		          uv_new[0] = 255;
		          uv_new[1] = 107;
		          
		        } else if (strcmp(filter -> fillcolour, "white") == 0) {
		        
		          /* Outside the circle, set pixel to white */
		          y_new[0] = 255; // Y component for white color
		          uv_new[0] = 128; // U component for white color
		          uv_new[1] = 128; // V component for white color
		          
		        } else if (strcmp(filter -> fillcolour, "black") == 0) {
		        
		          /* Outside the circle, set pixel to white */
		          y_new[0] = 0; // Y component for white color
		          uv_new[0] = 128; // U component for white color
		          uv_new[1] = 128; // V component for white color
		          
		        } else {

				        if (filter -> strict) {
				          GST_ELEMENT_ERROR(filter, RESOURCE, NOT_FOUND, ("Invalid colour for fill property."),
				            ("Enter valid fill colour."));
				          exit(1);
			        } else {
			          g_warning("Invalid fill colour give valid colour red,black,green,white and blue\n");
			          filter -> fillcolour = "white";
			          g_print("fill colour set to : <%s> \n", filter -> fillcolour);
	          	}
        		}
      		}
    		}
			}
			
    GstVideoInfo video_info_scale;
    GstVideoFrame vframe_scale;

    gsize s_size = (filter -> scalesize[0]) * (filter -> scalesize[1]) * (1.5);

    /* creating scaling buffer*/
    scalebuf = gst_buffer_new_allocate(NULL, s_size, NULL);

    if (!gst_buffer_make_writable(scalebuf)) {
      GST_ELEMENT_ERROR(filter, RESOURCE, NOT_FOUND, ("Failed to make the buffer writable"),
        ("Failed to make the  scale buffer writable"));
      exit(1);
    }

    if (!gst_video_info_from_caps( & video_info_scale, incomingcaps)) {
      GST_ELEMENT_ERROR(filter, RESOURCE, NOT_FOUND, ("fail to get videoinfo"),
        ("fail to get videoinfo for scale buffer"));
      exit(1);
    }

    if (!gst_video_frame_map( & vframe_scale, & video_info_scale, scalebuf, GST_MAP_WRITE)) {
      GST_ELEMENT_ERROR(filter, RESOURCE, NOT_FOUND, ("failed to map videoframe"),
        ("failed to map videoframe for scaling buffer"));
      exit(1);

    }

    guint8 * y_pixels_scale = GST_VIDEO_FRAME_PLANE_DATA( & vframe_scale, 0); // Y plane
    guint8 * uv_pixels_scale = GST_VIDEO_FRAME_PLANE_DATA( & vframe_scale, 1); // UV plane (interleaved)

    guint y_stride_scale = GST_VIDEO_FRAME_PLANE_STRIDE( & vframe_scale, 0); // Y plane stride
    guint uv_stride_scale = GST_VIDEO_FRAME_PLANE_STRIDE( & vframe_scale, 1); // UV plane stride

    guint y_pixel_stride_scale = GST_VIDEO_FRAME_COMP_PSTRIDE( & vframe_scale, 0);
    guint uv_pixel_stride_scale = GST_VIDEO_FRAME_COMP_PSTRIDE( & vframe_scale, 1);

    for (gint start_h = 0; start_h < filter -> scalesize[1]; start_h++) {
      for (gint start_w = 0; start_w < filter -> scalesize[0]; start_w++) {
        //this is nearest neighbour algorithm
        gint new_x = (start_w * width1) / filter -> scalesize[0];
        gint new_y = (start_h * height1) / filter -> scalesize[1];
	
        guint8 * y_new = y_pixels_N + new_y * y_stride_N + new_x * y_N_pixel_stride;
        guint8 * uv_new = uv_pixels_N + (new_y / 2) * uv_stride_N + (new_x / 2) * uv_N_pixel_stride;

        guint8 * y_org = y_pixels_scale + start_h * y_stride_scale + start_w * y_pixel_stride_scale;
        guint8 * uv_org = uv_pixels_scale + (start_h / 2) * uv_stride_scale + (start_w / 2) * uv_pixel_stride_scale;

        y_org[0] = y_new[0];
        uv_org[0] = uv_new[0];
        uv_org[1] = uv_new[1];
      }
    }

    gst_pad_set_caps(filter -> srcpad, incomingcaps);
    gst_video_frame_unmap( & vframe_new_buffer);
    gst_video_frame_unmap( & vframe_scale);
    break;
  }
 

  case CIRCLE: {

    gst_crop_portion_dimensions (filter, &new_buf_width, &new_buf_height);    
      
    if (filter -> scalesize[0] == 0 && filter -> scalesize[1] == 0) { //if user didn't give scale then new buffer is sent as it is otherwise set scale size
      filter -> scalesize[0] = new_buf_width;
      filter -> scalesize[1] = new_buf_height;
    }
    
    if (filter -> scale_default) {
      if (filter -> scalesize[0] > 3840 || filter -> scalesize[1] > 2160 || filter -> scalesize[0] < 0 || filter -> scalesize[1] < 0) {
        if (filter -> strict) {
          GST_ELEMENT_ERROR(filter, RESOURCE, NOT_FOUND, ("Invalid value for scale-size property."),
            ("Maximum resolution is 4k."));
          exit(1);
        } else {
          g_warning("Invalid value (negative or resolution higher than 4k) '<%d, %d>' for scale-size property. Valid values are positive integer numbers or inside 4k video resolution.", filter -> scalesize[0], filter -> scalesize[1]);

          filter -> scalesize[0] = new_buf_width;
          filter -> scalesize[1] = new_buf_height;				
          g_print("scale-size set to : <%d, %d> \n", filter -> scalesize[0], filter -> scalesize[1]);
          filter -> scale_default = FALSE;
        }
      }
    }
    gst_structure_set(structure, "width", G_TYPE_INT, filter -> scalesize[0], "height", G_TYPE_INT, filter -> scalesize[1], "format", G_TYPE_STRING, "NV12", NULL);

    caps = gst_caps_new_simple("video/x-raw",
      "format", G_TYPE_STRING, "NV12",
      "width", G_TYPE_INT, new_buf_width,
      "height", G_TYPE_INT, new_buf_height,
      "bpp", G_TYPE_INT, 12,
      NULL);
    
    size = new_buf_width * new_buf_height * (1.5);
    buffer_new = gst_buffer_new_allocate(NULL, size, NULL);

    if (!gst_buffer_make_writable(buffer_new)) {
      GST_ELEMENT_ERROR(filter, RESOURCE, NOT_FOUND, ("Failed to make the buffer writable"),
        ("Failed to make the  new buffer writable"));
      exit(1);
    }
		
    if (!gst_video_info_from_caps( & video_info_new_buffer, caps)) {
      GST_ELEMENT_ERROR(filter, RESOURCE, NOT_FOUND, ("fail to get videoinfo"),
        ("fail to get videoinfo for new buffer"));
      exit(1);
    }

    if (!gst_video_frame_map( & vframe_new_buffer, & video_info_new_buffer, buffer_new, GST_MAP_WRITE)) {
      GST_ELEMENT_ERROR(filter, RESOURCE, NOT_FOUND, ("failed to map videoframe"),
        ("failed to map videoframe of new buffer"));
     	exit(1);
    }

    guint8 * y_pixels_N = GST_VIDEO_FRAME_PLANE_DATA( & vframe_new_buffer, 0); // Y plane-Starting address of pixel
    guint8 * uv_pixels_N = GST_VIDEO_FRAME_PLANE_DATA( & vframe_new_buffer, 1); // UV plane

    guint y_stride_N = GST_VIDEO_FRAME_PLANE_STRIDE( & vframe_new_buffer, 0); // Y plane stride-Xaxis
    guint uv_stride_N = GST_VIDEO_FRAME_PLANE_STRIDE( & vframe_new_buffer, 1); // UV plane stride

    guint y_N_pixel_stride = GST_VIDEO_FRAME_COMP_PSTRIDE( & vframe_new_buffer, 0);
    guint uv_N_pixel_stride = GST_VIDEO_FRAME_COMP_PSTRIDE( & vframe_new_buffer, 1);

    GstVideoInfo video_info_org_buffer;
    GstVideoFrame vframe_org_buffer;

    if (!gst_video_info_from_caps( & video_info_org_buffer, gst_pad_get_current_caps(pad))) {
      GST_ELEMENT_ERROR(filter, RESOURCE, NOT_FOUND, ("fail to get videoinfo"),
        ("fail to get videoinfo for original buffer"));
      exit(1);
    }

    if (!gst_video_frame_map( & vframe_org_buffer, & video_info_org_buffer, buf, GST_MAP_READ)) {
      GST_ELEMENT_ERROR(filter, RESOURCE, NOT_FOUND, ("failed to map videoframe"),
        ("failed to map videoframe for original buffer"));
      exit(1);
    }

    guint8 * y_pixels_O = GST_VIDEO_FRAME_PLANE_DATA( & vframe_org_buffer, 0); // Y plane->starting of pixel for original frame
    guint8 * uv_pixels_O = GST_VIDEO_FRAME_PLANE_DATA( & vframe_org_buffer, 1); // UV plane

    guint y_stride_O = GST_VIDEO_FRAME_PLANE_STRIDE( & vframe_org_buffer, 0); // Y plane stride
    guint uv_stride_O = GST_VIDEO_FRAME_PLANE_STRIDE( & vframe_org_buffer, 1); // UV plane stride

    guint y_O_pixel_stride = GST_VIDEO_FRAME_COMP_PSTRIDE( & vframe_org_buffer, 0);
    guint uv_O_pixel_stride = GST_VIDEO_FRAME_COMP_PSTRIDE( & vframe_org_buffer, 1);
 
    gint width1, height1, start_x, start_y, bottom, right;
    width1 = new_buf_width;
    height1 = new_buf_height;
    start_x = filter->start_col;    
    start_y = filter->start_row;
    bottom = start_y + height1;
    right = start_x + width1;
    
    guint8 * y_new, * uv_new, * y_org, * uv_org;
    gint start = 0, end = 0, start1 = 0, end1 = 0;

    for (start = 0, start1 = start_y; start < height1 && start1 < bottom; start++, start1++) {
      for (end = 0, end1 = start_x; end < width1 && end1 < right; end++, end1++) {

        double distance = (end1 - filter -> circle_center_x) * (end1 - filter -> circle_center_x) + (start1 - filter -> circle_center_y) * (start1 - filter -> circle_center_y);

        y_new = y_pixels_N + start * y_stride_N + end * y_N_pixel_stride; //starting pixel of new buffer
        uv_new = uv_pixels_N + start / 2 * uv_stride_N + (end / 2) * uv_N_pixel_stride;

        y_org = y_pixels_O + start1 * y_stride_O + end1 * y_O_pixel_stride; //starting pixel of original buffer
        uv_org = uv_pixels_O + start1 / 2 * uv_stride_O + (end1 / 2) * uv_O_pixel_stride;

        if (distance <= filter -> circle_radius * filter -> circle_radius) { // data transferring
          y_new[0] = y_org[0];
          uv_new[0] = uv_org[0];
          uv_new[1] = uv_org[1];
        } else {

          if (strcmp(filter -> fillcolour, "red") == 0) {	//fill colour outside the shape
            y_new[0] = 76;
            uv_new[0] = 84;
            uv_new[1] = 255;
          }
          /* pixel values are set to green. */
          else if (strcmp(filter -> fillcolour, "green") == 0) {
            y_new[0] = 149;
            uv_new[0] = 43;
            uv_new[1] = 21;
          }
          /* pixel values are set to blue. */
          else if (strcmp(filter -> fillcolour, "blue") == 0) {
            y_new[0] = 29;
            uv_new[0] = 255;
            uv_new[1] = 107;
          } else if (strcmp(filter -> fillcolour, "white") == 0) {
            /* Outside the circle, set pixel to white */
            y_new[0] = 255; // Y component for white color
            uv_new[0] = 128; // U component for white color
            uv_new[1] = 128; // V component for white color
          } else if (strcmp(filter -> fillcolour, "black") == 0) {
            /* Outside the circle, set pixel to white */
            y_new[0] = 0; // Y component for white color
            uv_new[0] = 128; // U component for white color
            uv_new[1] = 128; // V component for white color
          } else {

            if (filter -> strict) {
              GST_ELEMENT_ERROR(filter, RESOURCE, NOT_FOUND, ("Invalid colour for fill property."),
                ("Enter valid fill colour."));
              exit(1);
            } else {
              g_warning("Invalid fill colour give valid colour red,black,green,white and blue\n");
              filter -> fillcolour = "white";
              g_print("fill colour set to : <%s> \n", filter -> fillcolour);

            }
          }
        }
      }
    }

    GstVideoInfo video_info_scale;
    GstVideoFrame vframe_scale;

    gsize s_size = (filter -> scalesize[0]) * (filter -> scalesize[1]) * (1.5);

    /* creating scaling buffer*/

    scalebuf = gst_buffer_new_allocate(NULL, s_size, NULL);

    if (!gst_buffer_make_writable(scalebuf)) {
      GST_ELEMENT_ERROR(filter, RESOURCE, NOT_FOUND, ("Failed to make the buffer writable"),
        ("Failed to make the  scale buffer writable"));
      exit(1);
    }

    if (!gst_video_info_from_caps( & video_info_scale, incomingcaps)) {
      GST_ELEMENT_ERROR(filter, RESOURCE, NOT_FOUND, ("fail to get videoinfo"),
        ("fail to get videoinfo for scale buffer"));
      exit(1);
    }

    if (!gst_video_frame_map( & vframe_scale, & video_info_scale, scalebuf, GST_MAP_WRITE)) {
      GST_ELEMENT_ERROR(filter, RESOURCE, NOT_FOUND, ("failed to map videoframe"),
        ("failed to map videoframe for scaling buffer"));
      exit(1);

    }

    guint8 * y_pixels_scale = GST_VIDEO_FRAME_PLANE_DATA( & vframe_scale, 0); // Y plane
    guint8 * uv_pixels_scale = GST_VIDEO_FRAME_PLANE_DATA( & vframe_scale, 1); // UV plane (interleaved)

    guint y_stride_scale = GST_VIDEO_FRAME_PLANE_STRIDE( & vframe_scale, 0); // Y plane stride
    guint uv_stride_scale = GST_VIDEO_FRAME_PLANE_STRIDE( & vframe_scale, 1); // UV plane stride

    guint y_pixel_stride_scale = GST_VIDEO_FRAME_COMP_PSTRIDE( & vframe_scale, 0);
    guint uv_pixel_stride_scale = GST_VIDEO_FRAME_COMP_PSTRIDE( & vframe_scale, 1);

    for (gint start_h = 0; start_h < filter -> scalesize[1]; start_h++) {
      for (gint start_w = 0; start_w < filter -> scalesize[0]; start_w++) {
        //this is nearest neighbour algorithm
        gint new_x = (start_w * width1) / filter -> scalesize[0];
        gint new_y = (start_h * height1) / filter -> scalesize[1];
	
        guint8 * y_new = y_pixels_N + new_y * y_stride_N + new_x * y_N_pixel_stride;
        guint8 * uv_new = uv_pixels_N + (new_y / 2) * uv_stride_N + (new_x / 2) * uv_N_pixel_stride;

        guint8 * y_org = y_pixels_scale + start_h * y_stride_scale + start_w * y_pixel_stride_scale;
        guint8 * uv_org = uv_pixels_scale + (start_h / 2) * uv_stride_scale + (start_w / 2) * uv_pixel_stride_scale;

        y_org[0] = y_new[0];
        uv_org[0] = uv_new[0];
        uv_org[1] = uv_new[1];
      }
    }

    gst_pad_set_caps(filter -> srcpad, incomingcaps);
    gst_video_frame_unmap( & vframe_new_buffer);
    gst_video_frame_unmap( & vframe_scale);
    break;
  }

  default:
    g_print("choose valid shape\n");
    break;
  }


  return gst_pad_push(filter -> srcpad, scalebuf);

}

void gst_crop_portion_dimensions (GstCropPortion *filt, int *wid, int *hgt) //calculate dimension of cropped shape
{
	int x_coord, y_coord;

	int strict_mode = filt->strict;

	x_coord = filt->coordinate[0];
	y_coord = filt->coordinate[1];

	*hgt = filt->Size;
	*wid = *hgt;

	if (filt->Shape_Type == SQUARE ) { //wid and hgt of sqaure and circle is same
		*hgt = 2 * (*hgt); 	//hgt is considered from center
		*wid = *hgt;
	}

	if (filt->Shape_Type == CIRCLE) { //wid and hgt of sqaure and circle is same
		*hgt = 2 * (*hgt); 	//hgt is considered from center

		*wid = *hgt;
	}


	if (filt->Shape_Type == RECTANGLE) {
		*wid = 4 * (*hgt);
		*hgt = 2 * (*hgt);
	}

	if (filt->Shape_Type == TRIANGLE) {//for triangle hgt is given from centroid in 2:1 ratio
		*hgt = *hgt * 1.5; 	
		*wid = ceil(2 * (*hgt * (1/sqrt(3))));	//for equilateral triangle hgt = sqrt(3)/2 * side 	
															
	}
	
	if (filt->Shape_Type == TRIANGLE) {
		filt->start_col = x_coord - (*wid / 2);
		filt->start_row = y_coord -  (filt->Size);	
	}	else {
			filt->start_col =  x_coord - (*wid / 2);
			filt->start_row =  y_coord - (*hgt / 2);
	}

	if ((filt->start_row % 2)) 		//adjusting starting coordinates to even values
		filt->start_row -= 1;	
	if ((filt->start_col % 2)) 
		filt->start_col -= 1;

	int x, y, h, w;
	x = filt->start_col + *wid;		//boundary condition for cheaking cropped wid (starting coordiante)going outside frame
	if (x > filt->frame_width) {
		x = filt->frame_width - 1;
	}
	
	if ( (filt->start_col < 0) && (x > 0) ) { //boundary condition for cheaking cropped wid going negative so making it 0
		filt->start_col = 0;
	}
	
	if ( (filt->start_col > filt->frame_width) && (x < filt->frame_width)) {
		filt->start_col = filt->frame_width - 1;		
	}

	y = filt->start_row + *hgt;

	if (y > filt->frame_height) { 	//boundary condition for cheaking cropped hgt (starting coordiante)going outside frame
		y = filt->frame_height - 1;
	}

	if ( (filt->start_row < 0) && (y >= 0) ) {
		filt->start_row = 0;
	}

	if ( (filt->start_row > filt->frame_height) && (y < filt->frame_height)) {
		filt->start_row = filt->frame_height - 1;
	}

	if ((filt->start_row % 2)) 		//adjusting starting coordinates to even values
		filt->start_row -= 1;	
	if ((filt->start_col % 2)) 
		filt->start_col -= 1;
		
	w = x - filt->start_col;
	h = y - filt->start_row ;
	
	int flag = 0;		// to check bound
		static int i = 0;
	if ( (w != *wid) || (h != *hgt) ) {
		if (i == 0) {		
			g_warning ("shape is out of bounds\n The maximum shape dimensions can be: width = %d\thight =%d\n",filt->frame_width, filt->frame_height);
			i++;
		}
		flag = 1;
		if (strict_mode)
			exit(1);
		if (i == 1) {
			g_print ("Cropping the shape within the frame.\n");
			i = 2;
		}
		*wid = w;
		*hgt = h;
	}

	if (*wid > filt->frame_width) { 	//check if entire shape frame is out of window
	
	*wid = filt->frame_width;
		filt->start_row = 0;
	}
	if (*hgt > filt->frame_height) {
		*hgt = filt->frame_height;
		filt->start_col = 0;
	}
	
	if ( (*wid % 2) != 0 )
		*wid -= 1;

	if ( (*hgt % 2) != 0 )
		*hgt -= 1;

	
	if (flag || filt->Shape_Type == TRIANGLE) {
		if ( ((*wid / 2) % 2) != 0 )
			*wid += 2;

		if ( ((*hgt / 2) % 2) != 0 )
			*hgt += 2;
	}
	
}

/* entry point to initialize the plug-in
 * initialize the plug-in itself
 * register the element factories and other features
 */
static gboolean
cropportion_init(GstPlugin * cropportion) {
  /* debug category for filtering log messages
   *
   * exchange the string 'Template cropportion' with your description
   */

  GST_DEBUG_CATEGORY_INIT(gst_crop_portion_debug, "cropportion",
    0, "Template cropportion");

  return GST_ELEMENT_REGISTER(crop_portion, cropportion);
}

/* PACKAGE: this is usually set by meson depending on some _INIT macro
 * in meson.build and then written into and defined in config.h, but we can
 * just set it ourselves here in case someone doesn't use meson to
 * compile this code. GST_PLUGIN_DEFINE needs PACKAGE to be defined.
 */
#ifndef PACKAGE
#define PACKAGE "myfirstcropportion"
#endif

/* gstreamer looks for this structure to register cropportions
 *
 * exchange the string 'Template cropportion' with your cropportion description
 */
GST_PLUGIN_DEFINE(GST_VERSION_MAJOR,
  GST_VERSION_MINOR,
  cropportion,
  "crop_portion",
  cropportion_init,
  PACKAGE_VERSION, GST_LICENSE, GST_PACKAGE_NAME, GST_PACKAGE_ORIGIN)
