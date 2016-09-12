# Copyright (C) 2013-2015 Yubico AB
#
# This program is free software; you can redistribute it and/or modify it
# under the terms of the GNU Lesser General Public License as published by
# the Free Software Foundation; either version 2.1, or (at your option)
# any later version.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser
# General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with this program; if not, see <http://www.gnu.org/licenses/>.

AM_CFLAGS = $(WARN_CFLAGS)
AM_CPPFLAGS=-I$(top_srcdir)/u2f-host -I$(top_builddir) -I$(top_builddir)/u2f-host

bin_PROGRAMS = u2f-host

u2f_host_SOURCES = u2f-host.c
u2f_host_LDADD = ../u2f-host/libu2f-host.la
u2f_host_LDADD += libu2f_cmd.la

noinst_LTLIBRARIES = libu2f_cmd.la
libu2f_cmd_la_SOURCES = cmdline.ggo cmdline.c cmdline.h
libu2f_cmd_la_CFLAGS =

cmdline.c cmdline.h: cmdline.ggo Makefile.am
	gengetopt --no-handle-help --input $^

BUILT_SOURCES = cmdline.c cmdline.h
MAINTAINERCLEANFILES = $(BUILT_SOURCES)

dist_man_MANS = u2f-host.1
DISTCLEANFILES = $(dist_man_MANS)

u2f-host.1: $(srcdir)/u2f-host.c $(srcdir)/cmdline.ggo $(top_srcdir)/configure.ac | $(builddir)/u2f-host$(EXEEXT)
	$(AM_V_GEN)$(HELP2MAN) \
		--output=$@ $(builddir)/u2f-host$(EXEEXT) \
		--name="Yubico Universal 2nd Factor (U2F) Host Tool" \
		--no-info
