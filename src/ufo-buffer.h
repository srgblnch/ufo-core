/*
 * Copyright (C) 2011-2013 Karlsruhe Institute of Technology
 *
 * This file is part of Ufo.
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __UFO_BUFFER_H
#define __UFO_BUFFER_H

#include <glib-object.h>

G_BEGIN_DECLS

#define UFO_TYPE_BUFFER             (ufo_buffer_get_type())
#define UFO_BUFFER(obj)             (G_TYPE_CHECK_INSTANCE_CAST((obj), UFO_TYPE_BUFFER, UfoBuffer))
#define UFO_IS_BUFFER(obj)          (G_TYPE_CHECK_INSTANCE_TYPE((obj), UFO_TYPE_BUFFER))
#define UFO_BUFFER_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST((klass), UFO_TYPE_BUFFER, UfoBufferClass))
#define UFO_IS_BUFFER_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE((klass), UFO_TYPE_BUFFER))
#define UFO_BUFFER_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS((obj), UFO_TYPE_BUFFER, UfoBufferClass))

#define UFO_TYPE_PARAM_BUFFER       (ufo_buffer_param_get_type())
#define UFO_IS_PARAM_SPEC_BUFFER(pspec)  (G_TYPE_CHECK_INSTANCE_TYPE((pspec), UFO_TYPE_PARAM_BUFFER))
#define UFO_BUFFER_PARAM_SPEC(pspec)     (G_TYPE_CHECK_INSTANCE_CAST((pspec), UFO_TYPE_PARAM_BUFFER, UfoBufferParamSpec))

/**
 * UfoMemLocation:
 * @UFO_LOCATION_INVALID: Memory is neither valid on host nor on device.
 * @UFO_LOCATION_HOST: Memory is valid on host memory.
 * @UFO_LOCATION_DEVICE: Memory is valid on device memory.
 *
 * Memory locations of a #UfoBuffer.
 */
typedef enum {
    UFO_LOCATION_INVALID,
    UFO_LOCATION_HOST,
    UFO_LOCATION_DEVICE
} UfoMemLocation;

typedef struct _UfoBuffer           UfoBuffer;
typedef struct _UfoBufferClass      UfoBufferClass;
typedef struct _UfoBufferPrivate    UfoBufferPrivate;
typedef struct _UfoBufferParamSpec  UfoBufferParamSpec;
typedef struct _UfoRequisition      UfoRequisition;

/**
 * UfoBuffer:
 *
 * Represents n-dimensional data. The contents of the #UfoBuffer structure are
 * private and should only be accessed via the provided API.
 */
struct _UfoBuffer {
    /*< private >*/
    GObject parent_instance;

    UfoBufferPrivate *priv;
};

/**
 * UFO_BUFFER_MAX_NDIMS:
 *
 * Maximum number of allowed dimensions. This is a pre-processor macro instead
 * of const variable because of <ulink
 * url="http://c-faq.com/ansi/constasconst.html">C constraints</ulink>.
 */
#define UFO_BUFFER_MAX_NDIMS 8

/**
 * UfoBufferClass:
 *
 * #UfoBuffer class
 */
struct _UfoBufferClass {
    /*< private >*/
    GObjectClass parent_class;
};

/**
 * UfoRequisition:
 * @n_dims: Number of dimensions
 * @dims: Size of dimension
 *
 * Used to specify buffer size requirements.
 */
struct _UfoRequisition {
    guint n_dims;
    gsize dims[UFO_BUFFER_MAX_NDIMS];
};

/**
 * UfoBufferParamSpec:
 *
 * UfoBufferParamSpec class
 */
struct _UfoBufferParamSpec {
    /*< private >*/
    GParamSpec  parent_instance;

    UfoBuffer   *default_value;
};

/**
 * UfoBufferDepth:
 * @UFO_BUFFER_DEPTH_8U: 8 bit unsigned
 * @UFO_BUFFER_DEPTH_16U: 16 bit unsigned
 *
 * Source depth of data as used in ufo_buffer_convert().
 */
typedef enum {
    UFO_BUFFER_DEPTH_8U,
    UFO_BUFFER_DEPTH_16U
} UfoBufferDepth;

UfoBuffer*  ufo_buffer_new                  (UfoRequisition *requisition,
                                             gpointer        context);
void        ufo_buffer_resize               (UfoBuffer      *buffer,
                                             UfoRequisition *requisition);
gint        ufo_buffer_cmp_dimensions       (UfoBuffer      *buffer,
                                             UfoRequisition *requisition);
void        ufo_buffer_get_requisition      (UfoBuffer      *buffer,
                                             UfoRequisition *requisition);
gsize       ufo_buffer_get_size             (UfoBuffer      *buffer);
void        ufo_buffer_copy                 (UfoBuffer      *src,
                                             UfoBuffer      *dst);
UfoBuffer  *ufo_buffer_dup                  (UfoBuffer      *buffer);
gfloat*     ufo_buffer_get_host_array       (UfoBuffer      *buffer,
                                             gpointer        cmd_queue);
gpointer    ufo_buffer_get_device_array     (UfoBuffer      *buffer,
                                             gpointer        cmd_queue);
void        ufo_buffer_discard_location     (UfoBuffer      *buffer,
                                             UfoMemLocation  location);
void        ufo_buffer_convert              (UfoBuffer      *buffer,
                                             UfoBufferDepth  depth);
GType       ufo_buffer_get_type             (void);

GParamSpec* ufo_buffer_param_spec           (const gchar*   name,
                                             const gchar*   nick,
                                             const gchar*   blurb,
                                             UfoBuffer*     default_value,
                                             GParamFlags    flags);
GType       ufo_buffer_param_get_type       (void);

G_END_DECLS

#endif
