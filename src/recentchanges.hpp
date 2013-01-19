/*
 * gnote
 *
 * Copyright (C) 2010-2013 Aurimas Cernius
 * Copyright (C) 2010 Debarshi Ray
 * Copyright (C) 2009 Hubert Figuiere
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



#ifndef __NOTE_RECENT_CHANGES_HPP_
#define __NOTE_RECENT_CHANGES_HPP_

#include <string>

#include <gtkmm/radiomenuitem.h>
#include <gtkmm/applicationwindow.h>

#include "note.hpp"
#include "searchnoteswidget.hpp"
#include "utils.hpp"

namespace gnote {
  class NoteManager;

class NoteRecentChanges
  : public MainWindow
{
public:
  NoteRecentChanges(NoteManager& m);
  virtual ~NoteRecentChanges();
  virtual void set_search_text(const std::string & value);
  virtual void present_note(const Note::Ptr & note);
  virtual void new_note();
  virtual void present_search();

  virtual void embed_widget(utils::EmbeddableWidget &);
  virtual void unembed_widget(utils::EmbeddableWidget &);
  virtual void foreground_embedded(utils::EmbeddableWidget &);
  virtual void background_embedded(utils::EmbeddableWidget &);
  virtual bool running()
    {
      return m_mapped;
    }
protected:
  virtual void on_show();
  virtual bool on_map_event(GdkEventAny *evt);
private:
  void on_open_note(const Note::Ptr &);
  void on_open_note_new_window(const Note::Ptr &);
  void on_delete_note();
  void on_close_window();
  bool on_delete(GdkEventAny *);
  bool on_key_pressed(GdkEventKey *);
  bool is_foreground(utils::EmbeddableWidget &);
  utils::EmbeddableWidget *currently_embedded();
  Gtk::Toolbar *make_toolbar();
  void on_embedded_name_changed(const std::string & name);

  NoteManager        &m_note_manager;
  SearchNotesWidget   m_search_notes_widget;
  Gtk::VBox           m_content_vbox;
  Gtk::VBox           m_embed_box;
  Gtk::ToolButton    *m_all_notes_button;
  std::list<utils::EmbeddableWidget*> m_embedded_widgets;
  bool                m_mapped;
  sigc::connection    m_current_embedded_name_slot;
};


}

#endif
