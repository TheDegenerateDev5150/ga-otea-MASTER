/*
 * gnote
 *
 * Copyright (C) 2012-2014,2016,2017,2019-2022 Aurimas Cernius
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


#include "debug.hpp"

#include <glibmm/i18n.h>
#include <glibmm/main.h>
#include <gtkmm/scrolledwindow.h>
#include <gtkmm/treeview.h>

#include "ignote.hpp"
#include "iconmanager.hpp"
#include "notemanager.hpp"
#include "notewindow.hpp"
#include "preferences.hpp"
#include "syncdialog.hpp"
#include "isyncmanager.hpp"


namespace gnote {
namespace sync {

namespace {

class TreeViewModel
  : public Gtk::TreeModelColumnRecord
{
public:
  TreeViewModel()
    {
      add(m_col1);
      add(m_col2);
    }

    Gtk::TreeModelColumn<Glib::ustring> m_col1;
    Gtk::TreeModelColumn<Glib::ustring> m_col2;
};


class SyncTitleConflictDialog
  : public Gtk::Dialog
{
public:
  SyncTitleConflictDialog(const Note::Ptr & existingNote, const std::vector<Glib::ustring> & noteUpdateTitles)
    : Gtk::Dialog(_("Note Conflict"), true)
    , m_existing_note(existingNote)
    , m_note_update_titles(noteUpdateTitles)
    {
      // Suggest renaming note by appending " (old)" to the existing title
      char *old = _(" (old)");
      Glib::ustring suggestedRenameBase = existingNote->get_title() + old;
      Glib::ustring suggestedRename = suggestedRenameBase;
      for(int i = 1; !is_note_title_available(suggestedRename); i++) {
        suggestedRename = suggestedRenameBase + " " + TO_STRING(i);
      }

      Gtk::Grid *outerVBox = Gtk::make_managed<Gtk::Grid>();
      outerVBox->set_row_spacing(8);
      outerVBox->set_margin(12);

      Gtk::Grid *hbox = Gtk::make_managed<Gtk::Grid>();
      hbox->set_column_spacing(8);
      Gtk::Image *image = Gtk::make_managed<Gtk::Image>();
      image->set_from_icon_name("dialog-warning");
      hbox->attach(*image, 0, 0, 1, 1);

      Gtk::Grid *vbox = Gtk::make_managed<Gtk::Grid>();
      vbox->set_row_spacing(8);

      m_header_label = Gtk::make_managed<Gtk::Label>();
      m_header_label->set_use_markup(true);
      m_header_label->property_xalign() = 0;
      m_header_label->set_use_underline(false);
      vbox->attach(*m_header_label, 0, 0, 1, 1);

      m_message_label = Gtk::make_managed<Gtk::Label>();
      m_message_label->property_xalign() = 0;
      m_message_label->set_use_underline(false);
      m_message_label->set_wrap(true);
      m_message_label->property_wrap() = true;
      vbox->attach(*m_message_label, 0, 1, 1, 1);

      vbox->set_hexpand(true);
      hbox->attach(*vbox, 1, 0, 1, 1);

      outerVBox->attach(*hbox, 0, 0, 1, 1);
      get_content_area()->append(*outerVBox);

      Gtk::Grid *renameHBox = Gtk::make_managed<Gtk::Grid>();
      renameRadio = Gtk::make_managed<Gtk::CheckButton>(_("Rename local note:"), true);
      renameRadio->signal_toggled().connect(sigc::mem_fun(*this, &SyncTitleConflictDialog::radio_toggled));
      Gtk::Grid *renameOptionsVBox = Gtk::make_managed<Gtk::Grid>();

      renameEntry = Gtk::make_managed<Gtk::Entry>();
      renameEntry->set_text(suggestedRename);
      renameEntry->signal_changed().connect(sigc::mem_fun(*this, &SyncTitleConflictDialog::rename_entry_changed));
      renameUpdateCheck = Gtk::make_managed<Gtk::CheckButton>(_("Update links in referencing notes"), true);
      renameOptionsVBox->attach(*renameEntry, 0, 0, 1, 1);
      renameHBox->attach(*renameRadio, 0, 0, 1, 1);
      renameHBox->attach(*renameOptionsVBox, 1, 0, 1, 1);
      get_content_area()->append(*renameHBox);

      deleteExistingRadio = Gtk::make_managed<Gtk::CheckButton>(_("Overwrite local note"), true);
      deleteExistingRadio->set_group(*renameRadio);
      deleteExistingRadio->signal_toggled().connect(sigc::mem_fun(*this, &SyncTitleConflictDialog::radio_toggled));
      get_content_area()->append(*deleteExistingRadio);

      alwaysDoThisCheck = Gtk::make_managed<Gtk::CheckButton>(_("Always perform this action"), true);
      get_content_area()->append(*alwaysDoThisCheck);

      continueButton = add_button(_("_Continue"), Gtk::ResponseType::ACCEPT);

      // Set initial dialog text
      header_text(_("Note conflict detected"));
      message_text(Glib::ustring::compose(
        _("The server version of \"%1\" conflicts with your local note.  What do you want to do with your local note?"),
        existingNote->get_title()));
    }
  void header_text(const Glib::ustring & value)
    {
      m_header_label->set_markup(Glib::ustring::compose(
        "<span size=\"large\" weight=\"bold\">%1</span>", value));
    }
  void message_text(const Glib::ustring & value)
    {
      m_message_label->set_text(value);
    }
  Glib::ustring renamed_title() const
    {
      return renameEntry->get_text();
    }
  bool always_perform_this_action() const
    {
      return alwaysDoThisCheck->get_active();
    }
  SyncTitleConflictResolution resolution() const
    {
      if(renameRadio->get_active()) {
        if(renameUpdateCheck->get_active()) {
          return RENAME_EXISTING_AND_UPDATE;
        }
        else {
          return RENAME_EXISTING_NO_UPDATE;
        }
      }
      else {
        return OVERWRITE_EXISTING;
      }
    }
private:
  void rename_entry_changed()
    {
      if(renameRadio->get_active() && !is_note_title_available(renamed_title())) {
        continueButton->set_sensitive(false);
      }
      else {
        continueButton->set_sensitive(true);
      }
    }
  bool is_note_title_available(const Glib::ustring & renamedTitle)
    {
      return std::find(m_note_update_titles.begin(), m_note_update_titles.end(), renamedTitle) == m_note_update_titles.end()
             && m_existing_note->manager().find(renamedTitle) == 0;
    }
  void radio_toggled()
    {
      // Make sure Continue button has the right sensitivity
      rename_entry_changed();

      // Update sensitivity of rename-related widgets
      renameEntry->set_sensitive(renameRadio->get_active());
      renameUpdateCheck->set_sensitive(renameRadio->get_active());
    }

  Note::Ptr m_existing_note;
  std::vector<Glib::ustring> m_note_update_titles;

  Gtk::Button *continueButton;

  Gtk::Entry *renameEntry;
  Gtk::CheckButton *renameUpdateCheck;
  Gtk::CheckButton *renameRadio;
  Gtk::CheckButton *deleteExistingRadio;
  Gtk::CheckButton *alwaysDoThisCheck;

  Gtk::Label *m_header_label;
  Gtk::Label *m_message_label;
};

} // annonymous namespace




SyncDialog::Ptr SyncDialog::create(IGnote & g, NoteManagerBase & m)
{
  return std::make_shared<SyncDialog>(g, m);
}


SyncDialog::SyncDialog(IGnote & g, NoteManagerBase & manager)
  : SyncUI(g, manager)
{
  m_progress_bar_timeout_id = 0;

  set_size_request(400, -1);

  // Outer box. Surrounds all of our content.
  Gtk::Grid *outerVBox = Gtk::make_managed<Gtk::Grid>();
  outerVBox->set_row_spacing(12);
  outerVBox->set_margin(6);
  outerVBox->set_expand(true);
  int outerVBoxRow = 0;
  get_content_area()->append(*outerVBox);

  // Top image and label
  Gtk::Grid *hbox = Gtk::make_managed<Gtk::Grid>();
  hbox->set_column_spacing(12);
  outerVBox->attach(*hbox, 0, outerVBoxRow++, 1, 1);

  m_image = Gtk::make_managed<Gtk::Image>();
  m_image->set_from_icon_name(IconManager::GNOTE);
  m_image->set_halign(Gtk::Align::START);
  m_image->set_valign(Gtk::Align::START);
  hbox->attach(*m_image, 0, 0, 1, 1);

  // Label header and message
  Gtk::Grid *vbox = Gtk::make_managed<Gtk::Grid>();
  vbox->set_row_spacing(6);
  vbox->set_hexpand(true);
  hbox->attach(*vbox, 1, 0, 1, 1);

  m_header_label = Gtk::make_managed<Gtk::Label>();
  m_header_label->set_use_markup(true);
  m_header_label->set_halign(Gtk::Align::START);
  m_header_label->set_use_underline(false);
  m_header_label->set_wrap(true);
  vbox->attach(*m_header_label, 0, 0, 1, 1);

  m_message_label = Gtk::make_managed<Gtk::Label>();
  m_message_label->set_halign(Gtk::Align::START);
  m_message_label->set_use_underline(false);
  m_message_label->set_wrap(true);
  m_message_label->set_size_request(250, -1);
  vbox->attach(*m_message_label, 0, 1, 1, 1);

  m_progress_bar = Gtk::make_managed<Gtk::ProgressBar>();
  m_progress_bar->set_orientation(Gtk::Orientation::HORIZONTAL);
  m_progress_bar->set_pulse_step(0.3);
  outerVBox->attach(*m_progress_bar, 0, outerVBoxRow++, 1, 1);

  m_progress_label = Gtk::make_managed<Gtk::Label>();
  m_progress_label->set_use_markup(true);
  m_progress_label->set_halign(Gtk::Align::START);
  m_progress_label->set_use_underline(false);
  m_progress_label->set_wrap(true);
  m_progress_label->property_wrap() = true;
  outerVBox->attach(*m_progress_label, 0, outerVBoxRow++, 1, 1);

  // Expander containing TreeView
  m_expander = Gtk::make_managed<Gtk::Expander>(_("Details"));
  m_expander->set_margin(6);
  g_signal_connect(m_expander->gobj(), "activate", G_CALLBACK(SyncDialog::on_expander_activated), this);
  m_expander->set_vexpand(true);
  outerVBox->attach(*m_expander, 0, outerVBoxRow++, 1, 1);

  // Contents of expander
  Gtk::Grid *expandVBox = Gtk::make_managed<Gtk::Grid>();
  m_expander->set_child(*expandVBox);

  // Scrolled window around TreeView
  Gtk::ScrolledWindow *scrolledWindow = Gtk::make_managed<Gtk::ScrolledWindow>();
  scrolledWindow->set_size_request(-1, 200);
  scrolledWindow->set_hexpand(true);
  scrolledWindow->set_vexpand(true);
  expandVBox->attach(*scrolledWindow, 0, 0, 1, 1);

  // Create model for TreeView
  // Work-around for GCC versions < 4.3 (http://gcc.gnu.org/bugs/#cxx_rvalbind)
  TreeViewModel tmp_model;
  m_model = Gtk::TreeStore::create(tmp_model);

  // Create TreeView, attach model
  Gtk::TreeView *treeView = Gtk::make_managed<Gtk::TreeView>();
  treeView->set_model(m_model);
  treeView->signal_row_activated().connect(sigc::mem_fun(*this, &SyncDialog::on_row_activated));
  scrolledWindow->set_child(*treeView);

  // Set up TreeViewColumns
  Gtk::CellRenderer *renderer = Gtk::make_managed<Gtk::CellRendererText>();
  Gtk::TreeViewColumn *column = Gtk::make_managed<Gtk::TreeViewColumn>(_("Note Title"), *renderer);
  column->set_sort_column(0);
  column->set_resizable(true);
  column->set_cell_data_func(*renderer, sigc::mem_fun(*this, &SyncDialog::treeview_col1_data_func));
  treeView->append_column(*column);

  renderer = Gtk::make_managed<Gtk::CellRendererText>();
  column = Gtk::make_managed<Gtk::TreeViewColumn>(_("Status"), *renderer);
  column->set_sort_column(1);
  column->set_resizable(true);
  treeView->append_column(*column);
  column->set_cell_data_func(*renderer, sigc::mem_fun(*this, &SyncDialog::treeview_col2_data_func));

  // Button to close dialog.
  m_close_button = add_button(_("_Close"), static_cast<int>(Gtk::ResponseType::CLOSE));
  m_close_button->set_sensitive(false);
}


void SyncDialog::treeview_col1_data_func(Gtk::CellRenderer *renderer, const Gtk::TreeIter<Gtk::TreeConstRow> & iter)
{
  Glib::ustring text;
  iter->get_value(0, text);
  static_cast<Gtk::CellRendererText*>(renderer)->property_text() = text;
}


void SyncDialog::treeview_col2_data_func(Gtk::CellRenderer *renderer, const Gtk::TreeIter<Gtk::TreeConstRow> & iter)
{
  Glib::ustring text;
  iter->get_value(1, text);
  static_cast<Gtk::CellRendererText*>(renderer)->property_text() = text;
}


void SyncDialog::on_realize()
{
  Gtk::Dialog::on_realize();

  SyncState state = m_gnote.sync_manager().state();
  if(state == IDLE) {
    // Kick off a timer to keep the progress bar going
    //m_progress_barTimeoutId = GLib.Timeout.Add (500, OnPulseProgressBar);
    Glib::RefPtr<Glib::TimeoutSource> timeout = Glib::TimeoutSource::create(500);
    timeout->connect(sigc::mem_fun(*this, &SyncDialog::on_pulse_progress_bar));
    timeout->attach();

    // Kick off a new synchronization
    m_gnote.sync_manager().perform_synchronization(this->shared_from_this());
  }
  else {
    // Adjust the GUI accordingly
    sync_state_changed(state);
  }
}


bool SyncDialog::on_pulse_progress_bar()
{
  if(m_gnote.sync_manager().state() == IDLE) {
    return false;
  }

  m_progress_bar->pulse();

  // Return true to keep things going well
  return true;
}


void SyncDialog::on_expander_activated(GtkExpander*, gpointer data)
{
  SyncDialog *this_ = static_cast<SyncDialog*>(data);
  if(this_->m_expander->get_expanded()) {
    this_->set_resizable(true);
  }
  else {
    this_->set_resizable(false);
  }
}


void SyncDialog::on_row_activated(const Gtk::TreeModel::Path & path, Gtk::TreeViewColumn*)
{
  // TODO: Store GUID hidden in model; use instead of title
  Gtk::TreeIter iter = m_model->get_iter(path);
  if(!iter) {
    return;
  }

  Glib::ustring noteTitle;
  iter->get_value(0, noteTitle);

  NoteBase::Ptr note = m_manager.find(noteTitle);
  if(note != 0) {
    present_note(std::static_pointer_cast<Note>(note));
  }
}


void SyncDialog::present_ui()
{
  present();
}


void SyncDialog::header_text(const Glib::ustring & value)
{
  m_header_label->set_markup(Glib::ustring::compose("<span size=\"large\" weight=\"bold\">%1</span>", value));
}


void SyncDialog::message_text(const Glib::ustring & value)
{
  m_message_label->set_text(value);
}


Glib::ustring SyncDialog::progress_text() const
{
  return m_progress_label->get_text();
}


void SyncDialog::progress_text(const Glib::ustring & value)
{
  m_progress_label->set_markup(
    Glib::ustring::compose("<span style=\"italic\">%1</span>", value));
}


void SyncDialog::add_update_item(const Glib::ustring & title, Glib::ustring & status)
{
  Gtk::TreeIter iter = m_model->append();
  iter->set_value(0, title);
  iter->set_value(1 , status);
}


void SyncDialog::sync_state_changed(SyncState state)
{
  // This event handler will be called by the synchronization thread
  utils::main_context_invoke([this, state]() { sync_state_changed_(state); });
}


void SyncDialog::sync_state_changed_(SyncState state)
{
  // FIXME: Change these strings to be user-friendly
  switch(state) {
  case ACQUIRING_LOCK:
    progress_text(_("Acquiring sync lock..."));
    break;
  case COMMITTING_CHANGES:
    progress_text(_("Committing changes..."));
    break;
  case CONNECTING:
    set_title(_("Synchronizing Notes"));
    header_text(_("Synchronizing your notes..."));
    message_text(_("This may take a while, kick back and enjoy!"));
    m_model->clear();
    progress_text(_("Connecting to the server..."));
    m_progress_bar->set_fraction(0);
    m_progress_bar->show();
    m_progress_label->show();
    break;
  case DELETE_SERVER_NOTES:
    progress_text(_("Deleting notes off of the server..."));
    m_progress_bar->pulse();
    break;
  case DOWNLOADING:
    progress_text(_("Downloading new/updated notes..."));
    m_progress_bar->pulse();
    break;
  case IDLE:
    //GLib.Source.Remove (m_progress_barTimeoutId);
    //m_progress_barTimeoutId = 0;
    m_progress_bar->set_fraction(0);
    m_progress_bar->hide();
    m_progress_label->hide();
    m_close_button->set_sensitive(true);
    break;
  case LOCKED:
    set_title(_("Server Locked"));
    header_text(_("Server is locked"));
    message_text(_("One of your other computers is currently synchronizing.  Please wait 2 minutes and try again."));
    progress_text("");
    break;
  case PREPARE_DOWNLOAD:
    progress_text(_("Preparing to download updates from server..."));
    break;
  case PREPARE_UPLOAD:
    progress_text(_("Preparing to upload updates to server..."));
    break;
  case UPLOADING:
    progress_text(_("Uploading notes to server..."));
    break;
  case FAILED:
    set_title(_("Synchronization Failed"));
    header_text(_("Failed to synchronize"));
    message_text(_("Could not synchronize notes.  Check the details below and try again."));
    progress_text("");
    break;
  case SUCCEEDED:
    {
      int count = m_model->children().size();
      set_title(_("Synchronization Complete"));
      header_text(_("Synchronization is complete"));
      Glib::ustring numNotesUpdated = ngettext("%1 note updated.", "%1 notes updated.", count);
      message_text(Glib::ustring::compose(numNotesUpdated, count) + "  " + _("Your notes are now up to date."));
      progress_text("");
    }
    break;
  case USER_CANCELLED:
    set_title(_("Synchronization Canceled"));
    header_text(_("Synchronization was canceled"));
    message_text(_("You canceled the synchronization.  You may close the window now."));
    progress_text("");
    break;
  case NO_CONFIGURED_SYNC_SERVICE:
    set_title(_("Synchronization Not Configured"));
    header_text(_("Synchronization is not configured"));
    message_text(_("Please configure synchronization in the preferences dialog."));
    progress_text("");
    break;
  case SYNC_SERVER_CREATION_FAILED:
    set_title(_("Synchronization Service Error"));
    header_text(_("Service error"));
    message_text(_("Error connecting to the synchronization service.  Please try again."));
    progress_text("");
    break;
  }
}


void SyncDialog::note_synchronized(const Glib::ustring & noteTitle, NoteSyncType type)
{
  // FIXME: Change these strings to be more user-friendly
  // TODO: Update status for a note when status changes ("Uploading" -> "Uploaded", etc)
  Glib::ustring statusText;
  switch(type) {
  case DELETE_FROM_CLIENT:
    statusText = _("Deleted locally");
    break;
  case DELETE_FROM_SERVER:
    statusText = _("Deleted from server");
    break;
  case DOWNLOAD_MODIFIED:
    statusText = _("Updated");
    break;
  case DOWNLOAD_NEW:
    statusText = _("Added");
    break;
  case UPLOAD_MODIFIED:
    statusText = _("Uploaded changes to server");
    break;
  case UPLOAD_NEW:
    statusText = _("Uploaded new note to server");
    break;
  }
  add_update_item(noteTitle, statusText);
}


void SyncDialog::note_conflict_detected(const Note::Ptr & localConflictNote,
                                        NoteUpdate remoteNote,
                                        const std::vector<Glib::ustring> & noteUpdateTitles)
{
  int dlgBehaviorPref = m_gnote.preferences().sync_configured_conflict_behavior();

  // This event handler will be called by the synchronization thread
  // so we have to use the delegate here to manipulate the GUI.
  // To be consistent, any exceptions in the delgate will be caught
  // and then rethrown in the synchronization thread.
  utils::main_context_call(
    [this, localConflictNote, remoteNote, noteUpdateTitles, dlgBehaviorPref]() {
      note_conflict_detected_(localConflictNote, remoteNote, noteUpdateTitles,
                              static_cast<SyncTitleConflictResolution>(dlgBehaviorPref),
                              OVERWRITE_EXISTING
      );
    });
}


void SyncDialog::note_conflict_detected_(
  const Note::Ptr & localConflictNote,
  NoteUpdate remoteNote,
  const std::vector<Glib::ustring> & noteUpdateTitles,
  SyncTitleConflictResolution savedBehavior,
  SyncTitleConflictResolution resolution)
{
  SyncTitleConflictDialog conflictDlg(localConflictNote, noteUpdateTitles);
  Gtk::ResponseType reponse = Gtk::ResponseType::OK;

  bool noteSyncBitsMatch = m_gnote.sync_manager().synchronized_note_xml_matches(
    localConflictNote->get_complete_note_xml(), remoteNote.m_xml_content);

  // If the synchronized note content is in conflict
  // and there is no saved conflict handling behavior, show the dialog
  if(!noteSyncBitsMatch && savedBehavior == 0) {
    reponse = static_cast<Gtk::ResponseType>(conflictDlg.run());
  }


  if(reponse == Gtk::ResponseType::CANCEL) {
    resolution = CANCEL;
  }
  else {
    if(noteSyncBitsMatch) {
      resolution = OVERWRITE_EXISTING;
    }
    else if(savedBehavior == 0) {
      resolution = conflictDlg.resolution();
    }
    else {
      resolution = savedBehavior;
    }

    switch(resolution) {
    case OVERWRITE_EXISTING:
      if(conflictDlg.always_perform_this_action()) {
        savedBehavior = resolution;
      }
      // No need to delete if sync will overwrite
      if(localConflictNote->id() != remoteNote.m_uuid) {
        m_manager.delete_note(localConflictNote);
      }
      break;
    case RENAME_EXISTING_AND_UPDATE:
      if(conflictDlg.always_perform_this_action()) {
        savedBehavior = resolution;
      }
      rename_note(localConflictNote, conflictDlg.renamed_title(), true);
      break;
    case RENAME_EXISTING_NO_UPDATE:
      if(conflictDlg.always_perform_this_action()) {
        savedBehavior = resolution;
      }
      rename_note(localConflictNote, conflictDlg.renamed_title(), false);
      break;
    case CANCEL:
      break;
    }
  }

  m_gnote.preferences().sync_configured_conflict_behavior(static_cast<int>(savedBehavior));

  conflictDlg.hide();

  // Let the SyncManager continue
  m_gnote.sync_manager().resolve_conflict(/*localConflictNote, */resolution);
}


void SyncDialog::rename_note(const Note::Ptr & note, Glib::ustring && newTitle, bool)
{
  Glib::ustring oldTitle = note->get_title();
  // Rename the note (skip for now...never using updateReferencingNotes option)
  //if (updateReferencingNotes) // NOTE: This might never work, or lead to a ton of conflicts
  // note.Title = newTitle;
  //else
  // note.RenameWithoutLinkUpdate (newTitle);
  //string oldContent = note.XmlContent;
  //note.XmlContent = NoteArchiver.Instance.GetRenamedNoteXml (oldContent, oldTitle, newTitle);

  // Preserve note information
  note->save(); // Write to file
  bool noteOpen = note->is_opened();
  Glib::ustring newContent = //note.XmlContent;
    m_manager.note_archiver().get_renamed_note_xml(note->xml_content(), oldTitle, newTitle);
  Glib::ustring newCompleteContent = //note.GetCompleteNoteXml ();
    m_manager.note_archiver().get_renamed_note_xml(note->get_complete_note_xml(), oldTitle, newTitle);
  //Logger.Debug ("RenameNote: newContent: " + newContent);
  //Logger.Debug ("RenameNote: newCompleteContent: " + newCompleteContent);

  // We delete and recreate the note to simplify content conflict handling
  m_manager.delete_note(note);

  // Create note with old XmlContent just in case GetCompleteNoteXml failed
  DBG_OUT("RenameNote: about to create %s", newTitle.c_str());
  Note::Ptr renamedNote = std::static_pointer_cast<Note>(m_manager.create(std::move(newTitle), std::move(newContent)));
  if(newCompleteContent != "") {// TODO: Anything to do if it is null?
    try {
      renamedNote->load_foreign_note_xml(newCompleteContent, OTHER_DATA_CHANGED);
    }
    catch(...) {} // TODO: Handle exception in case that newCompleteContent is invalid XML
  }
  if(noteOpen) {
    present_note(renamedNote);
  }
}

void SyncDialog::present_note(const Note::Ptr & note)
{
  MainWindow::present_in(m_gnote.get_window_for_note(), note);
}

}
}
