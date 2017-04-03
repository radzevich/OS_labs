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
#define HARD_DRIVE_INFO_FILE_L "/sys/class/block/sda/size"

void strRewrite(char *buf, char *str);
void generateError(char *err_message);
int countDirectrySize(char *dir_path, unsigned long *dir_size, unsigned long *file_size);
unsigned long getHardDriveSize();


int main(int argc, char *argv[])
{
    //Redirection error stream to error log file.
    FILE *error_log = freopen("error.log", "w", stderr);

	double part_size;
    unsigned long file_size = 0, dir_size = 0, hard_drive_size;

    //Checking command line arguments num.
    if (argc < 2)
        generateError("too few arguments");

    if (countDirectrySize(/*"/home/virtuain/Desktop"*/argv[1], &dir_size, &file_size) == 0)
    {
        printf("The total size of regular files is: %ld bytes\n", file_size);
        printf("The total size of occupied memory space: %ld bytes\n", file_size + dir_size);
        hard_drive_size = getHardDriveSize();
        part_size = (double)(dir_size + file_size) / hard_drive_size * 100;
        printf("The part of memory space is: %2f%%\n", part_size);
    }

    fclose(error_log);
	
	return 0;
}

int countDirectrySize(char *dir_path, unsigned long int *dir_size, unsigned long int *file_size)
{
	DIR *dir = NULL;
	struct dirent entry;
	struct dirent *entry_ptr;
	int retval = 0;
	char pathName[BUF_SIZE + 1];

    dir = opendir(dir_path);
	if (NULL == dir)
        generateError("opening directory");

    retval = readdir_r(dir, &entry, &entry_ptr);
	while (entry_ptr != NULL)
	{
		struct stat entry_info;

		// Short-circuit the . and .. entries. 
        if ((strncmp(entry.d_name, "..", BUF_SIZE) == 0))
		{
            retval = readdir_r(dir, &entry, &entry_ptr);
            continue;
        }

        (void)strncpy(pathName, dir_path, BUF_SIZE);
        (void)strncat(pathName, "/", BUF_SIZE);
        (void)strncat(pathName, entry.d_name, BUF_SIZE);

        if ((lstat(pathName, &entry_info) == 0) & !S_ISLNK(entry_info.st_mode))
        {
            if (S_ISDIR(entry_info.st_mode))
            {
                *dir_size += entry_info.st_size;

                if (!(strncmp(entry.d_name, ".", BUF_SIZE) == 0))
                {
                    countDirectrySize(pathName, dir_size, file_size);
                }
            }
            else if (S_ISREG(entry_info.st_mode))
            {
                *file_size += entry_info.st_size;
            }
        }
        retval = readdir_r(dir, &entry, &entry_ptr);
    }
    (void)closedir(dir);
    return 0;
}


//Getting hard drive's size
unsigned long int getHardDriveSize()
{
    FILE *ptr = NULL;
    unsigned long int size = 0;

    ptr = fopen(HARD_DRIVE_INFO_FILE_L, "r");

    if (NULL == ptr)
        generateError("opening of hardware info file");

    if (fscanf(ptr, "%ld", &size) <= 0)
        generateError("reading info file");

    return size << 9;
}


//Generating error with error message.
void generateError(char *err_message)
{
	fprintf(stderr, "2.c ERROR: %s: %s\n", err_message, strerror(errno));
	exit(1);
}
