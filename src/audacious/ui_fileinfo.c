/*
 * Audacious: A cross-platform multimedia player
 * Copyright (c) 2006 William Pitcock, Tony Vroon, George Averill,
 *                    Giacomo Lozito, Derek Pomery and Yoshiki Yazawa.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; under version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <glade/glade.h>
#include <string.h>
#include <stddef.h>
#include <stdio.h>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "glade.h"

#include "plugin.h"
#include "pluginenum.h"
#include "input.h"
#include "effect.h"
#include "strings.h"
#include "general.h"
#include "output.h"
#include "visualization.h"

#include "main.h"
#include "urldecode.h"
#include "util.h"
#include "dnd.h"
#include "titlestring.h"

#include "libaudacious/configdb.h"

#include "playlist.h"

#include "ui_main.h"
#include "ui_playlist.h"
#include "build_stamp.h"
#include "ui_fileinfo.h"
#include "ui_playlist.h"

GtkWidget *fileinfo_win;

static void
fileinfo_entry_set_text(const char *entry, const char *text)
{
	GladeXML *xml = g_object_get_data(G_OBJECT(fileinfo_win), "glade-xml");
	GtkWidget *widget = glade_xml_get_widget(xml, entry);

	if (xml == NULL || widget == NULL)
		return;

	gtk_entry_set_text(GTK_ENTRY(widget), text);
}

static void
fileinfo_entry_set_text_free(const char *entry, char *text)
{
	GladeXML *xml = g_object_get_data(G_OBJECT(fileinfo_win), "glade-xml");
	GtkWidget *widget = glade_xml_get_widget(xml, entry);

	if (xml == NULL || widget == NULL)
		return;

	gtk_entry_set_text(GTK_ENTRY(widget), text);

	g_free(text);
}

static void
fileinfo_entry_set_image(const char *entry, const char *text)
{
	GladeXML *xml = g_object_get_data(G_OBJECT(fileinfo_win), "glade-xml");
	GtkWidget *widget = glade_xml_get_widget(xml, entry);
	GdkPixbuf *pixbuf;
	int width, height;
	double aspect;
	GdkPixbuf *pixbuf2;

	if (xml == NULL || widget == NULL)
		return;

	pixbuf = gdk_pixbuf_new_from_file(text, NULL);

	if (pixbuf == NULL)
		return;

	width  = gdk_pixbuf_get_width(GDK_PIXBUF(pixbuf));
	height = gdk_pixbuf_get_height(GDK_PIXBUF(pixbuf));

	if(strcmp(DATA_DIR "/images/audio.png", text))
	{
		if(width == 0)
			width = 1;
		aspect = (double)height / (double)width;
		if(aspect > 1.0) {
			height = (int)(cfg.filepopup_pixelsize * aspect);
			width = cfg.filepopup_pixelsize;
		} else {
			height = cfg.filepopup_pixelsize;
			width = (int)(cfg.filepopup_pixelsize / aspect);
		}
		pixbuf2 = gdk_pixbuf_scale_simple(GDK_PIXBUF(pixbuf), width, height, GDK_INTERP_BILINEAR);
		g_object_unref(G_OBJECT(pixbuf));
		pixbuf = pixbuf2;
	}

	gtk_image_set_from_pixbuf(GTK_IMAGE(widget), GDK_PIXBUF(pixbuf));
	g_object_unref(G_OBJECT(pixbuf));
}

void fileinfo_hide(gpointer unused)
{
	gtk_widget_hide(fileinfo_win);

	/* Clear it out. */
	fileinfo_entry_set_text("entry_title", "");
	fileinfo_entry_set_text("entry_artist", "");
	fileinfo_entry_set_text("entry_album", "");
	fileinfo_entry_set_text("entry_comment", "");
	fileinfo_entry_set_text("entry_genre", "");
	fileinfo_entry_set_text("entry_year", "");
	fileinfo_entry_set_text("entry_track", "");
	fileinfo_entry_set_text("entry_location", "");

	fileinfo_entry_set_image("image_artwork", DATA_DIR "/images/audio.png");
}

void
create_fileinfo_window(void)
{
	const gchar *glade_file = DATA_DIR "/glade/fileinfo.glade";
	GladeXML *xml;
	GtkWidget *widget;

	xml = glade_xml_new_or_die(_("Track Information Window"), glade_file, NULL, NULL);

	glade_xml_signal_autoconnect(xml);

	fileinfo_win = glade_xml_get_widget(xml, "fileinfo_win");
	g_object_set_data(G_OBJECT(fileinfo_win), "glade-xml", xml);
	gtk_window_set_transient_for(GTK_WINDOW(fileinfo_win), GTK_WINDOW(mainwin));

	widget = glade_xml_get_widget(xml, "image_artwork");
	gtk_image_set_from_file(GTK_IMAGE(widget), DATA_DIR "/images/audio.png");

	widget = glade_xml_get_widget(xml, "btn_close");
	g_signal_connect(G_OBJECT(widget), "clicked", (GCallback) fileinfo_hide, NULL);
}

void
fileinfo_show_for_tuple(TitleInput *tuple)
{
	gchar *tmp = NULL;

	if (tuple == NULL)
		return;

	gtk_widget_realize(fileinfo_win);

	if (tuple->track_name)
		fileinfo_entry_set_text("entry_title", tuple->track_name);
	if (tuple->performer)
		fileinfo_entry_set_text("entry_artist", tuple->performer);
	if (tuple->album_name)
		fileinfo_entry_set_text("entry_album", tuple->album_name);
	if (tuple->comment)
		fileinfo_entry_set_text("entry_comment", tuple->comment);
	if (tuple->genre)
		fileinfo_entry_set_text("entry_genre", tuple->genre);

	tmp = g_strdup_printf("%s/%s", tuple->file_path, tuple->file_name);
	if(tmp){
		fileinfo_entry_set_text_free("entry_location", str_to_utf8(tmp));
		g_free(tmp);
		tmp = NULL;
	}

	if (tuple->year != 0)
		fileinfo_entry_set_text_free("entry_year", g_strdup_printf("%d", tuple->year));

	if (tuple->track_number != 0)
		fileinfo_entry_set_text_free("entry_track", g_strdup_printf("%d", tuple->track_number));

	tmp = fileinfo_recursive_get_image(tuple->file_path, tuple->file_name, 0);
	
	if(tmp)
	{
		fileinfo_entry_set_image("image_artwork", tmp);
		g_free(tmp);
	}
	
	gtk_widget_show(fileinfo_win);
}

static gboolean
has_front_cover_extension(const gchar *name)
{
	char *ext;

	ext = strrchr(name, '.');
	if (!ext) {
		/* No file extension */
		return FALSE;
	}

	return g_strcasecmp(ext, ".jpg") == 0 ||
	       g_strcasecmp(ext, ".jpeg") == 0 ||
	       g_strcasecmp(ext, ".png") == 0;
}

static gboolean
cover_name_filter(const gchar *name, const gchar *filter, const gboolean ret_on_empty)
{
	gboolean result = FALSE;
	gchar **splitted;
	gchar *current;
	gchar *lname;
	gint i;

	if (!filter || strlen(filter) == 0) {
		return ret_on_empty;
	}

	splitted = g_strsplit(filter, ",", 0);

	lname = g_strdup(name);
	g_strdown(lname);

	for (i = 0; !result && (current = splitted[i]); i++) {
		gchar *stripped = g_strstrip(g_strdup(current));
		g_strdown(stripped);

		result = result || strstr(lname, stripped);

		g_free(stripped);
	}

	g_free(lname);
	g_strfreev(splitted);

	return result;
}

/* Check wether it's an image we want */
static gboolean
is_front_cover_image(const gchar *imgfile)
{
	return cover_name_filter(imgfile, cfg.cover_name_include, TRUE) &&
	       !cover_name_filter(imgfile, cfg.cover_name_exclude, FALSE);
}

static gboolean
is_file_image(const gchar *imgfile, const gchar *file_name)
{
	char *imgfile_ext, *file_name_ext;
	size_t imgfile_len, file_name_len;

	imgfile_ext = strrchr(imgfile, '.');
	if (!imgfile_ext) {
		/* No file extension */
		return FALSE;
	}

	file_name_ext = strrchr(file_name, '.');
	if (!file_name_ext) {
		/* No file extension */
		return FALSE;
	}

	imgfile_len = (imgfile_ext - imgfile);
	file_name_len = (file_name_ext - file_name);

	if (imgfile_len == file_name_len) {
		return (g_ascii_strncasecmp(imgfile, file_name, imgfile_len) == 0);
	} else {
		return FALSE;
	}
}

gchar*
fileinfo_recursive_get_image(const gchar* path,
	const gchar* file_name, gint depth)
{
	GDir *d;

	if (cfg.recurse_for_cover && depth > cfg.recurse_for_cover_depth)
		return NULL;
	
	d = g_dir_open(path, 0, NULL);

	if (d) {
		const gchar *f;

		if (cfg.use_file_cover && file_name) {
			/* Look for images matching file name */
			while((f = g_dir_read_name(d))) { 
				gchar *newpath = g_strconcat(path, "/", f, NULL);

				if (!g_file_test(newpath, G_FILE_TEST_IS_DIR) &&
				    has_front_cover_extension(f) &&
				    is_file_image(f, file_name)) {
					g_dir_close(d);
					return newpath;
				}

				g_free(newpath);
			}
			g_dir_rewind(d);
		}
		
		/* Search for files using filter */
		while ((f = g_dir_read_name(d))) {
			gchar *newpath = g_strconcat(path, "/", f, NULL);

			if (!g_file_test(newpath, G_FILE_TEST_IS_DIR) &&
			    has_front_cover_extension(f) &&
			    is_front_cover_image(f)) {
				g_dir_close(d);
				return newpath;
			}

			g_free(newpath);
		}
		g_dir_rewind(d);

		/* checks whether recursive or not. */
		if (!cfg.recurse_for_cover) {
			g_dir_close(d);
			return NULL;
		}

		/* Descend into directories recursively. */
		while ((f = g_dir_read_name(d))) {
			gchar *newpath = g_strconcat(path, "/", f, NULL);
			
			if(g_file_test(newpath, G_FILE_TEST_IS_DIR)) {
				gchar *tmp = fileinfo_recursive_get_image(newpath,
					NULL, depth + 1);
				if(tmp) {
					g_free(newpath);
					g_dir_close(d);
					return tmp;
				}
			}

			g_free(newpath);
		}

		g_dir_close(d);
	}

	return NULL;
}

void
fileinfo_show_for_path(gchar *path)
{
	TitleInput *tuple = input_get_song_tuple(path);

	if (tuple == NULL)
		return input_file_info_box(path);

	fileinfo_show_for_tuple(tuple);

	bmp_title_input_free(tuple);
	tuple = NULL;
}