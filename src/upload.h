/*
Asunder

Copyright(C) 2021 Andreas Hünnebeck <andi@huennebeck-online.de>
Copyright(C) 2007 Andrew Smith <http://littlesvr.ca/contact.php>

Any code in this file may be redistributed or modified under the terms of
the GNU General Public Licence as published by the Free Software
Foundation; version 2 of the licence.
*/

#include <gtk/gtk.h>

// return the widget which contains the button to start the CDDB upload (for the main window)
GtkWidget * get_cddb_upload_button_widget(GtkWidget *main_win);

// return the widget which contains the toggle button to enable/disable CDDB upload (for the Preferences dialog)
GtkWidget * get_enable_cddb_upload_togglebutton(GtkWidget *prefs);

// return the widget which contains label and textfield to enter the email address (for the Preferences dialog)
GtkWidget * get_cddb_email_address_widget(GtkWidget *prefs);

// return 1 if CDDB upload is enabled, and 0 otherwise
int upload_is_enabled();

// return 1 if the email address in the Preference dialog is valid, and 0 otherwise
int upload_email_address_is_valid();

// shows/hides the upload button and the category box
void upload_toggle_enable(void);


