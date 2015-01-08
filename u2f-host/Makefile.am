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
AM_CPPFLAGS = -I$(top_srcdir)/gl -I$(top_builddir)/gl
AM_CPPFLAGS += -I$(srcdir)/.. -I$(builddir)/..
AM_CPPFLAGS += $(HIDAPI_CFLAGS) $(LIBJSON_CFLAGS)

lib_LTLIBRARIES = libu2f-host.la
u2f_host_includedir=$(includedir)/u2f-host
u2f_host_include_HEADERS = u2f-host.h u2f-host-types.h u2f-host-version.h

libu2f_host_la_SOURCES = u2f-host.h u2f-host-types.h u2f-host-version.h
libu2f_host_la_SOURCES += internal.h
libu2f_host_la_SOURCES += u2f-host.pc.in u2f-host.map
libu2f_host_la_SOURCES += global.c version.c error.c
libu2f_host_la_SOURCES += devs.c register.c authenticate.c u2fmisc.c
libu2f_host_la_SOURCES += inc/u2f.h inc/u2f_hid.h

libu2f_host_la_LIBADD = $(HIDAPI_LIBS) $(LIBJSON_LIBS)
libu2f_host_la_LIBADD += libu2f_b64.la
libu2f_host_la_LIBADD += ../gl/libgnu.la

libu2f_host_la_LDFLAGS = -no-undefined \
	-version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE)

if HAVE_LD_VERSION_SCRIPT
libu2f_host_la_LDFLAGS += -Wl,--version-script=$(srcdir)/u2f-host.map
else
libu2f_host_la_LDFLAGS += -export-symbols-regex '^u2fh_.*'
endif

noinst_LTLIBRARIES = libu2f_b64.la
libu2f_b64_la_SOURCES = cencode.c cdecode.c b64/cencode.h b64/cdecode.h
libu2f_b64_la_CFLAGS =

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = u2f-host.pc
