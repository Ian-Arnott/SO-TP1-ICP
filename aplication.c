// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
#include "lib.h"

typedef struct
{
    char **paths;
    int size;
} PathBuffer;

fd_set write_set;
fd_set read_set;

int fds_write[MAX_SLAVES];
int fds_read[MAX_SLAVES];
pid_t pids[MAX_SLAVES];
int pipes[MAX_SLAVES][4];
int path_total;
int path_read;
int slaves_count;
int slaves_necessary;
FILE *result;

void init_buffer(PathBuffer *buffer)
{
    buffer->paths = NULL;
    buffer->size = 0;
}

void resize_buffer(PathBuffer *buffer, int new_size)
{
    buffer->paths = realloc(buffer->paths, new_size * sizeof(char *));
	if (buffer->paths == NULL)
		exit(EXIT_FAILURE);
    buffer->size = new_size;
}

void add_path(PathBuffer *buffer, const char *path)
{
    size_t len = strlen(path) + 1;
    char *new_path = malloc(len);
    if (new_path == NULL)
    {
        return;
    }
    strcpy(new_path, path);
    resize_buffer(buffer, buffer->size + 1);
    buffer->paths[buffer->size - 1] = new_path;
}

void explore_paths(const char *path, PathBuffer *buffer)
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

            char new_path[MAX_PATH];
            snprintf(new_path, sizeof(new_path), "%s/%s", path, entry->d_name);
            explore_paths(new_path, buffer);
        }
        closedir(dir);
    }
    else
    {
        add_path(buffer, path);
    }
}

void free_paths(PathBuffer *buff)
{
    for (int i = 0; i < buff->size; i++)
    {
        free(buff->paths[i]);
    }
    free(buff->paths);
}

void MD5Simulation(const char *path, resultType *buffer)
{
    int i;

    fd_set read_set_aux;
    read_set_aux = read_set;

    if (path_read < path_total)
    {
        select(FD_SETSIZE, &read_set, NULL, NULL, NULL);

        for (i = 0; i < slaves_count; i++)
        {
            if (FD_ISSET(fds_read[i], &read_set))
            {
                char md5_result[4096];
                ssize_t bytes_read = read(fds_read[i], md5_result, sizeof(md5_result));
                if (bytes_read > 0)
                {
                    strncpy(buffer[path_read].md5, md5_result, 32);
                    buffer[path_read].md5[32] = 0;
                    strcpy(buffer[path_read].path, md5_result + 32);
                    buffer[path_read].pid = pids[i];
                    fprintf(result, "Archivo: %s MD5: %s PID: %d\n", buffer[path_read].path, buffer[path_read].md5, pids[i]);

                    path_read++;
                }
                if (path != NULL)
                {
                    write(fds_write[i], path, strlen(path) + 1);
                    break;
                }
            }
        }
        read_set = read_set_aux;
    }
}

void slave_dispatch(char *path)
{
    if (slaves_count < MAX_SLAVES)
    {
        pid_t slave_pid;

        if ((slave_pid = fork()) == 0)
        {
			int j;
			for(j = slaves_necessary; j >= 0; j--){
				if(j!=slaves_count){
					close(pipes[j][0]);
					close(pipes[j][1]);
					close(pipes[j][2]);
					close(pipes[j][3]);
				}
			}
			close(pipes[slaves_count][1]);
            close(pipes[slaves_count][2]);
            
            char in_fd_str[10], out_fd_str[10];
            sprintf(in_fd_str, "%d", pipes[slaves_count][0]);
            sprintf(out_fd_str, "%d", pipes[slaves_count][3]);

            char *args[] = {"./slave", in_fd_str, out_fd_str, NULL};
            execv("./slave", args);
            perror("execv");
            exit(EXIT_FAILURE);
        }
        else
        {
            FD_SET(pipes[slaves_count][1], &write_set);
            FD_SET(pipes[slaves_count][2], &read_set);
            pids[slaves_count] = slave_pid;
            fds_write[slaves_count] = pipes[slaves_count][1];
            fds_read[slaves_count] = pipes[slaves_count][2];

            write(fds_write[slaves_count], path, strlen(path) + 1);
            slaves_count++;
        }
    }
}

int main(int argc, char const *argv[])
{
    PathBuffer buffer;
    init_buffer(&buffer);
    slaves_count = 0;
    path_read = 0;
    FD_ZERO(&write_set);
    FD_ZERO(&read_set);
    

    int i;
    for (i = 1; i < argc; i++)
    {
        explore_paths(argv[i], &buffer);
    }
    path_total = buffer.size;
	slaves_necessary = MIN(path_total, MAX_SLAVES);

    // Build shared memory buffer
    const char *shm_name = "/shm1";
    const char *sem_name = "/sem1";
    size_t shm_size = path_total * sizeof(resultType);

    int fd = shm_create(shm_name, shm_size);
    if (fd == -1)
    {
        return 1;
    }

    void *addr = shm_map(fd, shm_size);
    if (addr == NULL)
    {
        shm_destroy(shm_name);
        return 1;
    }

    // Create semaphore
    sem_t *sem = create_semaphore(sem_name, 0);
    if (sem == NULL)
    {
        shm_unmap(addr, shm_size);
        shm_destroy(shm_name);
        return 1;
    }

    printf("%s %s %d\n", shm_name, sem_name,path_total);
    fflush(stdout);

    sleep(2);
    resultType *shared_data = (resultType *)addr;

	//Create pipes
	for (i = 0; i < slaves_necessary; i++)
    {
		pipe(&pipes[i][0]);
        pipe(&pipes[i][2]);
    }


    // Dispatch slaves
    for (i = 0; i < MIN(path_total, MAX_SLAVES); i++)
    {
        slave_dispatch(buffer.paths[i]);
    }

	// Close parent unnecesary pipes
	for (i = 0; i < slaves_necessary; i++)
    {
        close(pipes[i][0]);
        close(pipes[i][3]);
    }

    // Calculate results
    result = fopen("result.txt", "w");

    while (path_read < path_total)
    {
        if (i < path_total)
        {
            MD5Simulation(buffer.paths[i++], shared_data);
        }
        else
        {
            MD5Simulation(NULL, shared_data);
        }

        // Post semaphore to update view
        sem_post(sem);
    }

    // Wait for connected view
    int sem_value;
    sem_getvalue(sem, &sem_value);

    if (sem_value != path_total)
    {
        while (sem_getvalue(sem, &sem_value) > 0){sleep(1);}
    }

    // Clean up
    for (i = 0; i < slaves_count; i++)
    {
        close(fds_read[i]);
        close(fds_write[i]);
    }

    fclose(result);

    close_semaphore(sem);
    destroy_semaphore(sem_name);

    shm_unmap(addr, shm_size);
    shm_destroy(shm_name);

    free_paths(&buffer);

    return 0;
}