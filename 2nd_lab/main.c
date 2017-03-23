#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BUF_SIZE 256

void generateError(char *err_message);


int main(int argc, char *argv[])
{
	char buf[BUF_SIZE];
	FILE *ptr;

	//Checking command line arguments num
	if (argc < 2)
		generateError("too few arguments");

	//printf("%s\n", argv[1]);

	char cmd[100] = " find ";
	strcat(cmd, strcat(argv[1], " -type f")); 
	ptr = popen(cmd, "r");

	if (NULL == ptr)
		generateError("file not exist");

	while (!feof(ptr))
	{
		fgets(buf, sizeof(buf), ptr);
		printf("%s", buf);
	}

	pclose(ptr);

	
	return 0;
}


//Generating error with error message
void generateError(char *err_message)
{
	printf("ERROR: %s\n", err_message);
	exit(1);
}
