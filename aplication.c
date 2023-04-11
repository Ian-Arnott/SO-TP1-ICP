#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <math.h>
#include <linux/limits.h>
#include <sys/select.h>

#define MAX_SLAVES 5
#define MIN(a, b) ((a) < (b) ? (a) : (b))

/*
typedef struct
{
	char *path;
	char *md5;
	int pid;
} MD5Result;

typedef struct
{
	MD5Result* buffer;
	int size;
	int status;
} MD5Buffer;*/

fd_set write_set;
fd_set read_set;

int fds_write[MAX_SLAVES];
int fds_read[MAX_SLAVES];
pid_t pids[MAX_SLAVES];
int path_remain;
int path_read;
int slaves_count;
int created_slave;
FILE *result;

void MD5Simulation(const char *path)
{
	// fd_set write_set_aux;
	// write_set_aux = write_set;

	// select(FD_SETSIZE, NULL, &write_set, NULL, NULL);

	int i;
	// for (i = 0; i < slaves_count; i++)
	// {
	// 	if(FD_ISSET(fds_write[i], &write_set))
	// 	{
	// 		write(fds_write[i], path, strlen(path)+1);
	// 		write_set = write_set_aux;
	// 		path_remain++;
	// 		break;
	// 	}
	// }

	fd_set read_set_aux;
	read_set_aux = read_set;

	if (path_remain > 0)
	{
		select(FD_SETSIZE, &read_set, NULL, NULL, NULL);

		for (i = 0; i < slaves_count; i++)
		{
			if (FD_ISSET(fds_read[i], &read_set))
			{
				printf("estoy libre pid: %d, fd: %d\n", pids[i], fds_read[i]);
				char md5_result[4096];
				ssize_t bytes_read = read(fds_read[i], md5_result, sizeof(md5_result));
				printf("bytes read: %zd\n", bytes_read);
				if (bytes_read > 0)
				{
					char md5[33];
					char path[300];
					strncpy(md5, md5_result, 32);
					md5[32] = 0;
					strcpy(path, md5_result + 32);
					printf("md5_result: %s\n", md5_result);
					fprintf(result, "Archivo: %s MD5: %s PID: %d\n", path, md5, pids[i]);
					path_remain--;
					path_read++;
				}
				if (path != NULL)
				{
					printf("%s\n",path);
					write(fds_write[i], path, strlen(path) + 1);
					path_remain++;
					break;
				}
			}
		}
		read_set = read_set_aux;
	}
}

void slave_dispatch(const char *path)
{
	// if (buff->size < MAX_SLAVES)
	if (slaves_count < MAX_SLAVES)
	{
		pid_t slave_pid;
		int pipes[4];
		pipe(&pipes[0]);
		pipe(&pipes[2]);

		if ((slave_pid = fork()) == 0)
		{
			close(pipes[1]);
			close(pipes[2]);
			char in_fd_str[10], out_fd_str[10];
			sprintf(in_fd_str, "%d", pipes[0]);
			sprintf(out_fd_str, "%d", pipes[3]);

			char *args[] = {"./slave", in_fd_str, out_fd_str, NULL};
			execv("./slave", args);
			perror("execv");
			exit(EXIT_FAILURE);
		}
		else
		{
			close(pipes[0]);
			close(pipes[3]);
			FD_SET(pipes[1], &write_set);
			FD_SET(pipes[2], &read_set);
			pids[slaves_count] = slave_pid;
			fds_write[slaves_count] = pipes[1];
			fds_read[slaves_count] = pipes[2];
			printf("%s\n",path);
			write(fds_write[slaves_count], path, strlen(path) + 1);
			path_remain++;
			slaves_count++;
			created_slave = 1;
			/*
			buff->buffer[buff->size].pid = slave_pid;
			buff->size++;*/
		}
	}
	else
		created_slave = 0;
}

void explore_paths(const char *path)
{
	struct stat path_stat;
	stat(path, &path_stat);

	if (S_ISDIR(path_stat.st_mode))
	{
		DIR *dir = opendir(path);
		if (!dir)
		{
			perror("opendir");
			return;
		}

		struct dirent *entry;
		while ((entry = readdir(dir)) != NULL)
		{
			if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
				continue;

			char new_path[PATH_MAX];
			snprintf(new_path, sizeof(new_path), "%s/%s", path, entry->d_name);
			explore_paths(new_path);
		}
		closedir(dir);
	}
	else
	{
		slave_dispatch(path);
	}

	if (!created_slave)
	{
		if(!S_ISDIR(path_stat.st_mode))
			MD5Simulation(path);
	}
}

int main(int argc, char const *argv[])
{
	/*
	MD5Buffer buffer;
	buffer.size = 0;
	buffer.buffer = NULL;*/
	slaves_count = 0;
	path_remain = 0;
	path_read = 0;
	created_slave = 0;
	FD_ZERO(&write_set);
	FD_ZERO(&read_set);

	result = fopen("result.txt", "a");

	int i;
	for (i = 1; i < argc; i++)
	{
		explore_paths(argv[i]);
	}
	while (path_remain > 0)
	{
		MD5Simulation(NULL);
	}

	for (i = 0; i < slaves_count; i++)
	{
		close(fds_read[i]);
	}

	printf("Path read %d\n", path_read);

	return 0;
}