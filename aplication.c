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
    char **paths;
    int size;
} PathBuffer;

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
    resize_buffer(buffer, buffer->size + 1);
    buffer->paths[buffer->size - 1] = strdup(path);
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

void slave_dispatch(PathBuffer *buff)
{
    int num_slaves = MIN(MAX_SLAVES, buff->size);
    pid_t slave_pid[num_slaves];
    int pipes[num_slaves][4];
    int active_slaves = num_slaves;
    int next_path_index = 0;

    for (int i = 0; i < num_slaves; i++)
    {
        pipe(&pipes[i][0]);
        pipe(&pipes[i][2]);

        slave_pid[i] = fork();
        if (slave_pid[i] == 0)
        {
            close(pipes[i][0]);
            close(pipes[i][3]);
            char in_fd_str[10], out_fd_str[10];
            sprintf(in_fd_str, "%d", pipes[i][2]);
            sprintf(out_fd_str, "%d", pipes[i][1]);

            char *args[] = {"./esclavo", in_fd_str, out_fd_str, NULL};
            execv("./esclavo", args);
            perror("execv");
            exit(EXIT_FAILURE);
        }
    }

    printf("se crearon todos los esclavos\n");

    // Start by sending paths to slaves
    for (int i = 0; i < num_slaves && next_path_index < buff->size; i++)
    {
        write(pipes[i][3], buff->paths[next_path_index], strlen(buff->paths[next_path_index]) + 1);
        next_path_index++;
    }
    while (active_slaves > 0)
    {
        fd_set read_fds;
        FD_ZERO(&read_fds);
        int max_fd = -1;

        for (int i = 0; i < num_slaves; i++)
        {
            FD_SET(pipes[i][0], &read_fds);
            if (pipes[i][0] > max_fd)
            {
                max_fd = pipes[i][0];
            }
        }

        select(max_fd + 1, &read_fds, NULL, NULL, NULL);
        for (int i = 0; i < num_slaves; i++)
        {
            if (FD_ISSET(pipes[i][0], &read_fds))
            {
                char md5_result[34];
                ssize_t bytes_read = read(pipes[i][0], md5_result, sizeof(md5_result));
                if (bytes_read > 0)
                {
                    md5_result[33] = '\0';
                    printf("Archivo: %s MD5: %s\n", md5_result + 33 - bytes_read, md5_result);
                }
                else
                {
                    active_slaves--;
                }

                if (next_path_index < buff->size)
                {
                    write(pipes[i][3], buff->paths[next_path_index], strlen(buff->paths[next_path_index]) + 1);
                    next_path_index++;
                }
                else
                {
                    close(pipes[i][3]);
                }
            }
        }
    }

    for (int i = 0; i < num_slaves; i++)
    {
        close(pipes[i][0]);
        close(pipes[i][2]);
        close(pipes[i][3]);
    }
}

int main(int argc, char const *argv[])
{
    PathBuffer buffer;
    init_buffer(&buffer);
    for (int i = 1; i < argc; i++)
    {
        explore_paths(argv[i], &buffer);
    }
    slave_dispatch(&buffer);
    free_paths(&buffer);
    printf("termine!");
    return 0;
}