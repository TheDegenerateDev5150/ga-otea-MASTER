/*
 * gnote
 *
 * Copyright (C) 2017-2018 Aurimas Cernius
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


#include <glibmm/init.h>
#include <UnitTest++/UnitTest++.h>

int main(int /*argc*/, char ** /*argv*/)
{
  // force certain timezone so that time tests work
  setenv("TZ", "Europe/London", 1);
  Glib::init();

  return UnitTest::RunAllTests();
}

