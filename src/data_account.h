/* START_DECLARATION */
gchar *gsb_account_get_id ( gint no_account );
gint gsb_account_get_nb_rows ( gint no_account );
gboolean gsb_account_get_r ( gint no_account );
gboolean gsb_account_set_id ( gint no_account,
			      gchar *id );
gboolean gsb_account_set_nb_rows ( gint no_account,
				   gint nb_rows );
gboolean gsb_account_set_r ( gint no_account,
			     gboolean show_r );
/* END_DECLARATION */
