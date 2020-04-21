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

#include <zlib/ZGtkApp.hpp>
#include <zlib/ZGtkValue.hpp>

#include <zlib/libegg_eggtreemultidnd.h>

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
  void set_sort_func(
      gint column, GtkTreeIterCompareFunc fn,
      gpointer user_data, GDestroyNotify destroy);
  void set_default_sort_func(
      GtkTreeIterCompareFunc fn, gpointer user_data, GDestroyNotify destroy);
  gboolean has_default_sort_func();

  gboolean drag_data_get(GList *path_list, GtkSelectionData *data);
  gboolean drag_data_delete(GList *path_list);
};
#endif

template <typename T>
class TreeModel : public GObject {
public:
  static const char *rowsAtomName() {
    static const char *name = nullptr;
    if (!name) name = typeid(T).name();
    return name;
  }
  static GdkAtom rowsAtom() {
    static GdkAtom atom = nullptr;
    if (!atom) atom = gdk_atom_intern_static_string(rowsAtomName());
    return atom;
  }
  static constexpr gint nRowsTargets() { return 1; }
  static const GtkTargetEntry *rowsTargets() {
    static GtkTargetEntry rowsTargets_[] = {
      { (gchar *)nullptr, GTK_TARGET_SAME_APP, 0 }
    };
    if (!rowsTargets_[0].target) rowsTargets_[0].target =
      const_cast<gchar *>(reinterpret_cast<const gchar *>(rowsAtomName()));
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
#if 0
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
#endif
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
	i->set_sort_func = [](GtkTreeSortable *s,
	    gint column, GtkTreeIterCompareFunc fn,
	    gpointer user_data, GDestroyNotify destroy) {
	  g_return_if_fail(G_TYPE_CHECK_INSTANCE_TYPE((s), gtype()));
	  impl(s)->set_sort_func(column, fn, user_data, destroy);
	};
	i->set_default_sort_func = [](GtkTreeSortable *s,
	    GtkTreeIterCompareFunc fn,
	    gpointer user_data, GDestroyNotify destroy) {
	  g_return_if_fail(G_TYPE_CHECK_INSTANCE_TYPE((s), gtype()));
	  impl(s)->set_default_sort_func(fn, user_data, destroy);
	};
	i->has_default_sort_func = [](GtkTreeSortable *s) -> gboolean {
	  g_return_val_if_fail(G_TYPE_CHECK_INSTANCE_TYPE((s), gtype()), false);
	  return impl(s)->has_default_sort_func();
	};
      },
      nullptr,
      nullptr
    }; 

    static const GInterfaceInfo egg_tree_multi_drag_source_info {
      [](void *i_, void *) {
	auto i = static_cast<EggTreeMultiDragSourceIface *>(i_);
        i->drag_data_get = [](EggTreeMultiDragSource *s,
	    GList *path_list, GtkSelectionData *selection_data) -> gboolean {
	  g_return_val_if_fail(G_TYPE_CHECK_INSTANCE_TYPE((s), gtype()), false);
	  return impl(s)->drag_data_get(path_list, selection_data);
	};
	i->drag_data_delete = [](EggTreeMultiDragSource *s,
	    GList *path_list) -> gboolean {
	  g_return_val_if_fail(G_TYPE_CHECK_INSTANCE_TYPE((s), gtype()), false);
	  return impl(s)->drag_data_delete(path_list);
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
    g_type_add_interface_static(gtype_,
	EGG_TYPE_TREE_MULTI_DRAG_SOURCE, &egg_tree_multi_drag_source_info);

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

  // Drop(TreeModel *model, GtkSelectionData *data)
  template <typename Drop>
  void drag(GtkTreeView *view, GtkWidget *dest, Drop) {
    // gtk_tree_view_set_rubber_banding(view, false);

    gtk_drag_source_set(GTK_WIDGET(view),
	GDK_BUTTON1_MASK,
	rowsTargets(), nRowsTargets(),
	GDK_ACTION_COPY);

    egg_tree_multi_drag_add_drag_support(view);

#if 0
    g_signal_connect(G_OBJECT(view), "drag-data-get",
	ZGtk::callback([](GObject *o, GdkDragContext *,
	    GtkSelectionData *data, guint, guint) {
	  auto view = GTK_TREE_VIEW(o);
	  g_return_if_fail(!!view);
	  auto sel = gtk_tree_view_get_selection(view);
	  g_return_if_fail(!!sel);
	  GtkTreeModel *model = nullptr;
	  auto rows = gtk_tree_selection_get_selected_rows(sel, &model);
	  g_return_if_fail(!!rows);
	  g_return_if_fail(!!model);
	  egg_tree_multi_drag_source_drag_data_get(
	      EGG_TREE_MULTI_DRAG_SOURCE(model), rows, data);
	}), 0);
#endif
 
    gtk_drag_dest_set(dest,
	GTK_DEST_DEFAULT_ALL,
	TreeModel::rowsTargets(), TreeModel::nRowsTargets(),
	GDK_ACTION_COPY);

    g_signal_connect(G_OBJECT(dest), "drag-data-received",
	ZGtk::callback([](GtkWidget *widget, GdkDragContext *, int, int,
	    GtkSelectionData *data, guint, guint32, gpointer model_) {
	  ZuLambdaFn<Drop>::invoke(reinterpret_cast<T *>(model_), data);
	  g_signal_stop_emission_by_name(widget, "drag-data-received");
	}), reinterpret_cast<gpointer>(this));
  }

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
  void set_sort_func(
      gint column, GtkTreeIterCompareFunc fn,
      gpointer user_data, GDestroyNotify destroy) {
    return;
  }
  void set_default_sort_func(
      GtkTreeIterCompareFunc fn, gpointer user_data, GDestroyNotify destroy) {
    return;
  }
  gboolean has_default_sort_func() {
    return false;
  }
};

struct TreeSortFn {
  TreeSortFn() = default;
  TreeSortFn(
      GtkTreeIterCompareFunc fn_, gpointer userData_, GDestroyNotify dtor_) :
      fn{fn_}, userData{userData_}, dtor{dtor_} { }
  ~TreeSortFn() { if (userData && dtor) dtor(userData); }
  TreeSortFn(const TreeSortFn &) = delete;
  TreeSortFn &operator =(const TreeSortFn &) = delete;
  TreeSortFn(TreeSortFn &&o) : fn(o.fn), userData(o.userData), dtor(o.dtor) {
    o.dtor = nullptr;
  }
  TreeSortFn &operator =(TreeSortFn &&o) {
    fn = o.fn;
    userData = o.userData;
    dtor = o.dtor;
    o.dtor = nullptr;
    return *this;
  }

  ZuInline gint operator()(
      GtkTreeModel *model, GtkTreeIter *a, GtkTreeIter *b) const {
    return fn(model, a, b, userData);
  }

  ZuInline bool operator !() const { return !fn; }
  ZuOpBool

  GtkTreeIterCompareFunc	fn = nullptr;
  gpointer			userData = nullptr;
  GDestroyNotify		dtor = nullptr;
};

template <
  typename T,
  unsigned N,
  int DefaultCol = GTK_TREE_SORTABLE_UNSORTED_SORT_COLUMN_ID,
  int DefaultOrder = GTK_SORT_ASCENDING>
class SortableTreeModel : public TreeModel<T> {
private:
  T *impl() { return static_cast<T *>(this); }

public:
  gboolean get_sort_column_id(gint *col_, GtkSortType *order_) {
    if (col_) *col_ = col;
    if (order_) *order_ = order;
    switch ((int)col) {
      case GTK_TREE_SORTABLE_DEFAULT_SORT_COLUMN_ID:
      case GTK_TREE_SORTABLE_UNSORTED_SORT_COLUMN_ID:
	return false;
      default:
	return true;
    }
  }
  void set_sort_column_id(gint col_, GtkSortType order_) {
    if (ZuUnlikely(col == col_ && order == order_)) return;

    col = col_;
    order = order_;

    // emit gtk_tree_sortable_sort_column_changed()
    gtk_tree_sortable_sort_column_changed(GTK_TREE_SORTABLE(this));

    const TreeSortFn *fn = &colFn[col];

    if (!*fn) fn = &defaultFn;

    impl()->sort(col, order, *fn);
  }
  void set_sort_func(
      gint col, GtkTreeIterCompareFunc fn,
      gpointer userData, GDestroyNotify dtor) {
    colFn[col] = TreeSortFn{fn, userData, dtor};
  }
  void set_default_sort_func(
      GtkTreeIterCompareFunc fn, gpointer userData, GDestroyNotify dtor) {
    defaultFn = TreeSortFn{fn, userData, dtor};
  }
  gboolean has_default_sort_func() {
    return !!defaultFn;
  }

  gint		col = DefaultCol;
  GtkSortType	order = static_cast<GtkSortType>(DefaultOrder);
  TreeSortFn	defaultFn;
  TreeSortFn	colFn[N];
};

// wrapper for GtkTreeIter
template <typename T>
class TreeIter : public GtkTreeIter {
  ZuAssert(sizeof(T) < 3 * sizeof(gpointer));

public:
  TreeIter() : GtkTreeIter{0} { }
  TreeIter(TreeIter &&o) : GtkTreeIter{o.stamp} {
    new (&user_data) T{ZuMv(o.data())};
  }
  TreeIter &operator =(TreeIter &&o) {
    if (stamp) data().~T();
    stamp = o.stamp;
    new (&user_data) T{ZuMv(o.data())};
  }
  ~TreeIter() {
    if (stamp) data().~T();
  }

  template <typename ...Args>
  TreeIter(void *model, Args &&... args) :
      GtkTreeIter{(gint)(uintptr_t)model} {
    new (&user_data) T{ZuFwd<Args>(args)...};
  }

  bool valid(void *model) {
    return stamp == (gint)(uintptr_t)model;
  }

  ZuInline static T &data(GtkTreeIter *iter) {
    T *ZuMayAlias(ptr) = reinterpret_cast<T *>(&iter->user_data);
    return *ptr;
  }
  ZuInline static T *ptr(GtkTreeIter *iter) {
    T *ZuMayAlias(ptr) = reinterpret_cast<T *>(&iter->user_data);
    return ptr;
  }

  ZuInline T &data() { return data(this); }
  ZuInline T *ptr() { return ptr(this); }
};

} // ZGtk

#endif /* ZGtkTreeModel_HPP */
