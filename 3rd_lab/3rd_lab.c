//Кол-во процессов > 1.

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


#define BUF_SIZE 256
#define TRUE 1
#define FALSE 0
#define DIR_SEPARATOR '/'
#define DEFAULT_DEV_ID -1
#define INODE_ARRAY_IN_SIZE 1024 * 128


struct inode_set
{
    unsigned int count;
    unsigned int size;
    ino_t *array;
} inode_set;

struct p_set
{
    int **pipes;
    char *seted;
    int count;
    int size;
    int max_fd;
};


typedef struct p_set pipe_set;


void generateError(char *err_message);
//Viewing directry files.
unsigned long scanDirectory(char *dir_path, dev_t dev_id, pipe_set *p_set);
//Checking is file a directory or is a regular one.
void processAccordingToFileType(char *path, dev_t dev_id, pipe_set *p_set);
//Checking file type and file processing.
int processFile(char* path, pipe_set *p_set);
//Check if the number of launched processes is less than the number of processes entered by user.
int thereAreNoFreeProcesses(int cur_pid_num);
//Checking file descriptors for data from child processes.
void checkPipes(pipe_set *p_set);
//Returns created pipe.
int *getPipe(pipe_set *p_set);
//Excluding pipe from the pipe set.
void excludeFromPipeSet(pipe_set *p_set, int index);
//Allocating memory for pipe set.
pipe_set *allocatePipeSet(int num);
void countPeriods(char *path, int fd);
int createInodeArray();
int addToInodeArray(ino_t inod);
int inodeInSet(ino_t inod);
void clearInodeArray();


int pid_count;
int user_pid_count = 0;


void sigchld_handler (int signum)
{
    pid_t p;
    int status;

    /* loop as long as there are children to process */
    while (1) {

       /* retrieve child process ID (if any) */
       p = waitpid(-1, &status, WNOHANG);

       /* check for conditions causing the loop to terminate */
       if (p == -1) {
           /* continue on interruption (EINTR) */
           if (errno == EINTR) {
               continue;
           }
           /* break on anything else (EINVAL or ECHILD according to manpage) */
           break;
       }
       else if (p == 0) {
           /* no more children to process, so break */
           break;
       }

    }
}

int main(int argc, char *argv[])
{
    struct sigaction action;
    pipe_set *p_set;

    action.sa_handler = sigchld_handler;
    sigaction(SIGCHLD, &action, 0);
    //Checking command line arguments num.
    if (argc < 3)
        generateError("too few arguments");

    if (createInodeArray() != 0)
    	generateError("inode_set creating");	

    user_pid_count = atoi(argv[2]);
    p_set = allocatePipeSet(user_pid_count);
    
    processAccordingToFileType(argv[1], DEFAULT_DEV_ID, p_set); 
    checkPipes(p_set);  
    
    clearInodeArray();
	
	return 0;
}

//Checking is file a directory or is a regular one.
void processAccordingToFileType(char *path, dev_t dev_id, pipe_set *p_set)
{

    struct stat entry_info;
    pid_t pid;
    int status;

    if (lstat(path, &entry_info) != 0)  
        generateError("stat error");
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
            generateError("allocating memory");
    }
    
    //File processing
    //Checking if file is regular.
    if (S_ISREG(entry_info.st_mode))
    {
        processFile(path, p_set);
    }
    //Checking if file is a directory. 
    else if (S_ISDIR(entry_info.st_mode))
    {
        scanDirectory(path, dev_id, p_set);            
    }
    
}

//Viewing directry files.
unsigned long scanDirectory(char *dir_path, dev_t dev_id, pipe_set *p_set)
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
        processAccordingToFileType(pathName, dev_id, p_set);
        retval = readdir_r(dir, &entry, &entry_ptr);
        if (retval != 0)
    		generateError("reading directory");
    }

    if (closedir(dir) != 0)
    	generateError("closing directory");

    return 0;
}


//Checking file type and file processing.
int processFile(char* path, pipe_set *p_set)
{  
    pid_t pid;
    int fd[2];
    int *pnt;

    if (thereAreNoFreeProcesses(p_set->count))
    {
        checkPipes(p_set);
    }

    pnt = getPipe(p_set);
    fd[0] = pnt[0];
    fd[1] = pnt[1];

    if ((pid = fork()) < 0) {
        generateError("process creating");
    }   
    else if (pid == 0) {
        close(fd[0]);
        countPeriods(path, fd[1]);
    }
    else if (pid > 0) {
        close(fd[1]);
    }
                 
    return 0;
}

//Checking file descriptors for data from child processes.
void checkPipes(pipe_set *p_set)
{
    fd_set set;
    int ready_fd_num;
    struct timeval timeout;
    size_t total_readen, bytes_to_read;
    ssize_t bytes_readen;
    char buf[256];
    //do 
   // {
        printf("select\n");
        FD_ZERO(&set);
        timeout.tv_sec = 2;
        timeout.tv_usec = 0;

        for (int i = 0; i < pid_count; i++)
        {
            FD_SET(p_set->pipes[i][0], &set);
        }

        //printf("%d  %d\n", p_set->max_fd, ready_fd_num);
        //ready_fd_num = select(p_set->max_fd, &set, &set, NULL, &timeout);
    //} while (0 == ready_fd_num);

    //if (ready_fd_num < 0)
    //    generateError("select failed");

    for (int i = 0; i < p_set->size; i++)
    {
       // if (FD_ISSET(p_set->pipes[i][0], &set))
        if (p_set->seted[i])
        {
            //Reading file processing result from the pipe.
            bytes_to_read = 255;
            total_readen = 0;
            while (1)
            {
                bytes_readen = read(p_set->pipes[i][0], buf, bytes_to_read);

                if ( bytes_readen <= 0 )
                break;

                total_readen += bytes_readen;
            }

            buf[total_readen] = '\0';

            printf("%s\n", buf);

            close(p_set->pipes[i][0]);
            excludeFromPipeSet(p_set, i);
        }
    }
}


void countPeriods(char *path, int fd)
{
    write(fd, path, strlen(path));
    printf("%d\n", getpid());
    //sleep(2);
    exit(0);
}


//Check if the number of launched processes is less 
//than the number of processes entered by user.
int thereAreNoFreeProcesses(int cur_pid_num)
{
    return (cur_pid_num >= user_pid_count - 1);
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


//Allocating memory for pipe set.
pipe_set *allocatePipeSet(int num)
{
    pipe_set *p_set = (pipe_set*)calloc(1, sizeof(pipe_set));

    p_set->count = 0;
    p_set->size = num;
    p_set->max_fd = 0;
    p_set->pipes = (int**)calloc(num, sizeof(int*));
    p_set->seted = (char*)calloc(num, sizeof(char));

    for (int i = 0; i < num; i++)
    {
        p_set->pipes[i] = (int*)calloc(2, sizeof(int));
    }

    return p_set;
}

//Returns created pipe.
int *getPipe(pipe_set *p_set)
{
    int index = 0;

    while ((p_set->seted[index]) && (index < p_set->size))
        index++;

    if (index == p_set->size)
    {
        printf("error: including to pipe");
        return NULL;
    }

    pipe(p_set->pipes[index]);
    
    //printf("%d %d\n", p_set->pipes[index][0], p_set->pipes[index][1]);
    p_set->seted[index] = TRUE;

    if (p_set->pipes[index][0] > p_set->max_fd)
        p_set->max_fd = p_set->pipes[index][0];

    p_set->count++;

    return p_set->pipes[index];
}

//Excluding pipe from the pipe set.
void excludeFromPipeSet(pipe_set *p_set, int index)
{
    p_set->seted[index] = FALSE;
    p_set->count--;
}