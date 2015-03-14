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

LIBJSONVERSION=0.11
HIDAPIHASH=0cbc3a409bcb45cefb3edbf144d64ddd4e0821ce
PACKAGE=libu2f-host

all: usage clean prepare json-c hidapi doit

.PHONY: usage clean prepare json-c hidapi
usage:
	@if test -z "$(VERSION)"; then \
		echo "Try this instead:"; \
		echo "  make VERSION=[VERSION]"; \
		echo "For example:"; \
		echo "  make VERSION=1.6.0"; \
		exit 1; \
	fi

clean:
	rm -rf tmp 

prepare:
	mkdir -p tmp/root/licenses

json-c:
	cd tmp && \
	cp ../json-c-$(LIBJSONVERSION).tar.gz . || \
	curl -O -L https://s3.amazonaws.com/json-c_releases/releases/json-c-$(LIBJSONVERSION).tar.gz && \
	tar xfz json-c-$(LIBJSONVERSION).tar.gz && \
	cd json-c-$(LIBJSONVERSION) && \
	./configure --prefix=$(PWD)/tmp$(ARCH)/root CFLAGS=-mmacosx-version-min=10.6 && \
	make install check && \
	cp COPYING $(PWD)/tmp$(ARCH)/root/licenses/json-c.txt

hidapi:
	cd tmp && \
	git clone https://github.com/signal11/hidapi.git && \
	cd hidapi && \
	git checkout $(HIDAPIHASH) && \
	./bootstrap && \
	./configure --prefix=$(PWD)/tmp$(ARCH)/root CFLAGS=-mmacosx-version-min=10.6 && \
	make install check && \
	cp LICENSE-gpl3.txt $(PWD)/tmp$(ARCH)/root/licenses/hidapi.txt

doit:
	cd tmp && \
	cp ../$(PACKAGE)-$(VERSION).tar.xz . && \
	tar xfJ $(PACKAGE)-$(VERSION).tar.xz && \
	cd $(PACKAGE)-$(VERSION)/ && \
	PKG_CONFIG_PATH=$(PWD)/tmp$(ARCH)/root/lib/pkgconfig ./configure --prefix=$(PWD)/tmp$(ARCH)/root CFLAGS=-mmacosx-version-min=10.6 && \
	make install check && \
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
