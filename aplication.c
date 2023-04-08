#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <dirent.h>
#include <sys/stat.h>
#include <math.h>
#include <sys/select.h>

#define MAX_SLAVES 5
#define MIN(a, b) ((a) < (b) ? (a) : (b))

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
} MD5Buffer;

fd_set write_set;
fd_set read_set;

int fds_write[MAX_SLAVES];
int fds_read[MAX_SLAVES];
pid_t pids[MAX_SLAVES];

void MD5Simulation(const char *path, MD5Buffer *buff) 
{
	fd_set write_set_aux;
	write_set_aux = write_set;
	fd_set read_set_aux;
	read_set_aux = read_set;

    select(FD_SETSIZE, NULL, &write_set, NULL, NULL);
	
	int i;
	for (i = 0; i < MAX_SLAVES; i++)
	{
		if(FD_ISSET(fds_write[i], &write_set))
		{
			write(fds_write[i], path, strlen(path)+1);
			printf("escribí\n");
		}
			
	}

	select(FD_SETSIZE, &read_set, NULL, NULL, NULL);

	for (i = 0; i < MAX_SLAVES; i++)
	{
		if(FD_ISSET(fds_read[i], &read_set)) 
		{
			printf("leí\n");
			char md5_result[4096];
			ssize_t bytes_read = read(fds_read[i], md5_result, sizeof(md5_result));
			printf("bytes read: %d\n", bytes_read);
			if (bytes_read > 0)
			{
				md5_result[33] = '\0';
				printf("Archivo: %s MD5: %s\n", md5_result + 33 - bytes_read, md5_result);
			}
		}
	}

	write_set = write_set_aux;
	read_set = read_set_aux;
}

void slave_dispatch(const char *path, MD5Buffer *buff)
{
	if (buff->size < MAX_SLAVES) 
	{
		pid_t slave_pid;
		int pipes[4];
		pipe(&pipes[0]);
		pipe(&pipes[2]);

		if ((slave_pid = fork()) == 0)
		{
			close(pipes[1]);
			close(pipes[3]);
			char in_fd_str[10], out_fd_str[10];
			sprintf(in_fd_str, "%d", pipes[0]);
			sprintf(out_fd_str, "%d", pipes[2]);

			char *args[] = {"./slave", in_fd_str, out_fd_str, NULL};
			execv("./slave", args);
			perror("execv");
			exit(EXIT_FAILURE);
		}
		else {
			close(pipes[0]);
			close(pipes[2]);
			FD_SET(pipes[1], &write_set);
			FD_SET(pipes[3], &read_set);
			fds_read[buff->size] = pipes[3];
			fds_write[buff->size] = pipes[1];
			pids[buff->size] = slave_pid;
			buff->size++;
		}
	}

	MD5Simulation(path, buff);
}

void explore_paths(const char *path, MD5Buffer *buff)
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
            explore_paths(new_path, buff);
        }
        closedir(dir);
    }
    else
    {
        slave_dispatch(path, buff);
    }
}

int main(int argc, char const *argv[])
{
    MD5Buffer buffer;
	buffer.size = 0;
	buffer.buffer = NULL;
	FD_ZERO(&write_set);
	FD_ZERO(&read_set);
	
	int i;
    for (i = 1; i < argc; i++)
    {
        explore_paths(argv[i], &buffer);
    }

    printf("Termine!");

    return 0;
}