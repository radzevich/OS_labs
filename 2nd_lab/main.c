//**********************Linux "du" like program********************************************************//
//Counts the total size of files in directory and all subdirectories.
//Arguments: 
//  - path to the directory or file.
//Return values:
//  - for each file and directory returns it's read size;
//  - return total size of entered file or directory, it's block size and the efficiency of memory use.
//******************************************************************************************************//

//WARNING: my default block size is 512 bits but it can be different for your machine.
//If you are not lazy you can add getting it from some system file (it is somewhere on /sys).


#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <limits.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>

#define BUF_SIZE 256
#define TRUE 1
#define HARD_DRIVE_INFO_FILE_L "/sys/class/block/sda/size"
#define DIR_SEPARATOR '/'
#define DEFAULT_DEV_ID -1
#define INODE_ARRAY_IN_SIZE 1024 * 128 
#define BLOCK_SIZE 512
#define RESULT_LENGTH 9


void strRewrite(char *buf, char *str);
void generateError(char *err_message);
unsigned long countDirSize(char *dir_path, dev_t dev_id, unsigned long *block_size);
unsigned long processAccordingToFileType(char *path, dev_t dev_id, unsigned long *block_size);
unsigned int getSymbolicLincSize(char *link_path);
unsigned long getHardDriveSize();
char *getTime();
int createInodeArray();
int addToInodeArray(ino_t inod);
int inodeInSet(ino_t inod);
void clearInodeArray();

struct inode_set
{
	unsigned int count;
	unsigned int size;
	ino_t *array;
} inode_set;


int main(int argc, char *argv[])
{
    //Redirection error stream to error log file.
    //FILE *error_log = freopen("error.log", "w", stderr);

	double part_size;
    unsigned long file_size = 0, hard_drive_size = 0;

    //Checking command line arguments num.
    if (argc < 2)
        generateError("too few arguments");

    if (createInodeArray() != 0)
    	generateError("inode_set creating");	

    file_size = processAccordingToFileType(argv[1], DEFAULT_DEV_ID, &hard_drive_size);

    if (hard_drive_size != 0)
    {
    	part_size = (double)(file_size) / hard_drive_size * 100;
    	printf("%10ld  %10ld  %f%%  total\n", file_size, hard_drive_size, part_size);
    }
    else
    {
        printf("%10ld  %10ld  0%%  total\n", file_size, hard_drive_size);
    }
    
    clearInodeArray();
    //fclose(error_log);
	
	return 0;
}


//Check of file type and proceess it accordingly.
unsigned long processAccordingToFileType(char *path, dev_t dev_id, unsigned long *block_size)
{
	struct stat entry_info;
	unsigned long int file_size = 0, total_size = 0;
	dev_t par_dev_id = dev_id;

	if (lstat(path, &entry_info) != 0)  
		generateError("stat error");

	//If device id of current file and of parent one differs than do not process file. 
	if (dev_id == DEFAULT_DEV_ID)
	{
		dev_id = entry_info.st_dev;
	}
	else if (dev_id != entry_info.st_dev)
	{
		return 0;
	}

	if (entry_info.st_nlink > 1)
	{
		if (inodeInSet(entry_info.st_ino) != 0)
			return 0;
		else if (addToInodeArray(entry_info.st_ino) != 0)
			generateError("allocating memory");
	}
    //Counting file size according to it's type.
    if (S_ISREG(entry_info.st_mode))
    {
    	//Adding size of current file to total sum.
        file_size += entry_info.st_size;
        *block_size += entry_info.st_blocks * BLOCK_SIZE;
    } 
    else if (S_ISLNK(entry_info.st_mode))
    {
      	//Getting symlic link length.
       	file_size += entry_info.st_size;
        *block_size += entry_info.st_blocks * BLOCK_SIZE;
        
    }
    else if (S_ISDIR(entry_info.st_mode))
    {
      	//Getting directory size.
       	file_size += countDirSize(path, dev_id, &total_size) + (8 * BLOCK_SIZE);
       	*block_size += total_size;

       	printf("%10ld  %10ld  %s\n", file_size, total_size, path);	     		
    }
 	
 	return file_size;	
}

//Counting directory size. 
unsigned long countDirSize(char *dir_path, dev_t dev_id, unsigned long *block_size)
{
	DIR *dir;
	struct dirent entry;
	struct dirent *entry_ptr;
	int retval = 0, len = 0;
	unsigned long file_size = 0;
	char *pathName;

	dir = opendir(dir_path);
	if (NULL == dir)
        generateError("opening directory");

    //Directory files processing.
    retval = readdir_r(dir, &entry, &entry_ptr);
    if (retval != 0)
    	generateError("reading directory");

	while (entry_ptr != NULL)
	{
		//Short-circuit the  ".." and "." entries. 
        if ((strncmp(entry.d_name, "..", BUF_SIZE) == 0) || (strncmp(entry.d_name, ".", BUF_SIZE) == 0))
		{
            retval = readdir_r(dir, &entry, &entry_ptr);
            if (retval != 0)
    			generateError("reading directory");
            continue;
        }

        //File name making.
        len = sizeof(dir_path) + sizeof(entry.d_name) + 1;
        pathName = (char *)calloc(len, sizeof(char));
        
        if (pathName != NULL)
        {
        	(void)strncpy(pathName, dir_path, len);
        	if (pathName[strlen(pathName) - 1] != '/')
            	(void)strncat(pathName, "/", len);
        	(void)strncat(pathName, entry.d_name, len);
    	}

        //Recursive counting of subdirectories and files.
        file_size += processAccordingToFileType(pathName, dev_id, block_size);
        retval = readdir_r(dir, &entry, &entry_ptr);
        if (retval != 0)
    		generateError("reading directory");
    }
    if (closedir(dir) != 0)
    	generateError("closing directory");

    return file_size;
}


//Counting symlic link length.
unsigned int getSymbolicLincSize(char *link_path)
{
	DIR *dir;
	struct dirent entry;
	struct dirent *entry_ptr;
	int retval = 0, len = 0;

	dir = opendir(link_path);

    //Directory files processing.
    retval = readdir_r(dir, &entry, &entry_ptr);
    len = strlen(entry.d_name);

    closedir(dir);
    return len;
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



//**************************INODE ARRAY FOR HARD LINKS KEEPING**********************************//
int createInodeArray()
{
	inode_set.array = (ino_t *)calloc(INODE_ARRAY_IN_SIZE, sizeof(ino_t));
	inode_set.size = INODE_ARRAY_IN_SIZE;
	inode_set.count = 0;

	if (NULL == inode_set.array)
		return 1;
	else
		return 0;
}


void clearInodeArray()
{
	free(inode_set.array);
}


int addToInodeArray(ino_t inod)
{
	if (inode_set.count >= inode_set.size)
	{
		if (realloc(inode_set.array, inode_set.size * 2) != NULL)
			inode_set.size *= 2;
		else
			return 1;
	}

	inode_set.array[inode_set.count] = inod;
	inode_set.count++;

	return 0;
} 


int inodeInSet(ino_t inod)
{
	for (int i = 0; i < inode_set.count; i++)
	{
		if (inode_set.array[i] == inod)
			return 1;
	}
	return 0;
}



