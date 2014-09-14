/*
 * gnote
 *
 * Copyright (C) 2014 Aurimas Cernius
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


#include "testsyncaddin.hpp"
#include "testsyncmanager.hpp"

namespace test {

SyncManager::SyncManager(gnote::NoteManagerBase & manager)
  : gnote::sync::SyncManager(manager)
{
  m_client = gnote::sync::SyncClient::Ptr(new test::SyncClient(manager));
}

test::SyncClient::Ptr SyncManager::get_client(const std::string & manifest)
{
  SyncClient::Ptr client = dynamic_pointer_cast<SyncClient>(m_client);
  client->set_manifest_path(manifest);
  client->reparse();
  return client;
}

void SyncManager::reset_client()
{
}

void SyncManager::perform_synchronization(const gnote::sync::SyncUI::Ptr & sync_ui)
{
  m_sync_ui = sync_ui;
  synchronization_thread();
}

void SyncManager::resolve_conflict(gnote::sync::SyncTitleConflictResolution resolution)
{
}

bool SyncManager::synchronized_note_xml_matches(const std::string & noteXml1, const std::string & noteXml2)
{
  return false;
}

gnote::sync::SyncServiceAddin *SyncManager::get_sync_service_addin(const std::string & sync_service_id)
{
  return new SyncAddin();
}

gnote::sync::SyncServiceAddin *SyncManager::get_configured_sync_service()
{
  return get_sync_service_addin("");
}

}

