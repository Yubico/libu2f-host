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

LIBJSONVERSION=0.11
HIDAPIHASH=a6a622ffb680c55da0de787ff93b80280498330f
PACKAGE=libu2f-host

all: usage doit

.PHONY: usage
usage:
	@if test -z "$(VERSION)"; then \
		echo "Try this instead:"; \
		echo "  make VERSION=[VERSION]"; \
		echo "For example:"; \
		echo "  make VERSION=1.6.0"; \
		exit 1; \
	fi

doit:
	rm -rf tmp && mkdir tmp && cd tmp && \
	mkdir -p root/licenses && \
	cp ../json-c-$(LIBJSONVERSION).tar.gz . || \
	curl -O -L https://s3.amazonaws.com/json-c_releases/releases/json-c-$(LIBJSONVERSION).tar.gz && \
	tar xfz json-c-$(LIBJSONVERSION).tar.gz && \
	cd json-c-$(LIBJSONVERSION) && \
	./configure --prefix=$(PWD)/tmp$(ARCH)/root CFLAGS=-mmacosx-version-min=10.6 && \
	make install check && \
	cp COPYING $(PWD)/tmp$(ARCH)/root/licenses/json-c.txt && \
	cd .. && \
	git clone https://github.com/signal11/hidapi.git && \
	cd hidapi && \
	git checkout $(HIDAPIHASH) && \
	./bootstrap && \
	./configure --prefix=$(PWD)/tmp$(ARCH)/root CFLAGS=-mmacosx-version-min=10.6 && \
	make install check && \
	cp LICENSE-gpl3.txt $(PWD)/tmp$(ARCH)/root/licenses/hidapi.txt && \
	cd .. && \
	cp ../$(PACKAGE)-$(VERSION).tar.xz . && \
	tar xfJ $(PACKAGE)-$(VERSION).tar.xz && \
	cd $(PACKAGE)-$(VERSION)/ && \
	PKG_CONFIG_PATH=$(PWD)/tmp$(ARCH)/root/lib/pkgconfig ./configure --prefix=$(PWD)/tmp$(ARCH)/root CFLAGS=-mmacosx-version-min=10.6 && \
	make install check && \
	install_name_tool -id @executable_path/../lib/libjson-c.2.dylib $(PWD)/tmp/root/lib/libjson-c.2.dylib && \
	install_name_tool -id @executable_path/../lib/libjson.0.dylib $(PWD)/tmp/root/lib/libjson.0.dylib && \
	install_name_tool -change $(PWD)/tmp/root/lib/libjson-c.2.dylib @executable_path/../lib/libjson-c.2.dylib $(PWD)/tmp/root/lib/libjson.dylib && \
	install_name_tool -id @executable_path/../lib/libhidapi.0.dylib $(PWD)/tmp/root/lib/libhidapi.0.dylib && \
	install_name_tool -id @executable_path/../lib/libu2f-host.0.dylib $(PWD)/tmp/root/lib/libu2f-host.0.dylib && \
	install_name_tool -change $(PWD)/tmp/root/lib/libjson-c.2.dylib @executable_path/../lib/libjson-c.2.dylib $(PWD)/tmp/root/lib/libu2f-host.0.dylib && \
	install_name_tool -change $(PWD)/tmp/root/lib/libhidapi.0.dylib @executable_path/../lib/libhidapi.0.dylib $(PWD)/tmp/root/lib/libu2f-host.0.dylib && \
	for executable in $(PWD)/tmp/root/bin/*; do \
	install_name_tool -change $(PWD)/tmp/root/lib/libjson-c.2.dylib @executable_path/../lib/libjson-c.2.dylib $$executable ; \
	install_name_tool -change $(PWD)/tmp/root/lib/libhidapi.0.dylib @executable_path/../lib/libhidapi.0.dylib $$executable ; \
	install_name_tool -change $(PWD)/tmp/root/lib/libu2f-host.0.dylib @executable_path/../lib/libu2f-host.0.dylib $$executable ; \
	done && \
	rm $(PWD)/tmp/root/lib/*.la && \
	rm -rf $(PWD)/tmp/root/lib/pkgconfig && \
	cp COPYING $(PWD)/tmp$(ARCH)/root/licenses/$(PACKAGE).txt && \
	cd .. && \
	cd root && \
	zip -r ../../$(PACKAGE)-$(VERSION)-mac.zip *

upload:
	@if test ! -d "$(YUBICO_WWW_REPO)"; then \
		echo "www repo not found!"; \
		echo "Make sure that YUBICO_WWW_REPO is set"; \
		exit 1; \
		fi
	gpg --detach-sign --default-key $(PGPKEYID) $(PACKAGE)-$(VERSION)-mac.zip
	gpg --verify $(PACKAGE)-$(VERSION)-mac.zip.sig
	$(YUBICO_WWW_REPO)/publish $(PACKAGE) $(VERSION) $(PACKAGE)-$(VERSION)-mac.zip*
