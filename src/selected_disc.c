/*
Asunder

Copyright(C) 2021 Andreas Hünnebeck <andi@huennebeck-online.de>
Copyright(C) 2007 Andrew Smith <http://littlesvr.ca/contact.php>

Any code in this file may be redistributed or modified under the terms of
the GNU General Public Licence as published by the Free Software 
Foundation; version 2 of the licence.
*/

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include "selected_disc.h"	// own interface first, says Lakos

#include <gtk/gtk.h>
#include "interface.h"


static cddb_disc_t * glb_selected_disc = NULL;
static GMutex glb_selected_disc_mutex;


void glb_selected_disc_init()
{
    g_mutex_init(&glb_selected_disc_mutex);
    glb_selected_disc = NULL;
}


void glb_selected_disc_free()
{
	g_mutex_lock(&glb_selected_disc_mutex);

	if (glb_selected_disc)
	{
		cddb_disc_destroy(glb_selected_disc);
		glb_selected_disc = NULL;
	}

	g_mutex_unlock(&glb_selected_disc_mutex);
}


void glb_selected_disc_clone_from(cddb_disc_t * source)
{
	g_mutex_lock(&glb_selected_disc_mutex);

	if (glb_selected_disc)
	{
		cddb_disc_destroy(glb_selected_disc);
		glb_selected_disc = NULL;
	}

	glb_selected_disc = cddb_disc_clone(source);

	g_mutex_unlock(&glb_selected_disc_mutex);
}


cddb_disc_t * glb_selected_disc_get_clone()
{
	g_mutex_lock(&glb_selected_disc_mutex);

	if (glb_selected_disc)
	{
		cddb_disc_t * clone = cddb_disc_clone(glb_selected_disc);
		g_mutex_unlock(&glb_selected_disc_mutex);
		return clone;
	}

	g_mutex_unlock(&glb_selected_disc_mutex);
	show_simple_dialog_message("No disc selected.");

	return NULL;
}
