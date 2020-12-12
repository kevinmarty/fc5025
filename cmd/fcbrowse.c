#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <ctype.h>
#include <time.h>
#include "phys.h"
#include "log.h"
#include "formats.h"
#include "drives.h"
#include "fc5025.h"

static char *my_name;
static char cwd[100]={'/','\0'};
static int time_to_go=0;

static int usage(void) {
	fprintf(stderr,"Usage: %s [-d device] -f format\n",my_name);
	return 1;
}

static void ls(struct phys *phys, struct log *log, char *args) {
	struct directory_entry *files, *file;
	int count;
	int newcount;
	char timestamp[17];
	struct tm *tm;

	if(args[0]!='\0') {
		printf("Usage: ls\n");
		return;
	}

	count=log->file_list(phys,log,NULL,0,cwd);

	if(count<0) {
		fprintf(stderr,"Unable to get file listing!\n");
		return;
	}
	if(count==0) {
		printf("No files.\n");
		return;
	}

	files=malloc(count*sizeof(struct directory_entry));
	if(!files) {
		fprintf(stderr,"Out of memory!\n");
		return;
	}

	newcount=log->file_list(phys,log,files,count,cwd);
	if(newcount<count)
		count=newcount;

	if(count<0) {
		fprintf(stderr,"Unable to get file listing!\n");
		free(files);
		return;
	}
	if(count==0) {
		printf("No files?\n");
		free(files);
		return;
	}

	file=files;
	while(count--) {
		printf("%c%c%c ",S_ISDIR(file->mode)?'d':'-',(file->mode&S_IRUSR)?'r':'-',(file->mode&S_IWUSR)?'w':'-');
		printf("%7d ",(int)(file->size));
		if(!(tm=localtime(&(file->mtime))) || strftime(timestamp,sizeof(timestamp),"%Y-%m-%d %H:%M",tm)<=0)
			strcpy(timestamp,"???");
		if(file->mtime==0)
			strcpy(timestamp,"---- -- -- --:--");
		printf("%16s ",timestamp);
		printf("%s\n",file->name);
		free(file->name);
		file++;
	}

	free(files);
}

static void quit(struct phys *phys, struct log *log, char *args) {
	time_to_go=1;
}

static void pwd(struct phys *phys, struct log *log, char *args) {
	if(args[0]!='\0') {
		printf("Usage: pwd\n");
		return;
	}

	printf("%s\n",cwd);
}

static void normalize_path(char *path) {
	char *p=path, *q;

	while(*p!='\0') {
		if(p[0]=='/' && (p[1]=='/' || p[1]=='\0')) {
			memmove(p,p+1,strlen(p+1)+1);
			continue;
		}
		q=strchr(p,'/');
		if(q && (!strcmp(q,"/..") || !strncmp(q,"/../",4)) && q!=p) {
			p--;
			memmove(p,q+3,strlen(q+3)+1);
			p=path;
			continue;
		}
		p++;
	}
	if(path[0]=='\0')
		strcpy(path,"/");
}

static void cd(struct phys *phys, struct log *log, char *args) {
	char oldcwd[100];

	if(args[0]=='\0') {
		printf("Usage: cd directory\n");
		return;
	}

	strcpy(oldcwd,cwd);
	if(args[0]=='/') {
		if((strlen(args)+1)>sizeof(cwd))
			return;
		strcpy(cwd,args);
	} else {
		if((strlen(cwd)+1+strlen(args)+1)>sizeof(cwd))
			return;
		strcat(cwd,"/");
		strcat(cwd,args);
	}
	normalize_path(cwd);
	if(!log->is_path_valid(phys,log,cwd)) {
		printf("New path is not valid.\n");
		strcpy(cwd,oldcwd);
	}
}

static void lcd(struct phys *phys, struct log *log, char *args) {
	char dir[200];

	if(args[0]=='\0') {
		printf("Usage: lcd directory\n");
		return;
	}

	if(chdir(args)<0) {
		perror("chdir");
		return;
	}

	if(getcwd(dir,sizeof(dir))==NULL) {
		perror("getcwd");
	} else {
		printf("Local directory now %s\n",dir);
	}
}

static void get(struct phys *phys, struct log *log, char *args) {
	int size;
	unsigned char *buf;
	FILE *f;
	char *src, *dest;

	if(args[0]=='\0') {
		printf("Usage: get filename\n");
		return;
	}

	src=malloc(strlen(cwd)+1+strlen(args)+1);
	if(!src) {
		printf("Out of memory!\n");
		return;
	}
	strcpy(src,cwd);
	strcat(src,"/");
	strcat(src,args);
	normalize_path(src);

	size=log->file_size(phys,log,src);
	if(size<0) {
		printf("Could not get file size.\n");
		free(src);
		return;
	}

	buf=malloc((size==0)?1:size);
	if(!buf) {
		printf("Insufficient memory for buffer!\n");
		free(src);
		return;
	}

	dest=strrchr(args,'/');
	if(dest) {
		dest++;
	} else {
		dest=args;
	}

	f=fopen(dest,"wb");
	if(!f) {
		perror("fopen");
		printf("Unable to open destination file.\n");
		free(src);
		free(buf);
		return;
	}

	if(size>0) {
		if(log->read_file(phys,log,buf,src)!=0) {
			printf("Unable to read file.\n");
			fclose(f);
			free(src);
			free(buf);
			return;
		}

		if(fwrite(buf,size,1,f)!=1) {
			perror("fwrite");
			printf("Unable to write destination file.\n");
			fclose(f);
			free(src);
			free(buf);
			return;
		}
	}

	if(fclose(f)!=0) {
		perror("fclose");
		printf("Unable to write destination file.\n");
		free(src);
		free(buf);
		return;
	}

	free(src);
	free(buf);
	printf("Copied %s\n",args);
}

static void nop(struct phys *phys, struct log *log, char *args) {
}

static void dispatch_command(struct phys *phys, struct log *log, char *cmd, char *arg) {
	static struct {
		char *name;
		void (*handler)(struct phys *, struct log *, char *);
	} commands[]={
		{"ls", ls},
		{"dir", ls},
		{"exit", quit},
		{"quit", quit},
		{"pwd", pwd},
		{"cd", cd},
		{"lcd", lcd},
		{"get", get},
		{"", nop},
		{NULL, NULL}
	}, *command;

	for(command=commands;command->name!=NULL;command++) {
		if(strcasecmp(cmd,command->name)==0)
			return command->handler(phys,log,arg);
	}

	printf("I don't know that word.\n");
}

static void cli(struct phys *phys, struct log *log) {
	char cmd[100];
	char *arg;

	while(!time_to_go) {
		printf("%s> ",my_name);
		fflush(stdout);
		if(!fgets(cmd,sizeof(cmd),stdin))
			return;
		if(strlen(cmd)>0 && isspace(cmd[strlen(cmd)-1]))
			cmd[strlen(cmd)-1]='\0'; /* elide \n */
		if(strchr(cmd,' ')) {
			arg=strchr(cmd,' ')+1;
			*strchr(cmd,' ')='\0';
			while(isspace(*arg))
				arg++;
		} else {
			arg=cmd+strlen(cmd);
		}
		dispatch_command(phys,log,cmd,arg);
	}
}

int main(int argc, char *argv[]) {
	int c;
	char *dev_id=NULL, *fmt_id=NULL;
	extern char *optarg;
	extern int optind;
	struct format_info *format;
	struct phys *phys;
	struct log *log;

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
	if(!format->log) {
		fprintf(stderr,"%s: No logical filesystem support for '%s'\n",my_name,fmt_id);
		return usage();
	}
	phys=format->phys;
	log=format->log;
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

	if(fc_recalibrate()!=0) {
		fprintf(stderr,"%s: Unable to recalibrate drive.\n",my_name);
		close_drive();
		return 1;
	}
	if(fc_set_density(phys->density(phys))!=0) {
		fprintf(stderr,"%s: Unable to set density.\n",my_name);
		close_drive();
		return 1;
	}
	if(phys->prepare(phys)!=0) {
		fprintf(stderr,"%s: Unable to begin reading disk.\n",my_name);
		close_drive();
		return 1;
	}
	cli(phys,log);

	close_drive();
	return 0;
}
