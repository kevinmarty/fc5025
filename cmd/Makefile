OBJDIRS=OBJ.linux OBJ.macosx-ppc OBJ.macosx-univ OBJ.macosx-x86 OBJ.win32
DUMMY:=$(shell mkdir -p $(OBJDIRS))

HOST_OBJS=fcformats fcdrives fcimage fcbrowse
CFLAGS=-I../backend
BACKEND_A=../backend/OBJ.linux/backend.a

WIN32_CC=i586-mingw32msvc-cc
WIN32_CFLAGS=-mms-bitfields -I../backend
WIN32_USBDIR=../libusb-win32-device-bin-0.1.12.2
WIN32_BACKEND_A=../backend/OBJ.win32/backend.a

MACOSX_PPC_CC=/usr/local/powerpc-apple-darwin9/bin/powerpc-apple-darwin9-gcc
MACOSX_X86_CC=/usr/local/i686-apple-darwin9/bin/i686-apple-darwin9-gcc
LIPO=/usr/local/powerpc-apple-darwin9/bin/powerpc-apple-darwin9-lipo

XCODE_DIR=/Developer
MAC_NATIVE_CC=$(XCODE_DIR)/usr/bin/gcc-4.0 -isysroot $(XCODE_DIR)/SDKs/MacOSX10.4u.sdk -mmacosx-version-min=10.4

MACOSX_LDFLAGS=-L../libusb.pkg/Contents/usr/local/lib
MACOSX_PPC_BACKEND_A=../backend/OBJ.macosx-ppc/backend.a
MACOSX_X86_BACKEND_A=../backend/OBJ.macosx-x86/backend.a

HOST_OS=$(shell uname -s)

ifeq "$(HOST_OS)" "Linux"
  all: linux
endif

ifeq "$(HOST_OS)" "Darwin"
  MACOSX_PPC_CC=$(MAC_NATIVE_CC) -arch ppc
  MACOSX_X86_CC=$(MAC_NATIVE_CC) -arch i386
  LIPO=lipo
  HOST_PROCESSOR=$(shell uname -p)
  ifeq "$(HOST_PROCESSOR)" "powerpc"
    all: macosx-ppc
  else
    all: macosx-x86
  endif
endif

linux: $(addprefix OBJ.linux/, $(HOST_OBJS))
	cp $(addprefix OBJ.linux/, $(HOST_OBJS)) .

win32: $(addsuffix .exe, $(addprefix OBJ.win32/, $(HOST_OBJS)))

macosx-ppc: $(addprefix OBJ.macosx-ppc/, $(HOST_OBJS))

macosx-x86: $(addprefix OBJ.macosx-x86/, $(HOST_OBJS))

macosx-univ: $(addprefix OBJ.macosx-univ/, $(HOST_OBJS))

OBJ.linux/%: OBJ.linux/%.o
	cc $(LDFLAGS) -o $@ $^ $(BACKEND_A) -lusb

OBJ.win32/%.exe: OBJ.win32/%.o
	$(WIN32_CC) -Wl,-subsystem,console -o $@ $< $(WIN32_BACKEND_A) -L$(WIN32_USBDIR)/lib/gcc -lusb -lwsock32

OBJ.macosx-ppc/%: OBJ.macosx-ppc/%.o
	$(MACOSX_PPC_CC) $(MACOSX_LDFLAGS) -o $@ $^ $(MACOSX_PPC_BACKEND_A) -lusb

OBJ.macosx-x86/%: OBJ.macosx-x86/%.o
	$(MACOSX_X86_CC) $(MACOSX_LDFLAGS) -o $@ $^ $(MACOSX_X86_BACKEND_A) -lusb

OBJ.macosx-univ/%: OBJ.macosx-ppc/% OBJ.macosx-x86/%
	$(LIPO) OBJ.macosx-ppc/$* OBJ.macosx-x86/$* -create -output $@

OBJ.linux/%.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@

OBJ.win32/%.o: %.c
	$(WIN32_CC) -c $(WIN32_CFLAGS) $< -o $@

OBJ.macosx-ppc/%.o: %.c
	$(MACOSX_PPC_CC) -c $(CFLAGS) $< -o $@

OBJ.macosx-x86/%.o: %.c
	$(MACOSX_X86_CC) -c $(CFLAGS) $< -o $@

clean:
	rm -rf $(OBJDIRS) $(HOST_OBJS)
	mkdir -p $(OBJDIRS)

uphost:
	scp $(addprefix OBJ.linux/, $(HOST_OBJS)) scalpel:fiddler/

.SUFFIXES: .exe
