/*
Asunder

Copyright(C) 2021 Andreas Hünnebeck <andi@huennebeck-online.de>
Copyright(C) 2007 Andrew Smith <http://littlesvr.ca/contact.php>

Any code in this file may be redistributed or modified under the terms of
the GNU General Public Licence as published by the Free Software
Foundation; version 2 of the licence.
*/

#include <cddb/cddb.h>

// this set of functions represents the selected disc. The access functions are thread safe.

// initialize the selected disc - you need to do this once in main()
void glb_selected_disc_init();

// free the selected disc
void glb_selected_disc_free();

// set the selected disc to the specified 'source'
void glb_selected_disc_clone_from(cddb_disc_t * source);

// get a clone of the selected disc. It's your responsibility to free it when no longer needed
cddb_disc_t * glb_selected_disc_get_clone();

