#include <stdbool.h>
#include <cddb/cddb.h>

#define PROGRAM_NAME "Asunder"
#define PROGRAM_VERSION "3.0.1"

// Lookup pointers to GUI elements:
#define SINGLE_ARTIST_BUTTON		"single_artist"
#define ALBUM_CATEGORY_LABEL		"album_category_lbl"
#define ALBUM_CATEGORY_COMBO_BOX	"album_category"
#define UPLOAD_BUTTON				"upload_button"

extern const char* GBLprogramName;

GtkWidget* create_main (void);
GtkWidget* create_prefs (void);
GtkWidget* create_ripping (void);

void disable_all_main_widgets(void);
void enable_all_main_widgets(void);
void disable_flac_widgets(void);
void enable_flac_widgets(void);
void disable_mp3_widgets(void);
void enable_mp3_widgets(void);
void disable_ogg_widgets(void);
void enable_ogg_widgets(void);
void disable_opus_widgets(void);
void enable_opus_widgets(void);
void disable_wavpack_widgets(void);
void enable_wavpack_widgets(void);
void disable_monkey_widgets(void);
void enable_monkey_widgets(void);
void disable_musepack_widgets(void);
void enable_musepack_widgets(void);
void disable_fdkaac_widgets(void);
void enable_fdkaac_widgets(void);
void show_aboutbox (void);
void show_completed_dialog(int numOk, int numFailed);
void toggle_allow_tracknum(void);

void show_simple_dialog_message(const char* msg);
