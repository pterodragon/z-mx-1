/* eggtreemultidnd.c
 * Copyright(C) 2001  Red Hat, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or(at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, see
 * <http://www.gnu.org/licenses/>.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>
#include <gtk/gtk.h>

#include <zlib/libegg_eggtreemultidnd.h>

#define EGG_TREE_MULTI_DND_STRING "EggTreeMultiDndString"

typedef struct {
  guint pressed_button;
  gint x;
  gint y;
  guint button_release_handler;
  guint drag_data_get_handler;
  guint drag_end_handler;
  GList *event_list;
} EggTreeMultiDndData;

GType egg_tree_multi_drag_source_get_type(void)
{
  static GType our_type = 0;

  if (!our_type) {
    const GTypeInfo our_info = {
      sizeof(EggTreeMultiDragSourceIface), /* class_size */
      NULL,		/* base_init */
      NULL,		/* base_finalize */
      NULL,
      NULL,		/* class_finalize */
      NULL,		/* class_data */
      0,
      0,              /* n_preallocs */
      NULL
    };

    our_type = g_type_register_static(
	G_TYPE_INTERFACE, "EggTreeMultiDragSource", &our_info, 0);
  }
  
  return our_type;
}

/**
 * egg_tree_multi_drag_source_drag_data_delete:
 * @drag_source: a #EggTreeMultiDragSource
 * @path: row that was being dragged
 * 
 * Asks the #EggTreeMultiDragSource to delete the row at @path, because
 * it was moved somewhere else via drag-and-drop. Returns %FALSE
 * if the deletion fails because @path no longer exists, or for
 * some model-specific reason. Should robustly handle a @path no
 * longer found in the model!
 * 
 * Return value: %TRUE if the row was successfully deleted
 **/
gboolean egg_tree_multi_drag_source_drag_data_delete(
    EggTreeMultiDragSource *drag_source, GList *path_list)
{
  EggTreeMultiDragSourceIface *iface =
    EGG_TREE_MULTI_DRAG_SOURCE_GET_IFACE(drag_source);

  g_return_val_if_fail(EGG_IS_TREE_MULTI_DRAG_SOURCE(drag_source), FALSE);
  g_return_val_if_fail(iface->drag_data_delete != NULL, FALSE);
  g_return_val_if_fail(path_list != NULL, FALSE);

  return (*iface->drag_data_delete)(drag_source, path_list);
}

/**
 * egg_tree_multi_drag_source_drag_data_get:
 * @drag_source: a #EggTreeMultiDragSource
 * @path: row that was dragged
 * @selection_data: a #EggSelectionData to fill with data from the dragged row
 * 
 * Asks the #EggTreeMultiDragSource to fill in @selection_data with a
 * representation of the row at @path. @selection_data->target gives
 * the required type of the data.  Should robustly handle a @path no
 * longer found in the model!
 * 
 * Return value: %TRUE if data of the required type was provided 
 **/
gboolean egg_tree_multi_drag_source_drag_data_get(
    EggTreeMultiDragSource *drag_source, GList *path_list,
    GtkSelectionData *selection_data)
{
  EggTreeMultiDragSourceIface *iface =
    EGG_TREE_MULTI_DRAG_SOURCE_GET_IFACE(drag_source);

  g_return_val_if_fail(EGG_IS_TREE_MULTI_DRAG_SOURCE(drag_source), FALSE);
  g_return_val_if_fail(iface->drag_data_get != NULL, FALSE);
  g_return_val_if_fail(path_list != NULL, FALSE);
  g_return_val_if_fail(selection_data != NULL, FALSE);

  return (*iface->drag_data_get)(drag_source, path_list, selection_data);
}

static void stop_drag_check(GtkWidget *widget)
{
  EggTreeMultiDndData *priv_data;
  GList *l;

  priv_data = g_object_get_data(G_OBJECT(widget), EGG_TREE_MULTI_DND_STRING);
  
  for (l = priv_data->event_list; l != NULL; l = l->next)
    gdk_event_free(l->data);
  
  if (priv_data->event_list) {
    g_list_free(priv_data->event_list);
    priv_data->event_list = NULL;
  }

  if (priv_data->button_release_handler) {
    g_signal_handler_disconnect(widget, priv_data->button_release_handler);
    priv_data->button_release_handler = 0;
  }
}

static void egg_tree_multi_drag_drag_end(
    GtkWidget *widget, GdkDragContext *context, gpointer user_data)
{
  stop_drag_check(widget); /* the event_list is discarded */
}

static gboolean egg_tree_multi_drag_button_release_event(
    GtkWidget *widget, GdkEventButton *event, gpointer data)
{
  EggTreeMultiDndData *priv_data;
  GList *l;

  priv_data = g_object_get_data(G_OBJECT(widget), EGG_TREE_MULTI_DND_STRING);

  for (l = priv_data->event_list; l != NULL; l = l->next) 
    gtk_propagate_event(widget, l->data);
  
  stop_drag_check(widget);

  return FALSE;
}

static void egg_tree_multi_drag_drag_data_get(
    GtkWidget *widget, GdkDragContext *context,
    GtkSelectionData *selection_data, guint info, guint time)
{
  GtkTreeView *tree_view;
  GtkTreeSelection *selection;
  GtkTreeModel *model = NULL;
  GList *path_list;

  tree_view = GTK_TREE_VIEW(widget);

  if (tree_view == NULL) return;

  selection = gtk_tree_view_get_selection(tree_view);

  if (selection == NULL) return;

  path_list = gtk_tree_selection_get_selected_rows(selection, &model);

  if (path_list == NULL) return;

  if (model == NULL) return;

  if (!EGG_IS_TREE_MULTI_DRAG_SOURCE(model)) return;

  egg_tree_multi_drag_source_drag_data_get(
      EGG_TREE_MULTI_DRAG_SOURCE(model), path_list, selection_data);
}

static gboolean egg_tree_multi_drag_button_press_event(
    GtkWidget *widget, GdkEventButton *event, gpointer data)
{
  GtkTreeView *tree_view;
  GtkTreePath *path = NULL;
  GtkTreeViewColumn *column = NULL;
  gint cell_x, cell_y;
  GtkTreeSelection *selection;
  EggTreeMultiDndData *priv_data;

  tree_view = GTK_TREE_VIEW(widget);
  priv_data = g_object_get_data(G_OBJECT(tree_view), EGG_TREE_MULTI_DND_STRING);

  if (priv_data == NULL) {
    priv_data = g_new0(EggTreeMultiDndData, 1);
    g_object_set_data(
	G_OBJECT(tree_view), EGG_TREE_MULTI_DND_STRING, priv_data);
  }

  if (g_list_find(priv_data->event_list, event))
    return FALSE;

  if (priv_data->event_list) {
    /* save the event to be propagated in order */
    priv_data->event_list =
      g_list_append(priv_data->event_list, gdk_event_copy((GdkEvent *)event));
    return TRUE;
  }
  
  if (event->type == GDK_2BUTTON_PRESS)
    return FALSE;

  gtk_tree_view_get_path_at_pos(
      tree_view, event->x, event->y, &path, &column, &cell_x, &cell_y);

  selection = gtk_tree_view_get_selection(tree_view);

  if (path) {
    gboolean drag = gtk_tree_selection_path_is_selected(selection, path);

    gboolean call_parent =
      !drag ||
      (event->state & (GDK_CONTROL_MASK | GDK_SHIFT_MASK)) ||
      event->button != 1;

    if (call_parent) {
      (GTK_WIDGET_GET_CLASS(tree_view))->button_press_event(widget, event);
      drag = gtk_tree_selection_path_is_selected(selection, path);
    }

    if (drag) {
      priv_data->pressed_button = event->button;
      priv_data->x = event->x;
      priv_data->y = event->y;

      if (!call_parent)
	priv_data->event_list =
	  g_list_append(priv_data->event_list,
	      gdk_event_copy((GdkEvent*)event));

      priv_data->button_release_handler =
	g_signal_connect(G_OBJECT(tree_view), "button-release-event",
	    G_CALLBACK(egg_tree_multi_drag_button_release_event), NULL);
      if (!priv_data->drag_data_get_handler)
	priv_data->drag_data_get_handler =
	  g_signal_connect(G_OBJECT(tree_view), "drag-data-get",
	      G_CALLBACK(egg_tree_multi_drag_drag_data_get), NULL);
      if (!priv_data->drag_end_handler)
	priv_data->drag_end_handler =
	  g_signal_connect(G_OBJECT(tree_view), "drag-end",
	      G_CALLBACK(egg_tree_multi_drag_drag_end), NULL);
    }

    gtk_tree_path_free(path);

    return call_parent || drag;
  }

  return FALSE;
}

void egg_tree_multi_drag_add_drag_support(GtkTreeView *tree_view)
{
  g_return_if_fail(GTK_IS_TREE_VIEW(tree_view));
  g_signal_connect(G_OBJECT(tree_view), "button-press-event",
      G_CALLBACK(egg_tree_multi_drag_button_press_event), NULL);
}
