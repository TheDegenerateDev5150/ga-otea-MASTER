/*
 * gnote
 *
 * Copyright (C) 2011,2013-2014,2017,2019 Aurimas Cernius
 * Copyright (C) 2010 Debarshi Ray
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __NOTE_RENAME_DIALOG_HPP_
#define __NOTE_RENAME_DIALOG_HPP_

#include <map>

#include <gtkmm/grid.h>
#include <gtkmm/liststore.h>
#include <gtkmm/radiobutton.h>
#include <gtkmm/treeview.h>

#include "note.hpp"

namespace gnote {

class IGnote;


// Values should match with those in data/gnote.schemas.in
enum NoteRenameBehavior {
  NOTE_RENAME_ALWAYS_SHOW_DIALOG = 0,
  NOTE_RENAME_ALWAYS_REMOVE_LINKS = 1,
  NOTE_RENAME_ALWAYS_RENAME_LINKS = 2
};

class ModelColumnRecord
  : public Gtk::TreeModelColumnRecord
{
public:

  ModelColumnRecord();
  virtual ~ModelColumnRecord();

  const Gtk::TreeModelColumn<bool> & get_column_selected() const;
  gint get_column_selected_num() const;

  const Gtk::TreeModelColumn<Glib::ustring> & get_column_title() const;
  gint get_column_title_num() const;

  const Gtk::TreeModelColumn<NoteBase::Ptr> & get_column_note() const;
  gint get_column_note_num() const;

private:

  enum {
    COLUMN_BOOL = 0,
    COLUMN_TITLE,
    COLUMN_NOTE,
    COLUMN_COUNT
  };

  Gtk::TreeModelColumn<bool> m_column_selected;
  Gtk::TreeModelColumn<Glib::ustring> m_column_title;
  Gtk::TreeModelColumn<NoteBase::Ptr> m_column_note;
};

class NoteRenameDialog
  : public Gtk::Dialog
{
public:

  typedef std::shared_ptr<std::map<NoteBase::Ptr, bool> > MapPtr;

  NoteRenameDialog(const NoteBase::List & notes,
                   const Glib::ustring & old_title,
                   const NoteBase::Ptr & renamed_note,
                   IGnote & g);
  MapPtr get_notes() const;
  NoteRenameBehavior get_selected_behavior() const;

private:

  void on_advanced_expander_changed(bool expanded);
  void on_always_rename_clicked();
  void on_always_show_dlg_clicked();
  void on_never_rename_clicked();
  bool on_notes_model_foreach_iter_accumulate(
         const Gtk::TreeIter & iter,
         const MapPtr & notes) const;
  bool on_notes_model_foreach_iter_select(const Gtk::TreeIter & iter,
                                          bool select);
  void on_notes_view_row_activated(const Gtk::TreeModel::Path & p,
                                   Gtk::TreeView::Column *,
                                   const Glib::ustring & old_title);
  void on_select_all_button_clicked(bool select);
  void on_toggle_cell_toggled(const Glib::ustring & p);

  IGnote & m_gnote;
  ModelColumnRecord m_model_column_record;
  Glib::RefPtr<Gtk::ListStore> m_notes_model;
  Gtk::Button m_dont_rename_button;
  Gtk::Button m_rename_button;
  Gtk::Button m_select_all_button;
  Gtk::Button m_select_none_button;
  Gtk::RadioButton m_always_show_dlg_radio;
  Gtk::RadioButton m_always_rename_radio;
  Gtk::RadioButton m_never_rename_radio;
  Gtk::Grid m_notes_box;
};

}

#endif
