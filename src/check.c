/* ************************************************************************** */
/* Ce fichier s'occupe de la vérification des montants rapprochés.            */
/* Il est introduit dans la version 0.5.4 suite à la découverte d'un bogue    */
/* qui provoquait les rapprochements des contre-opérations lors du            */
/* rapprochement d'opérations (qui étaient donc des virements)                */
/*                                                                            */
/* 				check.c                                       */
/*                                                                            */
/*     Copyright (C)	2004 Alain Portal (dionysos@grisbi.org)		      */
/*			http://www.grisbi.org   			      */
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

#include "include.h"
#include "structures.h"
#include "variables-extern.c"
#include "check.h"

#include "devises.h"
#include "dialog.h"



/******************************************************************************/
/* reconciliation_check.                                                      */
/* Cette fonction est appelée après la création de toutes les listes.         */
/* Elle permet de vérifier la cohérence des rapprochements suite à la         */
/* découverte des bogues #466 et #488.                                        */
/******************************************************************************/
void reconciliation_check ( void )
{
  gint affected_account = 0;
  gint tested_account = 0;
  GSList *pUserAccountsList = NULL;
  gchar *pHint = NULL, *pText;

  /* s'il n'y a pas de compte, on quitte */
  if ( !nb_comptes )
    return;

  /* si l'utilisateur n'abandonne pas, faire */
  pText = _("This will check that the last reconciliation amounts matches with "
	    "total amounts of reconcilied transactions (and initial balance).");
  if ( question_yes_no ( pText ))
  {
    /* On fera la vérification des comptes dans l'ordre préféré
       de l'utilisateur. On fait une copie de la liste car
       on va y écrire dedans pour sortir de la boucle de test des comptes. */

    pUserAccountsList = g_slist_copy ( ordre_comptes );

    /* Pour chacun des comptes, faire */
    do
    {
      p_tab_nom_de_compte_variable = p_tab_nom_de_compte + GPOINTER_TO_INT ( pUserAccountsList -> data );
    
      /* Si le compte a été rapproché au moins une fois.
         Seule la date permet de l'affirmer. */
      if ( DATE_DERNIER_RELEVE )
      {
        GSList *pTransactionList;
        gdouble reconcilied_amount = 0;

        /* On va recalculer le montant rapproché du compte (c-à-d le solde initial
           plus le montant des opérations rapprochées) et le comparer à la valeur
           stockée dans le fichier. Si les valeurs diffèrent, on affiche une boite
	   d'avertissement */
      
        reconcilied_amount = SOLDE_INIT;

        /* On récupère la liste des opérations */
        pTransactionList = LISTE_OPERATIONS;

        while ( pTransactionList )
        {
	  struct structure_operation *pTransaction;

	  pTransaction = pTransactionList -> data;

	  /* On ne prend en compte que les opérations rapprochées.
	     On ne prend pas en compte les opérations de ventilation. */
	  if ( pTransaction -> pointe == OPERATION_RAPPROCHEE
	       &&
	       !pTransaction -> no_operation_ventilee_associee )
	  {
	      reconcilied_amount += calcule_montant_devise_renvoi ( pTransaction -> montant,
								    DEVISE,
								    pTransaction -> devise,
								    pTransaction -> une_devise_compte_egale_x_devise_ope,
								    pTransaction -> taux_change,
								    pTransaction -> frais_change );
	  }
	  pTransactionList = pTransactionList -> next;
	}

	if ( fabs ( reconcilied_amount - SOLDE_DERNIER_RELEVE ) >= 0.01 )
	{
	  affected_account++;

	  pText = g_strdup_printf ( _("Last reconciliation amount : %f\n"
				      "Computed reconciliation amount : %f\n\n"
				      "Do you to continue test?"),
				    reconcilied_amount,
				    SOLDE_DERNIER_RELEVE );
	  pHint = g_strdup_printf ( _("Account name : %s\n"),
				    NOM_DU_COMPTE);
	  if ( !question_yes_no_hint ( pHint, pText ) )
	  {
	    /* En utilisant une méthode de sortie de boucle plus propre,
	       il est possible de se passer de pUserAccountsList et
	       et de travailler directement avec ordre_comptes. */
	    pUserAccountsList -> next = NULL;
	  }
	  free ( pHint );
	  free ( pText );
	}
	tested_account++;
      }
    }
    while ( (  pUserAccountsList = pUserAccountsList -> next ) );

    if ( !affected_account )
    {
	dialogue ( _("About reconciliation, your accounting is sane.") );
    }
    else
    {
      pText = g_strdup_printf ( _("There are %d accounts in your file.  %d were tested.\n"
				  "In %d account(s), stored and computed reconciliation "
				  "don't match.  You should correct this quickly.\n"
				  "Generally, in affected accounts, there are too many reconciled "
				  "transactions. And these transactions are transfert between "
				  "your Grisbi accounts."),
				nb_comptes,
				tested_account,
				affected_account );
      dialogue ( pText );
      free ( pText );
    }
  }
  g_slist_free ( pUserAccountsList );
}
