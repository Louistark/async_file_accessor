#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <pthread.h>
#include <stdint.h>
#include <unistd.h>

#define FILE_SIZE 1024
#define THREAD_POOL_SIZE 4
#define ASYNC_FILE_READ 1
#define ASYNC_FILE_WRITE 2

typedef struct {
    int type;
    off_t offset;
    size_t size;
    void* buffer;
} async_file_task_t;

typedef struct {
    async_file_task_t* tasks;
    int size;
    int capacity;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
} async_file_task_queue_t;

async_file_task_queue_t g_task_queue;

pthread_t g_threads[THREAD_POOL_SIZE];

int g_file_fd;
char* g_buffer;

pthread_mutex_t g_file_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t g_file_cond = PTHREAD_COND_INITIALIZER;
int g_file_done = 0;

void async_file_task_queue_init(async_file_task_queue_t* queue) {
    queue->size = 0;
    queue->capacity = 16;
    queue->tasks = (async_file_task_t*)malloc(sizeof(async_file_task_t) * queue->capacity);
    pthread_mutex_init(&queue->mutex, NULL);
    pthread_cond_init(&queue->cond, NULL);
}

void async_file_task_queue_push(async_file_task_queue_t* queue, async_file_task_t* task) {
    pthread_mutex_lock(&queue->mutex);
    if (queue->size == queue->capacity) {
        queue->capacity *= 2;
        queue->tasks = (async_file_task_t*)realloc(queue->tasks, sizeof(async_file_task_t) * queue->capacity);
    }
    queue->tasks[queue->size++] = *task;
    pthread_cond_signal(&queue->cond);
    pthread_mutex_unlock(&queue->mutex);
}

async_file_task_t async_file_task_queue_pop(async_file_task_queue_t* queue) {
    pthread_mutex_lock(&queue->mutex);
    while (queue->size == 0) {
        pthread_cond_wait(&queue->cond, &queue->mutex);
    }
    async_file_task_t task = queue->tasks[0];
    queue->size--;
    for (int i = 0; i < queue->size; i++) {
        queue->tasks[i] = queue->tasks[i + 1];
    }
    pthread_mutex_unlock(&queue->mutex);
    return task;
}

void async_file_read(void* arg) {
    while (1) {
        async_file_task_t task = async_file_task_queue_pop(&g_task_queue);
        if (task.type != ASYNC_FILE_READ) {
            continue;
        }
        uint8_t* buffer = (uint8_t*)task.buffer;
        off_t offset = task.offset;
        size_t size = task.size;
        ssize_t ret = pread(g_file_fd, buffer, size, offset);
        if (ret == -1) {
            perror("pread failed");
            exit(1);
        }
    }
}

void async_file_write(void* arg) {
    while (1) {
        async_file_task_t task = async_file_task_queue_pop(&g_task_queue);
        if (task.type != ASYNC_FILE_WRITE) {
            continue;
        }
        uint8_t* buffer = (uint8_t*)task.buffer;
        off_t offset = task.offset;
        size_t size = task.size;
        memcpy(g_buffer + offset, buffer, size);
        msync(g_buffer + offset, size, MS_SYNC);
    }
}

void async_file_init() {
    async_file_task_queue_init(&g_task_queue);

    for (int i = 0; i < THREAD_POOL_SIZE; i++) {
        pthread_create(&g_threads[i], NULL, async_file_read, NULL);
        pthread_create(&g_threads[i], NULL, async_file_write, NULL);
    }
}

void async_file_read_async(off_t offset, size_t size, void* buffer) {
    async_file_task_t task;
    task.type = ASYNC_FILE_READ;
    task.offset = offset;
    task.size = size;
    task.buffer = buffer;
    printf("async_file_read_async.\n");
    async_file_task_queue_push(&g_task_queue, &task);
}

void async_file_write_async(off_t offset, size_t size, void* buffer) {
    async_file_task_t task;
    task.type = ASYNC_FILE_WRITE;
    task.offset = offset;
    task.size = size;
    task.buffer = buffer;
    async_file_task_queue_push(&g_task_queue, &task);
}

int main() {

    // 打开文件
    printf("Open file.\n");
    g_file_fd = open("test.txt", O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
    abort();
    if (g_file_fd == -1) {
        perror("open failed");
        exit(1);
    }

    // 将文件扩展到指定大小
    printf("将文件扩展到指定大小.\n");
    int ret = ftruncate(g_file_fd, FILE_SIZE);
    if (ret == -1) {
        perror("ftruncate failed");
        exit(1);
    }

    // 将文件映射到内存中
    printf("将文件映射到内存中.\n");
    g_buffer = mmap(NULL, FILE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, g_file_fd, 0);
    if (g_buffer == MAP_FAILED) {
        perror("mmap failed");
        exit(1);
    }

    // 初始化异步文件读写线程池
    printf("初始化异步文件读写线程池.\n");
    async_file_init();

    // 异步读取文件
    printf("异步读取文件.\n");
    char read_buffer[FILE_SIZE];
    async_file_read_async(0, FILE_SIZE, read_buffer);

    // 等待异步读取完成
    printf("等待异步读取完成.\n");
    pthread_mutex_lock(&g_file_mutex);
    printf("1.\n");
    while (!g_file_done) {
        printf("2.\n");
        pthread_cond_wait(&g_file_cond, &g_file_mutex);
    }
    printf("3.\n");
    g_file_done = 0;
    printf("4.\n");
    pthread_mutex_unlock(&g_file_mutex);

    // 输出读取到的文件内容
    printf("输出读取到的文件内容.\n");
    printf("file content: %s\\n", read_buffer);

    // 异步写入文件
    printf("异步写入文件.\n");
    char write_buffer[] = "hello world";
    async_file_write_async(0, sizeof(write_buffer), write_buffer);

    // 等待异步写入完成
    printf("等待异步写入完成.\n");
    pthread_mutex_lock(&g_file_mutex);
    while (!g_file_done) {
        pthread_cond_wait(&g_file_cond, &g_file_mutex);
    }
    g_file_done = 0;
    pthread_mutex_unlock(&g_file_mutex);

    // 关闭文件
    printf("关闭文件.\n");
    close(g_file_fd);

    return 0;
}
