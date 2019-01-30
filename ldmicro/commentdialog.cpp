//-----------------------------------------------------------------------------
// Copyright 2007 Jonathan Westhues
//
// This file is part of LDmicro.
// 
// LDmicro is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// LDmicro is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with LDmicro.  If not, see <http://www.gnu.org/licenses/>.
//------
//
// Dialog to enter the text of a comment; make it long and skinny to
// encourage people to write it the way it will look on the diagram.
// Jonathan Westhues, Jun 2005
//-----------------------------------------------------------------------------
#include <linuxUI.h>
#include <stdio.h>
//#include <commctrl.h>

#include "ldmicro.h"

static HWID CommentDialog;

static HWID CommentTextbox;

static HWID CommentGrid;
static HWID CommentPackingBox;
static HWID OkButton;
static HWID CancelButton;

static void MakeControls(void)
{
    CommentTextbox = gtk_entry_new();
    gtk_entry_set_max_length (GTK_ENTRY (CommentTextbox), 0);
    gtk_widget_set_hexpand (CommentTextbox, TRUE);
    gtk_widget_set_vexpand (CommentTextbox, TRUE);

    OkButton = gtk_button_new_with_label ("OK");
    CancelButton = gtk_button_new_with_label ("Cancel");

    gtk_grid_attach (GTK_GRID (CommentGrid), CommentTextbox, 1, 2, 1, 1);
    gtk_grid_attach (GTK_GRID (CommentGrid), OkButton, 5, 2, 1, 1);
    gtk_grid_attach (GTK_GRID (CommentGrid), CancelButton, 5, 3, 1, 1);

    gtk_grid_set_column_spacing (GTK_GRID (CommentGrid), 1);
    gtk_box_pack_start(GTK_BOX(CommentPackingBox), CommentGrid, TRUE, TRUE, 0);
}

void CommentDialogGetData (char* comment){
    strncpy (comment, gtk_entry_get_text (GTK_ENTRY (CommentTextbox)),
            MAX_COMMENT_LEN-1);
    gtk_widget_set_sensitive (MainWindow, TRUE);
    DestroyWindow (CommentDialog);
    ProgramChanged();
}

// Mouse click callback
void CommentDialogMouseClick (HWID widget, gpointer data){
    CommentDialogGetData((char*)data);
}

// Checks for the required key press
gboolean CommentDialogKeyPress (HWID widget, GdkEventKey* event, gpointer data){
    if (event -> keyval == GDK_KEY_Return){
        CommentDialogGetData((char*)data);
    }
    else if (event -> keyval == GDK_KEY_Escape){
        DestroyWindow (CommentDialog);
        gtk_widget_set_sensitive (MainWindow, TRUE);
        ProgramChanged();
    }
    return FALSE;
}

void CommentCallDestroyWindow (HWID widget, gpointer data){
    DestroyWindow (CommentDialog);
    ProgramChanged();
    gtk_widget_set_sensitive (MainWindow, TRUE);
}

void ShowCommentDialog(char *comment)
{    
    CommentGrid = gtk_grid_new();
    CommentPackingBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

    CommentDialog = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(CommentDialog), "Comment");
    gtk_window_set_default_size(GTK_WINDOW(CommentDialog), 700, 50);
    gtk_window_set_resizable (GTK_WINDOW (CommentDialog), FALSE);
    gtk_widget_add_events (CommentDialog, GDK_KEY_PRESS_MASK);
    gtk_widget_add_events (CommentDialog, GDK_BUTTON_PRESS_MASK);

    MakeControls();
    gtk_entry_set_text (GTK_ENTRY (CommentTextbox), comment);
    gtk_container_add(GTK_CONTAINER(CommentDialog), CommentPackingBox);
    gtk_widget_set_sensitive (MainWindow, FALSE);
    gtk_widget_show_all (CommentDialog);
    gtk_widget_grab_focus (CommentTextbox);

    g_signal_connect (G_OBJECT (CommentDialog), "key-press-event",
                    G_CALLBACK(CommentDialogKeyPress), (gpointer)comment);
    g_signal_connect (G_OBJECT (OkButton), "clicked",
                    G_CALLBACK(CommentDialogMouseClick), (gpointer)comment);
    g_signal_connect (G_OBJECT (CancelButton), "clicked",
                    G_CALLBACK(CommentCallDestroyWindow), NULL);

    return;
}
