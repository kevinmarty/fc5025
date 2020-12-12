OBJDIRS=OBJ.linux OBJ.macosx-ppc OBJ.macosx-x86 OBJ.win32
DUMMY:=$(shell mkdir -p $(OBJDIRS))

GTK1FLAGS=`gtk-config --cflags`
GTK1LIBS=`gtk-config --libs`
GTK2FLAGS=`pkg-config gtk+-2.0 --cflags`
GTK2LIBS=`pkg-config gtk+-2.0 --libs`
INCLUDES=-I../backend
BACKEND_A=../backend/OBJ.linux/backend.a

WIN32_CC=i586-mingw32msvc-cc
WIN32_CFLAGS=-mms-bitfields
WIN32_USBDIR=../libusb-win32-device-bin-0.1.12.2
WIN32_GTKDIR=../gtk_2_0
WIN32_GTKFLAGS=-I$(WIN32_GTKDIR)/include/gtk-2.0 -I$(WIN32_GTKDIR)/include/glib-2.0 -I$(WIN32_GTKDIR)/lib/glib-2.0/include -I$(WIN32_GTKDIR)/lib/gtk-2.0/include -I$(WIN32_GTKDIR)/include/cairo -I$(WIN32_GTKDIR)/include/pango-1.0 -I$(WIN32_GTKDIR)/include/atk-1.0
WIN32_GTKLIBS=-L$(WIN32_GTKDIR)/lib -lgtk-win32-2.0 -lgdk-win32-2.0 -lgmodule-2.0 -lglib-2.0 -lgobject-2.0 -lm
WIN32_BACKEND_A=../backend/OBJ.win32/backend.a

MACOSX_EXTRA_GTKLIBS=-lpangoft2-1.0.0 -lfreetype -lfontconfig -lexpat -lpng12 -lpixman-1.0 -liconv

XCODE_DIR=/Developer
MAC_NATIVE_CC=$(XCODE_DIR)/usr/bin/gcc-4.0 -isysroot $(XCODE_DIR)/SDKs/MacOSX10.4u.sdk -mmacosx-version-min=10.4
MACOSX_GTKPPC_HOME=/Users/gtk-ppc
MACOSX_GTKX86_HOME=/Users/gtk-x86

MACOSX_LDFLAGS=-L../libusb.pkg/Contents/usr/local/lib
MACOSX_PPC_BACKEND_A=../backend/OBJ.macosx-ppc/backend.a
MACOSX_X86_BACKEND_A=../backend/OBJ.macosx-x86/backend.a

ifndef GTK_VERSION
  GTK_VERSION=$(shell pkg-config --exists gtk+-2.0 2>/dev/null; if [ $$? -eq 0 ]; then echo gtk2; else echo gtk1; fi)
endif
ifeq "$(GTK_VERSION)" "gtk2"
  GTKFLAGS=$(GTK2FLAGS)
  GTKLIBS=$(GTK2LIBS)
else
  GTKFLAGS=$(GTK1FLAGS)
  GTKLIBS=$(GTK1LIBS)
endif

HOST_OS=$(shell uname -s)

ifeq "$(HOST_OS)" "Linux"
  all: linux
endif

ifeq "$(HOST_OS)" "Darwin"
  MACOSX_PPC_CC=$(MAC_NATIVE_CC) -arch ppc
  MACOSX_X86_CC=$(MAC_NATIVE_CC) -arch i386
  HOST_PROCESSOR=$(shell uname -p)
  ifeq "$(HOST_PROCESSOR)" "powerpc"
    PKGCONFIG=$(MACOSX_GTKPPC_HOME)/gtk/inst/bin/pkg-config
    all: macosx-ppc
  else
    PKGCONFIG=$(MACOSX_GTKX86_HOME)/gtk/inst/bin/pkg-config
    all: macosx-x86
  endif
  MACOSX_PPC_GTKFLAGS=`PKG_CONFIG_PATH=$(MACOSX_GTKPPC_HOME)/gtk/inst/lib/pkgconfig $(PKGCONFIG) gtk+-2.0 --cflags`
  MACOSX_PPC_GTKLIBS=`PKG_CONFIG_PATH=$(MACOSX_GTKPPC_HOME)/gtk/inst/lib/pkgconfig $(PKGCONFIG) gtk+-2.0 --libs` $(MACOSX_EXTRA_GTKLIBS)
  MACOSX_X86_GTKFLAGS=`PKG_CONFIG_PATH=$(MACOSX_GTKX86_HOME)/gtk/inst/lib/pkgconfig $(PKGCONFIG) gtk+-2.0 --cflags`
  MACOSX_X86_GTKLIBS=`PKG_CONFIG_PATH=$(MACOSX_GTKX86_HOME)/gtk/inst/lib/pkgconfig $(PKGCONFIG) gtk+-2.0 --libs` $(MACOSX_EXTRA_GTKLIBS)
  MACOSX_PPC_GTKDIR=$(MACOSX_GTKPPC_HOME)/gtk/inst
  MACOSX_X86_GTKDIR=$(MACOSX_GTKX86_HOME)/gtk/inst
endif

linux: OBJ.linux/xdib
	cp OBJ.linux/xdib .

win32: OBJ.win32/windib.exe

macosx-ppc: OBJ.macosx-ppc/xdib

macosx-x86: OBJ.macosx-x86/xdib

OBJ.linux/xdib.o: xdib.c
	cc -o $@ $(GTKFLAGS) $(INCLUDES) -c $<

OBJ.win32/xdib.o: xdib.c
	$(WIN32_CC) $(WIN32_CFLAGS) -o $@ $(WIN32_GTKFLAGS) $(INCLUDES) -c $<

OBJ.macosx-ppc/xdib.o: xdib.c
	$(MACOSX_PPC_CC) -o $@ $(MACOSX_PPC_GTKFLAGS) $(INCLUDES) -DMACOSX -I$(MACOSX_PPC_GTKDIR)/include/igemacintegration -c $<

OBJ.macosx-x86/xdib.o: xdib.c
	$(MACOSX_X86_CC) -o $@ $(MACOSX_X86_GTKFLAGS) $(INCLUDES) -DMACOSX -I$(MACOSX_X86_GTKDIR)/include/igemacintegration -c $<

OBJ.linux/xdib: OBJ.linux/xdib.o
	cc -o $@ $^ $(BACKEND_A) -lusb $(GTKLIBS)

OBJ.win32/windib.exe: OBJ.win32/xdib.o
	$(WIN32_CC) -Wl,-subsystem,windows -o $@ $< $(WIN32_BACKEND_A) $(WIN32_GTKLIBS) -L$(WIN32_USBDIR)/lib/gcc -lusb -lwsock32 -lshfolder

OBJ.macosx-ppc/xdib: OBJ.macosx-ppc/xdib.o
	$(MACOSX_PPC_CC) -o $@ $(MACOSX_PPC_GTKLIBS) $< $(MACOSX_PPC_BACKEND_A) $(MACOSX_LDFLAGS) -lusb -ligemacintegration

OBJ.macosx-x86/xdib: OBJ.macosx-x86/xdib.o
	$(MACOSX_X86_CC) -o $@ $(MACOSX_X86_GTKLIBS) $^ $(MACOSX_X86_BACKEND_A) $(MACOSX_LDFLAGS) -lusb -ligemacintegration

clean:
	rm -rf $(OBJDIRS) xdib
	mkdir -p $(OBJDIRS)

uphost:
	scp OBJ.linux/xdib scalpel:fiddler/
