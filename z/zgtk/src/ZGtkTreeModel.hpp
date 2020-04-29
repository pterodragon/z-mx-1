//  -*- mode:c++; indent-tabs-mode:t; tab-width:8; c-basic-offset:2; -*-
//  vi: noet ts=8 sw=2 cino=l1,g0,N-s,j1,U1,i4

/*
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

// Gtk tree model wrapper

#ifndef ZGtkTreeModel_HPP
#define ZGtkTreeModel_HPP

#ifdef _MSC_VER
#pragma once
#endif

#ifndef ZGtkLib_HPP
#include <zlib/ZGtkLib.hpp>
#endif

#include <typeinfo>

#include <glib.h>

#include <gtk/gtk.h>

#include <zlib/ZuSort.hpp>

#include <zlib/ZmPLock.hpp>

#include <zlib/ZGtkApp.hpp>
#include <zlib/ZGtkValue.hpp>

namespace ZGtk {

// CRTP - implementation must conform to the following interface:
#if 0
struct App : public TreeModel<App> {
  GtkTreeModelFlags get_flags();
  gint get_n_columns();
  GType get_column_type(gint i);
  gboolean get_iter(GtkTreeIter *iter, GtkTreePath *path);
  GtkTreePath *get_path(GtkTreeIter *iter);
  void get_value(GtkTreeIter *iter, gint i, Value *value);
  gboolean iter_next(GtkTreeIter *iter);
  gboolean iter_children(GtkTreeIter *iter, GtkTreeIter *parent);
  gboolean iter_has_child(GtkTreeIter *iter);
  gint iter_n_children(GtkTreeIter *iter);
  gboolean iter_nth_child(GtkTreeIter *iter, GtkTreeIter *parent, gint n);
  gboolean iter_parent(GtkTreeIter *iter, GtkTreeIter *child);

  gboolean get_sort_column_id(gint *column, GtkSortType *order);
  void set_sort_column_id(gint column, GtkSortType order);
};
#endif

struct TreeModelDragData {
  GList *events = nullptr;
  guint	handler = 0;		// button release handler
};

template <typename T>
class TreeModel : public GObject {
public:
  static const char *typeName() {
    static const char *name = nullptr;
    if (!name) name = typeid(T).name();
    return name;
  }
  static GdkAtom rowsAtom() {
    static GdkAtom atom = nullptr;
    if (!atom) atom = gdk_atom_intern_static_string(typeName());
    return atom;
  }
  static constexpr gint nRowsTargets() { return 1; }
  static const GtkTargetEntry *rowsTargets() {
    static GtkTargetEntry rowsTargets_[] = {
      { (gchar *)nullptr, GTK_TARGET_SAME_APP, 0 }
    };
    if (!rowsTargets_[0].target) rowsTargets_[0].target =
      const_cast<gchar *>(reinterpret_cast<const gchar *>(typeName()));
    return rowsTargets_;
  }

private:
  T *impl() { return static_cast<T *>(this); }
  template <typename Ptr>
  static T *impl(Ptr *ptr) {
    return static_cast<T *>(reinterpret_cast<TreeModel *>(ptr));
  }

  static GType gtype_init() {
    static const GTypeInfo gtype_info{
      sizeof(GObjectClass),
      nullptr,					// base_init
      nullptr,					// base_finalize
      [](void *c_, void *) {
	auto c = static_cast<GObjectClass *>(c_);
	c->finalize = [](GObject *m) {
	  impl(m)->~T();
	};
      },
      nullptr,					// class finalize
      nullptr,					// class_data
      sizeof(T),
      0,					// n_preallocs
      [](GTypeInstance *m, void *) {
	char object[sizeof(GObject)];
	memcpy(object, reinterpret_cast<const GObject *>(m), sizeof(GObject));
	new (m) T{};
	memcpy(reinterpret_cast<GObject *>(m), object, sizeof(GObject));
      }
    };

    static const GInterfaceInfo tree_model_info{
      [](void *i_, void *) {
	auto i = static_cast<GtkTreeModelIface *>(i_);
	i->get_flags = [](GtkTreeModel *m) -> GtkTreeModelFlags {
	  g_return_val_if_fail(
	      G_TYPE_CHECK_INSTANCE_TYPE((m), gtype()),
	      (static_cast<GtkTreeModelFlags>(0)));
	  return impl(m)->get_flags();
	};
	i->get_n_columns = [](GtkTreeModel *m) -> gint {
	  g_return_val_if_fail(G_TYPE_CHECK_INSTANCE_TYPE((m), gtype()), 0);
	  return impl(m)->get_n_columns();
	};
	i->get_column_type = [](GtkTreeModel *m, gint i) -> GType {
	  g_return_val_if_fail(
	      G_TYPE_CHECK_INSTANCE_TYPE((m), gtype()), G_TYPE_INVALID);
	  return impl(m)->get_column_type(i);
	};
	i->get_iter = [](GtkTreeModel *m,
	    GtkTreeIter *iter, GtkTreePath *path) -> gboolean {
	  g_return_val_if_fail(
	      G_TYPE_CHECK_INSTANCE_TYPE((m), gtype()), false);
	  g_return_val_if_fail(!!path, false);
	  return impl(m)->get_iter(iter, path);
	};
	i->get_path = [](
	    GtkTreeModel *m, GtkTreeIter *iter) -> GtkTreePath * {
	  g_return_val_if_fail(
	      G_TYPE_CHECK_INSTANCE_TYPE((m), gtype()), nullptr);
	  g_return_val_if_fail(!!iter, nullptr);
	  return impl(m)->get_path(iter);
	};
	i->get_value = [](GtkTreeModel *m,
	    GtkTreeIter *iter, gint i, GValue *value) {
	  g_return_if_fail(G_TYPE_CHECK_INSTANCE_TYPE((m), gtype()));
	  g_return_if_fail(!!iter);
	  impl(m)->get_value(iter, i, reinterpret_cast<Value *>(value));
	};
	i->iter_next = [](GtkTreeModel *m, GtkTreeIter *iter) -> gboolean {
	  g_return_val_if_fail(
	      G_TYPE_CHECK_INSTANCE_TYPE((m), gtype()), false);
	  g_return_val_if_fail(!!iter, false);
	  return impl(m)->iter_next(iter);
	};
	i->iter_children = [](GtkTreeModel *m,
	    GtkTreeIter *iter, GtkTreeIter *parent) -> gboolean {
	  g_return_val_if_fail(
	      G_TYPE_CHECK_INSTANCE_TYPE((m), gtype()), false);
	  g_return_val_if_fail(!parent, false);
	  return impl(m)->iter_children(iter, parent);
	};
	i->iter_has_child = [](
	    GtkTreeModel *m, GtkTreeIter *iter) -> gboolean {
	  g_return_val_if_fail(
	      G_TYPE_CHECK_INSTANCE_TYPE((m), gtype()), false);
	  g_return_val_if_fail(!!iter, false);
	  return impl(m)->iter_has_child(iter);
	};
	i->iter_n_children = [](GtkTreeModel *m, GtkTreeIter *iter) -> gint {
	  g_return_val_if_fail(G_TYPE_CHECK_INSTANCE_TYPE((m), gtype()), 0);
	  return impl(m)->iter_n_children(iter);
	};
	i->iter_nth_child = [](GtkTreeModel *m,
	    GtkTreeIter *iter, GtkTreeIter *parent, gint n) -> gboolean {
	  g_return_val_if_fail(G_TYPE_CHECK_INSTANCE_TYPE((m), gtype()), false);
	  return impl(m)->iter_nth_child(iter, parent, n);
	};
	i->iter_parent = [](GtkTreeModel *m,
	    GtkTreeIter *iter, GtkTreeIter *child) -> gboolean {
	  g_return_val_if_fail(G_TYPE_CHECK_INSTANCE_TYPE((m), gtype()), false);
	  g_return_val_if_fail(!!child, false);
	  return impl(m)->iter_parent(iter, child);
	};
	i->ref_node = [](GtkTreeModel *m, GtkTreeIter *iter) {
	  g_return_if_fail(G_TYPE_CHECK_INSTANCE_TYPE((m), gtype()));
	  g_return_if_fail(!!iter);
	  return impl(m)->ref_node(iter);
	};
	i->unref_node = [](GtkTreeModel *m, GtkTreeIter *iter) {
	  g_return_if_fail(G_TYPE_CHECK_INSTANCE_TYPE((m), gtype()));
	  g_return_if_fail(!!iter);
	  return impl(m)->unref_node(iter);
	};
      },
      nullptr,
      nullptr
    };

    static const GInterfaceInfo tree_sortable_info{
      [](void *i_, void *) {
	auto i = static_cast<GtkTreeSortableIface *>(i_);
	i->get_sort_column_id = [](GtkTreeSortable *s,
	    gint *column, GtkSortType *order) -> gboolean {
	  g_return_val_if_fail(G_TYPE_CHECK_INSTANCE_TYPE((s), gtype()), false);
	  return impl(s)->get_sort_column_id(column, order);
	};
	i->set_sort_column_id = [](GtkTreeSortable *s,
	    gint column, GtkSortType order) {
	  g_return_if_fail(G_TYPE_CHECK_INSTANCE_TYPE((s), gtype()));
	  impl(s)->set_sort_column_id(column, order);
	};
	i->has_default_sort_func = [](GtkTreeSortable *s) -> gboolean {
	  return false;
	};
      },
      nullptr,
      nullptr
    }; 

    GType gtype_ = g_type_register_static(
	G_TYPE_OBJECT, "TreeModel", &gtype_info, (GTypeFlags)0);
    g_type_add_interface_static(gtype_,
	GTK_TYPE_TREE_MODEL, &tree_model_info);
    g_type_add_interface_static(gtype_,
	GTK_TYPE_TREE_SORTABLE, &tree_sortable_info);

    return gtype_;
  }

public:
  static GType gtype() {
    static GType gtype_ = 0;
    if (!gtype_) gtype_ = gtype_init();
    return gtype_;
  }

  static TreeModel *ctor() {
    return reinterpret_cast<TreeModel *>(g_object_new(gtype(), nullptr));
  }

  // multiple row drag-and-drop support
  //
  // Drag(TreeModel *model, GList *rows, GtkSelectionData *data) -> gboolean
  // Drop(TreeModel *model, GtkWidget *widget, GtkSelectionData *data)
  template <typename Drag, typename Drop>
  void drag(GtkTreeView *view, GtkWidget *dest, Drag, Drop) {
    gtk_drag_source_set(GTK_WIDGET(view),
	GDK_BUTTON1_MASK,
	rowsTargets(), nRowsTargets(),
	GDK_ACTION_COPY);

    g_signal_connect(G_OBJECT(view), "drag-data-get",
	ZGtk::callback([](GObject *o, GdkDragContext *,
	    GtkSelectionData *data, guint, guint) -> gboolean {
	  auto view = GTK_TREE_VIEW(o);
	  g_return_val_if_fail(!!view, false);
	  auto sel = gtk_tree_view_get_selection(view);
	  if (!sel) return false;
	  GtkTreeModel *model = nullptr;
	  auto rows = gtk_tree_selection_get_selected_rows(sel, &model);
	  if (!rows) return false;
	  g_return_val_if_fail(!!model, false);
	  return ZuLambdaFn<Drag>::invoke(impl(model), rows, data);
	}), 0);

    g_signal_connect(G_OBJECT(view), "drag-end",
	ZGtk::callback([](GtkWidget *widget,
	    GdkEventButton *event, gpointer) {
	  TreeModelDragData *dragData =
	    reinterpret_cast<TreeModelDragData *>(
		g_object_get_data(G_OBJECT(widget), typeName()));
	  g_return_if_fail(!!dragData);
	  dragEnd(widget, dragData);
	}), 0);
 
    g_signal_connect(G_OBJECT(view), "button-press-event",
	ZGtk::callback([](GtkWidget *widget,
	    GdkEventButton *event, gpointer) -> gboolean {
	  auto view = GTK_TREE_VIEW(widget);
	  g_return_val_if_fail(!!view, false);
	  TreeModelDragData *dragData =
	    reinterpret_cast<TreeModelDragData *>(
		g_object_get_data(G_OBJECT(view), typeName()));
	  if (!dragData) {
	    dragData = new TreeModelDragData();
	    g_object_set_data(G_OBJECT(view), typeName(), dragData);
	  }
	  if (g_list_find(dragData->events, event)) return false;
	  if (dragData->events) {
	    dragData->events = g_list_append(dragData->events,
		gdk_event_copy(reinterpret_cast<GdkEvent *>(event)));
	    return true;
	  }
	  if (event->type == GDK_2BUTTON_PRESS) return false;
	  GtkTreePath *path = nullptr;
	  GtkTreeViewColumn *column = nullptr;
	  gint cell_x, cell_y;
	  gtk_tree_view_get_path_at_pos(
	      view, event->x, event->y, &path, &column, &cell_x, &cell_y);
	  if (!path) return false;
	  auto selection = gtk_tree_view_get_selection(view);
	  bool drag = gtk_tree_selection_path_is_selected(selection, path);
	  bool call_parent =
	    !drag ||
	    (event->state & (GDK_CONTROL_MASK | GDK_SHIFT_MASK)) ||
	    event->button != 1;
	  if (call_parent) {
	    (GTK_WIDGET_GET_CLASS(view))->button_press_event(widget, event);
	    drag = gtk_tree_selection_path_is_selected(selection, path);
	  }
	  if (drag) {
	    if (!call_parent)
	      dragData->events = g_list_append(dragData->events,
		  gdk_event_copy((GdkEvent*)event));
	    dragData->handler =
	      g_signal_connect(G_OBJECT(view), "button-release-event",
		  ZGtk::callback([](GtkWidget *widget,
		      GdkEventButton *event, gpointer) -> gboolean {
		    TreeModelDragData *dragData =
		      reinterpret_cast<TreeModelDragData *>(
			  g_object_get_data(G_OBJECT(widget), typeName()));
		    g_return_val_if_fail(!!dragData, false);
		    for (GList *l = dragData->events; l; l = l->next)
		      gtk_propagate_event(widget,
			  reinterpret_cast<GdkEvent *>(l->data));
		    dragEnd(widget, dragData);
		    return false;
		  }), 0);
	  }
	  gtk_tree_path_free(path);
	  return call_parent || drag;
	}), 0);

    gtk_drag_dest_set(dest,
	GTK_DEST_DEFAULT_ALL,
	TreeModel::rowsTargets(), TreeModel::nRowsTargets(),
	GDK_ACTION_COPY);

    g_signal_connect(G_OBJECT(dest), "drag-data-received",
	ZGtk::callback([](GtkWidget *widget, GdkDragContext *, int, int,
	    GtkSelectionData *data, guint, guint32, gpointer model_) {
	  ZuLambdaFn<Drop>::invoke(reinterpret_cast<T *>(model_), widget, data);
	  g_signal_stop_emission_by_name(widget, "drag-data-received");
	}), this);
  }
private:
  static void dragEnd(GtkWidget *widget, TreeModelDragData *dragData) {
    for (GList *l = dragData->events; l; l = l->next)
      gdk_event_free(reinterpret_cast<GdkEvent *>(l->data));
    if (dragData->events) {
      g_list_free(dragData->events);
      dragData->events = nullptr;
    }
    if (dragData->handler) {
      g_signal_handler_disconnect(widget, dragData->handler);
      dragData->handler = 0;
    }
  }

public:
  void view(GtkTreeView *view) {
    gtk_tree_view_set_model(view, GTK_TREE_MODEL(this));
    g_object_unref(this); // view now owns model
  }

  // defaults for unsorted model
  gboolean get_sort_column_id(gint *column, GtkSortType *order) {
    if (column) *column = GTK_TREE_SORTABLE_UNSORTED_SORT_COLUMN_ID;
    if (order) *order = GTK_SORT_ASCENDING;
    return false;
  }
  void set_sort_column_id(gint column, GtkSortType order) {
    return;
  }
};

template <
  typename T,
  int DefaultCol = GTK_TREE_SORTABLE_UNSORTED_SORT_COLUMN_ID,
  int DefaultOrder = GTK_SORT_ASCENDING>
class TreeSortable : public TreeModel<T> {
private:
  T *impl() { return static_cast<T *>(this); }

public:
  gboolean get_sort_column_id(gint *col, GtkSortType *order) {
    if (col) *col = m_col;
    if (order) *order = m_order;
    switch ((int)m_col) {
      case GTK_TREE_SORTABLE_DEFAULT_SORT_COLUMN_ID:
      case GTK_TREE_SORTABLE_UNSORTED_SORT_COLUMN_ID:
	return false;
      default:
	return true;
    }
  }
  void set_sort_column_id(gint col, GtkSortType order) {
    if (ZuUnlikely(m_col == col && m_order == order)) return;

    m_col = col;
    m_order = order;

    // emit gtk_tree_sortable_sort_column_changed()
    gtk_tree_sortable_sort_column_changed(GTK_TREE_SORTABLE(this));

    impl()->sort(m_col, m_order);
  }

private:
  gint		m_col = DefaultCol;
  GtkSortType	m_order = static_cast<GtkSortType>(DefaultOrder);
};

// CRTP - implementation must conform to the following interface:
#if 0
struct App : public TreeArray<App, Data, NodeRef> {
  // Count(unsigned)
  // Iterate(NodeRef)
  template <typename Count, typename Iterate>
  void load(Count count, Iterate iterate);
  Data *data(Node *node);
  gint row(const Node *node);
  void row(Node *node, gint v);
  const ZvFieldFmt &fmt(unsigned col);
};
#endif
template <
  typename T,
  typename Data,
  typename NodeRef>
class TreeArray : public TreeSortable<TreeArray<T, Data, NodeRef> > {
  const NodeRef &nodeRef_();
  using Node = decltype(*nodeRef_());

private:
  T *impl() { return static_cast<T *>(this); }
  const T *impl() const { return static_cast<const T *>(this); }

public:
  void load(gint col, GtkSortType order) {
    this->set_sort_column_id(col, order);
    impl()->load(
	[this](unsigned count) { m_rows.size(count); },
	[this](NodeRef node) {
	  impl()->row(node, m_rows.length());
	  m_rows.push(ZuMv(node));
	});
  }
  GtkTreeModelFlags get_flags() {
    return static_cast<GtkTreeModelFlags>(
	GTK_TREE_MODEL_LIST_ONLY | GTK_TREE_MODEL_ITERS_PERSIST);
  }
  gint get_n_columns() {
    auto fields = Data::fields();
    return fields.length();
  }
  GType get_column_type(gint i) {
    return G_TYPE_STRING;
  }
  gboolean get_iter(GtkTreeIter *iter, GtkTreePath *path) {
    gint depth = gtk_tree_path_get_depth(path);
    if (depth != 1) return false;
    gint *indices = gtk_tree_path_get_indices(path);
    auto row = indices[0];
    iter->user_data = reinterpret_cast<gpointer>(static_cast<uintptr_t>(row));
  }
  GtkTreePath *get_path(GtkTreeIter *iter) {
    GtkTreePath *path = gtk_tree_path_new();
    gtk_tree_path_append_index(
	path, reinterpret_cast<uintptr_t>(iter->user_data));
    return path;
  }
  void get_value(GtkTreeIter *iter, gint col, Value *value) {
    gint row = reinterpret_cast<uintptr_t>(iter->user_data);
    if (ZuUnlikely(row >= m_rows.length())) return;
    auto fields = Data::fields();
    static ZtString s; // ok - this is single-threaded
    fields[col].print(s, impl()->data(m_rows[row]), impl()->fmt(col));
    value->init(G_TYPE_STRING);
    value->set_static_string(s);
  }
  gboolean iter_next(GtkTreeIter *iter) {
    gint row = reinterpret_cast<uintptr_t>(iter->user_data);
    if (ZuUnlikely(++row >= m_rows.length())) return false;
    iter->user_data = reinterpret_cast<gpointer>(static_cast<uintptr_t>(row));
  }
  gboolean iter_children(GtkTreeIter *iter, GtkTreeIter *parent) {
    if (parent) return false;
    iter->user_data = 0;
    return true;
  }
  gboolean iter_has_child(GtkTreeIter *parent) {
    return !parent;
  }
  gint iter_n_children(GtkTreeIter *parent) {
    if (parent) return 0;
    return m_rows.length();
  }
  gboolean iter_nth_child(GtkTreeIter *iter, GtkTreeIter *parent, gint row) {
    if (parent) return false;
    iter->user_data = reinterpret_cast<gpointer>(static_cast<uintptr_t>(row));
    return true;
  }
  gboolean iter_parent(GtkTreeIter *iter, GtkTreeIter *child) {
    return false;
  }
  void ref_node(GtkTreeIter *) { }
  void unref_node(GtkTreeIter *) { }

private:
  auto cmp_(gint col, GtkSortType order) {
    return [impl = impl(), col, descending = order == GTK_SORT_DESCENDING](
	const NodeRef &o1, const NodeRef &o2) {
      int v = Data::fields()[col].cmp(impl->data(o1), impl->data(o2));
      if (descending) v = -v;
      return v;
    };
  }

public:
  void sort(gint col, GtkSortType order) {
    unsigned n = m_rows.length();
    ZuSort(&m_rows[0], n, cmp_(col, order));
    ZtArray new_order(n);
    for (unsigned i = 0; i < n; i++) {
      new_order.push(impl()->row(m_rows[i]));
      impl()->row(m_rows[i], i);
    }
    GtkTreePath *path = gtk_tree_path_new();
    gtk_tree_model_rows_reordered(
	GTK_TREE_MODEL(this), path, nullptr, new_order.data());
    gtk_tree_path_free(path);
  }

  void add(NodeRef node) {
    gint col;
    GtkSortType order;
    unsigned i;
    if (this->get_sort_column_id(&col, &order)) {
      i = ZuSearch(&m_rows[0], m_rows.length(), node, cmp_(col, order),
	  [](int) constexpr { return false; }); // do not find
      impl()->row(node, ZuSearchPos(i));
      m_rows.splice(i, 0, ZuMv(node));
    } else {
      i = m_rows.length();
      impl()->row(node, i);
      m_rows.push(ZuMv(node));
    }
    GtkTreeIter iter;
    iter.user_data = reinterpret_cast<gpointer>(static_cast<uintptr_t>(i));
    GtkTreePath *path = gtk_tree_path_new();
    gtk_tree_path_append_index(path, i);
    gtk_tree_model_row_inserted(GTK_TREE_MODEL(this), path, &iter);
    gtk_tree_path_free(path);
  }

  void del(const Node *node) {
    unsigned i = impl()->row(node);
    GtkTreePath *path = gtk_tree_path_new();
    gtk_tree_path_append_index(path, i);
    gtk_tree_model_row_deleted(GTK_TREE_MODEL(this), path);
    gtk_tree_path_free(path);
    m_rows.splice(i, 1);
  }

private:
  ZtArray<NodeRef>	m_rows;
};

} // ZGtk

#endif /* ZGtkTreeModel_HPP */
