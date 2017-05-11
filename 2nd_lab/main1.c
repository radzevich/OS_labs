#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <limits.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>

#define BUF_SIZE 256
#define TRUE 1

void strRewrite(char *buf, char *str);
void generateError(char *err_message);
unsigned long countDirectrySize(FILE *dir_path);
unsigned long getHardwareSize();


int main(int argc, char *argv[])
{
	//Redirection error stream to error log file.
	FILE *error_log = freopen("error.log", "w", stderr);

	FILE *ptr;
	double part_size;
	unsigned long size, hardware_size;

	//Checking command line arguments num.
	if (argc < 2)
		generateError("too few arguments");



	size = countDirectrySize(ptr);
	printf("The total size is: %ld bytes\n", size);
	hardware_size = getHardwareSize();
	part_size = (double)size / hardware_size * 100;
	printf("The part of memory space is: %2f%%\n", part_size);

	pclose(ptr);

	fclose(error_log);
	
	return 0;
}

unsigned long countDirectrySize(FILE *dir_path)
{
	DIR *dir = NULL;
	struct dirent entry;
	struct dirent *entry_ptr;
	int retval = 0;
	char pathName[BUF_SIZE + 1];

	dir = opendir();
	if (NULL == dir)
		generateError("opening error");

	retval = readdir_r(dir, &entry, &entry_ptr)
	while (entry_ptr != NULL)
	{
		struct stat entry_info;

		// Short-circuit the . and .. entries. 
		if ((strncmp(entry.d_name, ".", BUF_SIZE) == 0) || (strncmp(entry.d_name, "..", BUF_SIZE) == 0)) 
		{
            retval = readdir_r( dir, &entry, &entryPtr );
            continue;
        }

        (void)strncpy(pathName, dir_path, BUF_SIZE);
        (void)strncat(pathName, "/", BUF_SIZE);
        (void)strncat(pathName, entry.d_name, BUF_SIZE);

        
	}
}

//Generating error with error message.
void generateError(char *err_message)
{
	fprintf(stderr, "2.c ERROR: %s: %s\n", err_message, strerror(errno));
	exit(1);
}