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

all: usage 32bit 64bit

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
	rm -rf tmp$(ARCH) && mkdir tmp$(ARCH) && cd tmp$(ARCH) && \
	mkdir -p root/licenses && \
	cp ../json-c-$(LIBJSONVERSION).tar.gz . || \
		wget --no-check-certificate https://s3.amazonaws.com/json-c_releases/releases/json-c-$(LIBJSONVERSION).tar.gz && \
	tar xfa json-c-$(LIBJSONVERSION).tar.gz && \
	cd json-c-$(LIBJSONVERSION) && \
	ac_cv_func_realloc_0_nonnull=yes ac_cv_func_malloc_0_nonnull=yes ./configure --host=$(HOST) --build=x86_64-unknown-linux-gnu --prefix=$(PWD)/tmp$(ARCH)/root && \
	make install && \
	cp COPYING $(PWD)/tmp$(ARCH)/root/licenses/json-c.txt && \
	cd .. && \
	git clone https://github.com/signal11/hidapi.git && \
	cd hidapi && \
	git checkout $(HIDAPIHASH) && \
	./bootstrap && \
	./configure --host=$(HOST) --build=x86_64-unknown-linux-gnu --prefix=$(PWD)/tmp$(ARCH)/root && \
	make install $(CHECK) && \
	cp LICENSE-gpl3.txt $(PWD)/tmp$(ARCH)/root/licenses/hidapi.txt && \
	cd .. && \
	cp ../$(PACKAGE)-$(VERSION).tar.xz . && \
	tar xfa $(PACKAGE)-$(VERSION).tar.xz && \
	cd $(PACKAGE)-$(VERSION)/ && \
	PKG_CONFIG_PATH=$(PWD)/tmp$(ARCH)/root/lib/pkgconfig lt_cv_deplibs_check_method=pass_all ./configure --host=$(HOST) --build=x86_64-unknown-linux-gnu --prefix=$(PWD)/tmp$(ARCH)/root LDFLAGS=-L$(PWD)/tmp$(ARCH)/root/lib CPPFLAGS="-I$(PWD)/tmp$(ARCH)/root/include" && \
	make install $(CHECK) && \
	cp COPYING $(PWD)/tmp$(ARCH)/root/licenses/$(PACKAGE).txt && \
	cd .. && \
	cd root && \
	zip -r ../../$(PACKAGE)-$(VERSION)-win$(ARCH).zip *

32bit:
	$(MAKE) -f windows.mk doit ARCH=32 HOST=i686-w64-mingw32

64bit:
	$(MAKE) -f windows.mk doit ARCH=64 HOST=x86_64-w64-mingw32
