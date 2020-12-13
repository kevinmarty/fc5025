WIN32_CC=i686-w64-mingw32-gcc
WIN32_CFLAGS=-mms-bitfields -mwindows -I../libusb-win32-device-bin-0.1.12.2/include -DWIN32
WIN32_AR=i686-w64-mingw32-gcc-ar
WIN32_RANLIB=i686-w64-mingw32-gcc-ranlib

MACOSX_PPC_CC=/usr/local/powerpc-apple-darwin9/bin/powerpc-apple-darwin9-gcc
MACOSX_PPC_AR=/usr/local/powerpc-apple-darwin9/bin/powerpc-apple-darwin9-ar
MACOSX_PPC_RANLIB=/usr/local/powerpc-apple-darwin9/bin/powerpc-apple-darwin9-ranlib
MACOSX_X86_CC=/usr/local/i686-apple-darwin9/bin/i686-apple-darwin9-gcc
MACOSX_X86_AR=/usr/local/i686-apple-darwin9/bin/i686-apple-darwin9-ar
MACOSX_X86_RANLIB=/usr/local/i686-apple-darwin9/bin/i686-apple-darwin9-ranlib

MACOSX_CFLAGS=-I../libusb.pkg/Contents/usr/local/include

XCODE_DIR=/Developer
MAC_NATIVE_CC=$(XCODE_DIR)/usr/bin/gcc-4.0 -isysroot $(XCODE_DIR)/SDKs/MacOSX10.4u.sdk -mmacosx-version-min=10.4

OBJECTS=formats.o drives.o endec_4and4.o endec_54gcr.o endec_5and3.o endec_6and2.o endec_fm.o endec_mfm.o crc.o phys_apple13.o phys_apple16.o phys_atari810.o phys_c1541.o phys_c1571.o phys_coco.o phys_kaypro2.o phys_kaypro4.o phys_mds.o phys_msdos.o phys_pmc.o phys_ti99.o phys_versa.o phys_vg4500.o phys_gen.o fc5025.o log_cpm22.o log_fat12.o log_prodos.o log_rsdos.o log_versa.o
OBJDIRS=OBJ.linux OBJ.macosx-ppc OBJ.macosx-x86 OBJ.win32
DUMMY:=$(shell mkdir -p $(OBJDIRS))

HOST_OS=$(shell uname -s)

ifeq "$(HOST_OS)" "Linux"
  all: linux
endif

ifeq "$(HOST_OS)" "Darwin"
  MACOSX_PPC_AR=$(AR)
  MACOSX_PPC_RANLIB=ranlib
  MACOSX_PPC_CC=$(MAC_NATIVE_CC) -arch ppc
  MACOSX_X86_AR=$(AR)
  MACOSX_X86_RANLIB=ranlib
  MACOSX_X86_CC=$(MAC_NATIVE_CC) -arch i386
  HOST_PROCESSOR=$(shell uname -p)
  ifeq "$(HOST_PROCESSOR)" "powerpc"
    all: macosx-ppc
  else
    all: macosx-x86
  endif
endif

linux: OBJ.linux/backend.a

win32: OBJ.win32/backend.a

macosx-ppc: OBJ.macosx-ppc/backend.a

macosx-x86: OBJ.macosx-x86/backend.a

OBJ.linux/backend.a: $(addprefix OBJ.linux/, $(OBJECTS))
	rm -f $@
	$(AR) rc $@ $^
	ranlib $@

OBJ.win32/backend.a: $(addprefix OBJ.win32/, $(OBJECTS))
	rm -f $@
	$(WIN32_AR) rc $@ $^
	$(WIN32_RANLIB) $@

OBJ.macosx-ppc/backend.a: $(addprefix OBJ.macosx-ppc/, $(OBJECTS))
	rm -f $@
	$(MACOSX_PPC_AR) rc $@ $^
	$(MACOSX_PPC_RANLIB) $@

OBJ.macosx-x86/backend.a: $(addprefix OBJ.macosx-x86/, $(OBJECTS))
	rm -f $@
	$(MACOSX_X86_AR) rc $@ $^
	$(MACOSX_X86_RANLIB) $@

OBJ.linux/%.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@

OBJ.win32/%.o: %.c
	$(WIN32_CC) -c $(WIN32_CFLAGS) $< -o $@

OBJ.macosx-ppc/%.o: %.c
	$(MACOSX_PPC_CC) -c $(MACOSX_CFLAGS) $< -o $@

OBJ.macosx-x86/%.o: %.c
	$(MACOSX_X86_CC) -c $(MACOSX_CFLAGS) $< -o $@

clean:
	rm -rf $(OBJDIRS)
	mkdir -p $(OBJDIRS)
