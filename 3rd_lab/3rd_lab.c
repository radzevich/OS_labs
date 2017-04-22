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
#define DIR_SEPARATOR '/'
#define DEFAULT_DEV_ID -1
#define INODE_ARRAY_IN_SIZE 1024 * 128


void generateError(char *err_message);
//Viewing directry files.
unsigned long scanDirectory(char *dir_path, dev_t dev_id);
//Checking file type and file processing.
int processFile(char* path, dev_t dev_id);
void countPeriods(char *path);
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
    //Redirect of error stream to error log file.
    //FILE *error_log = freopen("error.log", "w", stderr);

    //Checking command line arguments num.
    if (argc < 3)
        generateError("too few arguments");

    if (createInodeArray() != 0)
    	generateError("inode_set creating");	

    processFile(argv[1], DEFAULT_DEV_ID);   
    
    clearInodeArray();
    //fclose(error_log);
	
	return 0;
}


//Viewing directry files.
unsigned long scanDirectory(char *dir_path, dev_t dev_id)
{
	DIR *dir;
	struct dirent entry;
	struct dirent *entry_ptr;
	int retval = 0, len = 0;
	char *pathName;

	dir = opendir(dir_path);
	if (NULL == dir)
    {
        printf("%s %s\n", dir_path, strerror(errno));
        return 1;
    }

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

        //Recursive directory walking.
        processFile(pathName, dev_id);
        retval = readdir_r(dir, &entry, &entry_ptr);
        if (retval != 0)
    		generateError("reading directory");
    }

    if (closedir(dir) != 0)
    	generateError("closing directory");

    return 0;
}


//Checking file type and file processing.
int processFile(char* path, dev_t dev_id)
{
    struct stat entry_info;

    if (lstat(path, &entry_info) != 0)  
        generateError("stat error");

    //If device id of current file and of parent directory differs than do not process file. 
    if (dev_id == DEFAULT_DEV_ID)
    {
        dev_id = entry_info.st_dev;
    }
    else if (dev_id != entry_info.st_dev)
    {
        return 0;
    }

    //Checking hard links.
    if (entry_info.st_nlink > 1)
    {
        if (inodeInSet(entry_info.st_ino) == TRUE)
            return 0;
        else if (addToInodeArray(entry_info.st_ino) != 0)
            generateError("allocating memory");
    }
    //File processing
    //Checking if file is regular.
    if (S_ISREG(entry_info.st_mode))
    {
        countPeriods(path);
    }
    //Checking if file is a directory. 
    else if (S_ISDIR(entry_info.st_mode))
    {
        scanDirectory(path, dev_id);             
    }

    return 0;
}


void countPeriods(char *path)
{
    printf("%s\n", path);
}


//Generating error with error message.
void generateError(char *err_message)
{
	fprintf(stderr, "2.c ERROR: %s: %s\n", err_message, strerror(errno));
	exit(1);
}


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
			return TRUE;
	}
	return 0;
}