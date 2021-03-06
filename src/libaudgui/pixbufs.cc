/*
 * pixbufs.c
 * Copyright 2010-2012 John Lindgren
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions, and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions, and the following disclaimer in the documentation
 *    provided with the distribution.
 *
 * This software is provided "as is" and without any warranty, express or
 * implied. In no event shall the authors be liable for any damages arising from
 * the use of this software.
 */

#include <gdk-pixbuf/gdk-pixbuf.h>

#include <libaudcore/audstrings.h>
#include <libaudcore/drct.h>
#include <libaudcore/probe.h>
#include <libaudcore/runtime.h>

#include "internal.h"
#include "libaudgui-gtk.h"

static GdkPixbuf * current_pixbuf;

EXPORT GdkPixbuf * audgui_pixbuf_fallback ()
{
    static GdkPixbuf * fallback = nullptr;

    if (! fallback)
        fallback = gdk_pixbuf_new_from_file (filename_build
         ({aud_get_path (AudPath::DataDir), "images", "album.png"}), nullptr);

    if (fallback)
        g_object_ref ((GObject *) fallback);

    return fallback;
}

void audgui_pixbuf_uncache ()
{
    if (current_pixbuf)
    {
        g_object_unref ((GObject *) current_pixbuf);
        current_pixbuf = nullptr;
    }
}

EXPORT void audgui_pixbuf_scale_within (GdkPixbuf * * pixbuf, int size)
{
    int width = gdk_pixbuf_get_width (* pixbuf);
    int height = gdk_pixbuf_get_height (* pixbuf);

    if (width <= size && height <= size)
        return;

    if (width > height)
    {
        height = size * height / width;
        width = size;
    }
    else
    {
        width = size * width / height;
        height = size;
    }

    if (width < 1)
        width = 1;
    if (height < 1)
        height = 1;

    GdkPixbuf * pixbuf2 = gdk_pixbuf_scale_simple (* pixbuf, width, height, GDK_INTERP_BILINEAR);
    g_object_unref (* pixbuf);
    * pixbuf = pixbuf2;
}

EXPORT GdkPixbuf * audgui_pixbuf_request (const char * filename, bool * queued)
{
    const Index<char> * data = aud_art_request_data (filename, queued);
    if (! data)
        return nullptr;

    GdkPixbuf * p = audgui_pixbuf_from_data (data->begin (), data->len ());

    aud_art_unref (filename);
    return p;
}

EXPORT GdkPixbuf * audgui_pixbuf_request_current (bool * queued)
{
    if (queued)
        * queued = false;

    if (! current_pixbuf)
    {
        String filename = aud_drct_get_filename ();
        if (filename)
            current_pixbuf = audgui_pixbuf_request (filename, queued);
    }

    if (current_pixbuf)
        g_object_ref ((GObject *) current_pixbuf);

    return current_pixbuf;
}

EXPORT GdkPixbuf * audgui_pixbuf_from_data (const void * data, int64_t size)
{
    GdkPixbuf * pixbuf = nullptr;
    GdkPixbufLoader * loader = gdk_pixbuf_loader_new ();
    GError * error = nullptr;

    if (gdk_pixbuf_loader_write (loader, (const unsigned char *) data, size,
     & error) && gdk_pixbuf_loader_close (loader, & error))
    {
        if ((pixbuf = gdk_pixbuf_loader_get_pixbuf (loader)))
            g_object_ref (pixbuf);
    }
    else
    {
        AUDWARN ("While loading pixbuf: %s\n", error->message);
        g_error_free (error);
    }

    g_object_unref (loader);
    return pixbuf;
}
