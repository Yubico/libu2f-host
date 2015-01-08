# Copyright (C) 2013-2015 Yubico AB
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, eithe version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

AM_CFLAGS = $(WARN_CFLAGS)
AM_CPPFLAGS=-I$(srcdir)/.. -I$(builddir)/..

AM_LDFLAGS = -no-install
LDADD = ../u2f-host/libu2f-host.la

check_PROGRAMS = basic
TESTS = $(check_PROGRAMS)
