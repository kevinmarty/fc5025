#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <gtk/gtk.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>
#include <ctype.h>
#include "phys.h"
#include "log.h"
#include "formats.h"
#include "drives.h"
#include "fc5025.h"

#define VERSION_STRING "1309"

#ifdef WIN32
#include <glib/gstdio.h>
#include <windows.h>
#include <tchar.h>
#include <shlobj.h>
#define DIRECTORY_SEPARATOR "\\"
#else
#define g_fopen fopen
#define DIRECTORY_SEPARATOR "/"
#endif

#ifdef MACOSX
#include <gtkosxapplication.h>
#endif

#ifdef WIN32
#define MY_NAME "WinDIB"
#endif
#ifdef MACOSX
#define MY_NAME "MacDIB"
#endif
#ifndef MY_NAME
#define MY_NAME "xdib"
#endif

static GtkWidget *fname_field, *outdir_field;
static struct drive_info *selected_drive=NULL;
static struct format_info *selected_format=NULL;
static struct directory_entry *files=NULL;
static int files_count;
static GtkWidget *browsebutton, *imgbutton;
GtkWidget *copy_button, *copy_button_label, *cwd_label, *listbox, *pop_button;

static int errors;
static float progress;
static int drive_is_opened=0;
static char cwd[100]={'/','\0'};
static int selected_item_type;
static char *selected_item_name;
static int modal=0;
static int img_cancelled;

void get_directory_listing(void);

void refresh_screen(void) {
	while(gtk_events_pending())
		gtk_main_iteration();
}

gint disallow_delete(GtkWidget *widget, gpointer gdata) {
	return TRUE; /* prevent window from closing */
}

void quitpressed(GtkWidget *widget, gpointer gdata) {
	gtk_main_quit();
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

void enterdirpressed(GtkWidget *widget, gpointer gdata) {
	char *name=selected_item_name;

	if((strlen(cwd)+1+strlen(name)+1)>sizeof(cwd))
		return;
	strcat(cwd,"/");
	strcat(cwd,name);
	normalize_path(cwd);
	gtk_label_set(GTK_LABEL(cwd_label),"Reading disk...");
	refresh_screen();
	get_directory_listing();
	if(strcmp(cwd,"/")==0) {
		gtk_widget_set_sensitive(pop_button,0);
	} else {
		gtk_widget_set_sensitive(pop_button,1);
	}
	gtk_widget_set_sensitive(copy_button,0);
}

void do_copy(char *dest) {
	char *name=selected_item_name;
	int size;
	unsigned char *buf;
	FILE *f;
	char *src;
	struct phys *phys=selected_format->phys;
	struct log *log=selected_format->log;

	src=malloc(strlen(cwd)+1+strlen(name)+1);
	if(!src) {
		gtk_label_set(GTK_LABEL(cwd_label),"Out of memory!");
		return;
	}
	strcpy(src,cwd);
	strcat(src,"/");
	strcat(src,name);
	normalize_path(src);

	gtk_label_set(GTK_LABEL(cwd_label),"Reading file...");
	refresh_screen();

	size=log->file_size(phys,log,src);
	if(size<0) {
		gtk_label_set(GTK_LABEL(cwd_label),"Could not get file size.");
		free(src);
		return;
	}

	refresh_screen();

	buf=malloc((size==0)?1:size);
	if(!buf) {
		gtk_label_set(GTK_LABEL(cwd_label),"Insufficient memory for buffer!");
		free(src);
		return;
	}

	f=g_fopen(dest,"wb");
	if(!f) {
		gtk_label_set(GTK_LABEL(cwd_label),"Unable to open destination file.");
		free(src);
		free(buf);
		return;
	}

	if(size>0) {
		if(log->read_file(phys,log,buf,src)!=0) {
			gtk_label_set(GTK_LABEL(cwd_label),"Unable to read file.");
			fclose(f);
			free(src);
			free(buf);
			return;
		}

		refresh_screen();

		if(fwrite(buf,size,1,f)!=1) {
			gtk_label_set(GTK_LABEL(cwd_label),"Unable to write destination file.");
			fclose(f);
			free(src);
			free(buf);
			return;
		}
	}

	if(fclose(f)!=0) {
		gtk_label_set(GTK_LABEL(cwd_label),"Unable to write destination file.");
		free(src);
		free(buf);
		return;
	}

	free(src);
	free(buf);
	gtk_label_set(GTK_LABEL(cwd_label),"Copied file.");
}

void filew_ok(GtkWidget *widget, gpointer gdata) {
	GtkWidget *filew=gdata;
	char *dest=strdup(gtk_file_selection_get_filename(GTK_FILE_SELECTION(filew)));

	gtk_widget_destroy(filew);
	if(dest) {
		do_copy(dest);
		free(dest);
	}
}

void destroy_filew(GtkWidget *widget, gpointer gdata) {
	GtkWidget *filew=gdata;

	gtk_grab_remove(filew);
}

void lc(char *s) {
	while(*s!='\0') {
		*s=tolower(*s);
		s++;
	}
}

void copypressed(GtkWidget *widget, gpointer gdata) {
	GtkWidget *filew;
	char *dest;

	if(selected_item_type==2)
		return enterdirpressed(widget,gdata);

	dest=strdup(selected_item_name);
	if(!dest)
		return;
	lc(dest);
	filew=gtk_file_selection_new("Save as");
	gtk_file_selection_set_filename(GTK_FILE_SELECTION(filew),dest);
	free(dest);
	gtk_signal_connect(GTK_OBJECT(filew),"destroy",(GtkSignalFunc)destroy_filew,(gpointer)filew);
	gtk_signal_connect_object(GTK_OBJECT(GTK_FILE_SELECTION(filew)->cancel_button),"clicked",(GtkSignalFunc)gtk_widget_destroy,(gpointer)filew);
	gtk_signal_connect(GTK_OBJECT(GTK_FILE_SELECTION(filew)->ok_button),"clicked",(GtkSignalFunc)filew_ok,(gpointer)filew);
	gtk_widget_show(filew);
	gtk_grab_add(filew);
}

void poppressed(GtkWidget *widget, gpointer gdata) {
	selected_item_name="..";
	return enterdirpressed(NULL,NULL);
}

void message_selected(GtkWidget *widget, gpointer gdata) {
	gtk_widget_set_sensitive(copy_button,0);
	selected_item_type=0;
}

void file_selected(GtkWidget *widget, gpointer gdata) {
	gtk_widget_set_sensitive(copy_button,1);
	gtk_label_set(GTK_LABEL(copy_button_label),"Copy File");
	selected_item_type=1;
	selected_item_name=gdata;
}

void directory_selected(GtkWidget *widget, gpointer gdata) {
	gtk_widget_set_sensitive(copy_button,1);
	gtk_label_set(GTK_LABEL(copy_button_label),"Enter Directory");
	selected_item_type=2;
	selected_item_name=gdata;
}

gint directory_clicked(GtkWidget *widget, GdkEventButton *event, gpointer gdata) {
	if(event->type==GDK_2BUTTON_PRESS) { /* double-click */
		selected_item_type=2;
		selected_item_name=gdata;
		enterdirpressed(NULL,NULL);
		return TRUE;
	}
	return FALSE;
}

void add_list_item(GtkWidget *listbox, int type, char *text, char *filename) {
	GtkWidget *item;

	item=gtk_list_item_new_with_label(text);
	gtk_container_add(GTK_CONTAINER(listbox),item);
	switch(type) {
	case 0: /* message */
		gtk_signal_connect(GTK_OBJECT(item),"select",GTK_SIGNAL_FUNC(message_selected),NULL);
		break;
	case 1: /* file */
		gtk_signal_connect(GTK_OBJECT(item),"select",GTK_SIGNAL_FUNC(file_selected),filename);
		break;
	case 2: /* directory */
		gtk_signal_connect(GTK_OBJECT(item),"select",GTK_SIGNAL_FUNC(directory_selected),filename);
		gtk_signal_connect(GTK_OBJECT(item),"button_press_event",GTK_SIGNAL_FUNC(directory_clicked),filename);
		break;
	}
	gtk_widget_show(item);
}

void zap_files(void) {
	struct directory_entry *file=files;

	while(files_count--) {
		free(file->name);
		file++;
	}
	free(files);
	files=NULL;
}

void get_directory_listing(void) {
	struct directory_entry *file;
	int count, newcount;
	char timestamp[17], listingline[81];
	struct tm *tm;
	struct phys *phys=selected_format->phys;
	struct log *log=selected_format->log;

	gtk_list_clear_items(GTK_LIST(listbox),0,-1);
	if(files!=NULL)
		zap_files();

	count=log->file_list(phys,log,NULL,0,cwd);

	refresh_screen();

	if(count<0)
		return add_list_item(listbox,0,"Unable to get file listing!",NULL);
	if(count==0) {
		gtk_label_set(GTK_LABEL(cwd_label),cwd);
		return add_list_item(listbox,0,"No files.",NULL);
	}

	files=malloc(count*sizeof(struct directory_entry));
	if(!files)
		return add_list_item(listbox,0,"Out of memory!",NULL);

	newcount=log->file_list(phys,log,files,count,cwd);
	if(newcount<count)
		count=newcount;

	files_count=newcount;

	refresh_screen();

	if(count<0) {
		free(files);
		return add_list_item(listbox,0,"Unable to get file listing!",NULL);
	}
	if(count==0) {
		free(files);
		gtk_label_set(GTK_LABEL(cwd_label),cwd);
		return add_list_item(listbox,0,"No files?",NULL);
	}

	file=files;
	while(count--) {
		if(!(tm=localtime(&(file->mtime))) || strftime(timestamp,sizeof(timestamp),"%Y-%m-%d %H:%M",tm)<=0)
			strcpy(timestamp,"???");
		if(file->mtime==0)
			strcpy(timestamp,"---- -- -- --:--");
		snprintf(listingline,sizeof(listingline),"%c%c%c %7d %16s %s",S_ISDIR(file->mode)?'d':'-',(file->mode&S_IRUSR)?'r':'-',(file->mode&S_IWUSR)?'w':'-',(int)(file->size),timestamp,file->name);
		add_list_item(listbox,S_ISDIR(file->mode)?2:1,listingline,file->name);
		file++;
	}

	gtk_label_set(GTK_LABEL(cwd_label),cwd);
}

void browsedone(GtkWidget *widget, gpointer gdata) {
	GtkWidget *browse_window=gdata;

	gtk_widget_destroy(GTK_WIDGET(browse_window));
}

void destroy_browse(GtkWidget *widget, gpointer gdata) {
	GtkWidget *browse_window=gdata;

	gtk_grab_remove(GTK_WIDGET(browse_window));
	if(drive_is_opened)
		close_drive();
	drive_is_opened=0;
	if(files!=NULL)
		zap_files();
}

void browsefailed(GtkWidget *done_button_label, char *text) {
	if(text)
		gtk_label_set(GTK_LABEL(cwd_label),text);
	gtk_label_set(GTK_LABEL(done_button_label),"Bummer.");
}

void browsepressed(GtkWidget *widget, gpointer gdata) {
	GtkWidget *browse_window=gtk_dialog_new();
	GtkWidget *vbox=gtk_vbox_new(FALSE,5);
	GtkWidget *done_button=gtk_button_new();
	GtkWidget *done_button_label=gtk_label_new("Done");
	GtkWidget *scrollwin=gtk_scrolled_window_new(NULL,NULL);
	struct phys *phys=selected_format->phys;
	
	strcpy(cwd,"/");

	gtk_window_set_title(GTK_WINDOW(browse_window), "Browsing Disk Contents...");
	gtk_signal_connect(GTK_OBJECT(browse_window),"destroy",(GtkSignalFunc)destroy_browse,(gpointer)browse_window);
	gtk_container_border_width(GTK_CONTAINER(browse_window),10);

	cwd_label=gtk_label_new("Reading disk...");
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(browse_window)->vbox),cwd_label,TRUE,TRUE,0);
	gtk_widget_show(cwd_label);

	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(browse_window)->action_area),vbox,TRUE,TRUE,0);

	gtk_container_border_width(GTK_CONTAINER(scrollwin),10);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrollwin),GTK_POLICY_NEVER,GTK_POLICY_ALWAYS);
	gtk_widget_set_usize(GTK_WIDGET(scrollwin),500,400);
	gtk_box_pack_start(GTK_BOX(vbox),scrollwin,TRUE,TRUE,0);
	gtk_widget_show(scrollwin);

	listbox=gtk_list_new();
	gtk_list_set_selection_mode(GTK_LIST(listbox),GTK_SELECTION_SINGLE);
#if 0
	gtk_box_pack_start(GTK_BOX(vbox),listbox,TRUE,TRUE,0);
	gtk_widget_show(listbox);
#else
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrollwin),listbox);
	gtk_widget_show(listbox);
#endif

	pop_button=gtk_button_new_with_label("Leave Directory");
	gtk_signal_connect(GTK_OBJECT(pop_button),"clicked",GTK_SIGNAL_FUNC(poppressed),browse_window);
	gtk_box_pack_start(GTK_BOX(vbox),pop_button,TRUE,TRUE,0);
	gtk_widget_set_sensitive(pop_button,0);
	gtk_widget_show(pop_button);

	copy_button=gtk_button_new();
	copy_button_label=gtk_label_new("Copy File");
	gtk_container_add(GTK_CONTAINER(copy_button),copy_button_label);
	gtk_signal_connect(GTK_OBJECT(copy_button),"clicked",GTK_SIGNAL_FUNC(copypressed),browse_window);
	gtk_box_pack_start(GTK_BOX(vbox),copy_button,TRUE,TRUE,0);
	gtk_widget_show(copy_button_label);
	gtk_widget_set_sensitive(copy_button,0);
	gtk_widget_show(copy_button);

	gtk_container_add(GTK_CONTAINER(done_button),done_button_label);
	gtk_signal_connect(GTK_OBJECT(done_button),"clicked",GTK_SIGNAL_FUNC(browsedone),browse_window);
	gtk_box_pack_start(GTK_BOX(vbox),done_button,TRUE,TRUE,0);
	gtk_widget_show(done_button_label);
	gtk_widget_show(done_button);

	gtk_widget_show(vbox);
	gtk_widget_show(browse_window);
	gtk_grab_add(browse_window);

	refresh_screen();

	if(open_drive(selected_drive)!=0)
		return browsefailed(done_button_label,"Unable to open drive.");
	drive_is_opened=1;
	refresh_screen();
	if(fc_recalibrate()!=0)
		return browsefailed(done_button_label,"Unable to recalibrate drive.");
	refresh_screen();
	if(fc_set_density(phys->density(phys))!=0)
		return browsefailed(done_button_label,"Unable to set density.");
	refresh_screen();
	if(phys->prepare(phys)!=0)
		return browsefailed(done_button_label,"Unable to begin reading disk.");
	refresh_screen();
	get_directory_listing();
}

static void read_one_sector(struct phys *phys, unsigned char *buf, int track, int side, int sector, GtkWidget *error_label) {
	char errtext[80];

	if(phys->read_sector(phys,buf,track,side,sector)!=0) {
		if(phys->num_sides(phys)==1) {
			snprintf(errtext,sizeof(errtext),"Read error on track %d sector %d",track,sector);
			if(errors)
				snprintf(errtext,sizeof(errtext),"Multiple read errors, latest on track %d sector %d",track,sector);
		} else {
			snprintf(errtext,sizeof(errtext),"Read error on track %d side %d sector %d",track,side,sector);
			if(errors)
				snprintf(errtext,sizeof(errtext),"Multiple read errors, latest on track %d side %d sector %d",track,side,sector);
		}
		gtk_label_set(GTK_LABEL(error_label),errtext);
		refresh_screen();
		errors++;
	}
}

void increment_progressbar(GtkWidget *progressbar, float progress_per_halftrack) {
	progress+=progress_per_halftrack;
	if(progress>1.0)
		progress=1.0;
	gtk_progress_set_percentage(GTK_PROGRESS(progressbar),progress);
}

int image_track(FILE *f,struct phys *phys,int track,int side,GtkWidget *status_label,GtkWidget *error_label,GtkWidget *progressbar,float progress_per_halftrack) {
	unsigned char *buf;
	struct sector_list *sector_list, *sector_entry;
	int num_sectors=phys->num_sectors(phys,track,side);
	int halfway_mark=num_sectors>>1;

	if(fc_SEEK_abs(phys->physical_track(phys,track))!=0) {
		gtk_label_set(GTK_LABEL(status_label),"Unable to seek to track! Giving up.");
		return 1;
	}
	refresh_screen();
	buf=malloc(phys->track_bytes(phys,track,side));
	if(!buf) {
		gtk_label_set(GTK_LABEL(status_label),"Out of memory! Giving up.");
		return 1;
	}
	sector_list=malloc(sizeof(struct sector_list)*num_sectors);
	if(!sector_list) {
		gtk_label_set(GTK_LABEL(status_label),"Out of memory! Giving up.");
		free(buf);
		return 1;
	}
	phys->best_read_order(phys,sector_list,track,side);
	sector_entry=sector_list;
	while(num_sectors--) {
		read_one_sector(phys,buf+sector_entry->offset,track,side,sector_entry->sector,error_label);
		if(img_cancelled) {
			gtk_label_set(GTK_LABEL(status_label),"Cancelled.");
			free(sector_list);
			free(buf);
			return 1;
		}
		if(num_sectors==halfway_mark) {
			increment_progressbar(progressbar,progress_per_halftrack);
			refresh_screen();
		}
		sector_entry++;
	}
	free(sector_list);
	increment_progressbar(progressbar,progress_per_halftrack);
	refresh_screen();
	if(fwrite(buf,phys->track_bytes(phys,track,side),1,f)!=1) {
		gtk_label_set(GTK_LABEL(status_label),"Unable to write to destination file! Giving up.");
		free(buf);
		return 1;
	}
	refresh_screen();
	free(buf);
	return 0;
}

void auto_increment_filename(void) {
	const char *old_filename=gtk_entry_get_text(GTK_ENTRY(fname_field));
	char *new_filename;
	char *p;

	p=strrchr(old_filename,'.');
	if(!p || p==old_filename)
		return;
	new_filename=strdup(old_filename);
	if(!new_filename)
		return;
	p=strrchr(new_filename,'.')-1;
	if(!isdigit(*p)) {
		free(new_filename);
		return;
	}
	(*p)++;
	while(*p=='9'+1 && p!=new_filename && isdigit(*(p-1))) {
		*p='0';
		p--;
		(*p)++;
	}
	if(*p=='9'+1)
		*p='X';
	gtk_entry_set_text(GTK_ENTRY(fname_field),new_filename);
	free(new_filename);
}

void imgdone(GtkWidget *widget, gpointer gdata) {
	GtkWidget *image_window=gdata;

	gtk_widget_destroy(GTK_WIDGET(image_window));
}

void imgcancel(GtkWidget *widget, gpointer gdata) {
	img_cancelled=1;
}

void destroy_img(GtkWidget *widget, gpointer gdata) {
	GtkWidget *image_window=gdata;

	gtk_grab_remove(image_window);
	auto_increment_filename();
}

void imgfailed(GtkWidget *image_window, gint delete_signal, GtkWidget *status_label, GtkWidget *button_label, GtkWidget *button, GtkWidget *cancelbutton, char *text) {
	if(text)
		gtk_label_set(GTK_LABEL(status_label),text);
	gtk_label_set(GTK_LABEL(button_label),"Bummer.");
	gtk_widget_set_sensitive(cancelbutton,0);
	gtk_widget_set_sensitive(button,1);
	gtk_signal_disconnect(GTK_OBJECT(image_window),delete_signal);
	modal=0;
}

void imgpressed(GtkWidget *widget, gpointer gdata) {
	GtkWidget *image_window=gtk_dialog_new();
	GtkAdjustment *adj=(GtkAdjustment *)gtk_adjustment_new(0,0,400,0,0,0);
	GtkWidget *progressbar=gtk_progress_bar_new_with_adjustment(adj);
	GtkWidget *button=gtk_button_new();
	GtkWidget *cancelbutton=gtk_button_new_with_label("Cancel");
	char status_text[80];
	GtkWidget *status_label=gtk_label_new("Preparing...");
	GtkWidget *error_label=gtk_label_new("");
	GtkWidget *button_label=gtk_label_new("In progress...");
	float progress_per_halftrack;
	struct phys *phys=selected_format->phys;
	int track, side;
	char *out_filename;
	FILE *f;
	gint delete_signal;

	progress=0; errors=0;

	gtk_window_set_title(GTK_WINDOW(image_window), "Capturing Disk Image File...");
	delete_signal=gtk_signal_connect(GTK_OBJECT(image_window),"delete_event",GTK_SIGNAL_FUNC(disallow_delete),NULL);
	gtk_signal_connect(GTK_OBJECT(image_window),"destroy",(GtkSignalFunc)destroy_img,(gpointer)image_window);
	modal=1;
	gtk_container_border_width(GTK_CONTAINER(image_window),10);

	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(image_window)->vbox),status_label,TRUE,TRUE,0);
	gtk_widget_show(status_label);

	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(image_window)->vbox),error_label,TRUE,TRUE,0);
	gtk_widget_show(error_label);

	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(image_window)->vbox),progressbar,TRUE,TRUE,0);
	gtk_progress_set_percentage(GTK_PROGRESS(progressbar),0);
	gtk_widget_show(progressbar);

	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(image_window)->action_area),cancelbutton,TRUE,TRUE,0);
	gtk_signal_connect(GTK_OBJECT(cancelbutton),"clicked",GTK_SIGNAL_FUNC(imgcancel),image_window);
	gtk_widget_set_sensitive(cancelbutton,0);
	gtk_widget_show(cancelbutton);

	gtk_container_add(GTK_CONTAINER(button),button_label);
	gtk_signal_connect(GTK_OBJECT(button),"clicked",GTK_SIGNAL_FUNC(imgdone),image_window);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(image_window)->action_area),button,TRUE,TRUE,0);
	gtk_widget_set_sensitive(button,0);
	gtk_widget_show(button_label);
	gtk_widget_show(button);

	gtk_widget_show(image_window);
	gtk_grab_add(image_window);
	refresh_screen();

	if(open_drive(selected_drive)!=0)
		return imgfailed(image_window,delete_signal,status_label,button_label,button,cancelbutton,"Unable to open drive.");
	refresh_screen();

	out_filename=malloc(strlen(gtk_entry_get_text(GTK_ENTRY(outdir_field)))+1+strlen(gtk_entry_get_text(GTK_ENTRY(fname_field)))+1);
	if(!out_filename) {
		close_drive();
		return imgfailed(image_window,delete_signal,status_label,button_label,button,cancelbutton,"Out of memory!");
	}
	strcpy(out_filename,gtk_entry_get_text(GTK_ENTRY(outdir_field)));
	strcat(out_filename,DIRECTORY_SEPARATOR);
	strcat(out_filename,gtk_entry_get_text(GTK_ENTRY(fname_field)));
	f=g_fopen(out_filename,"wb");
	free(out_filename);
	if(!f) {
		close_drive();
		return imgfailed(image_window,delete_signal,status_label,button_label,button,cancelbutton,"Unable to open destination file.");
	}

	if(fc_recalibrate()!=0) {
		fclose(f);
		close_drive();
		return imgfailed(image_window,delete_signal,status_label,button_label,button,cancelbutton,"Unable to recalibrate drive.");
	}
	refresh_screen();

	if(fc_set_density(phys->density(phys))!=0) {
		fclose(f);
		close_drive();
		return imgfailed(image_window,delete_signal,status_label,button_label,button,cancelbutton,"Unable to set density.");
	}
	refresh_screen();

	if(phys->prepare(phys)!=0) {
		fclose(f);
		close_drive();
		return imgfailed(image_window,delete_signal,status_label,button_label,button,cancelbutton,"Unable to begin reading disk.");
	}
	img_cancelled=0;
	gtk_widget_set_sensitive(cancelbutton,1);
	refresh_screen();

	progress_per_halftrack=(float)1/(float)(phys->num_tracks(phys)*phys->num_sides(phys)*2);

	for(track=phys->min_track(phys);track<=phys->max_track(phys);track++) {
		for(side=phys->min_side(phys);side<=phys->max_side(phys);side++) {
			if(phys->num_sides(phys)==1) {
				snprintf(status_text,sizeof(status_text),"Reading track %d...\n",track);
			} else {
				snprintf(status_text,sizeof(status_text),"Reading track %d side %d...\n",track,side);
			}
			gtk_label_set(GTK_LABEL(status_label),status_text);
			refresh_screen();
			/* image the track */
			if(image_track(f,phys,track,side,status_label,error_label,progressbar,progress_per_halftrack)!=0) {
				fclose(f);
				close_drive();
				return imgfailed(image_window,delete_signal,status_label,button_label,button,cancelbutton,NULL);
			}
		}
	}

	if(fclose(f)!=0) {
		close_drive();
		return imgfailed(image_window,delete_signal,status_label,button_label,button,cancelbutton,"Unable to write to destination file!");
	}
	close_drive();

	if(!errors) {
		gtk_label_set(GTK_LABEL(status_label),"Successfully read disk.");
		gtk_label_set(GTK_LABEL(button_label),"Yay!");
	} else {
		gtk_label_set(GTK_LABEL(status_label),"Some sectors did not read correctly.");
		gtk_label_set(GTK_LABEL(button_label),"Bummer.");
	}
	gtk_widget_set_sensitive(cancelbutton,0);
	gtk_widget_set_sensitive(button,1);
	gtk_signal_disconnect(GTK_OBJECT(image_window),delete_signal);
	modal=0;
	return;
}

void update_sensitivity(void) {
	if(selected_drive==NULL) {
		gtk_widget_set_sensitive(browsebutton,0);
		gtk_widget_set_sensitive(imgbutton,0);
	} else {
		gtk_widget_set_sensitive(imgbutton,1);
		if(selected_format!=NULL && selected_format->log!=NULL) {
			gtk_widget_set_sensitive(browsebutton,1);
		} else {
			gtk_widget_set_sensitive(browsebutton,0);
		}
	}
}

void format_changed(GtkWidget *widget, gpointer data) {
	GtkWidget *new_selection;
	const char *old_filename=gtk_entry_get_text(GTK_ENTRY(fname_field));
	char *new_filename;
	char *p;

	selected_format=data;
	update_sensitivity();
	/*printf("selected %s\n",format->id);*/
	if(!selected_format->suffix)
		return;
	p=strrchr(old_filename,'.');
	if(!p)
		return;
	new_filename=malloc(strlen(old_filename)+strlen(selected_format->suffix));
	if(!new_filename)
		return;
	strncpy(new_filename,old_filename,(p-old_filename)+1);
	strcpy(new_filename+(p-old_filename)+1,selected_format->suffix);
	gtk_entry_set_text(GTK_ENTRY(fname_field),new_filename);
	free(new_filename);
}

void drive_changed(GtkWidget *widget, gpointer data) {
	selected_drive=data;
	update_sensitivity();
}

void add_formats(GtkWidget *menu) {
	GtkWidget *mitem;
	GSList *group;
	struct format_info *format;

	group=NULL;
	for(format=get_format_list();format->id!=NULL;format++) {
		if(selected_format==NULL)
			selected_format=format;
		mitem=gtk_radio_menu_item_new_with_label(group,format->desc);
		gtk_menu_append(GTK_MENU(menu),mitem);
		gtk_widget_show(mitem);
		gtk_signal_connect(GTK_OBJECT(mitem),"activate",GTK_SIGNAL_FUNC(format_changed),format);
	}
}

void add_drives(GtkWidget *menu) {
	GtkWidget *mitem;
	GSList *group=NULL;
	struct drive_info *drive, *drives;
	char label[100];

	drives=get_drive_list();
	if(!drives) {
		mitem=gtk_radio_menu_item_new_with_label(group,"No drives found.");
		gtk_menu_append(GTK_MENU(menu),mitem);
		gtk_widget_show(mitem);
		return;
	}

	for(drive=drives;drive->id[0]!='\0';drive++) {
		if(selected_drive==NULL)
			selected_drive=drive;
		snprintf(label,sizeof(label),"%s (%s)",drive->desc,drive->id);
		mitem=gtk_radio_menu_item_new_with_label(group,label);
		gtk_menu_append(GTK_MENU(menu),mitem);
		gtk_widget_show(mitem);
		gtk_signal_connect(GTK_OBJECT(mitem),"activate",GTK_SIGNAL_FUNC(drive_changed),drive);
	}
}

void populate_outdir(void) {
	char local_cwd[1024];

	if(getcwd(local_cwd,sizeof(local_cwd))) {
		gtk_entry_set_text(GTK_ENTRY(outdir_field),local_cwd);
	} else {
		gtk_entry_set_text(GTK_ENTRY(outdir_field),".");
	}
}

#ifdef MACOSX
static gboolean macosx_allow_quit(GtkWidget *widget, gpointer gdata) {
	return(modal?TRUE:FALSE);
}
#endif

int main(int argc, char *argv[]) {
	GtkWidget *window, *vbox, *quitbutton, *srcdrop, *srcdrop_menu, *fmtdrop, *fmtdrop_menu, *mitem, *frame, *versionlabel;
	GSList *group;

#ifdef MACOSX
	char *homedir=getenv("HOME");
	if(homedir)
		chdir(homedir);
#endif
#ifdef WIN32
	TCHAR my_documents[MAX_PATH];
	if(SHGetFolderPath(NULL,CSIDL_PERSONAL,NULL,0,my_documents)==0)
		SetCurrentDirectory(my_documents);
#endif
	gtk_init(&argc,&argv);
#ifdef MACOSX
	GtkOSXApplication *theApp=g_object_new(GTK_TYPE_OSX_APPLICATION,NULL);
	g_signal_connect(theApp,"NSApplicationBlockTermination",G_CALLBACK(macosx_allow_quit),NULL);
#endif
	window=gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(window),"Disk Image and Browse");
	gtk_signal_connect(GTK_OBJECT(window),"destroy",GTK_SIGNAL_FUNC(quitpressed),NULL);
	gtk_container_border_width(GTK_CONTAINER(window),5);
	vbox=gtk_vbox_new(FALSE,5);
	frame=gtk_frame_new("Source Drive");
	gtk_container_border_width(GTK_CONTAINER(frame),5);
	gtk_box_pack_start(GTK_BOX(vbox),frame,FALSE,FALSE,0);
	gtk_widget_show(frame);
	srcdrop=gtk_option_menu_new();
	srcdrop_menu=gtk_menu_new();
	add_drives(srcdrop_menu);
	gtk_option_menu_set_menu(GTK_OPTION_MENU(srcdrop),srcdrop_menu);
	gtk_widget_show(srcdrop);
	/*gtk_box_pack_start(GTK_BOX(vbox),srcdrop,FALSE,FALSE,0);*/
	gtk_container_add(GTK_CONTAINER(frame),srcdrop);
	frame=gtk_frame_new("Disk Type");
	gtk_box_pack_start(GTK_BOX(vbox),frame,FALSE,FALSE,0);
	gtk_widget_show(frame);
	fmtdrop=gtk_option_menu_new();
	fmtdrop_menu=gtk_menu_new();
	add_formats(fmtdrop_menu);
	gtk_option_menu_set_menu(GTK_OPTION_MENU(fmtdrop),fmtdrop_menu);
	/*gtk_signal_connect(GTK_OBJECT(fmtdrop_menu),"deactivate",GTK_SIGNAL_FUNC(quitpressed),NULL);*/
	gtk_widget_show(fmtdrop);
	gtk_container_add(GTK_CONTAINER(frame),fmtdrop);
	browsebutton=gtk_button_new_with_label("Browse Disk Contents");
	gtk_box_pack_start(GTK_BOX(vbox),browsebutton,FALSE,FALSE,0);
	gtk_signal_connect(GTK_OBJECT(browsebutton),"clicked",GTK_SIGNAL_FUNC(browsepressed),NULL);
	gtk_widget_show(browsebutton);
	frame=gtk_frame_new("Output Image Directory");
	gtk_box_pack_start(GTK_BOX(vbox),frame,FALSE,FALSE,0);
	gtk_widget_show(frame);
	outdir_field=gtk_entry_new();
	populate_outdir();
	gtk_widget_show(outdir_field);
	gtk_container_add(GTK_CONTAINER(frame),outdir_field);
	frame=gtk_frame_new("Output Image Filename");
	gtk_box_pack_start(GTK_BOX(vbox),frame,FALSE,FALSE,0);
	gtk_widget_show(frame);
	fname_field=gtk_entry_new();
	gtk_entry_set_text(GTK_ENTRY(fname_field),"disk0001.dsk");
	gtk_widget_show(fname_field);
	gtk_container_add(GTK_CONTAINER(frame),fname_field);
	imgbutton=gtk_button_new_with_label("Capture Disk Image File");
	gtk_box_pack_start(GTK_BOX(vbox),imgbutton,FALSE,FALSE,0);
	gtk_signal_connect(GTK_OBJECT(imgbutton),"clicked",GTK_SIGNAL_FUNC(imgpressed),NULL);
	gtk_widget_show(imgbutton);
	quitbutton=gtk_button_new_with_label("Quit");
	gtk_box_pack_start(GTK_BOX(vbox),quitbutton,FALSE,FALSE,0);
	gtk_signal_connect(GTK_OBJECT(quitbutton),"clicked",GTK_SIGNAL_FUNC(quitpressed),NULL);
	versionlabel=gtk_label_new(MY_NAME " version " VERSION_STRING);
	gtk_box_pack_start(GTK_BOX(vbox),versionlabel,FALSE,FALSE,0);
	gtk_widget_show(versionlabel);
	gtk_container_add(GTK_CONTAINER(window),vbox);
	gtk_widget_show(quitbutton);
	gtk_widget_show(vbox);
	/*gtk_widget_set_usize(window,100,1);*/
	gtk_window_set_default_size(GTK_WINDOW(window),300,1);
	gtk_widget_show(window);
	update_sensitivity();
#ifdef MACOSX
	GtkWidget *appmenu=gtk_menu_bar_new();
	gtk_osxapplication_set_menu_bar(theApp,GTK_MENU_SHELL(appmenu));
	gtk_osxapplication_ready(theApp);
#endif
	gtk_main();
	return 0;
}
