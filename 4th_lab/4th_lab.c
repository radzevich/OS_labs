#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <netinet/in.h>
#include <time.h>


#define CHILD_NUM 8			//количество дочерних процессов
#define CHILD_ACTION_INITIALIZED 0
#define CONFIRMED 1
#define PIDS_FILE "pids.txt"
#define S0 0
#define S1 1
#define S2 2


int proc_id = 0;
const char *process_name;
int child_pids[CHILD_NUM];
int process_tree_initialized = 0;
long timer;
int main_counter = 0;


char signal_scheme[CHILD_NUM + 1][CHILD_NUM + 1] = {
	/* 	  	 0   1   2   3   4   5   6   7   8  */
	/*0*/	S0, S0, S0, S0, S0, S0, S0, S0, S0,  
	/*1*/   S0, S0, S2, S0, S0, S0, S0, S0, S0, 
	/*2*/   S0, S0, S0, S1, S1, S1, S0, S0, S0, 
	/*3*/	S0, S0, S0, S0, S0, S0, S0, S1, S0, 
	/*4*/   S0, S0, S0, S0, S0, S0, S1, S0, S0, 
	/*5*/   S0, S0, S0, S0, S0, S0, S0, S0, S1,
	/*6*/	S0, S0, S0, S0, S0, S0, S0, S0, S0,
	/*7*/	S0, S0, S0, S0, S0, S0, S0, S0, S0,
	/*8*/	S0, S2, S0, S0, S0, S0, S0, S0, S0,
};


int fork_with_action();
void print_error(const char *error_message);
pid_t create_process_tree();
void print_out(int proc_id, int pid, FILE *file);
void save_pid_to_file(char *path, int pid);
void read_from_file(char *path, int *buf, int buf_size);
int get_sender_id(int *child_pids, pid_t pid, int size);
void initialize_proc_pids(char *path);
void action(pid_t sender_pid, int signum);
int *get_proc_group(int *size);
int get_signal();
void output(int signum, int sender_id);
int set_signal_group();
void unset_signal_group();
int get_adress_pid();
void termination_handler(int signum);
long get_time();


void sighandler(int signum, siginfo_t *info, void *context)
{
	pid_t sender_pid;
	sigset_t mask, signal_set;
	int status;

    sigemptyset(&mask);
    //sigaddset(&mask, SIGTERM);
    sigaddset(&mask, SIGUSR1);
    sigaddset(&mask, SIGUSR2);
    sigprocmask(SIG_BLOCK, &mask, NULL);

	sender_pid = info->si_pid;
	//Если пришёл сигнал в первый процесс, то наращиваем счётчик.
	if (proc_id == 1)
	{
		main_counter++;
		if (101 <= main_counter)
		{
			kill(getppid(), SIGTERM);
			//termination_handler(SIGTERM);
			sleep(3);
			exit(0);
		}
	}

	action(sender_pid, signum);
}


void action(pid_t sender_pid, int signum)
{
	int sender_id, count, sig, status;
	int *group;

	switch (signum)
	{
		case SIGUSR1:
			sig = S1;
			break;
		case SIGUSR2:
			sig = S2;
			break;
	}

	//Проверка, успела ли произойти запись pid-ов процессов из файла в массив.
	if (process_tree_initialized == 0)
		initialize_proc_pids(PIDS_FILE);

	//Получение номера прцесса-отправителя сигнала.
	sender_id = get_sender_id(child_pids, sender_pid, CHILD_NUM);
	//Вывод сообщения о поступившем сигнале.
	output(sig, sender_id);
	//Посылка собственного сигнала дальше по дереву.
	kill(get_adress_pid(), get_signal());
}


void termination_handler(int signum)
{
	sigset_t signal_set;
	int sig;

    sigemptyset(&signal_set);
    sigaddset(&signal_set, SIGTERM);
    sigprocmask(SIG_BLOCK, &signal_set, NULL);

    sigemptyset(&signal_set);
    sigaddset(&signal_set, SIGCHLD);

    if (proc_id == 0)
    {
    	kill(child_pids[0], SIGTERM);
    	usleep(20000);
    }
    else if (proc_id == 1)
    {
    	kill(0, SIGTERM);
    	kill(-getpgid(child_pids[2]), SIGTERM);
    	usleep(10000);
    }

    kill(getppid(), SIGCHLD);
    printf("Proc %d terminated\n", proc_id);
    exit(0);
}

//Сообщение о сигнале.
void output(int signum, int sender_id)
{
	printf("%d  %d  %d  %d/%d  ", proc_id, getpid(), getppid(), sender_id, proc_id);
	switch (signum)
	{
		case S1:
			printf("SIGUSR1  ");
			break;
		case S2:
			printf("SIGUSR2  ");
			break;
	}
	printf("%ld\n", get_time() - timer);
}


long get_time() 
{
	struct timeval timer;
    gettimeofday(&timer, NULL);

    return timer.tv_usec/1000;
} 


int main(int argc, char const *argv[])
{
	pid_t pid, gpid;
	int sig;
	FILE *file;
	sigset_t signal_set;
	struct sigaction action;

    sigemptyset(&signal_set);
	
    process_name = argv[0];
    timer = get_time();
    
    if ((file = fopen(PIDS_FILE, "w")) != NULL)
    	fclose(file);

    pid = create_process_tree();
    initialize_proc_pids(PIDS_FILE);	
    action.sa_handler = termination_handler;
    sigaction(SIGTERM, &action, 0);

	//Посыл первого сигнала
	if (proc_id == 1)
	{
		set_signal_group();
		kill(get_adress_pid(), SIGUSR2);
	}

	while (1)
	{
		pause();
	}

	return 0;
}

//Создание дерева процессов
pid_t create_process_tree()
{
	int sig;
	pid_t pid;
	sigset_t signal_set;
	struct sigaction action;

    sigemptyset(&signal_set);
	sigaddset(&signal_set, SIGUSR1);  

	//Создаём 1-ый процесс.
	//fork_with_action создаёт процесс и устанавливает обработчик.
	pid = fork_with_action();

	if (pid == 0)
	{
		proc_id = 1;

		//Создаём 2, 3, 4 и 5 процессы.
		for (int i = 2; i <= 5; i++)
		{
			pid = fork_with_action();

			if (pid == 0) {
				proc_id = i;
				break;
			}
		}

		if (proc_id == 5)
		{
			//6, 7, 8 процессы
			for (int i = 6; i <= 8; i++)
			{
				pid = fork_with_action();

				if (pid == 0) {
					proc_id = i;
					break;
				}
			}
			if (proc_id == 5)
			{
				//Сигнал 1-му процессу о завершении создания дочерних.
				kill(getppid(), SIGUSR1);
			}
		}
		//1-ый процесс дожидается получения сигнала о том, что все дочерние процессы созданы.
		else if (proc_id == 1)
		{
			if (sigwait(&signal_set, &sig) > 0)
			{
				print_error("waiting for signal");
				exit (1);
			}
		}
	}	
	else
	{ 
		//Блокировка сигнала SIGUSR1.
		sigprocmask(SIG_BLOCK, &signal_set, NULL);
	}
	return pid;
}


int fork_with_action()
{
	int sig;
	pid_t pid, buf;
	FILE *file;
	sigset_t signal_set, mask;
	struct sigaction action;

	//Блокируем сигнал SIGUSR1, чтобы при его посылке дочерним процессом (ниже)
	//не произошёл вызов обработчика в родительском процессе.
	sigemptyset(&signal_set);
	sigaddset(&signal_set, SIGUSR1); 
    sigprocmask(SIG_BLOCK, &signal_set, NULL); 

	if ((pid = fork()) < 0)
	{
		print_error("process creating");
		return -1;
	}

	if (pid == 0)
	{
		//Добавляем pid процесса в файл.
		save_pid_to_file(PIDS_FILE, (int)getpid());

		//Устанавливаем обработчик.
		memset(&action, 0, sizeof(action)); 
  		action.sa_sigaction = sighandler; 
  		action.sa_flags = SA_SIGINFO; 
  		if ((sigaction(SIGUSR1, &action, NULL) != 0) & (sigaction(SIGUSR2, &action, NULL)) != 0)
  			print_error("action"); 
  		//Сигнал родительскому процессу о том, что дочерний создан и его обработчик сигналов установлен.
		kill(getppid(), SIGUSR1);
	}
	//Родительский процесс дожидается завершения создания дочернего.
	else
	{
		if (sigwait(&signal_set, &sig) > 0)
		{
			print_error("waiting for signal");
			exit (1);
		}
	}
	//Разблокировка сигнала SIGUSR1.
	sigprocmask(SIG_UNBLOCK, &signal_set, NULL); 
	return pid;
}

//Процессы 3, 4 и 5 объединяются в группу, чтобы их обработчики можно было вызвать
//однократны kill(-group_pid, signum).
int set_signal_group()
{
	if (setpgid(child_pids[2], child_pids[2]) != 0)
				printf("error\n");
	if (setpgid(child_pids[3], child_pids[2]) != 0)
				printf("error\n");
	if (setpgid(child_pids[4], child_pids[2]) != 0)
				printf("error\n");					
}

void unset_signal_group()
{
	if (setpgid(child_pids[1], child_pids[2]) != 0)
				printf("error1\n");
	if (setpgid(child_pids[2], child_pids[2]) != 0)
				printf("error2\n");
	if (setpgid(child_pids[3], child_pids[2]) != 0)
				printf("error3\n");
	if (setpgid(child_pids[4], child_pids[2]) != 0)
				printf("error4\n");
}

//Получение пидов процессов, которым нужно отослать сигнал.
int get_adress_pid()
{
	pid_t pid;
	int count = 0;

	//Все процессы, за исключением второго посылают сигнал лишь одному процессу,
	//поэтому просто возвращаем пид этого процесса.
	if (proc_id != 2)
	{
		for (int i = 1; i <= CHILD_NUM; i++)
		{
			if (signal_scheme[proc_id][i] != 0)
			{
				//Раскомментировать, если что-то не так - может поможет.
				/*if (process_tree_initialized == 0)
				{
					initialize_proc_pids(PIDS_FILE);
				}*/
				return child_pids[i - 1];
			}
		}
	}
	//Для 2-го возвращаем id группы {3, 4, 5}, которую мы создали ранее.
	else 
	{
		return -getpgid(child_pids[2]);
	}
}

//Добавление пида в конец файла.
//Т.к. при создании процессов родительский процесс не приступает к созданию нового дочернего,
//пока не закончилось создание предыдущего дочернего процесса, то пиды будут упорядочены по номерам процессов.
void save_pid_to_file(char *path, int pid)
{
	FILE *file;

	//Дописываем новый пид в конец файла.
	if ((file = fopen(PIDS_FILE, "ab")) != NULL) 
	{
		fwrite(&pid, sizeof(pid), 1, file);
		fflush(file);
		fclose(file);
	}
	else
	{
		print_error("opening file");
	}
}

//Читаем все пиды из буфера.
void initialize_proc_pids(char *path)
{
	int count = 0;
	FILE *file;

	while (count < CHILD_NUM)
	{
		count = 0;
		if ((file = fopen("pids.txt", "rb")) != NULL) 
		{
			for (int i = 0; i < CHILD_NUM; i++)
			{
				fread(&child_pids[i], sizeof(*child_pids), CHILD_NUM, file);
				//Если мы считали нулевое значение, значит какой-то процесс ещё не записал свой pid в файл,
				//а значит файл ещё не полон и нужно повторить чтение.
				if (child_pids[i] == 0)
					break;
				count++;
			}
	
			fclose(file);
		}
	}

	process_tree_initialized = 1;	
}

//Получение номера процесса по его pid-у.
int get_sender_id(int *child_pids, pid_t pid, int size)
{
	for (int i = 0; i < size; i++)
	{
		if (pid == child_pids[i])
			return i + 1;
	}
	return 0;
}

//Получение номера сигнала, который может отправить процесс.
int get_signal()
{
	int i;

	for (i = 1; i <= CHILD_NUM; i++)
	{
		if (signal_scheme[proc_id][i] != 0)
		{
			break;
		}
	}

	switch (signal_scheme[proc_id][i])
	{
		case S1:
			return SIGUSR1;
		case S2:
			return SIGUSR2;
	}
	return 0;
}

//Вывод ошибок.
void print_error(const char *error_message)
{
	printf("%s: %s: %s\n", process_name, error_message, strerror(errno));
}