.PHONY: all build clean test install debian version-sync

all: build

build:
	cmake -B build && cmake --build build

test:
	cd build && ctest --output-on-failure

install: build
	cd build && make install

debian: version-sync
	dpkg-buildpackage -b -uc -us

VERSION := $(shell git describe --tags --abbrev=0 | sed 's/^v//')

version-sync:
	@echo "Updating debian changelog to $(VERSION)..."
	@sed -i '1s/.*/open-ndi-monitor ($(VERSION)) stable; urgency=low/' debian/changelog
	@sed -i '1s/  \*.*/  * Version $(VERSION)/' debian/changelog
	@echo "Version updated"

clean:
	rm -rf build/
	rm -rf obj-x86_64-linux-gnu/
	rm -f *.deb *.changes *.buildinfo
	rm -rf debian/open-ndi-monitor/ debian/.debhelper/ debian/debhelper-build-stamp
	rm -f debian/files debian/*.substvars debian/*.postrm.debhelper debian/*.prerm.debhelper