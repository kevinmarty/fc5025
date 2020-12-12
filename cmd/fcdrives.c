#include <stdio.h>
#include "drives.h"

int main(int argc, char *argv[]) {
	struct drive_info *drive, *drives;

	drives=get_drive_list();
	if(!drives)
		return 1;
	for(drive=drives;drive->id[0]!='\0';drive++) {
		printf("%s\t%s\n",drive->id,drive->desc);
	}

	return 0;
}
