#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BUF_SIZE 400
#define TRUE 1

void strRewrite(char *buf, char *str);
void generateError(char *err_message);
int countDirectrySize(FILE *path_list);


int main(int argc, char *argv[])
{
	FILE *ptr;

	//Checking command line arguments num.
	if (argc < 2)
		generateError("too few arguments");

	char cmd[BUF_SIZE] = " find ";
	strcat(cmd, strcat(argv[1], " -type f")); 
	ptr = popen(cmd, "r");

	printf("1\n");

	if (NULL == ptr)
		generateError("file not exist");

	printf("The total size is: %d bytes\n", countDirectrySize(ptr));
	pclose(ptr);
	
	return 0;
}


//Generating error with error message.
void generateError(char *err_message)
{
	fprintf(stderr, "ERROR: %s\n", err_message);
	exit(1);
}


//Countig total size of directry and it's subdirectries.
int countDirectrySize(FILE *path_list)
{
	int size;
	char buf[20];
	FILE *ptr;
	char cmd[BUF_SIZE];
	char path[BUF_SIZE];

	printf("2\n");

	while (TRUE)
	{
		if (fgets(path, BUF_SIZE, path_list) != NULL)
		{
			strRewrite(cmd, path);
			ptr = popen(cmd, "r");

			if (NULL == (ptr = popen(cmd, "r")))
				generateError("popen failed");	

			fgets(buf, sizeof(buf), ptr);	
			size += atoi(buf);
			pclose(ptr);
		}
		else
			break;
	}
	
	return size;
}


void strRewrite(char *buf, char *str)
{
	memset(buf, 0 , sizeof(buf));
	strcpy(buf, " stat -c%s ");
	strcat(buf, str);
}
