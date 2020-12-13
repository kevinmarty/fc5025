#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "phys.h"
#include "log.h"

extern struct phys phys_apple33;
extern struct phys phys_apple32;
extern struct phys phys_applepro;
extern struct phys phys_c1541;
extern struct phys phys_c1571;
extern struct phys phys_ti99;
extern struct phys phys_ti99ds180;
extern struct phys phys_ti99ds360;
extern struct phys phys_atari810;
extern struct phys phys_msdos1200;
extern struct phys phys_msdos360;
extern struct phys phys_mdsad;
extern struct phys phys_mdsad350;
extern struct phys phys_kaypro2;
extern struct phys phys_kaypro4;
extern struct phys phys_vg4500;
extern struct phys phys_pmc;
extern struct phys phys_coco;
extern struct phys phys_versa;

extern struct log log_applepro;
extern struct log log_fat12;
extern struct log log_cpm22_kp2;
extern struct log log_cpm22_kp4;
extern struct log log_cpm30_pmc;
extern struct log log_rsdos;
extern struct log log_versa;

static struct format_info {
	char *id;
	char *desc;
	char *suffix;
	struct phys *phys;
	struct log *log;
	int sides_interleaved;
} formats[]={
	{"apple33","Apple DOS 3.3 (16-sector)","dsk",&phys_apple33,NULL,1},
	{"apple32","Apple DOS 3.2 (13-sector)","d13",&phys_apple32,NULL,1},
	{"applepro","Apple ProDOS","po",&phys_applepro,&log_applepro,1},
	{"c1541","Commodore 1541","d64",&phys_c1541,NULL,1},
	{"c1571","Commodore 1571","d71",&phys_c1571,NULL,0},
	{"ti99","TI-99/4A 90k","v9t9",&phys_ti99,NULL,1},
	{"ti99ds180","TI-99/4A 180k","dsk",&phys_ti99ds180,NULL,1},
	{"ti99ds360","TI-99/4A 360k","dsk",&phys_ti99ds360,NULL,1},
	{"atari810","Atari 810","xfd",&phys_atari810,NULL,1},
	{"msdos12","MS-DOS 1200k","img",&phys_msdos1200,&log_fat12,1},
	{"msdos360","MS-DOS 360k","img",&phys_msdos360,&log_fat12,1},
	{"mdsad","North Star MDS-A-D 175k","nsi",&phys_mdsad,NULL,1},
	{"mdsad350","North Star MDS-A-D 350k","nsi",&phys_mdsad350,NULL,1},
	{"kaypro2","Kaypro 2 CP/M 2.2","dsk",&phys_kaypro2,&log_cpm22_kp2,1},
	{"kaypro4","Kaypro 4 CP/M 2.2","dsk",&phys_kaypro4,&log_cpm22_kp4,1},
	{"vg4500","CalComp Vistagraphics 4500","img",&phys_vg4500,NULL,1},
	{"pmc","PMC MicroMate","img",&phys_pmc,&log_cpm30_pmc,1},
	{"coco","Tandy Color Computer Disk BASIC","dsk",&phys_coco,&log_rsdos,1},
	{"versa","Motorola VersaDOS","img",&phys_versa,&log_versa,1},
	{NULL,NULL,NULL,NULL}
};

struct format_info *get_format_list(void) {
	return formats;
}

struct format_info *format_by_id(char *id) {
	struct format_info *format;

	for(format=formats;format->id!=NULL;format++) {
		if(strcasecmp(format->id,id)==0)
			return format;
	}

	return NULL;
}
