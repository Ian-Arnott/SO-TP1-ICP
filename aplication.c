#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/select.h>
#include <linux/limits.h>
#include "shm_lib.h"

#define MAX_SLAVES 5
#define MIN(a, b) ((a) < (b) ? (a) : (b))

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
int path_remain;
int path_read;
int slaves_count;
int created_slave;
FILE *result;

void init_buffer(PathBuffer *buffer)
{
    buffer->paths = NULL;
    buffer->size = 0;
}

void resize_buffer(PathBuffer *buffer, int new_size)
{
    buffer->paths = realloc(buffer->paths, new_size * sizeof(char *));
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

            char new_path[PATH_MAX];
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

    if (path_read < path_remain)
    {
        select(FD_SETSIZE, &read_set, NULL, NULL, NULL);

        for (i = 0; i < slaves_count; i++)
        {
            if (FD_ISSET(fds_read[i], &read_set))
            {
                // printf("estoy libre pid: %d, fd: %d\n", pids[i], fds_read[i]);
                char md5_result[4096];
                ssize_t bytes_read = read(fds_read[i], md5_result, sizeof(md5_result));
                // printf("bytes read: %zd\n", bytes_read);
                if (bytes_read > 0)
                {
                    char md5[33];
                    char path[MAX_PATH];
                    strncpy(md5, md5_result, 32);
                    md5[32] = 0;
                    strcpy(path, md5_result + 32);
                    fprintf(result, "Archivo: %s MD5: %s PID: %d\n", path, md5, pids[i]);

                    // Semaphore and view implementation

                    strncpy(buffer[path_read].md5, md5, 32);
                    buffer[path_read].md5[32] = 0;
                    strcpy(buffer[path_read].path, path);
                    buffer[path_read].pid = pids[i];
                    // Finish semaphore use

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
    created_slave = 0;
    FD_ZERO(&write_set);
    FD_ZERO(&read_set);
    result = fopen("result.txt", "w");
    FILE * state = fopen("state.txt","w+");

    int i;

    for (i = 1; i < argc; i++)
    {
        explore_paths(argv[i], &buffer);
    }

    path_remain = buffer.size;

    // Build shared memory buffer

    const char *shm_name = "/shm1";
    const char *sem_name = "/sem1";
    size_t shm_size = path_remain * sizeof(resultType);

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

    printf("%d\n", path_remain);
    fflush(stdout);

    fprintf(state, "se hizo el PRINTF para PIPE\n");
    sleep(3);
    resultType *shared_data = (resultType *)addr;

    // Calculate MD5
    for (i = 0; i < MIN(path_remain, MAX_SLAVES); i++)
    {
        slave_dispatch(buffer.paths[i]);
    }

    while (path_read < path_remain)
    {
        if (i < path_remain)
        {
            MD5Simulation(buffer.paths[i++], shared_data);
        }
        else
        {
            MD5Simulation(NULL, shared_data);
        }

        sem_post(sem);
        fprintf(state, "POST SEMAFORO\t");
        // printf("DUP DI DUP -- doing stuff !\t");

    }

    // Clean up

    for (i = 0; i < slaves_count; i++)
    {
        close(fds_read[i]);
        close(fds_write[i]);
    }

    fclose(result);

    
    int sem_value;
    sem_getvalue(sem,&sem_value);
    fprintf(state, "\nVALOR SEMAFORO FINAL: %d de %d archivos totales\n", sem_value, path_remain);
    
    close_semaphore(sem);
    destroy_semaphore(sem_name);

    shm_unmap(addr, shm_size);
    shm_destroy(shm_name);

    free_paths(&buffer);

    return 0;
}