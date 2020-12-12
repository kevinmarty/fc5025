ISCC=wine "c:/Program Files/Inno Setup 5/ISCC"

all: FC5025_Windows_Setup.exe

FC5025_Windows_Setup.exe: windib.exe windib.iss cmd.pdf
	$(ISCC) windib.iss

windib.exe:
	make -C ../backend clean
	make -C ../backend win32
	make -C ../cmd clean
	make -C ../cmd win32
	make -C ../xdib clean
	make -C ../xdib win32
	cp ../xdib/OBJ.win32/windib.exe ./

cmd.ps:
	(man ../cmd/fcbrowse.1; printf '\f'; man ../cmd/fcdrives.1; printf '\f'; man ../cmd/fcformats.1; printf '\f'; man ../cmd/fcimage.1) | a2ps --margin=26 -o $@ -1 -B

cmd.pdf: cmd.ps
	ps2pdf $< temp.pdf
	pdfopt temp.pdf $@
	rm -f temp.pdf

clean:
	rm -f windib.exe FC5025_Windows_Setup.exe cmd.pdf cmd.ps

upsrc:
	scp -pr ../../dib lizard:fiddler/
