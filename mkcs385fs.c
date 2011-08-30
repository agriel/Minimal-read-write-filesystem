#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<unistd.h>
#include<dirent.h>

char megabuf[10000000];

int main(int argc, char** argv) {
	if(argc<3) {
		fprintf(stderr,"Usage: mkcs385fs <directory>/ <image>\n");
		exit(1);
	}
	
	/* create disk header */
	DIR *dir = opendir(argv[1]);
	FILE* image = fopen(argv[2],"w+");
	if(!image) {
		perror("Couldn't open image file.");
		exit(1);
	}
	struct dirent *entry;

	struct {
		char name[26];
		int size;
	} record;

	char path[100];	
	struct stat st;
	int filecount = 0;
	while(entry=readdir(dir)) {
		if(filecount++<3) continue;

		sprintf(path,"%s%s",argv[1],entry->d_name);
		stat(path,&st);
		
		memset(&record,0,sizeof(record));
		strncpy(record.name,entry->d_name,25);
		record.size = st.st_size;
		fwrite(&record,sizeof(record),1,image);
	}

	fseek(image,1024,SEEK_SET);

	/* add the files */
	closedir(dir);
	opendir(argv[1]);
	filecount=0;
	while(entry=readdir(dir)) {
		if(filecount++<3) continue;

		sprintf(path,"%s%s",argv[1],entry->d_name);
		FILE* f = fopen(path,"r");
		int read_bytes = fread(megabuf,1,sizeof(megabuf),f);
		fwrite(megabuf,read_bytes,1,image);
		fclose(f);
		printf("read %d bytes\n",read_bytes);

		int tail = read_bytes % 1024;
		fseek(image,1024-tail,SEEK_CUR);
	}

	fseek(image,4096,SEEK_CUR); // padding to make sure every file uses at least a page
	fwrite("",1,1,image);
	fclose(image);
}
