// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
#include "lib.h"

int shm_create(const char *name, size_t size) {
    int fd = shm_open(name, O_CREAT | O_RDWR | O_EXCL, S_IRUSR | S_IWUSR);
    if (fd == -1) {
        perror("shm_open");
        return -1;
    }

    if (ftruncate(fd, size) == -1) {
        perror("ftruncate");
        close(fd);
        return -1;
    }

    return fd;
}

int shm_connect(const char *name) {
    int fd = shm_open(name, O_RDWR, 0);
    if (fd == -1) {
        perror("shm_open");
        return -1;
    }

    return fd;
}

void *shm_map(int fd, size_t size) {
    void *addr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (addr == MAP_FAILED) {
        perror("mmap");
        close(fd);
        return NULL;
    }

    return addr;
}

int shm_unmap(void *addr, size_t size) {
    if (munmap(addr, size) == -1) {
        perror("munmap");
        return -1;
    }

    return 0;
}

int shm_destroy(const char *name) {
    if (shm_unlink(name) == -1) {
        perror("shm_unlink");
        return -1;
    }

    return 0;
}

sem_t *create_semaphore(const char *name, unsigned int value) {
    sem_t *sem = sem_open(name, O_CREAT | O_EXCL, S_IRUSR | S_IWUSR, value);
    if (sem == SEM_FAILED) {
        perror("sem_open");
        return NULL;
    }

    return sem;
}

sem_t *connect_semaphore(const char *name) {
    sem_t *sem = sem_open(name, 0);
    if (sem == SEM_FAILED) {
        perror("sem_open");
        return NULL;
    }

    return sem;
}

int close_semaphore(sem_t *sem) {
    if (sem_close(sem) == -1) {
        perror("sem_close");
        return -1;
    }

    return 0;
}

int destroy_semaphore(const char *name) {
    if (sem_unlink(name) == -1) {
        perror("sem_unlink");
        return -1;
    }

    return 0;
}
