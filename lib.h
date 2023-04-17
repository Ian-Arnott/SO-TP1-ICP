#ifndef LIB_H
#define LIB_H

// Library for SHM, semaphore and APP and VIEW definitions

#include <stddef.h>
#include <semaphore.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/select.h>
#include <errno.h>


#define MAX_SLAVES 5
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX_PATH 128
#define MD5_SIZE 33 // 32 characthers + '\0'

// Result buffer structure
typedef struct resultType
{
char path[MAX_PATH];
char md5[MD5_SIZE];
int pid;
}resultType;

int ftruncate(int fd, off_t length);

int shm_create(const char *name, size_t size);
int shm_connect(const char *name);
void *shm_map(int fd, size_t size);
int shm_unmap(void *addr, size_t size);
int shm_destroy(const char *name);

sem_t *create_semaphore(const char *name, unsigned int value);
sem_t *connect_semaphore(const char *name);
int close_semaphore(sem_t *sem);
int destroy_semaphore(const char *name);

#endif
