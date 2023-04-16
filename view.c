#include "lib.h"

int main(int argc, char const *argv[])
{
    char shm_name[MAX_PATH];
    char sem_name[MAX_PATH];
    int task_remaining = -1;

    if (argc > 1 && argc != 4)
    {
        printf("Usage error: %s <shm_name> <sem_name> <task_amount>", argv[0]);
        exit(EXIT_FAILURE);
    }
    else if (argc == 4)
    {
        strcpy(shm_name,argv[1]);
        strcpy(sem_name,argv[2]);
        task_remaining = atoi(argv[3]);
        sleep(1);
    }
    else if (argc == 1)
    {
        char q_aux[16] = "";
        scanf("%s %s %s", shm_name, sem_name, q_aux);
        task_remaining = atoi(q_aux);
    }

    // Connect and map memory
    size_t shm_size = task_remaining * sizeof(resultType);

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

    // Connect to semaphore
    sem_t *sem = connect_semaphore(sem_name);
    if (sem == NULL)
    {
        shm_unmap(addr, shm_size);
        return 1;
    }

    resultType *shared_data = (resultType *)addr;

    // Print results
    int total_tasks = task_remaining;
    int i = 1;
    
    printf("<============================== RESULTADOS ==============================>\n");
    while (task_remaining > 0)
    {
        sem_wait(sem);
        printf("Archivo %d de %d: %s MD5: %s PID: %d\n", i++, total_tasks, shared_data[total_tasks - task_remaining].path, shared_data[total_tasks - task_remaining].md5, shared_data[total_tasks - task_remaining].pid);
        task_remaining--;
        sleep(1);
    }

    printf("\nTermine!\n");

    // Clean up
    close_semaphore(sem);
    shm_unmap(addr, shm_size);
    
    return 0;
}