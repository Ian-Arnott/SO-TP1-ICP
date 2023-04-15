#include "shm_lib.h"
#include <time.h>

int main(int argc, char const *argv[])
{
    const char *shm_name = "/shm1";
    const char *sem_name = "/sem1";
    int task_remaining = -1;

    if (argc > 2)
    {
        printf("Usage error: %s <task_amount>", argv[0]);
        exit(EXIT_FAILURE);
    }
    else if (argc == 2)
    {
        task_remaining = atoi(argv[1]);
        sleep(1);
    }
    else if (argc == 1)
    {
        char aux[128] = "";
        printf("about to wait!\n");
        // scanf("%s\n",&task_remaining);
        scanf("%s",aux);
        task_remaining = atoi(aux);
        printf("waited!!\n");
    }

    // printf("----lectura----\n%d\n--------------\n", task_remaining);

    size_t shm_size = task_remaining * sizeof(resultType);

    // Wait for process 1 to create the shared memory object and semaphore

    int fd = shm_connect(shm_name);
    if (fd == -1)
    {
        return 1;
    }

    void *addr = shm_map(fd, shm_size);
    if (addr == NULL)
    {
        return 1;
    }

    sem_t *sem = connect_semaphore(sem_name);
    if (sem == NULL)
    {
        shm_unmap(addr, shm_size);
        return 1;
    }

    // Use shared memory and semaphores here
    resultType *shared_data = (resultType *)addr;

    int total_tasks = task_remaining;
    int i = 1;
    
    printf("<============================== RESULTADOS ==============================>\n");
    while (task_remaining > 0)
    {
        sem_wait(sem);
        printf("Archivo %d de %d: %s MD5: %s PID: %d\n", i++, total_tasks, shared_data[total_tasks - task_remaining].path, shared_data[total_tasks - task_remaining].md5, shared_data[total_tasks - task_remaining].pid);
        task_remaining--;
        
    }

    printf("\nTermine!\n");
    // Clean up
    close_semaphore(sem);

    shm_unmap(addr, shm_size);
    return 0;
}