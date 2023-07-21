#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#define TASK_QUEUE_SIZE 10  // 任务队列大小
#define THREAD_POOL_SIZE 5  // 线程池大小
#define SENTINEL_TASK_NUM 5 // 哨兵任务数量

typedef struct task_t
{
    void *(*function)(void *); // 任务函数指针
    void *argument;            // 任务函数参数
    bool is_sentinel;          // 是否为哨兵任务
} task_t;

typedef struct task_queue_t
{
    task_t task_queue[TASK_QUEUE_SIZE]; // 任务队列
    int head;                           // 任务队列头指针
    int tail;                           // 任务队列尾指针
    int count;                          // 任务队列中任务数目
    pthread_mutex_t lock;               // 任务队列锁
    pthread_cond_t not_empty;           // 任务队列非空条件变量
    pthread_cond_t not_full;            // 任务队列未满条件变量
} task_queue_t;

typedef struct thread_pool_info_t
{
    int maxThreadIdx;     // 最大线程索引
    int busyThreadNum;    // 忙碌线程数目
    int idleThreadNum;    // 空闲线程数目
    int activeThreadNum;  // 活跃线程数目
    bool isRunning;       // 是否运行
    pthread_mutex_t lock; // 信息锁
} thread_pool_info_t;

typedef struct thread_pool_t
{
    pthread_t thread_pool[THREAD_POOL_SIZE]; // 线程池
    task_queue_t task_queue;                 // 任务队列
    thread_pool_info_t info;                 // 线程池信息
} thread_pool_t;

// 工作线程
void *worker_thread(void *arg)
{
    thread_pool_t  *thread_pool = (thread_pool_t *)arg;     // 获取线程池指针
    task_queue_t   *task_queue = &(thread_pool->task_queue); // 获取任务队列指针
    task_t          task;

    // 获取线程编号
    pthread_mutex_lock(&(thread_pool->info.lock));
    int idx = thread_pool->info.maxThreadIdx++;
    printf("worker_thread[%d]: 获取线程池指针，获取任务队列指针.\n", idx);
    pthread_mutex_unlock(&(thread_pool->info.lock));

    while (true)
    {
        pthread_mutex_lock(&(thread_pool->task_queue.lock)); // 加锁，保证线程安全
        while (thread_pool->task_queue.count == 0 && thread_pool->info.isRunning)
        { // 如果任务队列为空，则等待
            printf("worker_thread[%d]: 任务队列为空，则等待.\n", idx);
            pthread_cond_wait(&(thread_pool->task_queue.not_empty), &(thread_pool->task_queue.lock));
        }

        if (!thread_pool->info.isRunning && thread_pool->task_queue.count == 0 && !task.is_sentinel)
        { // 如果线程池已经结束，并且任务队列为空，且当前任务不是哨兵任务，则退出线程
            printf("worker_thread[%d]: 线程池已经结束，并且任务队列为空，且当前任务不是哨兵任务，则退出线程.\n", idx);
            pthread_mutex_unlock(&(thread_pool->task_queue.lock));
            break;
        }

        printf("worker_thread[%d]: 获取锁成功. count = %d, isRunning = %d.\n", idx, thread_pool->task_queue.count, thread_pool->info.isRunning);
        task.function = thread_pool->task_queue.task_queue[thread_pool->task_queue.head].function; // 获取任务函数指针
        task.argument = thread_pool->task_queue.task_queue[thread_pool->task_queue.head].argument; // 获取任务函数参数
        thread_pool->task_queue.head = (thread_pool->task_queue.head + 1) % TASK_QUEUE_SIZE;        // 更新任务队列头指针
        thread_pool->task_queue.count--;                                                            // 更新任务队列中任务数目
        pthread_cond_signal(&(thread_pool->task_queue.not_full));                                   // 通知任务队列未满
        pthread_mutex_unlock(&(thread_pool->task_queue.lock));                                      // 解锁

        if (task.is_sentinel)
        { // 如果当前任务是哨兵任务，则退出线程
            printf("worker_thread[%d]: 收到哨兵任务，线程退出.\n", idx);
            break;
        }

        pthread_mutex_lock(&(thread_pool->info.lock)); // 加锁，保证线程池信息的安全
        thread_pool->info.busyThreadNum++;             // 更新线程池信息中忙碌线程数目
        thread_pool->info.idleThreadNum--;             // 更新线程池信息中空闲线程数目
        thread_pool->info.activeThreadNum++;           // 更新线程池信息中活跃线程数目
        pthread_mutex_unlock(&(thread_pool->info.lock));

        printf("worker_thread[%d]: 执行任务.\n", idx);
        (*(task.function))(task.argument); // 执行任务

        pthread_mutex_lock(&(thread_pool->info.lock)); // 加锁，保证线程池信息的安全
        thread_pool->info.busyThreadNum--;             // 更新线程池信息中忙碌线程数目
        thread_pool->info.idleThreadNum++;             // 更新线程池信息中空闲线程数目
        thread_pool->info.activeThreadNum--;           // 更新线程池信息中活跃线程数目
        pthread_mutex_unlock(&(thread_pool->info.lock));
    }

    printf("worker_thread[%d]: 线程退出.\n", idx);
    pthread_exit(NULL); // 线程退出
}

// 初始化线程池
void thread_pool_init(thread_pool_t *thread_pool)
{
    int i;

    pthread_mutex_init(&(thread_pool->task_queue.lock), NULL);     // 初始化任务队列锁
    pthread_mutex_init(&(thread_pool->info.lock), NULL);            // 初始化任务队列锁
    pthread_cond_init(&(thread_pool->task_queue.not_empty), NULL); // 初始化任务队列非空条件变量
    pthread_cond_init(&(thread_pool->task_queue.not_full), NULL);  // 初始化任务队列未满条件变量
    thread_pool->task_queue.head = 0;                              // 初始化任务队列头指针
    thread_pool->task_queue.tail = 0;                              // 初始化任务队列尾指针
    thread_pool->task_queue.count = 0;                             // 初始化任务队列中任务数目
    thread_pool->info.maxThreadIdx = 0;
    thread_pool->info.busyThreadNum = 0;
    thread_pool->info.idleThreadNum = THREAD_POOL_SIZE;
    thread_pool->info.activeThreadNum = 0;
    thread_pool->info.isRunning = true;

    for (i = 0; i < THREAD_POOL_SIZE; i++)
    { // 创建线程池中的线程
        printf("thread_pool_init: 创建线程池中的线程: %d.\n", i);
        pthread_create(&(thread_pool->thread_pool[i]), NULL, worker_thread, thread_pool);
    }

}

// 向线程池中提交任务
void thread_pool_submit(thread_pool_t *thread_pool, void *(*function)(void *), void *argument)
{
    task_t task = {function, argument, false};

    pthread_mutex_lock(&(thread_pool->task_queue.lock)); // 加锁，保证线程安全
    while (thread_pool->task_queue.count == TASK_QUEUE_SIZE)
    { // 如果任务队列已满，则等待
        printf("thread_pool_submit: 如果任务队列已满，则等待.\n");
        pthread_cond_wait(&(thread_pool->task_queue.not_full), &(thread_pool->task_queue.lock));
    }

    thread_pool->task_queue.task_queue[thread_pool->task_queue.tail].function = function; // 将任务添加到任务队列中
    thread_pool->task_queue.task_queue[thread_pool->task_queue.tail].argument = argument;
    thread_pool->task_queue.task_queue[thread_pool->task_queue.tail].is_sentinel = false;
    thread_pool->task_queue.tail = (thread_pool->task_queue.tail + 1) % TASK_QUEUE_SIZE; // 更新任务队列尾指针
    thread_pool->task_queue.count++;                                                     // 更新任务队列中任务数目
    pthread_cond_signal(&(thread_pool->task_queue.not_empty));                           // 通知任务队列非空
    pthread_mutex_unlock(&(thread_pool->task_queue.lock));                                // 解锁
}

// 任务函数
void *task_function(void *arg)
{
    int *value = (int *)arg;
    printf("Task function executed with argument: %d\n", *value);
    free(value);
    return NULL;
}

int main()
{
    int i;
    int *value;

    printf("初始化线程池.\n");
    thread_pool_t thread_pool;
    thread_pool_init(&thread_pool); // 初始化线程池

    for (i = 0; i < TASK_QUEUE_SIZE; i++)
    { // 向线程池中提交任务
        printf("向线程池中提交任务: %d.\n", i);
        value = (int *)malloc(sizeof(int));
        *value = i;
        thread_pool_submit(&thread_pool, task_function, value);
    }

    thread_pool.info.isRunning = false;
    for (i = 0; i < SENTINEL_TASK_NUM; i++)
    { // 添加哨兵任务
        printf("向线程池中添加哨兵任务: %d.\n", i);
        task_t sentinel_task = {NULL, NULL, true};
        thread_pool_submit(&thread_pool, sentinel_task.function, sentinel_task.argument);
    }

    for (i = 0; i < THREAD_POOL_SIZE; i++)
    { // 等待所有工作线程结束
        printf("等待所有工作线程结束: %d.\n", i);
        pthread_join(thread_pool.thread_pool[i], NULL);
    }

    pthread_mutex_destroy(&(thread_pool.task_queue.lock));     // 销毁任务队列锁
    pthread_cond_destroy(&(thread_pool.task_queue.not_empty)); // 销毁任务队列非空条件变量
    pthread_cond_destroy(&(thread_pool.task_queue.not_full));  // 销毁任务队列未满条件变量

    return 0;
}
