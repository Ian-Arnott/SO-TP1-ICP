#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/select.h>

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        fprintf(stderr, "Uso: %s in_fd out_fd\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int in_fd = atoi(argv[1]);
    int out_fd = atoi(argv[2]);

    char path[4096];
    ssize_t bytes_read;

    while ((bytes_read = read(in_fd, path, sizeof(path))) > 0)
    {
        path[bytes_read - 1] = '\0';

        int pipe_fd[2];
        pipe(pipe_fd);

        pid_t md5sum_pid = fork();
        if (md5sum_pid == 0)
        {
            close(pipe_fd[0]);
            dup2(pipe_fd[1], STDOUT_FILENO);
            close(pipe_fd[1]);

            char *args[] = {"md5sum", path, NULL};
            execv("/usr/bin/md5sum", args);
            perror("execv");
            exit(EXIT_FAILURE);
        }
        else
        {
            close(pipe_fd[1]);
            int status;
            waitpid(md5sum_pid, &status, 0);

            if (WIFEXITED(status) && WEXITSTATUS(status) == 0) 
            {
                char md5_result[33];
                read(pipe_fd[0], md5_result, sizeof(md5_result));
                md5_result[32] = '\0';
                int result_size = strlen(path) + strlen(md5_result) + 1;
                char result[result_size];

                strcpy(result,md5_result);
                strcat(result,path);
                write(out_fd, result, strlen(result) + 1);
            }
            else
            {
                const char *error_msg = "Error al calcular el MD5\n";
                write(out_fd, error_msg, strlen(error_msg));
            }
            close(pipe_fd[0]);
        }
    }

    close(in_fd);
    close(out_fd);

    return 0;
}