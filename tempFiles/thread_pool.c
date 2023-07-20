#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define THREAD_POOL_SIZE 5 // 线程池大小
#define TASK_QUEUE_SIZE 10 // 任务队列大小

// 任务结构体
typedef struct {
    void *(*function)(void *); // 任务函数指针
    void *argument; // 任务参数
} task_t;

// 任务队列结构体
typedef struct {
    task_t task_queue[TASK_QUEUE_SIZE]; // 任务队列
    int head; // 任务队列头指针
    int tail; // 任务队列尾指针
    int count; // 任务队列中任务数目
    pthread_mutex_t lock; // 任务队列锁
    pthread_cond_t not_empty; // 任务队列非空条件变量
    pthread_cond_t not_full; // 任务队列未满条件变量
} task_queue_t;

// 线程池结构体
typedef struct {
    pthread_t thread_pool[THREAD_POOL_SIZE]; // 线程池中的线程
    task_queue_t task_queue; // 线程池的任务队列
} thread_pool_t;

// 工作线程函数
void *worker_thread(void *arg) {
    thread_pool_t *thread_pool = (thread_pool_t *)arg; // 获取线程池指针
    task_queue_t *task_queue = &(thread_pool->task_queue); // 获取任务队列指针
    task_t task;

    while (1) {
        pthread_mutex_lock(&(task_queue->lock)); // 加锁，保证线程安全
        while (task_queue->count == 0) { // 如果任务队列中没有任务，则等待
            pthread_cond_wait(&(task_queue->not_empty), &(task_queue->lock));
        }

        task.function = task_queue->task_queue[task_queue->head].function; // 获取任务
        task.argument = task_queue->task_queue[task_queue->head].argument;
        task_queue->head = (task_queue->head + 1) % TASK_QUEUE_SIZE; // 更新任务队列头指针
        task_queue->count--; // 更新任务队列中任务数目

        pthread_cond_signal(&(task_queue->not_full)); // 通知任务队列未满
        pthread_mutex_unlock(&(task_queue->lock)); // 解锁

        (*(task.function))(task.argument); // 执行任务
    }

    pthread_exit(NULL); // 线程退出
}

// 初始化线程池
void thread_pool_init(thread_pool_t *thread_pool) {
    int i;

    pthread_mutex_init(&(thread_pool->task_queue.lock), NULL); // 初始化任务队列锁
    pthread_cond_init(&(thread_pool->task_queue.not_empty), NULL); // 初始化任务队列非空条件变量
    pthread_cond_init(&(thread_pool->task_queue.not_full), NULL); // 初始化任务队列未满条件变量
    thread_pool->task_queue.head = 0; // 初始化任务队列头指针
    thread_pool->task_queue.tail = 0; // 初始化任务队列尾指针
    thread_pool->task_queue.count = 0; // 初始化任务队列中任务数目

    for (i = 0; i < THREAD_POOL_SIZE; i++) { // 创建线程池中的线程
        pthread_create(&(thread_pool->thread_pool[i]), NULL, worker_thread, thread_pool);
    }
}

// 向线程池中提交任务
void thread_pool_submit(thread_pool_t *thread_pool, void *(*function)(void *), void *argument) {
    task_queue_t *task_queue = &(thread_pool->task_queue); // 获取任务队列指针
    task_t task;

    pthread_mutex_lock(&(task_queue->lock)); // 加锁，保证线程安全
    while (task_queue->count == TASK_QUEUE_SIZE) { // 如果任务队列已满，则等待
        pthread_cond_wait(&(task_queue->not_full), &(task_queue->lock));
    }

    task.function = function; // 创建任务
    task.argument = argument;
    task_queue->task_queue[task_queue->tail].function = function; // 将任务添加到任务队列中
    task_queue->task_queue[task_queue->tail].argument = argument;
    task_queue->tail = (task_queue->tail + 1) % TASK_QUEUE_SIZE; // 更新任务队列尾指针
    task_queue->count++; // 更新任务队列中任务数目

    pthread_cond_signal(&(task_queue->not_empty)); // 通知任务队列非空
    pthread_mutex_unlock(&(task_queue->lock)); // 解锁
}

// 任务函数
void *task_function(void *arg) {
    int *value = (int *)arg;
    printf("Task function executed with argument: %d\\n", *value);
    free(value);
    return NULL;
}

int main() {
    int i;
    int *value;

    thread_pool_t thread_pool;
    thread_pool_init(&thread_pool); // 初始化线程池

    for (i = 0; i < TASK_QUEUE_SIZE; i++) { // 向线程池中提交任务
        value = (int *)malloc(sizeof(int));
        *value = i;
        thread_pool_submit(&thread_pool, task_function, value);
    }

    while (1); // 主函数等待

    return 0;
}
