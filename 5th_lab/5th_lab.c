#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <limits.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <dirent.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>


#define BUF_SIZE 256
#define TRUE 1
#define FALSE 0
#define DIR_SEPARATOR '/'
#define DEFAULT_DEV_ID -1
#define BLOCK_SIZE 8
#define INODE_ARRAY_IN_SIZE 1024 * 128
#define SAVE_TO_FILE


struct inode_set
{
    unsigned int count;
    unsigned int size;
    ino_t *array;
} inode_set;

/*
volatile struct thread_counter
{
    unsigned long th_blocked_tid;
    unsigned int th_num;
} counter;*/

struct params
{
    char *path;
    int index;    
};

int user_thread_num;
unsigned long *thread_arr;
char *dir_to_save;
char *key;
char *exec_name;


void generate_error(char *err_message);
//Viewing directry files.
unsigned long scanDirectory(char *dir_path, dev_t dev_id);
//Checking is file a directory or is a regular one.
void processAccordingToFileType(char *path, dev_t dev_id);
//Checking file type and file processing.
int processFile(char* path);
//Functions used to work with inode array.
int thereAreNoFreeThreads();
int createInodeArray();
int addToInodeArray(ino_t inod);
int inodeInSet(ino_t inod);
void clearInodeArray();
long getFileSize(FILE *file);
void read_from_file(unsigned char *buf, size_t bytes_to_read, FILE *file);
//Thread rowutine.
void *thread_start(void *arg);
//Finalization of thread work.
void change_thread_counter(unsigned long tid, int value);
//File encrypting procedure. Just a xor with a key.
long encrypt(char *buf, char *key, long buf_size);
//File encrypting procedure. Just a xor with a key.
long encrypt(char *buf, char *key, long buf_size);
//Return results to user through standart output stream.
void output(pthread_t tid, char *path, long bytes_num);
//Saving results to file.
void save_to_file (char *dir, char *file_path, char *buf, long file_size);
unsigned long *create_thread_array(int size);
int get_empty_thread(unsigned long *arr, int size);
void clear_thread_array(unsigned long *arr);
int thread_arr_is_empty(unsigned long *arr, int size);


int main(int argc, char *argv[])
{
    //Checking command line arguments num.
    if (argc < 5)
        generate_error("too few arguments");

    exec_name = argv[0];
    dir_to_save = argv[2];
    user_thread_num = atoi(argv[3]);
    key = argv[4];
    thread_arr = create_thread_array(atoi(argv[3]));

    if (createInodeArray() != 0)
    	generate_error("inode_set creating");

    if (user_thread_num <= 0)
    {
        printf("There are no available processes\n");
        return 0;
    }

    processAccordingToFileType(argv[1], DEFAULT_DEV_ID); 

    while (1)
    {
        if (thread_arr_is_empty(thread_arr, user_thread_num))
            break;
    }
    
    clearInodeArray();
	
	return 0;
}

//Checking is file a directory or is a regular one.
void processAccordingToFileType(char *path, dev_t dev_id)
{

    struct stat entry_info;
    pid_t pid;
    int status;

    if (lstat(path, &entry_info) != 0)  
        generate_error("stat error");
    //If device id of current file and of parent directory differs than do not process file. 
    if (dev_id == DEFAULT_DEV_ID) {
        dev_id = entry_info.st_dev;
    }
    else if (dev_id != entry_info.st_dev) {
        return;
    }

    //Checking hard links.
    if (entry_info.st_nlink > 1)
    {
        if (inodeInSet(entry_info.st_ino) == TRUE)
            return;
        else if (addToInodeArray(entry_info.st_ino) != 0)
            generate_error("allocating memory");
    }
    
    //File processing
    //Checking if file is regular.
    if (S_ISREG(entry_info.st_mode))
    {
        processFile(path);
    }
    //Checking if file is a directory. 
    else if (S_ISDIR(entry_info.st_mode))
    {
        scanDirectory(path, dev_id);            
    }
    
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

    //Processing of files in the current directory.
    retval = readdir_r(dir, &entry, &entry_ptr);
    if (retval != 0)
    	generate_error("reading directory");

	while ((entry_ptr != NULL) & (retval == 0))
	{
		//Short-circuit the  ".." and "." entries. 
        if ((strncmp(entry.d_name, "..", BUF_SIZE) == 0) || (strncmp(entry.d_name, ".", BUF_SIZE) == 0))
		{
            retval = readdir_r(dir, &entry, &entry_ptr);
            if (retval != 0)
    			generate_error("reading directory");
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
        processAccordingToFileType(pathName, dev_id);
        retval = readdir_r(dir, &entry, &entry_ptr);
        if (retval != 0)
    		generate_error("reading directory");
    }

    if (closedir(dir) != 0)
    	generate_error("closing directory");

    return 0;
}


//Checking file type and file processing.
int processFile(char* path)
{  
    pthread_t thread;
    int index;
    struct params *arg;

    while (1)
    {
        if ((index = get_empty_thread(thread_arr, user_thread_num)) >= 0) {
            thread_arr[index] = 1;
            break;
        }
    }

    arg = (struct params *)calloc(1, sizeof(struct params));
    arg->path = path;
    arg->index = index;

    if (pthread_create(&thread, NULL, thread_start, arg) != 0)
    {
        generate_error("thread creating");
        exit(1);
    }
                 
    return 0;
}


//Check if the number of launched processes is less 
//than the number of processes entered by user.
int thereAreNoFreeThreads()
{
//    return (counter.th_num >= user_thread_num);
}

//Generating error with error message.
void generate_error(char *err_message)
{
	fprintf(stderr, "%s ERROR: %s: %s\n", exec_name, err_message, strerror(errno));
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

//************************************************Algorithm****************************************************************

void read_from_file(unsigned char *buf, size_t bytes_to_read, FILE *file)
{
    size_t bytes_readen, total_readen;

    total_readen = 0;

    while (1)
    {
        bytes_readen = fread(buf + total_readen, sizeof(*buf), bytes_to_read - total_readen, file);

        total_readen += bytes_readen;

        if (bytes_readen >= bytes_to_read)
            break;
    }
}

//Return the size of file.
long getFileSize(FILE *file)
{
    long cur_pos, start_pos, end_pos;

    cur_pos = ftell(file);
    fseek(file, 0, SEEK_SET);
    start_pos = ftell(file);
    fseek(file, 0, SEEK_END);
    end_pos = ftell(file);
    fseek(file, cur_pos, SEEK_SET);

    return end_pos - start_pos;
}

//Thread rowutine.
void *thread_start(void *arg)
{
    FILE *file;
    unsigned char *buf;
    long encrypted_bytes_num, file_size;
    char *path = ((struct params *)arg)->path;

    file = fopen(((struct params *)arg)->path, "r");

    if (NULL != file)
    {
        if ((file_size = getFileSize(file)) != 0)
        {
            buf = (char *)calloc(file_size, sizeof(char));

            read_from_file(buf, file_size, file);
            encrypted_bytes_num = encrypt(buf, key, file_size);
            output(pthread_self(), path, encrypted_bytes_num);
            save_to_file(dir_to_save, path, buf, encrypted_bytes_num);
            free(buf);
            fclose(file);
        }
    }
    else
    {
        generate_error(path);  
    }

    thread_arr[((struct params *)arg)->index] = 0;
    free(arg);
    pthread_exit(NULL);
}

//File encrypting procedure. Just a xor with a key.
long encrypt(char *buf, char *key, long buf_size)
{
    int key_size, i;

    key_size = strlen(key);

    for (i = 0; i < buf_size; i++)
    {
        buf[i] ^= key[i % key_size];
    }

    return i;
}

//Return results to user through standart output stream.
void output(pthread_t tid, char *path, long bytes_num)
{
    printf("%lu %s %ld\n", tid, path, bytes_num);
}

//Return file name with a path to directory where encrypted files are.
char *get_file_name(char *dir, char *file_path)
{
    int file_name_start, file_path_len, dir_name_len, j, file_name_len;
    char *file_name;

    file_path_len = strlen(file_path);
    dir_name_len = strlen(dir);
    file_name_start = 0;

    for (int i = 0; i < file_path_len - 1; i++)
    {
        if (file_path[i] == DIR_SEPARATOR)
            file_name_start = i + 1;
    }

    file_name_len = file_path_len - file_name_start + dir_name_len + 11;
    file_name = (char *)calloc(file_name_len, sizeof(char));

    (void)strncpy(file_name, dir, file_name_len);
    if (dir[dir_name_len - 1] != '/')
    {
        (void)strncat(file_name, "/", file_name_len);
        dir_name_len++;
    }

    j = dir_name_len;
    for (int i = file_name_start; i < file_path_len; i++)
    {
        file_name[j++] = file_path[i];
    }

    (void)strncat(file_name, ".copy", file_name_len);

    return file_name;
}

//Saving results to file.
void save_to_file (char *dir, char *file_path, char *buf, long file_size)
{
    FILE *file;
    char *file_name;
    long total_written, items_written;

    file_name = get_file_name(dir, file_path);

    file = fopen(file_name, "w");
    if (NULL != file)
    {
        //Saving results in file.
        total_written = 0;

        while (1)
        {
            items_written = fwrite(buf + total_written, sizeof(*buf), file_size - total_written, file);

            total_written += items_written;

            if (total_written >= file_size)
                break;
        }
        fclose(file);
    }
    else
    {
        generate_error("opening file");
    }
    
    free(file_name);
}

//Finalization of thread work.
void change_thread_counter(unsigned long tid, int value)
{
   /* while (1)
    {
        if (counter.th_blocked_tid == 0)
        {
            counter.th_blocked_tid = tid;

            if (counter.th_blocked_tid == tid)
            {
                printf(" ");
                counter.th_num += value;
                counter.th_blocked_tid = 0;
                break;
            }
        }
    }*/
}

unsigned long *create_thread_array(int size)
{
    return (unsigned long *)calloc(size, sizeof(unsigned long));
}

int get_empty_thread(unsigned long *arr, int size)
{
    for (int i = 0; i < size; i++)
    {
        if (arr[i] == 0)
            return i;
    }
    return -1;
}

int thread_arr_is_empty(unsigned long *arr, int size)
{
    for (int i = 0; i < size; i++)
    {
        if (arr[i] > 0)
            return 0;
    }
    return 1;
}

void clear_thread_array(unsigned long *arr)
{
    free(arr);
}