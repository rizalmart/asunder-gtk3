#include <gtk/gtk.h>
#include <cddb/cddb.h>
#include <stdbool.h>

// LNR - I think the editable genre column should come before the static track time,
// but it could arguably come between artist and title so the two editable columns
// would be together. 
enum
{
    COL_RIPTRACK,
    COL_TRACKNUM,
    COL_TRACKNUM_VIS,
    COL_TRACKARTIST,
    COL_TRACKTITLE,
    COL_TRACKARTISTTITLE,
    COL_TRACKTIME,
    NUM_COLS
};

// creates a tree model that represents the data in the cddb_disc_t
GtkTreeModel * create_model_from_disc(cddb_disc_t * disc);

// open/close the drive's tray
void eject_disc(char * cdrom);

// looks up the given cddb_disc_t in the online database, and fills in the values
GList * lookup_disc(cddb_disc_t * disc);

// open connection to cddb server
// - for update specify the email address. On error NULL is returned
// - for read do not specify the email address. On error the program is terminated
cddb_conn_t * get_cddb_connection(const char* useThisEmailAddress);

// the main logic for scanning the discs
void refresh(void);

// updates all the necessary widgets with the data for the given cddb_disc_t
void update_tracklist(cddb_disc_t * disc);

// show message in the status line
extern void show_status_message(const char* msg);

// clear the status line
extern void clear_status_message();

void clear_widgets();


extern GList * gbl_disc_matches;

extern GtkWidget * win_main;
extern GtkWidget * win_prefs;
extern GtkWidget * win_ripping;
extern GtkWidget * win_about;
extern GtkWidget * win_upload;

extern GtkEntry* glb_album_artist;
extern GtkEntry* glb_album_title;
extern GtkComboBox* glb_album_category;
extern GtkEntry * glb_album_genre;
extern GtkEntry * glb_album_year;
extern GtkToggleButton * glb_single_artist_button;
extern GtkWidget * glb_tracklist;
extern GtkListStore* glb_get_tracklist_store(); // return store of glb_tracklist
extern GtkWidget * glb_upload_button;

extern int gbl_null_fd;

typedef enum JoinState
{
    JOIN_UNSET,
    JOIN_OFF,
    JOIN_ON
} JoinState;

extern JoinState join_artist_title;

#define EMPTY_YEAR_SHOWN	1900
#define EMPTY_ARTIST_SHOWN	"Unknown Artist"
#define EMPTY_ALBUM_SHOWN	"Unknown Album"
#define EMPTY_GENRE_SHOWN	"Unknown"
#define EMPTY_TRACK_SHOWN   "Track %d"


//#define DEBUG
