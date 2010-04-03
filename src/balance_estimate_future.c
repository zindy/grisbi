/* ************************************************************************** */
/*                                                                            */
/*     Copyright (C) 2007 Dominique Parisot                                   */
/*          zionly@free.org                                                   */
/*          2008-2010 Pierre Biava (grisbi@pierre.biava.name)                 */
/*          http://www.grisbi.org                                             */
/*                                                                            */
/*  This program is free software; you can redistribute it and/or modify      */
/*  it under the terms of the GNU General Public License as published by      */
/*  the Free Software Foundation; either version 2 of the License, or         */
/*  (at your option) any later version.                                       */
/*                                                                            */
/*  This program is distributed in the hope that it will be useful,           */
/*  but WITHOUT ANY WARRANTY; without even the implied warranty of            */
/*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the             */
/*  GNU General Public License for more details.                              */
/*                                                                            */
/*  You should have received a copy of the GNU General Public License         */
/*  along with this program; if not, write to the Free Software               */
/*  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA */
/*                                                                            */
/* ************************************************************************** */

/* ./configure --with-balance-estimate */

#include "include.h"
#include <config.h>

#ifdef ENABLE_BALANCE_ESTIMATE

/*START_INCLUDE*/
#include "balance_estimate_future.h"
#include "./balance_estimate_tab.h"
#include "./balance_estimate_config.h"
#include "./balance_estimate_data.h"
#include "./balance_estimate_hist.h"
#include "./gsb_combo_box.h"
#include "./dialog.h"
#include "./gsb_form.h"
#include "./gsb_form_scheduler.h"
#include "./gsb_form_widget.h"
#include "./gsb_data_form.h"
#include "./utils_dates.h"
#include "./gsb_data_account.h"
#include "./gsb_data_budget.h"
#include "./gsb_calendar_entry.h"
#include "./gsb_data_category.h"
#include "./gsb_data_fyear.h"
#include "./gsb_data_payee.h"
#include "./gsb_data_payment.h"
#include "./gsb_data_scheduled.h"
#include "./gsb_data_transaction.h"
#include "./gsb_fyear.h"
#include "./gsb_real.h"
#include "./gsb_scheduler.h"
#include "./gsb_payment_method.h"
#include "./gsb_transactions_list_sort.h"
#include "./gtk_combofix.h"
#include "./main.h"
#include "./mouse.h"
#include "./navigation.h"
#include "./include.h"
#include "./structures.h"
#include "./utils_editables.h"
#include "./traitement_variables.h"
#include "./erreur.h"
#include "./utils.h"
/*END_INCLUDE*/


/*START_STATIC*/
static gboolean bet_form_button_press_event ( GtkWidget *entry,
                        GdkEventButton *ev,
                        gint *ptr_origin );
static gboolean bet_form_clean ( gint account_number );
static gboolean bet_form_create_current_form ( GtkWidget *table );
static gboolean bet_form_create_scheduler_part ( GtkWidget *table );
static gboolean bet_form_entry_get_focus ( GtkWidget *entry );
static gboolean bet_form_entry_lose_focus ( GtkWidget *entry,
                        GdkEventFocus *ev,
                        gint *ptr_origin );
static gboolean bet_form_key_press_event ( GtkWidget *widget,
                        GdkEventKey *ev,
                        gint *ptr_origin );
static gboolean bet_form_scheduler_frequency_button_changed ( GtkWidget *combo_box,
						       gpointer null );
static GtkWidget *bet_form_scheduler_get_element_widget ( gint element_number );
static GtkWidget *bet_form_widget_get_widget ( gint element_number );
static gboolean bet_future_create_dialog ( void );
static gboolean bet_future_get_budget_data ( GtkWidget *widget,
                        gint budget_type,
                        struct_futur_data *scheduled );
static gboolean bet_future_get_category_data ( GtkWidget *widget,
                        gint budget_type,
                        struct_futur_data *scheduled );
static gboolean bet_future_set_form_data_from_line ( gint account_number,
                        gint number  );
static gboolean bet_future_take_data_from_form (  struct_futur_data *scheduled );
/*END_STATIC*/

/*START_EXTERN*/
extern gboolean balances_with_scheduled;
extern gchar* bet_duration_array[];
extern GdkColor couleur_fond[0];
extern GtkWidget *notebook_general;
extern gsb_real null_real;
extern GtkWidget *window;
/*END_EXTERN*/

#define BET_SCHEDULED_WIDTH 4
#define BET_SCHEDULED_HEIGHT 2


/** contains the list of the scheduled elements, ie list of link
 * between an element number and the pointer of its widget
 * for now, this list if filled at the opening of grisbi and never erased */
static GSList *bet_schedul_element_list = NULL;

/** contains a list of struct_element according to the current form */
static GSList *bet_form_list_widgets = NULL;

static GtkWidget *bet_dialog = NULL;

/**
 *
 *
 *
 *
 * */
gboolean bet_future_new_line_dialog ( GtkTreeModel *tab_model,
                        gchar *str_date )
{
    GtkWidget *widget;
    gchar *tmp_str;
    GDate *date;
    GDate *date_jour;
    gint result;

    if ( bet_dialog == NULL )
    {
        bet_future_create_dialog ( );
    }
    else
    {
        bet_form_clean ( gsb_gui_navigation_get_current_account ( ) );
        gtk_widget_show ( bet_dialog );
    }

    /* init data */
    widget = bet_form_widget_get_widget ( TRANSACTION_FORM_DATE );
    date = gsb_parse_date_string ( str_date );
    date_jour = gdate_today ( ); 
        
    if ( g_date_valid ( date ) )
    {
        if ( g_date_compare ( date_jour, date ) >= 0 )
        {
            g_date_free ( date );
            g_date_add_days ( date_jour, 1 );
            date = date_jour;
        }
    }
    else
    {
        g_date_add_days ( date_jour, 1 );
        date = date_jour;
    }

    gsb_form_widget_set_empty ( widget, FALSE );
    gsb_calendar_entry_set_date ( widget, date );

    gtk_dialog_set_response_sensitive ( GTK_DIALOG ( bet_dialog ), GTK_RESPONSE_OK, FALSE );

dialog_return:
	result = gtk_dialog_run ( GTK_DIALOG ( bet_dialog ));

    if ( result == GTK_RESPONSE_OK )
    {
        struct_futur_data *scheduled;

        scheduled = struct_initialise_bet_future ( );

        if ( !scheduled )
        {
            dialogue_error_memory ();
            gtk_widget_hide ( bet_dialog );
            return FALSE;
        }

        if ( bet_future_take_data_from_form ( scheduled ) == FALSE )
        {
            tmp_str = g_strdup ( _("Error: the frequency defined by the user or the amount is "
                                 "not specified or the date is invalid.") );
            dialogue_warning_hint ( tmp_str, _("One field is not filled in") );
            g_free ( tmp_str );
            goto dialog_return;
        }
        else
            bet_data_future_add_lines ( scheduled );

        bet_array_refresh_estimate_tab ( );
    }

    gtk_widget_hide ( bet_dialog );

    return FALSE;
}


/**
 *
 *
 *
 *
 * */
gboolean bet_future_create_dialog ( void )
{
    GtkWidget *vbox;
    GtkWidget *table;

    /* Create the dialog */
    bet_dialog = gtk_dialog_new_with_buttons ( _("Enter a budget line"),
					   GTK_WINDOW ( window ),
					   GTK_DIALOG_MODAL,
					   GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
					   GTK_STOCK_OK, GTK_RESPONSE_OK,
					   NULL );

    gtk_window_set_position ( GTK_WINDOW ( bet_dialog ), GTK_WIN_POS_CENTER_ON_PARENT );
    gtk_window_set_resizable ( GTK_WINDOW ( bet_dialog ), TRUE );
    gtk_dialog_set_default_response ( GTK_DIALOG ( bet_dialog ), GTK_RESPONSE_OK );

	vbox = gtk_vbox_new ( FALSE, 0 );
	gtk_box_pack_start ( GTK_BOX ( GTK_DIALOG ( bet_dialog )->vbox ), vbox, TRUE, TRUE, 0 );
	gtk_container_set_border_width ( GTK_CONTAINER ( vbox ), 12 );

    /* next we fill the bet_form */
    table = gtk_table_new ( BET_SCHEDULED_HEIGHT, BET_SCHEDULED_WIDTH, FALSE );
    gtk_table_set_col_spacings ( GTK_TABLE ( table ), 6 );
    gtk_widget_show ( table );
    gtk_box_pack_start ( GTK_BOX ( vbox ), table, FALSE, FALSE, 5 );

    bet_form_create_scheduler_part ( table );
    bet_form_create_current_form ( table );

	gtk_widget_show ( vbox );

    return FALSE;
 }


/**
 * create the scheduled part
 *
 * \param table a GtkTable with the dimension SCHEDULED_HEIGHT*SCHEDULED_WIDTH to be filled
 *
 * \return FALSE
 * */
gboolean bet_form_create_scheduler_part ( GtkWidget *table )
{
    
    GtkWidget *separator;
    GtkWidget *combo = NULL;
    gint column;

    devel_debug (NULL);
    if (!table)
        return FALSE;

    /* ok, now fill the form 
     * we play with height and width, but for now it's fix : 6 columns and 1 line */
	for ( column=0 ; column < SCHEDULED_WIDTH ; column++ )
	{
        GtkWidget *widget = NULL;
	    const gchar *tooltip_text = NULL;
	    gchar *text_frequency [] = { _("Once"), _("Weekly"), _("Monthly"), _("Bimonthly"),
                        _("Quarterly"), _("Yearly"), _("Custom"), NULL };
	    gchar *text_frequency_user [] = { _("Days"), _("Weeks"), _("Months"), _("Years"), NULL };
        gint element_number;
        struct_element *element;
        /* on tient compte que le premier widget utile est le troisième du formulaire */
	    element_number = column + 2;

	    switch ( element_number )
	    {
		case SCHEDULED_FORM_FREQUENCY_BUTTON:
		    widget = gsb_combo_box_new_with_index ( text_frequency,
                        G_CALLBACK ( bet_form_scheduler_frequency_button_changed ),
                        NULL );
            combo = widget;
		    tooltip_text = SPACIFY( _("Frequency") );
            gtk_widget_show ( widget );
		    break;

        case SCHEDULED_FORM_LIMIT_DATE:
            widget = gsb_calendar_entry_new ( FALSE );
            gsb_form_widget_set_empty ( widget, TRUE );
            gtk_entry_set_text ( GTK_ENTRY ( widget ),
					 _("Limit date") );
            g_signal_connect ( G_OBJECT (widget),
                        "button-press-event",
                        G_CALLBACK ( gsb_form_scheduler_button_press_event ),
                        GINT_TO_POINTER (element_number));
            g_signal_connect ( G_OBJECT (widget),
                        "focus-in-event",
                        G_CALLBACK ( gsb_form_entry_get_focus ),
                        GINT_TO_POINTER (element_number));
            g_signal_connect_after ( G_OBJECT (widget),
                        "focus-out-event",
                        G_CALLBACK ( gsb_form_scheduler_entry_lose_focus ),
                        GINT_TO_POINTER (element_number));
            tooltip_text = SPACIFY( _("Final date") );
            gtk_widget_show ( widget );
            break;

		case SCHEDULED_FORM_FREQUENCY_USER_ENTRY:
		    widget = gtk_entry_new ();
            gtk_entry_set_text ( GTK_ENTRY ( widget ),
					    _("Own frequency") );
            gsb_form_widget_set_empty ( widget, TRUE );
            g_signal_connect ( G_OBJECT (widget),
                        "focus-in-event",
                        G_CALLBACK (gsb_form_entry_get_focus),
                        GINT_TO_POINTER (element_number));
            g_signal_connect_after ( G_OBJECT (widget),
                        "focus-out-event",
                        G_CALLBACK ( gsb_form_scheduler_entry_lose_focus),
                        GINT_TO_POINTER (element_number));
		    tooltip_text = SPACIFY( _("Custom frequency") );
		    break;

		case SCHEDULED_FORM_FREQUENCY_USER_BUTTON:
		    widget = gsb_combo_box_new_with_index ( text_frequency_user,
							    NULL, NULL );
		    tooltip_text = SPACIFY( _("Custom frequency") );
            gsb_combo_box_set_index ( widget, 2 );
		    break;
	    }

	    if (!widget)
            continue;

	    if (tooltip_text)
            gtk_widget_set_tooltip_text ( GTK_WIDGET (widget),
                        tooltip_text);

        /* save the element */
	    element = g_malloc0 ( sizeof ( struct_element ) );
	    element -> element_number = element_number;
	    element -> element_widget = widget;
	    bet_schedul_element_list = g_slist_append ( bet_schedul_element_list, element );

	    /* set in the form */
        gtk_table_attach ( GTK_TABLE (table),
                        widget,
                        column, column+1,
                        0, 1,
                        GTK_EXPAND | GTK_FILL,
                        GTK_EXPAND | GTK_FILL,
                        0, 0 );
	}

    separator = gtk_hseparator_new ( );
    gtk_widget_show ( separator );
    gtk_table_attach ( GTK_TABLE (table),
                        separator,
                        0, 4,
                        1, 2,
                        GTK_EXPAND | GTK_FILL,
                        GTK_EXPAND | GTK_FILL,
                        0, 6);

    gsb_combo_box_set_index ( combo, 0 );

    return FALSE;
}


/**
 * fill the form according to the account_number :
 *
 * \param account_number the number of account
 * 
 * \return FALSE
 * */
gboolean bet_form_create_current_form ( GtkWidget *table )
{
	GtkWidget *widget;
    GtkWidget *credit;
    gint element_number;
    gint width = 250;
    gint row = 2;
    gint column = 0;
    gint account_number;
    struct_element *element;
    GSList *tmp_list;

    account_number = gsb_gui_navigation_get_current_account ( );

    element_number = TRANSACTION_FORM_DATE;
    widget = gsb_calendar_entry_new ( FALSE );
    gtk_widget_show ( widget );
    gtk_table_attach ( GTK_TABLE ( table ),
                        widget,
                        column, column+1,
                        row, row+1,
                        gsb_form_get_element_expandable ( element_number ),
                        gsb_form_get_element_expandable ( element_number ),
                        0, 0);
    element = g_malloc0 ( sizeof ( struct_element ) );
    element -> element_number = element_number;
    element -> element_widget = widget;
    bet_form_list_widgets = g_slist_append ( bet_form_list_widgets, element );
    column ++;

    element_number = TRANSACTION_FORM_PARTY;
    widget = gtk_combofix_new_complex (
                        gsb_data_payee_get_name_and_report_list ( ) );
    gtk_widget_set_size_request ( widget, width, -1 );
    gtk_combofix_set_force_text ( GTK_COMBOFIX (widget),
					  etat.combofix_force_payee );
    gtk_combofix_set_max_items ( GTK_COMBOFIX (widget),
					 etat.combofix_max_item );
    gtk_combofix_set_case_sensitive ( GTK_COMBOFIX (widget),
					      etat.combofix_case_sensitive );
    //~ gtk_combofix_set_enter_function ( GTK_COMBOFIX (widget),
					      //~ etat.combofix_enter_select_completion );
    /* we never mix the payee because the only case of the complex combofix is
     * for the report and there is non sense to mix report with the payee */
    gtk_combofix_set_mixed_sort ( GTK_COMBOFIX (widget),
					  FALSE );
    gtk_widget_show ( widget );
    gtk_table_attach ( GTK_TABLE ( table ),
                        widget,
                        column, column+1,
                        row, row+1,
                        gsb_form_get_element_expandable ( element_number ),
                        gsb_form_get_element_expandable ( element_number ),
                        0, 0);
    element = g_malloc0 ( sizeof ( struct_element ) );
    element -> element_number = element_number;
    element -> element_widget = widget;
    bet_form_list_widgets = g_slist_append ( bet_form_list_widgets, element );
    column ++;

    element_number = TRANSACTION_FORM_DEBIT;
    widget = gtk_entry_new ( );
    g_object_set_data ( G_OBJECT ( widget ), "element_number",
                        GINT_TO_POINTER ( TRANSACTION_FORM_DEBIT ) );
    g_signal_connect ( G_OBJECT ( widget ),
		                "changed",
		                G_CALLBACK ( gsb_form_widget_amount_entry_changed ),
		                NULL );
    gtk_widget_show ( widget );
    gtk_table_attach ( GTK_TABLE ( table ),
                        widget,
                        column, column+1,
                        row, row+1,
                        gsb_form_get_element_expandable ( element_number ),
                        gsb_form_get_element_expandable ( element_number ),
                        0, 0);
    element = g_malloc0 ( sizeof ( struct_element ) );
    element -> element_number = element_number;
    element -> element_widget = widget;
    bet_form_list_widgets = g_slist_append ( bet_form_list_widgets, element );
    column ++;

    element_number = TRANSACTION_FORM_CREDIT;
    widget = gtk_entry_new ( );
    credit = widget;
    g_object_set_data ( G_OBJECT ( widget ), "element_number",
                        GINT_TO_POINTER ( TRANSACTION_FORM_CREDIT ) );
    g_signal_connect ( G_OBJECT ( widget ),
		                "changed",
		                G_CALLBACK ( gsb_form_widget_amount_entry_changed ),
		                NULL );
    gtk_widget_show ( widget );
    gtk_table_attach ( GTK_TABLE ( table ),
                        widget,
                        column, column+1,
                        row, row+1,
                        gsb_form_get_element_expandable ( element_number ),
                        gsb_form_get_element_expandable ( element_number ),
                        0, 0);
    element = g_malloc0 ( sizeof ( struct_element ) );
    element -> element_number = element_number;
    element -> element_widget = widget;
    bet_form_list_widgets = g_slist_append ( bet_form_list_widgets, element );
    column = 0;
    row ++; 

    element_number = TRANSACTION_FORM_EXERCICE;
    widget = gsb_fyear_make_combobox (TRUE);
    gtk_widget_set_tooltip_text ( GTK_WIDGET (widget),
					  SPACIFY(_("Choose the financial year")));
    gtk_widget_show ( widget );
    gtk_table_attach ( GTK_TABLE ( table ),
                        widget,
                        column, column+1,
                        row, row+1,
                        gsb_form_get_element_expandable ( element_number ),
                        gsb_form_get_element_expandable ( element_number ),
                        0, 0);
    element = g_malloc0 ( sizeof ( struct_element ) );
    element -> element_number = element_number;
    element -> element_widget = widget;
    bet_form_list_widgets = g_slist_append ( bet_form_list_widgets, element );
    column ++;

    element_number = TRANSACTION_FORM_CATEGORY;
    widget = gtk_combofix_new_complex (
                         gsb_data_category_get_name_list ( TRUE, TRUE, FALSE, FALSE ) );
    gtk_widget_set_size_request ( widget, width, -1 );
    gtk_combofix_set_force_text ( GTK_COMBOFIX (widget),
					  etat.combofix_force_category );
    gtk_combofix_set_max_items ( GTK_COMBOFIX (widget),
					 etat.combofix_max_item );
    gtk_combofix_set_case_sensitive ( GTK_COMBOFIX (widget),
					      etat.combofix_case_sensitive );
    //~ gtk_combofix_set_enter_function ( GTK_COMBOFIX (widget),
					      //~ etat.combofix_enter_select_completion );
    gtk_combofix_set_mixed_sort ( GTK_COMBOFIX (widget),
					  etat.combofix_mixed_sort );
    gtk_widget_show ( widget );
    gtk_table_attach ( GTK_TABLE ( table ),
                        widget,
                        column, column+1,
                        row, row+1,
                        gsb_form_get_element_expandable ( element_number ),
                        gsb_form_get_element_expandable ( element_number ),
                        0, 0);
    element = g_malloc0 ( sizeof ( struct_element ) );
    element -> element_number = element_number;
    element -> element_widget = widget;
    bet_form_list_widgets = g_slist_append ( bet_form_list_widgets, element );
    column ++;

    element_number = TRANSACTION_FORM_TYPE;
    widget = gtk_combo_box_new ();
    gsb_payment_method_create_combo_list ( widget,
                        GSB_PAYMENT_DEBIT,
                        account_number, 0 );
    gtk_combo_box_set_active ( GTK_COMBO_BOX (widget), 0 );
    gtk_widget_set_tooltip_text ( GTK_WIDGET (widget),
                        SPACIFY(_("Choose the method of payment")));
    gtk_widget_show ( widget );
    gtk_table_attach ( GTK_TABLE ( table ),
                        widget,
                        column, column+1,
                        row, row+1,
                        gsb_form_get_element_expandable ( element_number ),
                        gsb_form_get_element_expandable ( element_number ),
                        0, 0);

    element = g_malloc0 ( sizeof ( struct_element ) );
    element -> element_number = element_number;
    element -> element_widget = widget;
    bet_form_list_widgets = g_slist_append ( bet_form_list_widgets, element );
    column = 1;
    row ++; 

    element_number = TRANSACTION_FORM_BUDGET;
    widget = gtk_combofix_new_complex (
                        gsb_data_budget_get_name_list (TRUE, TRUE));
    gtk_widget_set_size_request ( widget, width, -1 );
    gtk_combofix_set_force_text ( GTK_COMBOFIX (widget),
					  etat.combofix_force_category );
    gtk_combofix_set_max_items ( GTK_COMBOFIX (widget),
					 etat.combofix_max_item );
    gtk_combofix_set_case_sensitive ( GTK_COMBOFIX (widget),
					      etat.combofix_case_sensitive );
    //~ gtk_combofix_set_enter_function ( GTK_COMBOFIX (widget),
					      //~ etat.combofix_enter_select_completion );
    gtk_combofix_set_mixed_sort ( GTK_COMBOFIX (widget),
					  etat.combofix_mixed_sort );
    gtk_widget_show ( widget );
    gtk_table_attach ( GTK_TABLE ( table ),
                        widget,
                        column, column+1,
                        row, row+1,
                        gsb_form_get_element_expandable ( element_number ),
                        gsb_form_get_element_expandable ( element_number ),
                        0, 0);
    element = g_malloc0 ( sizeof ( struct_element ) );
    element -> element_number = element_number;
    element -> element_widget = widget;
    bet_form_list_widgets = g_slist_append ( bet_form_list_widgets, element );
    column = 0;
    row ++;

    element_number = TRANSACTION_FORM_NOTES;
	widget = gtk_entry_new();
    gtk_widget_show ( widget );
    gtk_table_attach ( GTK_TABLE ( table ),
                        widget,
                        column, column+4,
                        row, row+1,
                        gsb_form_get_element_expandable ( element_number ),
                        gsb_form_get_element_expandable ( element_number ),
                        0, 2);
    element = g_malloc0 ( sizeof ( struct_element ) );
    element -> element_number = element_number;
    element -> element_widget = widget;
    bet_form_list_widgets = g_slist_append ( bet_form_list_widgets, element );

    tmp_list = bet_form_list_widgets;

    while ( tmp_list )
    {
        struct_element*element;

        element = tmp_list -> data;

        widget = element -> element_widget;
        if ( GTK_IS_ENTRY ( widget ))
        {
            g_signal_connect ( G_OBJECT ( widget ),
                       "focus-in-event",
                       G_CALLBACK ( bet_form_entry_get_focus ),
                       GINT_TO_POINTER ( element -> element_number ) );
            g_signal_connect ( G_OBJECT ( widget ),
                       "focus-out-event",
                       G_CALLBACK ( bet_form_entry_lose_focus ),
                       GINT_TO_POINTER ( element -> element_number ) );
            g_signal_connect ( G_OBJECT ( widget ),
                       "button-press-event",
                       G_CALLBACK ( bet_form_button_press_event ),
                       GINT_TO_POINTER ( element -> element_number ) );
            g_signal_connect ( G_OBJECT ( widget ),
                       "key-press-event",
                       G_CALLBACK ( bet_form_key_press_event ),
                       GINT_TO_POINTER ( element -> element_number ) );
        }
        else
        {
            if ( GTK_IS_COMBOFIX ( widget ))
            {
                g_signal_connect ( G_OBJECT ( GTK_COMBOFIX ( widget ) -> entry ),
                           "focus-in-event",
                           G_CALLBACK ( bet_form_entry_get_focus ),
                           GINT_TO_POINTER ( element -> element_number ) );
                g_signal_connect ( G_OBJECT ( GTK_COMBOFIX (widget ) -> entry ),
                           "focus-out-event",
                           G_CALLBACK ( bet_form_entry_lose_focus ),
                           GINT_TO_POINTER ( element -> element_number ) );
                g_signal_connect ( G_OBJECT ( GTK_COMBOFIX (widget ) -> entry ),
                           "button-press-event",
                           G_CALLBACK ( bet_form_button_press_event ),
                           GINT_TO_POINTER ( element -> element_number ) );
                g_signal_connect ( G_OBJECT ( GTK_COMBOFIX (widget ) -> entry ),
                           "key-press-event",
                           G_CALLBACK ( bet_form_key_press_event ),
                           GINT_TO_POINTER ( element -> element_number ) );
            }
            else
            /* neither an entry, neither a combofix */
            g_signal_connect ( G_OBJECT ( widget ),
                       "key-press-event",
                       G_CALLBACK ( bet_form_key_press_event ),
                       GINT_TO_POINTER ( element -> element_number ) );
        }
        tmp_list = tmp_list -> next;
    }

    bet_form_clean ( account_number );

    return FALSE;
}


/**
 * clean the form according to the account_number
 * and set the default values
 *
 * \param account number
 *
 * \return FALSE
 * */
gboolean bet_form_clean ( gint account_number )
{
    GSList *tmp_list;

    /* clean the scheduled widget */
    tmp_list = bet_schedul_element_list;

    while (tmp_list)
    {
        struct_element *element;

        element = tmp_list -> data;

        /* better to protect here if widget != NULL (bad experience...) */
        if (element -> element_widget)
            gtk_widget_set_sensitive ( element -> element_widget, TRUE );

        tmp_list = tmp_list -> next;
    }
    
    /* clean the transactions widget */
    tmp_list = bet_form_list_widgets;

    while (tmp_list)
    {
        struct_element *element;

        element = tmp_list -> data;

        /* better to protect here if widget != NULL (bad experience...) */
        if (element -> element_widget)
        {
            /* some widgets can be set unsensitive because of the children of splits,
             * so resensitive all to be sure */
            gtk_widget_set_sensitive ( element -> element_widget, TRUE );

            switch (element -> element_number)
            {
            case TRANSACTION_FORM_DATE:
                if ( !strlen ( gtk_entry_get_text ( GTK_ENTRY (
                 element -> element_widget ) ) ) )
                {
                    gsb_form_widget_set_empty ( element -> element_widget, TRUE );
                    gtk_entry_set_text ( GTK_ENTRY ( element -> element_widget ), _("Date") );
                }
                break;

            case TRANSACTION_FORM_EXERCICE:
                /* editing a transaction can show some fyear wich shouldn't be showed,
                 * so hide them here */
                gsb_fyear_update_fyear_list ();

                /* set the combo_box on 'Automatic' */
                gsb_fyear_set_combobox_history ( element -> element_widget, 0 );

                break;

            case TRANSACTION_FORM_PARTY:
                gsb_form_widget_set_empty ( GTK_COMBOFIX ( element -> element_widget ) -> entry,
                            TRUE );
                gtk_combofix_set_text ( GTK_COMBOFIX ( element -> element_widget ),
                            _("Payee") );
                break;

            case TRANSACTION_FORM_DEBIT:
                gsb_form_widget_set_empty ( element -> element_widget,
                            TRUE );
                gtk_entry_set_text ( GTK_ENTRY ( element -> element_widget ),
                         _("Debit") );
                break;

            case TRANSACTION_FORM_CREDIT:
                gsb_form_widget_set_empty ( element -> element_widget,
                            TRUE );
                gtk_entry_set_text ( GTK_ENTRY ( element -> element_widget ),
                         _("Credit") );
                break;

            case TRANSACTION_FORM_CATEGORY:
                gsb_form_widget_set_empty ( GTK_COMBOFIX ( element -> element_widget ) -> entry,
                            TRUE );
                gtk_combofix_set_text ( GTK_COMBOFIX ( element -> element_widget ),
                            _("Categories : Sub-categories") );
                break;

            case TRANSACTION_FORM_BUDGET:
                gsb_form_widget_set_empty ( GTK_COMBOFIX ( element -> element_widget ) -> entry,
                            TRUE );
                gtk_combofix_set_text ( GTK_COMBOFIX ( element -> element_widget ),
                            _("Budgetary line") );
                break;

            case TRANSACTION_FORM_NOTES:
                gsb_form_widget_set_empty ( element -> element_widget,
                            TRUE );
                gtk_entry_set_text ( GTK_ENTRY ( element -> element_widget ),
                         _("Notes") );
                break;

            case TRANSACTION_FORM_TYPE:
                gsb_payment_method_set_combobox_history ( element -> element_widget,
                                      gsb_data_account_get_default_debit (account_number));
                break;

            }
        }
        tmp_list = tmp_list -> next;
    }

    return FALSE;
}


/**
 * called when the frequency button is changed
 * show/hide the necessary widget according to its state
 *
 * \param combo_box
 *
 * \return FALSE 
 * */
gboolean bet_form_scheduler_frequency_button_changed ( GtkWidget *combo_box,
						       gpointer null )
{
    gchar *selected_item;

    selected_item = gtk_combo_box_get_active_text ( GTK_COMBO_BOX ( combo_box ) );
    
    if ( !strcmp ( selected_item, _("Once") ) )
    {
        gtk_widget_hide ( bet_form_scheduler_get_element_widget (
                        SCHEDULED_FORM_LIMIT_DATE ) );
        gtk_widget_hide ( bet_form_scheduler_get_element_widget (
                        SCHEDULED_FORM_FREQUENCY_USER_ENTRY ) );
        gtk_widget_hide ( bet_form_scheduler_get_element_widget (
                        SCHEDULED_FORM_FREQUENCY_USER_BUTTON ) );
    }
    else
    {
        gtk_widget_show ( bet_form_scheduler_get_element_widget (
                        SCHEDULED_FORM_LIMIT_DATE ) );

        if ( !strcmp ( selected_item, _("Custom") ) )
        {
            gtk_widget_show ( bet_form_scheduler_get_element_widget (
                        SCHEDULED_FORM_FREQUENCY_USER_ENTRY));
            gtk_widget_show ( bet_form_scheduler_get_element_widget (
                        SCHEDULED_FORM_FREQUENCY_USER_BUTTON ) );
        }
        else
        {
            gtk_widget_hide ( gsb_form_scheduler_get_element_widget (
                        SCHEDULED_FORM_FREQUENCY_USER_ENTRY ) );
            gtk_widget_hide ( gsb_form_scheduler_get_element_widget (
                        SCHEDULED_FORM_FREQUENCY_USER_BUTTON ) );
        }
    }
    g_free ( selected_item );
    gtk_dialog_set_response_sensitive ( GTK_DIALOG ( bet_dialog ), GTK_RESPONSE_OK, TRUE );

    return FALSE;
}


/**
 * called when an entry get the focus, if the entry is free,
 * set it normal and erase the help content
 *
 * \param entry
 *
 * \return FALSE
 * */
gboolean bet_form_entry_get_focus ( GtkWidget *entry )
{
    /* the entry can be a combofix or a real entry */
    if (GTK_IS_COMBOFIX ( entry ))
    {
        if ( gsb_form_widget_check_empty (GTK_COMBOFIX (entry) -> entry))
        {
            gtk_combofix_set_text ( GTK_COMBOFIX (entry), "" );
            gsb_form_widget_set_empty ( GTK_COMBOFIX (entry) -> entry,
                        FALSE );
        }
    }
    else
    {
        if ( gsb_form_widget_check_empty (entry) )
        {
            gtk_entry_set_text ( GTK_ENTRY (entry), "" );
            gsb_form_widget_set_empty ( entry,
                        FALSE );
        }
    }
    /* sensitive the valid button */
    gtk_dialog_set_response_sensitive ( GTK_DIALOG ( bet_dialog ), GTK_RESPONSE_OK, TRUE );

    return FALSE;
}


/**
 * called when an entry lose the focus
 *
 * \param entry
 * \param ev
 * \param ptr_origin a pointer gint wich is the number of the element
 *
 * \return FALSE
 * */
gboolean bet_form_entry_lose_focus ( GtkWidget *entry,
                        GdkEventFocus *ev,
                        gint *ptr_origin )
{
    GtkWidget *widget;
    gchar *string;
    gint element_number;
    gint account_number;

    /* still not found, if change the content of the form, something come in entry
     * wich is nothing, so protect here */
    if ( !GTK_IS_WIDGET ( entry )
     ||
     !GTK_IS_ENTRY ( entry ))
        return FALSE;

    /* remove the selection */
    gtk_editable_select_region ( GTK_EDITABLE ( entry ), 0, 0 );
    element_number = GPOINTER_TO_INT ( ptr_origin );
    account_number = gsb_form_get_account_number ();

    /* sometimes the combofix popus stays showed, so remove here */
    if ( element_number == TRANSACTION_FORM_PARTY
     ||
     element_number == TRANSACTION_FORM_CATEGORY
     ||
     element_number == TRANSACTION_FORM_BUDGET )
    {
        widget = bet_form_widget_get_widget ( element_number );
        gtk_grab_remove ( GTK_COMBOFIX ( widget ) -> popup );
        gdk_pointer_ungrab ( GDK_CURRENT_TIME );
        gtk_widget_hide ( GTK_COMBOFIX ( widget ) ->popup );
    }

    /* string will be filled only if the field is empty */
    string = NULL;
    switch ( element_number )
    {
    case TRANSACTION_FORM_DATE :
        if ( !strlen ( gtk_entry_get_text ( GTK_ENTRY ( entry ) ) ) )
            string = gsb_form_widget_get_name ( TRANSACTION_FORM_DATE );
        break;

    case TRANSACTION_FORM_PARTY :
        if ( !strlen ( gtk_entry_get_text ( GTK_ENTRY ( entry ) ) ) )
            string = gsb_form_widget_get_name ( TRANSACTION_FORM_PARTY );
        break;

    case TRANSACTION_FORM_DEBIT :
        /* we change the payment method to adapt it for the debit */
        if ( strlen ( gtk_entry_get_text ( GTK_ENTRY ( entry ) ) ) )
        {
        /* empty the credit */
        widget = bet_form_widget_get_widget ( TRANSACTION_FORM_CREDIT );
        if ( !gsb_form_widget_check_empty ( widget ) )
        {
            gtk_entry_set_text ( GTK_ENTRY ( widget ),
                        gsb_form_widget_get_name ( TRANSACTION_FORM_CREDIT ) );
            gsb_form_widget_set_empty ( widget, TRUE );
        }

        widget = bet_form_widget_get_widget ( TRANSACTION_FORM_TYPE );

        /* change the method of payment if necessary
         * (if grey, it's a child of split so do nothing) */
        if ( widget
            &&
            GTK_WIDGET_SENSITIVE ( widget ) )
        {
            /* change the signe of the method of payment and the contra */
            if ( gsb_payment_method_get_combo_sign ( widget ) == GSB_PAYMENT_CREDIT )
            {
                gsb_payment_method_create_combo_list ( widget,
                                            GSB_PAYMENT_DEBIT,
                                            account_number, 0 );
            }
        }
        gsb_form_check_auto_separator (entry);
        }
        else
        {
            /* si pas de nouveau débit on essaie de remettre l'ancien crédit */
            if ( (string = gsb_form_widget_get_old_credit ( ) ) )
            {
                GtkWidget *widget_prov;

                widget_prov = bet_form_widget_get_widget ( TRANSACTION_FORM_CREDIT );

                gtk_entry_set_text ( GTK_ENTRY ( widget_prov ), string );
                gsb_form_widget_set_empty ( widget_prov, FALSE );
                g_free ( string );
                
                widget = bet_form_widget_get_widget ( TRANSACTION_FORM_TYPE );
                if ( widget
                     &&
                     GTK_WIDGET_SENSITIVE ( widget ) )
                {
                    /* change the signe of the method of payment and the contra */
                    if ( gsb_payment_method_get_combo_sign ( widget ) == GSB_PAYMENT_DEBIT )
                    {
                        gsb_payment_method_create_combo_list ( widget,
                                            GSB_PAYMENT_CREDIT,
                                            account_number, 0 );
                    }
                }
            }
            string = gsb_form_widget_get_name ( TRANSACTION_FORM_DEBIT );
        }
        break;

    case TRANSACTION_FORM_CREDIT :
        /* we change the payment method to adapt it for the debit */
        if ( strlen ( gtk_entry_get_text ( GTK_ENTRY ( entry ) ) ) )
        {
        /* empty the credit */
        widget = bet_form_widget_get_widget ( TRANSACTION_FORM_DEBIT );
        if ( !gsb_form_widget_check_empty ( widget ) )
        {
            gtk_entry_set_text ( GTK_ENTRY ( widget ),
                        gsb_form_widget_get_name ( TRANSACTION_FORM_DEBIT ) );
            gsb_form_widget_set_empty ( widget, TRUE );
        }
        widget = bet_form_widget_get_widget ( TRANSACTION_FORM_TYPE);

        /* change the method of payment if necessary
         * (if grey, it's a child of split so do nothing) */
        if ( widget
             &&
             GTK_WIDGET_SENSITIVE (widget))
        {
            /* change the signe of the method of payment and the contra */
            if ( gsb_payment_method_get_combo_sign ( widget ) == GSB_PAYMENT_DEBIT )
            {
                gsb_payment_method_create_combo_list ( widget,
                        GSB_PAYMENT_CREDIT,
                        account_number, 0 );
            }
        }
        gsb_form_check_auto_separator (entry);
        }
        else
        {
            /* si pas de nouveau credit on essaie de remettre l'ancien débit */
            if ( (string = gsb_form_widget_get_old_debit ( ) ) )
            {
                GtkWidget * widget_prov;

                widget_prov = bet_form_widget_get_widget ( TRANSACTION_FORM_DEBIT );

                gtk_entry_set_text ( GTK_ENTRY ( widget_prov ), string );
                gsb_form_widget_set_empty ( widget_prov, FALSE );
                g_free ( string );

                widget = bet_form_widget_get_widget ( TRANSACTION_FORM_TYPE );
                if ( widget
                     &&
                     GTK_WIDGET_SENSITIVE ( widget ) )
                {
                    /* change the signe of the method of payment and the contra */
                    if ( gsb_payment_method_get_combo_sign ( widget ) == GSB_PAYMENT_CREDIT )
                    {
                        gsb_payment_method_create_combo_list ( widget,
                                            GSB_PAYMENT_DEBIT,
                                            account_number, 0 );
                    }
                }
            }
            string = gsb_form_widget_get_name ( TRANSACTION_FORM_CREDIT );
        }
        break;

    case TRANSACTION_FORM_CATEGORY :
	    if ( !strlen ( gtk_entry_get_text ( GTK_ENTRY ( entry ) ) ) )
		    string = gsb_form_widget_get_name ( TRANSACTION_FORM_CATEGORY );
	    break;

    case TRANSACTION_FORM_BUDGET :
        if ( !strlen ( gtk_entry_get_text ( GTK_ENTRY ( entry ) ) ) )
		    string = gsb_form_widget_get_name ( TRANSACTION_FORM_BUDGET );
	    break;

	case TRANSACTION_FORM_NOTES :
	    if ( !strlen ( gtk_entry_get_text ( GTK_ENTRY ( entry ) ) ) )
		    string = _(gsb_form_widget_get_name ( element_number ) );
	    break;

	default :
	    break;

    }

    /* if string is not NULL, the entry is empty so set the empty field to TRUE */
    if ( string )
    {
        switch ( element_number)
        {
            case TRANSACTION_FORM_PARTY :
            case TRANSACTION_FORM_CATEGORY :
            case TRANSACTION_FORM_BUDGET :
            /* need to work with the combofix to avoid some signals if we work
             * directly on the entry */
            gtk_combofix_set_text ( GTK_COMBOFIX (
                        bet_form_widget_get_widget ( element_number ) ),
                        _(string) );
            break;

            default:
                gtk_entry_set_text ( GTK_ENTRY ( entry ), string );
            break;
        }
        gsb_form_widget_set_empty ( entry, TRUE );
    }

    return FALSE;
}


/**
 * return the widget of the element_number given in param,
 * for the bet scheduler part of the form
 *
 * \param element_number
 *
 * \return a GtkWidget * or NULL
 * */
GtkWidget *bet_form_scheduler_get_element_widget ( gint element_number )
{
    return gsb_form_get_element_widget_from_list ( element_number,
                        bet_schedul_element_list );
}


/**
 * return the pointer to the widget corresponding to the given element
 *
 * \param element_number
 *
 * \return the widget or NULL
 * */
GtkWidget *bet_form_widget_get_widget ( gint element_number )
{
    return gsb_form_get_element_widget_from_list ( element_number,
                        bet_form_list_widgets );
}


/**
 * called when press a key on an element of the form
 *
 * \param widget wich receive the signal
 * \param ev
 * \param ptr_origin a pointer number of the element
 * 
 * \return FALSE
 * */
gboolean bet_form_key_press_event ( GtkWidget *widget,
                        GdkEventKey *ev,
                        gint *ptr_origin )
{
    gint element_number;
    gint account_number;
    GtkWidget *widget_prov;

    element_number = GPOINTER_TO_INT (ptr_origin);
    account_number = gsb_form_get_account_number ();

    /* if conf.entree = 1, entry finish the transaction, else does as tab */
    if ( !conf.entree
	 &&
	 ( ev -> keyval == GDK_Return 
	   ||
	   ev -> keyval == GDK_KP_Enter ))
	ev->keyval = GDK_Tab ;

    switch ( ev -> keyval )
    {
    case GDK_1:
    case GDK_2:
    case GDK_3:
    case GDK_4:
    case GDK_5:
    case GDK_6:
    case GDK_7:
    case GDK_8:
    case GDK_9:
    case GDK_0:
        switch ( element_number )
        {
        case TRANSACTION_FORM_DEBIT:
            widget_prov = bet_form_widget_get_widget ( TRANSACTION_FORM_CREDIT );
            if ( !gsb_form_widget_check_empty ( widget_prov ) )
            {
                gtk_entry_set_text ( GTK_ENTRY ( widget_prov ),
                         gsb_form_widget_get_name ( TRANSACTION_FORM_CREDIT ) );
                gsb_form_widget_set_empty ( widget_prov, TRUE );
            }
            break;
        case TRANSACTION_FORM_CREDIT:
            widget_prov = bet_form_widget_get_widget ( TRANSACTION_FORM_DEBIT );
            if ( !gsb_form_widget_check_empty ( widget_prov ) )
            {
                gtk_entry_set_text ( GTK_ENTRY (widget_prov),
                            gsb_form_widget_get_name (TRANSACTION_FORM_DEBIT));
                gsb_form_widget_set_empty ( widget_prov, TRUE );
            }
            break;
        }
        break;
	case GDK_Escape :
	    gsb_form_escape_form ();
	    break;

	case GDK_KP_Enter :
	case GDK_Return :

	    break;

	case GDK_KP_Add:
	case GDK_plus:
	case GDK_equal:		/* This should make all our US users happy */

	    /* increase the check of 1 */
	    if (element_number == TRANSACTION_FORM_CHEQUE)
	    {
		increment_decrement_champ ( widget,
					    1 );
		return TRUE;
	    }
	    break;

	case GDK_KP_Subtract:
	case GDK_minus:

	    /* decrease the check of 1 */
	    if (element_number == TRANSACTION_FORM_CHEQUE)
	    {
		increment_decrement_champ ( widget,
					    -1 );
		return TRUE;
	    }
	    break;
    }

    return FALSE;
}


/**
 * called when we press the button in an entry field in
 * the form
 *
 * \param entry wich receive the signal
 * \param ev can be NULL
 * \param ptr_origin a pointer to int on the element_number
 *
 * \return FALSE
 * */
gboolean bet_form_button_press_event ( GtkWidget *entry,
                        GdkEventButton *ev,
                        gint *ptr_origin )
{
    GtkWidget *date_entry;
    gint element_number;
    

    element_number = GPOINTER_TO_INT ( ptr_origin );

	/* set the current date into the date entry */
	date_entry = bet_form_widget_get_widget (TRANSACTION_FORM_DATE);
	if ( gsb_form_widget_check_empty ( date_entry ) )
	{
        gtk_entry_set_text ( GTK_ENTRY ( date_entry ), gsb_date_today ( ) );
	    gsb_form_widget_set_empty ( date_entry, FALSE );
	}

    return FALSE;
}


/**
 * initialise les données du formulaire
 *
 *
 *
 * */
gboolean bet_future_set_form_data_from_line ( gint account_number,
                        gint number  )
{
    GtkWidget *widget;
    GHashTable *future_list;
    gchar *key;
    struct_futur_data *scheduled;

    if ( account_number == 0 )
        key = g_strconcat ("0:", utils_str_itoa ( number ), NULL );
    else
        key = g_strconcat ( utils_str_itoa ( account_number ), ":",
                        utils_str_itoa ( number ), NULL );

    future_list = bet_data_future_get_list ( );

    scheduled = g_hash_table_lookup ( future_list, key );
    if ( scheduled == NULL )
        return FALSE;

    /* On traite les données de la planification */
    widget = bet_form_scheduler_get_element_widget ( SCHEDULED_FORM_FREQUENCY_BUTTON );
    gsb_combo_box_set_index ( widget, scheduled -> frequency );
    gtk_widget_set_sensitive ( widget, FALSE );

    if ( scheduled -> frequency > 0 )
    {
        if ( scheduled -> limit_date && g_date_valid ( scheduled -> limit_date ) )
        {
            widget = bet_form_scheduler_get_element_widget ( SCHEDULED_FORM_LIMIT_DATE );
            gsb_form_widget_set_empty ( widget, FALSE );
            gsb_calendar_entry_set_date ( widget, scheduled -> limit_date );
            gtk_widget_set_sensitive ( widget, FALSE );
        }

        if ( scheduled -> user_entry > 0 )
        {
            widget = bet_form_scheduler_get_element_widget (
                        SCHEDULED_FORM_FREQUENCY_USER_ENTRY );
            gsb_form_widget_set_empty ( widget, FALSE );
            gtk_entry_set_text ( GTK_ENTRY ( widget ),
                        utils_str_itoa ( scheduled -> user_entry ) );
            gtk_widget_set_sensitive ( widget, FALSE );

            widget = bet_form_scheduler_get_element_widget (
                    SCHEDULED_FORM_FREQUENCY_USER_BUTTON );
            gsb_combo_box_set_index ( widget, scheduled -> user_interval );
            gtk_widget_set_sensitive ( widget, FALSE );
        }
    }

    /* On traite les données de transaction */
    widget = bet_form_widget_get_widget ( TRANSACTION_FORM_DATE );
    gsb_calendar_entry_set_date ( widget, scheduled -> date );
    gsb_form_widget_set_empty ( widget, FALSE );

    if ( scheduled -> fyear_number > 0 )
    {
        widget = bet_form_widget_get_widget ( TRANSACTION_FORM_EXERCICE );
        gsb_fyear_set_combobox_history ( widget, scheduled -> fyear_number );
    }

    widget = bet_form_widget_get_widget ( TRANSACTION_FORM_PARTY );
    gsb_form_widget_set_empty ( GTK_COMBOFIX ( widget ) -> entry, FALSE );
    gtk_combofix_set_text ( GTK_COMBOFIX ( widget ),
                        gsb_data_payee_get_name ( scheduled -> party_number, FALSE ) );
    gtk_editable_set_position ( GTK_EDITABLE ( GTK_COMBOFIX ( widget ) -> entry ), 0 );
    
    if ( scheduled -> amount.mantissa < 0 )
    {
        widget = bet_form_widget_get_widget ( TRANSACTION_FORM_DEBIT );
        gtk_entry_set_text ( GTK_ENTRY ( widget ), gsb_real_get_string (
                        gsb_real_opposite ( scheduled -> amount ) ) );
    }
    else
    {
        widget = bet_form_widget_get_widget ( TRANSACTION_FORM_CREDIT );
        gtk_entry_set_text ( GTK_ENTRY ( widget ), gsb_real_get_string ( scheduled -> amount ) );
    }
    gsb_form_widget_set_empty ( widget, FALSE );

    widget = bet_form_widget_get_widget ( TRANSACTION_FORM_TYPE );
    gsb_payment_method_set_combobox_history ( widget, scheduled -> payment_number );

    if ( scheduled -> category_number > 0 )
    {
        widget = bet_form_widget_get_widget ( TRANSACTION_FORM_CATEGORY );
        gsb_form_widget_set_empty ( GTK_COMBOFIX ( widget ) -> entry, FALSE );
        gtk_combofix_set_text ( GTK_COMBOFIX ( widget ),
                        gsb_data_category_get_name ( scheduled -> category_number,
                        scheduled -> sub_category_number, NULL) );
    }

    if ( scheduled -> budgetary_number > 0 )
    {
        widget = bet_form_widget_get_widget ( TRANSACTION_FORM_BUDGET );
        gsb_form_widget_set_empty ( GTK_COMBOFIX ( widget ) -> entry, FALSE );
        gtk_combofix_set_text ( GTK_COMBOFIX ( widget ),
                        gsb_data_budget_get_name (  scheduled -> budgetary_number,
                        scheduled -> sub_budgetary_number, NULL ) );
    }
    
    if ( scheduled -> notes && strlen ( scheduled -> notes ) > 0 )
    {
        widget = bet_form_widget_get_widget ( TRANSACTION_FORM_NOTES );
        gsb_form_widget_set_empty ( widget, FALSE );
        gtk_entry_set_text ( GTK_ENTRY ( widget ), scheduled -> notes );
    }
    
    return TRUE;
}


/**
 * récupère les données du formulaire
 *
 *
 *
 * */
gboolean bet_future_take_data_from_form (  struct_futur_data *scheduled )
{
    GtkWidget *widget;
    gint budget_type;

    /* données liées au compte */
    scheduled -> account_number = gsb_gui_navigation_get_current_account ( );

    /* On traite les données de la planification */
    widget = bet_form_scheduler_get_element_widget ( SCHEDULED_FORM_FREQUENCY_BUTTON );
    scheduled -> frequency = gsb_combo_box_get_index ( widget );

    switch ( scheduled -> frequency )
    {
        case 1:
        case 2:
        case 3:
        case 4:
        case 5:
            widget = bet_form_scheduler_get_element_widget ( SCHEDULED_FORM_LIMIT_DATE );
            if ( gsb_form_widget_check_empty ( widget ) == FALSE )
                scheduled -> limit_date = gsb_calendar_entry_get_date ( widget );
            else
                scheduled -> limit_date = NULL;
            break;
        case 6:
            widget = bet_form_scheduler_get_element_widget ( SCHEDULED_FORM_LIMIT_DATE );
            if ( gsb_form_widget_check_empty ( widget ) == FALSE )
                scheduled -> limit_date = gsb_calendar_entry_get_date ( widget );
            else
                scheduled -> limit_date = NULL;

            widget = bet_form_scheduler_get_element_widget (
                        SCHEDULED_FORM_FREQUENCY_USER_ENTRY );
            if ( gsb_form_widget_check_empty ( widget ) == FALSE )
                scheduled -> user_entry = utils_str_atoi (
                        gtk_entry_get_text ( GTK_ENTRY ( widget ) ) );
            if ( scheduled -> user_entry )
            {
                widget = bet_form_scheduler_get_element_widget (
                        SCHEDULED_FORM_FREQUENCY_USER_BUTTON );
                scheduled -> user_interval = gsb_combo_box_get_index ( widget );
            }
            else
                return FALSE;
    }

    /* On traite les données de transaction */
    widget = bet_form_widget_get_widget ( TRANSACTION_FORM_DATE );
    if ( gsb_form_widget_check_empty( widget ) == FALSE )
    {
        GDate *date_tomorrow;

        date_tomorrow = gsb_date_tomorrow ( );
        scheduled -> date = gsb_calendar_entry_get_date ( widget );
        if ( scheduled -> date == NULL
         || 
         g_date_compare ( scheduled -> date, date_tomorrow ) < 0 )
        {
            g_date_free ( date_tomorrow );
            return FALSE;
        }
        g_date_free ( date_tomorrow );
    }
    else
        return FALSE;

    widget = bet_form_widget_get_widget ( TRANSACTION_FORM_EXERCICE );
    if ( gsb_form_widget_check_empty( widget ) == FALSE )
        scheduled -> fyear_number = gsb_fyear_get_fyear_from_combobox ( widget,
                        scheduled -> date );
    else
        scheduled -> fyear_number = 0;

    widget = bet_form_widget_get_widget ( TRANSACTION_FORM_PARTY );
    if ( gsb_form_widget_check_empty ( widget ) == FALSE )
        scheduled -> party_number = gsb_data_payee_get_number_by_name (
                        gtk_combofix_get_text ( GTK_COMBOFIX ( widget ) ), TRUE );
    else
        scheduled -> party_number = 0;

    widget = bet_form_widget_get_widget ( TRANSACTION_FORM_DEBIT );
    if ( gsb_form_widget_check_empty ( widget ) == FALSE )
    {
            gsb_form_check_auto_separator ( widget );
		    scheduled -> amount = gsb_real_opposite (
                        gsb_utils_edit_calculate_entry ( widget ) );
        budget_type = 1;
    }
    else
    {
        widget = bet_form_widget_get_widget ( TRANSACTION_FORM_CREDIT );
        if ( gsb_form_widget_check_empty ( widget ) == FALSE )
        {
            gsb_form_check_auto_separator ( widget );
            scheduled -> amount = gsb_utils_edit_calculate_entry ( widget );
            budget_type = 0;
        }
        else
            return FALSE;
    }

    widget = bet_form_widget_get_widget ( TRANSACTION_FORM_TYPE );
    if ( gsb_form_widget_check_empty( widget ) == FALSE )
        scheduled -> payment_number = 
                        gsb_payment_method_get_selected_number ( widget );
    else
        scheduled -> payment_number = 0;

    widget = bet_form_widget_get_widget ( TRANSACTION_FORM_CATEGORY );
    if ( gsb_form_widget_check_empty( widget ) == FALSE )
        bet_future_get_category_data ( widget, budget_type, scheduled );
    else
    {
        scheduled -> category_number = 0;
        scheduled -> sub_category_number = 0;
    }

    widget = bet_form_widget_get_widget ( TRANSACTION_FORM_BUDGET );
    if ( gsb_form_widget_check_empty( widget ) == FALSE )
        bet_future_get_budget_data ( widget, budget_type, scheduled );
    else
    {
        scheduled -> budgetary_number = 0;
        scheduled -> sub_budgetary_number = 0;
    }

    widget = bet_form_widget_get_widget ( TRANSACTION_FORM_NOTES );
    if ( gsb_form_widget_check_empty( widget ) == FALSE )
        scheduled -> notes = g_strdup ( gtk_entry_get_text ( GTK_ENTRY ( widget ) ) );
    else
        scheduled -> notes = NULL;
    
    return TRUE;
}


/**
 * récupère l'imputation et la sous imputation budgétaire 
 *
 *
 * \return FALSE
 * */
gboolean bet_future_get_budget_data ( GtkWidget *widget,
                        gint budget_type,
                        struct_futur_data *scheduled )
{
    const gchar *string;
    gchar **tab_char;

    string = gtk_combofix_get_text ( GTK_COMBOFIX ( widget ) );
    if ( string && strlen ( string ) > 0 )
    {
        tab_char = g_strsplit ( string, " : ", 2 );
        scheduled -> budgetary_number = gsb_data_budget_get_number_by_name (
                        tab_char[0], TRUE, budget_type );

        if ( tab_char[1] && strlen ( tab_char[1] ) )
            scheduled -> sub_budgetary_number = gsb_data_budget_get_sub_budget_number_by_name (
                        scheduled -> budgetary_number, tab_char[1], TRUE );
        else
            scheduled -> sub_budgetary_number = 0;
    }
    else
    {
        scheduled -> budgetary_number = 0;
        scheduled -> sub_budgetary_number = 0;
    }
        
    return FALSE;
}


/**
 * récupère la catégorie et la sous catégorie 
 *
 *
 * \return FALSE
 * */
gboolean bet_future_get_category_data ( GtkWidget *widget,
                        gint budget_type,
                        struct_futur_data *scheduled )
{
    const gchar *string;
    gchar **tab_char;

    string = gtk_combofix_get_text ( GTK_COMBOFIX ( widget ) );
    if ( string && strlen ( string ) > 0 )
    {
        tab_char = g_strsplit ( string, " : ", 2 );
        scheduled -> category_number = gsb_data_category_get_number_by_name (
                        tab_char[0], TRUE, budget_type );

        if ( tab_char[1] && strlen ( tab_char[1] ) )
            scheduled -> sub_category_number = 
                        gsb_data_category_get_sub_category_number_by_name (
                        scheduled -> category_number, tab_char[1], TRUE );
        else
            scheduled -> sub_category_number = 0;
    }
    else
    {
        scheduled -> category_number = 0;
        scheduled -> category_number = 0;
    }
        
    return FALSE;
}
/**
 *
 *
 *
 *
 * */
gboolean bet_future_modify_line ( gint account_number,
                        gint number,
                        gint mother_row )
{
    gchar *tmp_str;
    gint result;

    if ( bet_dialog == NULL )
    {
        bet_future_create_dialog ( );
    }
    else
    {
        bet_form_clean ( gsb_gui_navigation_get_current_account ( ) );
        gtk_widget_show ( bet_dialog );
    }

     /* init data */
    bet_future_set_form_data_from_line ( account_number, number  );

dialog_return:
	result = gtk_dialog_run ( GTK_DIALOG ( bet_dialog ));

    if ( result == GTK_RESPONSE_OK )
    {
        struct_futur_data *scheduled;

        scheduled = struct_initialise_bet_future ( );

        if ( !scheduled )
        {
            dialogue_error_memory ();
            gtk_widget_hide ( bet_dialog );
            return FALSE;
        }

        if ( bet_future_take_data_from_form ( scheduled ) == FALSE )
        {
            tmp_str = g_strdup ( _("Error: the frequency defined by the user or the amount is "
                                 "not specified or the date is invalid.") );
            dialogue_warning_hint ( tmp_str, _("One field is not filled in") );
            g_free ( tmp_str );
            goto dialog_return;
        }
        else
        {
            scheduled -> number = number;
            scheduled -> mother_row = mother_row;
            bet_data_future_modify_lines ( scheduled );
        }

        bet_array_refresh_estimate_tab ( );
    }

    gtk_widget_hide ( bet_dialog );

    return FALSE;
}
/* Local Variables: */
/* c-basic-offset: 4 */
/* End: */
#endif /* ENABLE_BALANCE_ESTIMATE */