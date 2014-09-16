# Copyright (C) 2013-2014 Yubico AB
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

# Release

ChangeLog:
	cd $(srcdir) && git2cl > ChangeLog

release:
	@head -3 $(srcdir)/NEWS | \
		grep -q "Version $(VERSION) .released `date -I`" || \
		(echo 'error: Update date/version in $(srcdir)/NEWS'; exit 1)
	rm -f $(srcdir)/ChangeLog
	make ChangeLog distcheck
	make -f windows.mk VERSION=$(VERSION)
	gpg --detach-sign $(PACKAGE)-$(VERSION).tar.gz
	gpg --detach-sign $(PACKAGE)-$(VERSION)-win32.zip
	gpg --detach-sign $(PACKAGE)-$(VERSION)-win64.zip
	gpg --verify $(PACKAGE)-$(VERSION).tar.gz.sig $(PACKAGE)-$(VERSION)-win32.zip $(PACKAGE)-$(VERSION)-win64.zip
	cd $(srcdir) && git push
	cd $(srcdir) && git tag -s -m "$(PACKAGE) $(VERSION)" $(PACKAGE)-$(VERSION)
	cd $(srcdir) && git push --tags
