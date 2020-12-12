PACKAGEMAKER=/Developer/Tools/packagemaker

MACOSX_GTKPPC_HOME=/Users/gtk-ppc
MACOSX_GTKX86_HOME=/Users/gtk-x86

all: backend cmd macdib FC5025_MacOSX_Setup.dmg

backend:
	make -C ../backend clean
	make -C ../backend macosx-ppc macosx-x86

cmd:
	make -C ../cmd clean
	make -C ../cmd macosx-univ
	cp ../cmd/OBJ.macosx-univ/fcbrowse ../cmd/OBJ.macosx-univ/fcdrives ../cmd/OBJ.macosx-univ/fcformats ../cmd/OBJ.macosx-univ/fcimage .
	for i in fcbrowse fcdrives fcformats fcimage; do install_name_tool -change /usr/local/lib/libusb-0.1.4.dylib @executable_path/../libusb.pkg/Contents/usr/local/lib/libusb-0.1.4.dylib $$i; done

macdib:
	make -C ../xdib clean
	make -C ../xdib macosx-ppc macosx-x86
	cp ../xdib/OBJ.macosx-ppc/xdib macdib-ppc
	cp ../xdib/OBJ.macosx-x86/xdib macdib-x86
	install_name_tool -change /usr/local/lib/libusb-0.1.4.dylib @executable_path/../libusb.pkg/Contents/usr/local/lib/libusb-0.1.4.dylib macdib-ppc
	install_name_tool -change /usr/local/lib/libusb-0.1.4.dylib @executable_path/../libusb.pkg/Contents/usr/local/lib/libusb-0.1.4.dylib macdib-x86

bundle: macdib
	rm -rf MacDIB.app MacDIB-ppc.app MacDIB-x86.app MacDIB-dual.app
	echo '*** PPC ***'
	TARGET_ARCH=ppc GTKHOME=$(MACOSX_GTKPPC_HOME) PATH=$(MACOSX_GTKPPC_HOME)/gtk/inst/bin:$$PATH PKG_CONFIG_PATH=$(MACOSX_GTKPPC_HOME)/gtk/inst/lib/pkgconfig ige-mac-bundler macdib.bundle
	find MacDIB.app -name '*.dylib' -exec strip -x '{}' \;
	find MacDIB.app -name '*.so' -exec strip -x '{}' \;
	cp launcher.sh MacDIB.app/Contents/MacOS/MacDIB
	mv MacDIB.app MacDIB-ppc.app
	echo '*** X86 ***'
	mkdir -p MacDIB-x86.app/Contents/MacOS MacDIB-x86.app/Contents/Resources/lib/gtk-2.0/2.10.0/engines MacDIB-x86.app/Contents/Resources/lib/gtk-2.0/2.10.0/immodules MacDIB-x86.app/Contents/Resources/lib/gtk-2.0/2.10.0/printbackends MacDIB-x86.app/Contents/Resources/lib/gtk-2.0/include MacDIB-x86.app/Contents/Resources/lib/gtk-2.0/modules MacDIB-x86.app/Contents/Resources/share/icons/hicolor
	cp -R -p MacDIB-ppc.app/Contents/Resources/etc MacDIB-x86.app/Contents/Resources/
	cp -p MacDIB-ppc.app/Contents/Resources/share/icons/hicolor/index.theme MacDIB-x86.app/Contents/Resources/share/icons/hicolor/
	cd MacDIB-ppc.app/Contents/Resources && find lib -type f -exec cp -p $(MACOSX_GTKX86_HOME)/gtk/inst/'{}' ../../../MacDIB-x86.app/Contents/Resources/'{}' \;
	cp ../libusb.pkg/Contents/usr/local/lib/libusb-0.1.4.dylib MacDIB-x86.app/Contents/Resources/lib/
	echo -n '????' > MacDIB-x86.app/Contents/PkgInfo
	cp Info.plist MacDIB-x86.app/Contents/
	cp ../xdib/OBJ.macosx-x86/xdib MacDIB-x86.app/Contents/MacOS/MacDIB-bin
	find MacDIB-x86.app -name '*.dylib' -exec strip -x '{}' \;
	find MacDIB-x86.app -name '*.so' -exec strip -x '{}' \;
	cp launcher.sh MacDIB-x86.app/Contents/MacOS/MacDIB
	echo '*** Dual ***'
	mkdir -p MacDIB-dual.app/Contents/MacOS MacDIB-dual.app/Contents/Resources MacDIB-dual.app/Contents/Resources
	echo -n '????' > MacDIB-dual.app/Contents/PkgInfo
	cp Info.plist MacDIB-dual.app/Contents/
	cp -R -p MacDIB-ppc.app/Contents/Resources MacDIB-dual.app/Contents/Resources/ppc
	cp -R -p MacDIB-x86.app/Contents/Resources MacDIB-dual.app/Contents/Resources/x86
	cp ../xdib/OBJ.macosx-ppc/xdib MacDIB-dual.app/Contents/MacOS/MacDIB-ppc-bin
	cp ../xdib/OBJ.macosx-x86/xdib MacDIB-dual.app/Contents/MacOS/MacDIB-x86-bin
	cp launcher-dual.sh MacDIB-dual.app/Contents/MacOS/MacDIB
	rm -rf MacDIB.app MacDIB-ppc.app MacDIB-x86.app
	mv MacDIB-dual.app MacDIB.app

package: cmd
	rm -rf "FC5025 Command Line Tools (optional).pkg" package_tmp
	mkdir package_tmp
	mkdir -p package_tmp/usr/local/bin package_tmp/usr/local/lib package_tmp/usr/local/man/man1
	cp ../cmd/OBJ.macosx-univ/fcbrowse ../cmd/OBJ.macosx-univ/fcdrives ../cmd/OBJ.macosx-univ/fcformats ../cmd/OBJ.macosx-univ/fcimage package_tmp/usr/local/bin
	cp ../cmd/*.1 package_tmp/usr/local/man/man1
	cp ../libusb.pkg/Contents/usr/local/lib/libusb-0.1.4.dylib package_tmp/usr/local/lib
	chmod -R 755 package_tmp
	sudo chown -R root:wheel package_tmp
	$(PACKAGEMAKER) -build -p "FC5025 Command Line Tools (optional).pkg" -f package_tmp -i cmd.plist -d Description.plist
	sudo rm -rf package_tmp

FC5025_MacOSX_Setup.dmg: bundle package
	rm -rf package_tmp
	mkdir package_tmp
	cp -Rp MacDIB.app package_tmp
	ln -s /Applications package_tmp
	cp -Rp "FC5025 Command Line Tools (optional).pkg" package_tmp
	cp Readme.txt package_tmp
	hdiutil create FC5025_MacOSX_Setup.dmg -srcfolder package_tmp -uid 99 -gid 99 -imagekey zlib-level=9 -ov -volname "FC5025 Mac OS X Setup"
	rm -rf package_tmp
	hdiutil internet-enable -yes FC5025_MacOSX_Setup.dmg

clean:
	rm -f .backend_built .cmd_built fcbrowse fcdrives fcformats fcimage macdib macdib-x86 macdib-ppc FC5025_MacOSX_Setup.dmg
	rm -rf MacDIB.app

upsrc:
	scp -pr ../../dib fred@fred:.

