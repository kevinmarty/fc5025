#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef WIN32
#include <fcntl.h>
#include <io.h>
#endif
#include "phys.h"
#include "formats.h"
#include "drives.h"
#include "fc5025.h"

static char *my_name;
static int errors=0;

static void read_one_sector(struct phys *phys, unsigned char *buf, int track, int side, int sector) {
	if(phys->num_sides(phys)==1) {
		fprintf(stderr,"\rReading track %d sector %d... ",track,sector);
	} else {
		fprintf(stderr,"\rReading track %d side %d sector %d... ",track,side,sector);
	}
	fflush(stderr);
	if(phys->read_sector(phys,buf,track,side,sector)!=0) {
		errors++;
		if(phys->num_sides(phys)==1) {
			fprintf(stderr,"Read error on track %d sector %d; continuing.\n",track,sector);
		} else {
			fprintf(stderr,"Read error on track %d side %d sector %d; continuing.\n",track,side,sector);
		}
	}
}

static int image_track(FILE *f,struct phys *phys,int track,int side) {
	unsigned char *buf;
	struct sector_list *sector_list, *sector_entry;
	int num_sectors=phys->num_sectors(phys,track,side);

	if(fc_SEEK_abs(phys->physical_track(phys,track))!=0) {
		fprintf(stderr,"\nUnable to seek to track %d! Giving up.\n",track);
		return 1;
	}
	buf=malloc(phys->track_bytes(phys,track,side));
	if(!buf) {
		fprintf(stderr,"\nOut of memory! Giving up.\n");
		return 1;
	}
	sector_list=malloc(sizeof(struct sector_list)*num_sectors);
	if(!sector_list) {
		fprintf(stderr,"\nOut of memory! Giving up.\n");
		free(buf);
		return 1;
	}
	phys->best_read_order(phys,sector_list,track,side);
	sector_entry=sector_list;
	while(num_sectors--) {
		read_one_sector(phys,buf+sector_entry->offset,track,side,sector_entry->sector);
		sector_entry++;
	}
	free(sector_list);
	if(fwrite(buf,phys->track_bytes(phys,track,side),1,f)!=1) {
		putc('\n',stderr);
		perror("fwrite");
		fprintf(stderr,"Unable to write to destination file! Giving up.\n");
		free(buf);
		return 1;
	}
	free(buf);
	return 0;
}

static int usage(void) {
	fprintf(stderr,"Usage: %s [-d device] -f format outfile\n",my_name);
	return 1;
}

int main(int argc, char *argv[]) {
	int c;
	char *dev_id=NULL, *fmt_id=NULL, *output=NULL;
	extern char *optarg;
	extern int optind;
	struct format_info *format;
	struct phys *phys;
	FILE *f;
	int track, side;
	int must_close_output=0;

	my_name=argv[0];
	while((c=getopt(argc,argv,"d:f:"))!=-1) {
		switch(c) {
		case 'd':
			dev_id=optarg;
			break;
		case 'f':
			fmt_id=optarg;
			break;
		case '?':
			return usage();
		}
	}
	if(!fmt_id) {
		fprintf(stderr,"%s: Missing -f option.\n",my_name);
		return usage();
	}
	format=format_by_id(fmt_id);
	if(!format) {
		fprintf(stderr,"%s: Invalid format '%s'\n",my_name,fmt_id);
		return usage();
	}
	phys=format->phys;
	if(optind<argc) {
		output=argv[optind];
		optind++;
	} else {
		fprintf(stderr,"%s: Missing destination filename.\n",my_name);
		return usage();
	}
	if(optind<argc) {
		fprintf(stderr,"%s: Too many arguments.\n",my_name);
		return usage();
	}

	if(!get_drive_list()) {
		fprintf(stderr,"%s: No devices found.\n",my_name);
		return 1;
	}
	if(dev_id) {
		if(open_drive_by_id(dev_id)!=0) {
			fprintf(stderr,"%s: Unable to open device '%s'.\n",my_name,dev_id);
			return 1;
		}
	} else {
		dev_id=open_default_drive();
		if(!dev_id) {
			fprintf(stderr,"%s: Unable to open default device.\n",my_name);
			return 1;
		}
	}

	if(strcmp(output,"-")==0) {
		f=stdout;
		output="standard output";
#ifdef WIN32
		if(_setmode(_fileno(stdout),_O_BINARY)==-1) {
			perror("_setmode");
			fprintf(stderr,"%s: Unable to set stdout to binary mode.\n",my_name);
			close_drive();
			return 1;
		}
#endif
	} else {
		f=fopen(output,"wb");
		must_close_output=1;
	}

	if(!f) {
		if(must_close_output)
			perror("fopen");
		fprintf(stderr,"%s: Unable to open destination file.\n",my_name);
		close_drive();
		return 1;
	}

	fprintf(stderr,"Imaging %s disk in %s to %s...\n",format->desc,dev_id,output);

	if(fc_recalibrate()!=0) {
		fprintf(stderr,"%s: Unable to recalibrate drive.\n",my_name);
		if(must_close_output)
			fclose(f);
		close_drive();
		return 1;
	}
	if(fc_set_density(phys->density(phys))!=0) {
		fprintf(stderr,"%s: Unable to set density.\n",my_name);
		if(must_close_output)
			fclose(f);
		close_drive();
		return 1;
	}
	if(phys->prepare(phys)!=0) {
		fprintf(stderr,"%s: Unable to begin reading disk.\n",my_name);
		if(must_close_output)
			fclose(f);
		close_drive();
		return 1;
	}
	if (format->sides_interleaved) {
		for(track=phys->min_track(phys);track<=phys->max_track(phys);track++) {
			for(side=phys->min_side(phys);side<=phys->max_side(phys);side++) {
				if(image_track(f,phys,track,side)!=0) {
					if(must_close_output)
						fclose(f);
					close_drive();
					return 1;
				}
			}
		}
	} else {
		for(side=phys->min_side(phys);side<=phys->max_side(phys);side++) {
			for(track=phys->min_track(phys);track<=phys->max_track(phys);track++) {
				if(image_track(f,phys,track,side)!=0) {
					if(must_close_output)
						fclose(f);
					close_drive();
					return 1;
				}
			}
		}
	}

	if(fflush(f)!=0) {
		perror("fflush");
		if(must_close_output)
			fclose(f);
		fprintf(stderr,"Unable to write to destination file!\n");
		close_drive();
		return 1;
	}

	if(must_close_output && fclose(f)!=0) {
		perror("fclose");
		fprintf(stderr,"Unable to write to destination file!\n");
		close_drive();
		return 1;
	}

	close_drive();

	if(errors) {
		fprintf(stderr,"\nSome sectors did not read correctly.\n");
		return 1;
	} else {
		fprintf(stderr,"\nDone!\n");
		return 0;
	}
}
