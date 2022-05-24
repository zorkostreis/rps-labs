#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

int main(int argc, char** argv)
{
	if (argc < 3)
	{
		printf("Unacceptable number of arguments\n");
		printf("First - old_folder(input); second - new_folder(output)\n");
		exit(1);
	}
	int fds[2];
	pipe(fds); // Создание канала
	if (fork()) // Создание процесса-потомка
	{
		dup2(fds[1], 1);
		close(fds[0]);
		close(fds[1]);
		execl("/bin/tar", "tar", "cf", "-", argv[1], NULL);
	}
	else
	{
		dup2(fds[0], 0);
		close(fds[0]);
		close(fds[1]);
		execl("/bin/tar", "tar", "xfC", "-", argv[2], NULL);
	}
	return 0;
}