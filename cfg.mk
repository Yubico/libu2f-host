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

CFGFLAGS = --enable-gtk-doc --enable-gtk-doc-pdf --enable-gcc-warnings

ifeq ($(.DEFAULT_GOAL),abort-due-to-no-makefile)
.DEFAULT_GOAL := bootstrap
endif

autoreconf:
	autoreconf --install --verbose

bootstrap: autoreconf
	./configure $(CFGFLAGS)
	make

INDENT_SOURCES = `find . -name '*.[ch]' -o -name '*.h.in' | \
	grep -v -e /gl/ -e build-aux`

update-copyright-env = UPDATE_COPYRIGHT_HOLDER="Yubico AB" \
	UPDATE_COPYRIGHT_USE_INTERVALS=1

local-checks-to-skip = sc_bindtextdomain sc_immutable_NEWS sc_program_name
local-checks-to-skip += sc_prohibit_strcmp sc_unmarked_diagnostics
local-checks-to-skip += sc_GPL_version

exclude_file_name_regexp--sc_m4_quote_check = ^gl/m4/
exclude_file_name_regexp--sc_makefile_at_at_check = ^maint.mk|gl/Makefile.am
exclude_file_name_regexp--sc_prohibit_undesirable_word_seq = ^maint.mk
exclude_file_name_regexp--sc_prohibit_atoi_atof = ^src/u2f-host.c
exclude_file_name_regexp--sc_space_tab = ^gtk-doc/gtk-doc.make
exclude_file_name_regexp--sc_trailing_blank = ^u2f-host/cdecode.c|u2f-host/cencode.c|u2f-host/inc/u2f.h|u2f-host/inc/u2f_hid.h
