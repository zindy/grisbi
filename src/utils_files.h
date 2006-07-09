#ifndef _H_UTILS_FILES
#define _H_UTILS_FILES 1

#include "config.h"
#include <sys/stat.h>

/* START_INCLUDE_H */
#include "utils_files.h"
/* END_INCLUDE_H */


/*START_DECLARATION*/
gint get_line_from_file ( FILE *fichier,
			  gchar **string );
GtkWidget * my_file_chooser ();
gchar* my_get_grisbirc_dir(void);
gchar* my_get_gsb_file_default_dir(void);
gchar * safe_file_name ( gchar* filename );
FILE* utf8_fopen(gchar* utf8filename,gchar* mode);
gint utf8_remove(const gchar* utf8filename);
gint utf8_stat(gchar* utf8filename,struct stat* filestat);
/*END_DECLARATION*/

#endif
