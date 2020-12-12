#include <stdio.h>
#include "formats.h"

int main(int argc, char *argv[]) {
	struct format_info *format, *formats;

	formats=get_format_list();
	for(format=formats;format->id!=NULL;format++) {
		printf("%s\t%s\n",format->id,format->desc);
	}

	return 0;
}
