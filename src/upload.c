/*
Asunder

Copyright(C) 2021 Andreas Hünnebeck <andi@huennebeck-online.de>
Copyright(C) 2007 Andrew Smith <http://littlesvr.ca/contact.php>

Any code in this file may be redistributed or modified under the terms of
the GNU General Public Licence as published by the Free Software 
Foundation; version 2 of the licence.
*/

#ifdef HAVE_CONFIG_H
#    include <config.h>
#endif

#include "upload.h"	// own interface first, says Lakos

#include <string.h>

#include "main.h"
#include "prefs.h"
#include "interface.h"
#include "selected_disc.h"
#include "support.h"
#include "util.h"

#define HAS_POSIX_REGEX 1
#if HAS_POSIX_REGEX
#   include <regex.h>
#endif

#define TEST_UPLOAD 0	// set to 1 to enable test mode.

#define GLADE_HOOKUP_OBJECT(component,widget,name) \
    g_object_set_data_full (G_OBJECT (component), name, \
        g_object_ref (widget), (GDestroyNotify) g_object_unref)

#define GLADE_HOOKUP_OBJECT_NO_REF(component,widget,name) \
    g_object_set_data (G_OBJECT (component), name, widget)

static void on_upload_button_clicked(GtkButton* button, gpointer user_data);
static void on_enable_cddb_upload_toggled(GtkToggleButton *togglebutton, gpointer user_data);
static int set_disc_from_GUI(cddb_disc_t * disc);
static int is_different(const char* orig, const char* fromGui);
static GtkWidget* create_upload_check (cddb_disc_t* disc);
static void get_disc_info(cddb_disc_t * disc, char* buf, size_t size);
static void on_check_response(GtkDialog *dialog, gint response_id, gpointer user_data);
static gpointer upload_to_cddb_server(gpointer user_data);
static void show_cddb_connection_error(cddb_conn_t * conn, const char* failed_task, const char* possible_cause);
static void disable_update_button(void);
static void enable_update_button(void);
static bool is_valid_email_address(const char* email_address);


/*** public functions ***/

GtkWidget * get_cddb_upload_button_widget(GtkWidget *main_win)
{
    GtkWidget* hbox_ub1 = gtk_hbox_new(FALSE, 5);
    gtk_widget_show(hbox_ub1);

    GtkWidget* upload_button = gtk_button_new ();
    if (upload_is_enabled())
    {
    	gtk_widget_show(upload_button);
    }
    gtk_box_pack_start(GTK_BOX (hbox_ub1), upload_button, FALSE, FALSE, 5);

    GtkWidget* alignment_ub = gtk_alignment_new (0.5, 0.5, 0, 0);
    gtk_widget_show (alignment_ub);
    gtk_container_add (GTK_CONTAINER (upload_button), alignment_ub);

    GtkWidget* hbox_ub2 = gtk_hbox_new (FALSE, 2);
    gtk_widget_show (hbox_ub2);
    gtk_container_add (GTK_CONTAINER (alignment_ub), hbox_ub2);

    GtkWidget* label_ub = gtk_label_new_with_mnemonic (_("Upload to CDDB"));
    gtk_widget_show (label_ub);
    gtk_box_pack_start (GTK_BOX (hbox_ub2), label_ub, FALSE, FALSE, 0);

    GLADE_HOOKUP_OBJECT (main_win, upload_button, UPLOAD_BUTTON);
    g_signal_connect ((gpointer) upload_button, "clicked", G_CALLBACK (on_upload_button_clicked), NULL);

    return hbox_ub1;
}


GtkWidget * get_enable_cddb_upload_togglebutton(GtkWidget *prefs)
{
    /* enable CDDB upload: */
    GtkWidget* enable_cddb_upload = gtk_check_button_new_with_mnemonic (_("Enable CDDB upload"));
    gtk_widget_show (enable_cddb_upload);

    GLADE_HOOKUP_OBJECT (prefs, enable_cddb_upload, "enable_cddb_upload");
    g_signal_connect ((gpointer) enable_cddb_upload, "toggled", G_CALLBACK (on_enable_cddb_upload_toggled), NULL);

    return enable_cddb_upload;
}


GtkWidget * get_cddb_email_address_widget(GtkWidget *prefs)
{
	GtkWidget * hbox = gtk_hbox_new (FALSE, 0);
    gtk_widget_show (hbox);

    GtkWidget * label = gtk_label_new (_("E-Mail: "));
    gtk_widget_show (label);
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 5);

    GtkWidget * cddbEmailAddress = gtk_entry_new ();
    gtk_widget_show (cddbEmailAddress);
    gtk_box_pack_start (GTK_BOX (hbox), cddbEmailAddress, TRUE, TRUE, 5);
    GLADE_HOOKUP_OBJECT (prefs, cddbEmailAddress, "cddb_email_address");

    gtk_widget_set_tooltip_text( cddbEmailAddress, _("Enter a valid email if you want to update disc info into gnudb"));

    return hbox;
}


int upload_is_enabled()
{
    if (global_prefs->cddb_upload_enabled && (0 < strlen(global_prefs->cddb_email_address)))
    {
    	return 1;
    }
    return 0;
}

void
upload_toggle_enable(void)
{
    if (upload_is_enabled())
    {
    	gtk_widget_show(glb_upload_button);
        gtk_widget_show(lookup_widget(win_main, ALBUM_CATEGORY_LABEL));
        gtk_widget_show(lookup_widget(win_main, ALBUM_CATEGORY_COMBO_BOX));
    }
    else
    {
    	gtk_widget_hide(glb_upload_button);
        gtk_widget_hide(lookup_widget(win_main, ALBUM_CATEGORY_LABEL));
        gtk_widget_hide(lookup_widget(win_main, ALBUM_CATEGORY_COMBO_BOX));
    }
}

int upload_email_address_is_valid()
{
    // cddb mail address (only when cddb upload is enabled)
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(lookup_widget(win_prefs, "enable_cddb_upload"))))
    {
    	const char* email_address = gtk_entry_get_text(GTK_ENTRY(lookup_widget(win_prefs, "cddb_email_address")));
    	if ( ! email_address || (0 == strlen(email_address)))
    	{
    		show_simple_dialog_message("CDDB upload requires a valid email address");
    		return 0;
    	}
    	if ( ! is_valid_email_address(email_address))
    	{
    		show_simple_dialog_message("Invalid email address");
    		return 0;
    	}
    }
    return 1;
}

static void on_enable_cddb_upload_toggled(GtkToggleButton *togglebutton, gpointer user_data)
{
	GtkWidget* email_address_w = lookup_widget(win_prefs, "cddb_email_address");
    if (gtk_toggle_button_get_active(togglebutton))
    {
    	gtk_widget_set_sensitive(email_address_w, 1);
    }
    else
    {
    	gtk_widget_set_sensitive(email_address_w, 0);
    }
}


// open dialog to show disc data to be uploaded to CDDB
static void on_upload_button_clicked(GtkButton* button, gpointer user_data)
{
	// check if CD is inserted:
	GtkListStore * store = glb_get_tracklist_store();
	if (NULL == store)
    {
    	show_simple_dialog_message("No CD is inserted. Please insert a CD into the CD-ROM drive.");
        return;
    }
	if ( ! upload_is_enabled())
	{
		show_simple_dialog_message(
				"CDDB upload is not possible without a valid email address.\n"
				"Open the preferences and set a valid email address in the 'Advanced' tab");
		return;
	}

	cddb_disc_t* new_disc = glb_selected_disc_get_clone();
	if ( ! new_disc)
	{
		show_simple_dialog_message(
				"CDDB upload:\n"
				"out of memory?");
        return;
	}

	int num_changes = set_disc_from_GUI(new_disc);
	if (num_changes < 1)
	{
		show_simple_dialog_message("No changes to upload.");
	    cddb_disc_destroy(new_disc);
        return;
	}

	// create and show the upload check dialog window:
	win_upload = create_upload_check(new_disc);
    gtk_widget_show(win_upload);
}

/*** private functions ***/


// set the disc parameters and all track data from the GUI into the specified disc
// - return number of changes
static int set_disc_from_GUI(cddb_disc_t * disc)
{
	const char* orig_album_artist = cddb_disc_get_artist(disc);
    const char* orig_album_title = cddb_disc_get_title(disc);

    // before setting artist and title handle the revision first
	// - CDDB silently ignores uploads of changed entries if they have the same (or lower) revision number than
	//   the existing entry.
	unsigned int revision = cddb_disc_get_revision(disc);
	if (revision == 0)
	{
		// this could be a disc missing in the database, or the first version of an existing entry
		if (orig_album_artist && orig_album_title)
		{
			// artist and title found:
			// this must be an existing entry, so we must increase the revision
			cddb_disc_set_revision(disc, 1);
		}
	}
	else
	{
		// this is an existing entry, so we must increase the revision
		cddb_disc_set_revision(disc, revision + 1);
	}

	int num_changes = 0;

	/* set the artist */
	const char* new_album_artist = gtk_entry_get_text(glb_album_artist);
	if (0 != strcmp(new_album_artist, EMPTY_ARTIST_SHOWN))
	{
		num_changes += is_different(orig_album_artist, new_album_artist);
		cddb_disc_set_artist (disc, new_album_artist);
	}

	/* set the title */
    const char* new_album_title = gtk_entry_get_text(glb_album_title);
	if (0 != strcmp(new_album_title, EMPTY_ALBUM_SHOWN))
	{
		num_changes += is_different(orig_album_title, new_album_title);
		cddb_disc_set_title (disc, new_album_title);
	}

    /* set the category */
    cddb_cat_t orig_album_category = cddb_disc_get_category(disc);
    cddb_cat_t new_album_category =  gtk_combo_box_get_active(glb_album_category);
    num_changes += (orig_album_category != new_album_category ? 1 : 0);
    cddb_disc_set_category(disc, new_album_category);

    /* set the genre */
    const char* new_album_genre = gtk_entry_get_text(glb_album_genre);
	if (0 != strcmp(new_album_genre, EMPTY_GENRE_SHOWN))
	{
	    const char* orig_album_genre = cddb_disc_get_genre(disc);
		num_changes += is_different(orig_album_genre, new_album_genre);
		cddb_disc_set_genre (disc, new_album_genre);
	}

	/* set the year */
    int orig_album_year = cddb_disc_get_year(disc);
    int new_album_year = atoi(gtk_entry_get_text(glb_album_year));
    if (new_album_year == EMPTY_YEAR_SHOWN)
    {
    	new_album_year = 0;
    }
    num_changes += (orig_album_year != new_album_year ? 1 : 0);
    cddb_disc_set_year(disc, new_album_year);

    /* check 'Single Artist' button */
    gboolean is_single_artist = gtk_toggle_button_get_active(glb_single_artist_button);

    /* set the tracks */
    GtkListStore * store = glb_get_tracklist_store();
    GtkTreeModel* store_model = GTK_TREE_MODEL(store);
    cddb_track_t * track;
    gboolean rowsleft;
    GtkTreeIter iter;
    for (track = cddb_disc_get_track_first(disc), rowsleft = gtk_tree_model_get_iter_first(store_model, &iter);
    	 track && rowsleft;
    	 track = cddb_disc_get_track_next(disc),  rowsleft = gtk_tree_model_iter_next(store_model, &iter))
    {
    	/* get track artist and title */
        char* new_track_artist = NULL;
        char* new_track_title = NULL;
        int track_num = 0;
        gtk_tree_model_get(store_model, &iter,
            COL_TRACKARTIST, &new_track_artist,
			COL_TRACKTITLE,  &new_track_title,
			COL_TRACKNUM,    &track_num,
            -1);
        const char* real_new_track_artist = new_track_artist;
        if (is_single_artist || (0 == strcmp(real_new_track_artist, EMPTY_ARTIST_SHOWN)))
        {
        	// single artist selected, or track artist is not set: use album artist for this track
        	real_new_track_artist = new_album_artist;
        }

    	/* set track artist */
    	const char* orig_track_artist = cddb_track_get_artist(track);
    	if (0 != strcmp(real_new_track_artist, EMPTY_ARTIST_SHOWN))
    	{
    		num_changes += is_different(orig_track_artist, real_new_track_artist);
    		cddb_track_set_artist(track, real_new_track_artist);
    	}
        free(new_track_artist);

    	/* set track title */
        char empty_track_title[16];
        snprintf(empty_track_title, sizeof(empty_track_title) - 1, EMPTY_TRACK_SHOWN, track_num);
        if (0 != strcmp(new_track_title, empty_track_title))
        {
        	const char* orig_track_title = cddb_track_get_title(track);
        	num_changes += is_different(orig_track_title, new_track_title);
        	cddb_track_set_title(track, new_track_title);
        }
        free(new_track_title);
    }

    return num_changes;
}


// compare an string from the original disc with a string from the GUI
// - 'orig' may be NULL
// - return 1 if 'orig' and 'fromGui' differs
// - return 0 othwerwise
static int is_different(const char* orig, const char* fromGui)
{
	if (orig)
	{
		return 0 != strcmp(orig, fromGui);
	}
	return (0 != strlen(fromGui));
}


// create a dialog window which shows the data to be uploaded and Cancel/OK buttons
static GtkWidget* create_upload_check (cddb_disc_t* disc)
{
	//  Layout:
    //               --- Window Title ---
    //  vbox       : Multiline-Textbuffer
    //  action-area:           Cancel, Ok

	// create window with title:
	GtkWidget* upload_check_window = gtk_dialog_new ();
    gtk_window_set_transient_for (GTK_WINDOW(upload_check_window), GTK_WINDOW(win_main));
    gtk_window_set_title (GTK_WINDOW (upload_check_window), _("CDDB Upload Overview"));
    gtk_window_set_modal (GTK_WINDOW (upload_check_window), TRUE);
    gtk_window_set_type_hint (GTK_WINDOW (upload_check_window), GDK_WINDOW_TYPE_HINT_DIALOG);

    // get the inherent vbox:
    GtkWidget* vbox = gtk_dialog_get_content_area(GTK_DIALOG(upload_check_window));
    gtk_widget_show (vbox);

    // create Multiline-Textbuffer in vbox:
    GtkWidget* info_text = gtk_text_view_new();
	GtkTextBuffer* text_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(info_text));
//	GtkTextTag* font_tag = gtk_text_buffer_create_tag (text_buffer, "font", "font", "fixed", NULL); // use fix font, see http://www.bravegnu.org/gtktext/x113.html
	char disc_info[(7 + 99) * 200]; // space for 7 info lines and 99 tracks, each 200 chars wide
	get_disc_info(disc, disc_info, sizeof(disc_info));
	gtk_text_buffer_set_text(text_buffer, disc_info, -1);
//	GtkTextIter text_start, text_end;
//	gtk_text_buffer_get_selection_bounds (text_buffer, &text_start, &text_end);
//	gtk_text_buffer_apply_tag (text_buffer, font_tag, &text_start, &text_end);
    gtk_box_pack_start (GTK_BOX (vbox), info_text, TRUE, TRUE, 0);
    gtk_widget_show (info_text);

    // the action area at the bottom with the Cancel and Ok buttons:
    GtkWidget* dialog_action_area1 = gtk_dialog_get_action_area(GTK_DIALOG (upload_check_window));
    gtk_widget_show (dialog_action_area1);
    gtk_button_box_set_layout (GTK_BUTTON_BOX (dialog_action_area1), GTK_BUTTONBOX_END);

    GtkWidget* cancel_button = gtk_button_new_from_stock ("gtk-cancel");
    gtk_widget_show (cancel_button);
    gtk_dialog_add_action_widget (GTK_DIALOG (upload_check_window), cancel_button, GTK_RESPONSE_CANCEL);
	gtk_widget_set_can_default(cancel_button, TRUE);

    GtkWidget* ok_button = gtk_button_new_from_stock ("gtk-ok");
    gtk_widget_show (ok_button);
    gtk_dialog_add_action_widget (GTK_DIALOG (upload_check_window), ok_button, GTK_RESPONSE_OK);
    gtk_widget_set_can_default(ok_button, TRUE);

    // the call back function which is called when the user clicks on OK or Cancel:
    g_signal_connect ((gpointer) upload_check_window, "response",
                                        G_CALLBACK (on_check_response),
										disc);

    return upload_check_window;
}


// gather the data from the specified disc and write them into the specified buffer
static void get_disc_info(cddb_disc_t * disc, char* buf, size_t size)
{
#define WRITE_APPEND(...) \
        { 	int written = snprintf(buf, remaining, __VA_ARGS__); \
            if ((written < 0) || (written > remaining)) \
				return; /* size of buf is too small */ \
            remaining -= written; buf += written;    }

	int remaining = size;

	WRITE_APPEND("Disc ID : %08x\n", cddb_disc_get_discid(disc));
	WRITE_APPEND("Revision: %d\n", cddb_disc_get_revision(disc));
	WRITE_APPEND("Artist  : %s\n", cddb_disc_get_artist(disc));
    WRITE_APPEND("Title   : %s\n", cddb_disc_get_title(disc));
    WRITE_APPEND("Category: %s\n", CDDB_CATEGORY[cddb_disc_get_category(disc)]);
    WRITE_APPEND("Genre   : %s\n", cddb_disc_get_genre(disc));
    WRITE_APPEND("Year    : %d\n", cddb_disc_get_year(disc));

    /* show the tracks */
    int track_num = 1;
    cddb_track_t * track;
    for (track = cddb_disc_get_track_first(disc);
    	 track;
    	 track = cddb_disc_get_track_next(disc), ++track_num)
    {
    	WRITE_APPEND("Track %2d: %s - %s\n", track_num, cddb_track_get_artist(track), cddb_track_get_title(track));
    }
}


// this function is called when the user clicks on OK or Cancel:
static void on_check_response(GtkDialog* dialog, gint response_id, gpointer user_data)
{
    if (response_id == GTK_RESPONSE_OK)
    {
    	// user clicked OK: create a thread which runs upload_to_cddb_server():
        GError* error = NULL;
        (void)g_thread_create(upload_to_cddb_server, user_data, FALSE, &error); // no need to join, thread should finish latest after 10s
        if (error)
        {
    		show_simple_dialog_message(error->message);
    		free(error);
        }
    }

    gtk_widget_destroy(GTK_WIDGET(dialog));
}


// the thread that uploads disc data to CDDB
// - data is a cddb_disc_t pointer
static gpointer upload_to_cddb_server(gpointer user_data)
{
	cddb_disc_t* disc = (cddb_disc_t*)user_data;
	if ( ! disc)
	{
		show_simple_dialog_message(
				"CDDB upload:\n"
				"missing disc data");
        return NULL;
	}

    gdk_threads_enter();
        disable_update_button();

        show_status_message("<b>Sending disc info to the CDDB server...</b>");
    gdk_threads_leave();
    // Note: do not leave this function without calling enable_update_button()!

    // set up the connection to the cddb server
    cddb_conn_t * conn = get_cddb_connection(global_prefs->cddb_email_address); /* email required for upload */
    if ( ! conn)
    {
    	show_cddb_connection_error(conn, "CDDB upload", NULL);
    }
    else
    {
    	// for uploads use the HTTP protocol
    	cddb_set_server_port(conn, 80);
    	cddb_http_enable(conn);
#if TEST_UPLOAD
    	// for tests use test-submit.cgi
    	// see mail from Rene Mogensen to Andreas Hünnebeck from 2021-05-06:
    	// the database is not updated, but you will receive a mail within 2 minutes which shows the uploaded data.
    	cddb_set_http_path_submit(conn, "/~cddb/test-submit.cgi");
    	log_disc_info(disc, "test upload_to_cddb_server(): ");
#else
    	log_disc_info(disc, "upload_to_cddb_server(): ");
#endif
    	int success = cddb_write(conn, disc);
    	if (0 == success)
    	{
        	show_cddb_connection_error(conn, "CDDB upload", "could be a wrong port number");
    	}
    	else
    	{
    		gdk_threads_enter();
#if TEST_UPLOAD
    		show_simple_dialog_message(
    				"CDDB test upload succeeded.\n"
    				"You should receive an email within 2 minutes.\n"
    				"The database is NOT updated.");
#else
    		show_simple_dialog_message(
    				"CDDB upload succeeded.\n"
    				"It may take up to 10 minutes for the change to become visible.");
#endif
    		gdk_threads_leave();
    	}

    	cddb_destroy(conn);
    	conn = NULL;
    }

    cddb_disc_destroy(disc);
    disc = NULL;

	gdk_threads_enter();
    	clear_status_message();

    	enable_update_button();
    gdk_threads_leave();

    return NULL;
}


static void show_cddb_connection_error(cddb_conn_t * conn, const char* failed_task, const char* possible_cause)
{
	cddb_error_t error_code = cddb_errno(conn);
	char error_msg[1024];
	unsigned int max_buf = sizeof(error_msg) -1;
	if (error_code != CDDB_ERR_OK)
	{
		snprintf(error_msg, max_buf, "%s failed: %s", failed_task, cddb_error_str(error_code));
	}
	else
	{
		if (possible_cause)
		{
			snprintf(error_msg, max_buf, "%s failed: Unknown reason - %s", failed_task, possible_cause);
		}
		else
		{
			snprintf(error_msg, max_buf, "%s failed: Unknown reason", failed_task);
		}
	}
	
	debugLog(error_msg);
    gdk_threads_enter();
		show_simple_dialog_message(error_msg);
	gdk_threads_leave();
}


static void disable_update_button(void)
{
    gtk_widget_set_sensitive(glb_upload_button, FALSE);
}


static void enable_update_button(void)
{
    gtk_widget_set_sensitive(glb_upload_button, TRUE);
}


static bool is_valid_email_address(const char* email_address)
{
#if HAS_POSIX_REGEX
	regex_t regex;
	int bad_regular_expression = regcomp(
			&regex,
			"^[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\\.[a-zA-Z]{2,4}$",
			REG_EXTENDED | REG_NEWLINE);
	if ( ! bad_regular_expression)
	{
        size_t nmatch = 1;
        regmatch_t pmatch[1];

        int nomatch = regexec(&regex, email_address, nmatch, pmatch, 0);
        regfree(&regex);
        if (nomatch)
        {
        	return 0;
        }
		return 1;
	}
	regfree(&regex);
	// in case of bad expression use the simple algorithm below
#endif

	// a better way would be to call regcomp(POSIX)

	int num_at = 0;				// number of '@' characters
	int pos_of_at = -1;			// position of last '@' character
	int num_dots = 0;			// number of '.' characters
	int pos_of_dot = -1;	// position of last '.' character

	int len = strlen(email_address);
	int i;
	for (i = 0; i < len; ++i)
	{
		char c = email_address[i];
		switch (c)
		{
		case '@':
			++ num_at;
			pos_of_at = i;
			break;

		case '.':
			++num_dots;
			pos_of_dot = i;
			break;

		case ' ':
		case '\t':
			return 0;	// illegal character

		default:
			break;
		}
	}
	if (num_at > 1)
	{
		return 0;	// too many '@' characters
	}
	if (num_dots == 0)
	{
		return 0;	// missing '.' character
	}
	if (pos_of_dot < pos_of_at)
	{
		return 0;	// missing '.' after '@'
	}
	return 1;
}
