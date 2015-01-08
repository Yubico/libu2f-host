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

SUBDIRS = gl u2f-host src tests gtk-doc

ACLOCAL_AMFLAGS = -I m4 -I gl/m4

DISTCHECK_CONFIGURE_FLAGS = --enable-gtk-doc

EXTRA_DIST = 70-u2f.rules BLURB cfg.mk macosx.mk windows.mk

udevrulesdir = @udevrulesdir@
dist_udevrules_DATA = 70-u2f.rules

# Release

ChangeLog:
	cd $(srcdir) && git2cl > ChangeLog

release: my-release
my-release:
	@if test ! -d "$(YUBICO_WWW_REPO)"; then \
		echo "www repo not found!"; \
		echo "Make sure that YUBICO_WWW_REPO is set"; \
		exit 1; \
	fi
	@head -3 $(srcdir)/NEWS | \
		grep -q "Version $(VERSION) .released `date -I`" || \
		(echo 'error: Update date/version in $(srcdir)/NEWS'; exit 1)
	rm -f $(srcdir)/ChangeLog
	make ChangeLog syntax-check distcheck
	make -f windows.mk VERSION=$(VERSION)
	gpg --detach-sign $(PACKAGE)-$(VERSION).tar.xz
	gpg --detach-sign $(PACKAGE)-$(VERSION)-win32.zip
	gpg --detach-sign $(PACKAGE)-$(VERSION)-win64.zip

	gpg --verify $(PACKAGE)-$(VERSION).tar.xz.sig
	gpg --verify $(PACKAGE)-$(VERSION)-win32.zip.sig
	gpg --verify $(PACKAGE)-$(VERSION)-win64.zip.sig
	cd $(srcdir) && git push
	cd $(srcdir) && git tag -s -m "$(PACKAGE) $(VERSION)" $(PACKAGE)-$(VERSION)
	cd $(srcdir) && git push --tags
	$(YUBICO_WWW_REPO)/publish $(PACKAGE) $(VERSION) $(PACKAGE)-$(VERSION).tar.xz*
	$(YUBICO_WWW_REPO)/publish $(PACKAGE) $(VERSION) $(PACKAGE)-$(VERSION)-win32.zip*
	$(YUBICO_WWW_REPO)/publish $(PACKAGE) $(VERSION) $(PACKAGE)-$(VERSION)-win64.zip*
	rsync -a $(srcdir)/gtk-doc/html/ $(YUBICO_WWW_REPO)/$(PACKAGE)/gtk-doc/
