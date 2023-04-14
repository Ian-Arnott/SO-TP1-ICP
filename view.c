#include "shm_lib.h"

int main(int argc, char const *argv[])
{
    char *shm_name = "/shm_buffer";
    char *sem_name = "/result_sem";
    int task_remaining = -1;

    if (argc > 2)
    {
        fprintf(stderr, "Usage: %s <task_count>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    else if (argc == 2)
    {
        task_remaining = atoi(argv[1]);
    }
    else if (argc == 1)
    {
        scanf("%10d", &task_remaining);
    }


    int shm_fd = shm_connect(shm_name);
    if (shm_fd == -1)
    {
        exit(EXIT_FAILURE);
    }

    size_t shm_size = task_remaining * sizeof(resultType);
    void *shm_address = shm_map(shm_fd, shm_size);
    if (shm_address == NULL)
    {
        exit(EXIT_FAILURE);
    }
    
    sem_t *sem = connect_semaphore(sem_name);
    if (sem == NULL)
    {
        shm_unmap(shm_address, shm_size);
        exit(EXIT_FAILURE);
    }

    resultType *shared_data = (resultType *) shm_address;
    int total_tasks = task_remaining;
    printf("<============================== RESULTS ==============================>\n");
    while ( task_remaining > 0)
    {
        sem_wait(sem);
        printf("Archivo: %s MD5: %s PID: %d\n",shared_data[total_tasks-task_remaining].path,shared_data[total_tasks-task_remaining].md5,shared_data[total_tasks-task_remaining].pid);
        task_remaining--;
        sem_post(sem);
    }

    printf("\nFinished! \n %d of %d tasks remain \n", task_remaining,total_tasks);

     // Clean up
    close_semaphore(sem);

    shm_unmap(shm_address, shm_size);

}