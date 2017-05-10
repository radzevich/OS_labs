#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <limits.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/shm.h>
#include <dirent.h>
#include <unistd.h>
#include <signal.h>


#define BUF_SIZE 256
#define TRUE 1
#define FALSE 0
#define DIR_SEPARATOR '/'
#define DEFAULT_DEV_ID -1
#define INODE_ARRAY_IN_SIZE 1024 * 128
#define BLOCK_SIZE 8
#define PRD_TABLE_CACHE "prd_cache"

struct inode_set
{
    unsigned int count;
    unsigned int size;
    ino_t *array;
} inode_set;

struct period
{
    unsigned char val;
    long len;
    long count;
};

struct period_set
{
    struct period prd_table[BLOCK_SIZE / 2 + 2];
    int count;
};

struct period_list
{
    struct period *prd_table;
    int count;
    int size;
};


typedef struct period_set prd_set;
typedef struct period_list prd_list;


//Global table of periods for each byte.
prd_set prd_array[1 << BLOCK_SIZE];
//ID for sharing memory.
int shmid = 0;
//The maximal number of child processes can be created.
int user_proc_num = 0;
//The current number of working child processes.
int proc_counter = 0;


void generateError(char *err_message);
//Viewing directry files.
unsigned long scanDirectory(char *dir_path, dev_t dev_id);
//Checking is file a directory or is a regular one.
void processAccordingToFileType(char *path, dev_t dev_id);
//Checking file type and file processing.
int processFile(char* path);
//Functions used to work with inode array.
int thereAreNoFreeProcesses();
int createInodeArray();
int addToInodeArray(ino_t inod);
int inodeInSet(ino_t inod);
void clearInodeArray();
//Functions used to count periods.
void initializePeriodTable(char *path);
//Adding new group to period list.
void addToPeriodList(prd_list *p_list, struct period *prd);
//Group bits and add it to period set.
void processValue(prd_set *p_set, unsigned char val);
//Add groups of pits to period set.
void setValue(prd_set *p_set, unsigned char prd_val, long prd_len);
void saveToFile(prd_set *p_set, char *path);
long getFileSize(FILE *file);
void readFromFile(unsigned char *buf, size_t bytes_to_read, FILE *file);
//Main algorithm function. Open file, read it in buf, call processing data functions.
void countPeriods(char *path);
//Add the first or the last group of bits to period set.
void setEndPointValue(prd_set *p_set, unsigned char prd_val, long prd_len);
//Adding new group to period list.
void addToPeriodList(prd_list *p_list, struct period *prd);
//Processing groups of bits and adding it to result table.
void processData(prd_list *p_list, unsigned char *data, long data_size);


int main(int argc, char *argv[])
{
    //Checking command line arguments num.
    if (argc < 3)
        generateError("too few arguments");

    if (createInodeArray() != 0)
    	generateError("inode_set creating");

    shmid = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | 0666);	

    user_proc_num = atoi(argv[2]);
    if (user_proc_num <= 0)
    {
        printf("There are no available processes\n");
        return 0;
    }

    initializePeriodTable(PRD_TABLE_CACHE);
    processAccordingToFileType(argv[1], DEFAULT_DEV_ID); 
    
    clearInodeArray();
    
    while (proc_counter > 0)
    {
        wait(NULL);
        proc_counter--;
    }
	
	return 0;
}

//Checking is file a directory or is a regular one.
void processAccordingToFileType(char *path, dev_t dev_id)
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
        processAccordingToFileType(pathName, dev_id);
        retval = readdir_r(dir, &entry, &entry_ptr);
        if (retval != 0)
    		generateError("reading directory");
    }

    if (closedir(dir) != 0)
    	generateError("closing directory");

    return 0;
}


//Checking file type and file processing.
int processFile(char* path)
{  
    pid_t pid;
    int sig;

    if (thereAreNoFreeProcesses())
    {
        wait(NULL);
        proc_counter--;
    }

    if ((pid = fork()) < 0) {
        generateError("process creating");
    }   
    else if (pid == 0) {
        countPeriods(path);
    }
    else if (pid > 0) 
    {
        proc_counter++;
    }
                 
    return 0;
}


//Check if the number of launched processes is less 
//than the number of processes entered by user.
int thereAreNoFreeProcesses()
{
    return (proc_counter >= user_proc_num);
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


void initializePeriodTable(char *path)
{
    memset((char*)prd_array, 0, sizeof(prd_array));
    
    for (int i = 0; i < 1 << BLOCK_SIZE; i++)
    {
        processValue(&(prd_array[i]), i);
    }
}


//************************************************Algorithm****************************************************************
//Group bits and add it to period set.
void processValue(prd_set *p_set, unsigned char val)
{
    char prev_bit_val = -1, cur_bit_val;
    int prd_len, shift;

    shift = BLOCK_SIZE - 1;
    prev_bit_val = ((1 << shift) & val) >> shift;
    shift--;
    prd_len = 1;

    while (shift >= 0)
    {
        cur_bit_val = ((1 << shift) & val) >> shift;

        if (cur_bit_val == prev_bit_val)
        {
            prd_len++;
        }
        else
        {
            if (p_set->count == 0)
                setEndPointValue(p_set, prev_bit_val, prd_len);
            else
                setValue(p_set, prev_bit_val, prd_len);

            prd_len = 1;
        }

        if (0 == shift)
        {
            setEndPointValue(p_set, cur_bit_val, prd_len);
        }

        prev_bit_val = cur_bit_val;
        shift--;
    }
}

//Add the first or the last group of bits to period set.
void setEndPointValue(prd_set *p_set, unsigned char prd_val, long prd_len)
{
    p_set->prd_table[p_set->count].val = prd_val;
    p_set->prd_table[p_set->count].len = prd_len;
    p_set->prd_table[p_set->count].count = 1;
    p_set->count++;

}

//Add groups of pits to period set.
void setValue(prd_set *p_set, unsigned char prd_val, long prd_len)
{
    for (int i = 1; i < p_set->count; i++)
    {
        if ((p_set->prd_table[i].val == prd_val) && (p_set->prd_table[i].len == prd_len))
        {
            p_set->prd_table[i].count++;
            return;
        }
    }

    p_set->prd_table[p_set->count].val = prd_val;
    p_set->prd_table[p_set->count].len = prd_len;
    p_set->prd_table[p_set->count].count = 1;
    p_set->count++;
}


void saveToFile(prd_set *p_set, char *path)
{
    FILE *file;
    size_t items_written, total_written;

    file = fopen(path, "w");
    if (NULL == file)
    {
        printf("error: opening file\n");
        exit(1);
    }

    //Saving results in file.
    total_written = 0;

    while (1)
    {
        items_written = fwrite(p_set + total_written, sizeof(prd_set), p_set->count - total_written, file);

        if (items_written <= 0)
            break;

        total_written += items_written;
    }

    fclose(file);
}


void readFromFile(unsigned char *buf, size_t bytes_to_read, FILE *file)
{
    size_t bytes_readen, total_readen;

    total_readen = 0;

    while (1)
    {
        bytes_readen = fread(buf + total_readen, sizeof(*buf), bytes_to_read - total_readen, file);

        if (bytes_readen <= 0)
            break;

        total_readen += bytes_readen;
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

//Check if terminal is not used by another process, block it and print results.
void returnResult(char *path, prd_list *p_list)
{
    int *block_id;
    int pid;
    
    if (NULL == (block_id = (int *)shmat(shmid, NULL, 0))) {       
        printf("error: sharing memory\n");     
        exit(1);     
    }

    pid = getpid();

    while (1)
    {
        if (*block_id == 0)
        {
            *block_id = pid;
            if (*block_id == pid)
            {
                printf("%s\n", path);
                for (int i = 0; i < p_list->count; i++)
                {
                    printf("val: %u  len: %ld  count: %ld\n", (unsigned char)p_list->prd_table[i].val, 
                                                                             p_list->prd_table[i].len, 
                                                                             p_list->prd_table[i].count);
                }
                printf("\n");
                *block_id = 0;
                break;    
            }
        }
        usleep(pid);
    }
}

//Main algorithm function. Open file, read it in buf, call processing data functions.
void countPeriods(char *path)
{
    FILE *file;
    long file_size;
    unsigned char *buf;
    prd_list *p_list;  

    file = fopen(path, "r");
    if (NULL == file)
    {
        printf("error: opening file\n");
        exit(1);
    }

    file_size = getFileSize(file);

    buf = (unsigned char*)calloc(file_size, sizeof(char));
    p_list = (prd_list *)calloc(1, sizeof(prd_list));
    p_list->size = BLOCK_SIZE;
    p_list->prd_table = (struct period *)calloc(BLOCK_SIZE, sizeof(struct period));

    readFromFile(buf, file_size, file);
    processData(p_list, buf, file_size);
    returnResult(path, p_list);

    fclose(file);
    free(p_list);
    exit(0);
}

//Adding new group to period list.
void addToPeriodList(prd_list *p_list, struct period *prd)
{
    for (int i = 0; i < p_list->count; i++)
    {
        if ((p_list->prd_table[i].val == prd->val) && (p_list->prd_table[i].len == prd->len))
        {
            p_list->prd_table[i].count += prd->count;
            return;
        }
    }
    if (p_list->count >= p_list->size - 1)
    {
        p_list->size += p_list->size / 2;
        p_list->prd_table = (struct period *)realloc(p_list->prd_table, p_list->size * sizeof(struct period));
        if (p_list == NULL)
        {
            printf("error allcating period_list\n");
            exit(1);
        }
    }
    
    p_list->prd_table[p_list->count].val = prd->val;
    p_list->prd_table[p_list->count].len = prd->len;
    p_list->prd_table[p_list->count].count = prd->count;
    p_list->count++;
}

//Next group of bits preview
long checkNext(unsigned char *data, unsigned char val, long data_size)
{
    long index = 0;
    prd_set *prd;

    while (index < data_size)
    {
        prd = &prd_array[(unsigned char)data[index]];

        if ((prd->prd_table[0].val == val) && (prd->count == 1)) {
            index++;
        }
        else
            break;
    }
    return index;
}

//Processing groups of bits and adding it to result table.
void processData(prd_list *p_list, unsigned char *data, long data_size)
{
    long index = 0, repeats = 0, prev_processed = 0, i;
    long prev_len = 0;
    unsigned char val;
    struct period prd;


    while (index < data_size)
    {
        val = prd_array[data[index]].prd_table[0].val;
        repeats = checkNext(&data[index], val, data_size - index - 1);
        prev_processed = 0;
        index += repeats;
        prd.val = val;
        prd.len = prev_len + (repeats) * BLOCK_SIZE;
        prd.count = 1;

        if (val == prd_array[data[index]].prd_table[0].val) {
            prd.len += prd_array[data[index]].prd_table[0].len;
            prev_processed = 1; 
        }

        addToPeriodList(p_list, &prd);     

        for (i = prev_processed; i < prd_array[data[index]].count - 1; i++)
        {
            addToPeriodList(p_list, &prd_array[data[index]].prd_table[i]);   
        }

        prev_len = 0;

        if (prd_array[data[index]].count > 1)
        {
            if ((index < data_size - 1) && 
                (prd_array[data[index]].prd_table[i].val == prd_array[data[index + 1]].prd_table[0].val)) {
                prev_len = prd_array[data[index]].prd_table[i].len;
            } else { 
                prd.len = prd_array[data[index]].prd_table[i].len;
                prd.val = prd_array[data[index]].prd_table[i].val;
                prd.count = 1;
                addToPeriodList(p_list, &prd);  
            }
        }
        index++;
    }
}
