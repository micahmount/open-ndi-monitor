.PHONY: all build clean test install debian

all: build

build:
	cmake -B build && cmake --build build

test:
	cd build && ctest --output-on-failure

install: build
	cd build && make install

debian:
	dpkg-buildpackage -b -uc -us

clean:
	rm -rf build/
	rm -rf obj-x86_64-linux-gnu/
	rm -f *.deb *.changes *.buildinfo
	rm -rf debian/*.deb debian/*.changes debian/*.buildinfo
	rm -rf debian/open-ndi-monitor/ debian/.debhelper/ debian/debhelper-build-stamp
	rm -f debian/files debian/*.substvars debian/*.postrm.debhelper debian/*.prerm.debhelper

version-bump:
	./scripts/increment-version.sh

rebuild: version-bump build