#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>

#define TASK_QUEUE_SIZE 10              // 任务队列大小
#define THREAD_POOL_SIZE 5              // 线程池大小
#define SENTINEL_TASK_NUM 5             // 哨兵任务数量

typedef struct __task
{
    void *(*function)(void *);          // 任务函数指针
    void *argument;                     // 任务函数参数
    bool is_sentinel;                   // 是否为哨兵任务
} task_t;

typedef struct __task_queue
{
    task_t task_queue[TASK_QUEUE_SIZE]; // 任务队列
    int head;                           // 任务队列头指针
    int tail;                           // 任务队列尾指针
    int count;                          // 任务队列中任务数目
    pthread_mutex_t lock;               // 任务队列锁
    pthread_cond_t not_empty;           // 任务队列非空条件变量
    pthread_cond_t not_full;            // 任务队列未满条件变量
} task_queue_t;

typedef struct __thread_pool_info
{
    int maxThreadIdx;                   // 最大线程索引
    int busyThreadNum;                  // 忙碌线程数目
    int idleThreadNum;                  // 空闲线程数目
    int activeThreadNum;                // 活跃线程数目
    bool isRunning;                     // 是否运行
    pthread_mutex_t lock;               // 信息锁
} thread_pool_info_t;

typedef struct __thread_pool
{
    pthread_t thread_pool[THREAD_POOL_SIZE];    // 线程池
    task_queue_t task_queue;                    // 任务队列
    thread_pool_info_t info;                    // 线程池信息
} thread_pool_t;

// 工作线程函数
void *worker_thread(void *arg)
{
    // 获取线程池指针，获取任务队列指针
    thread_pool_t  *thread_pool = (thread_pool_t *)arg;
    task_queue_t   *task_queue = &(thread_pool->task_queue);
    task_t          task;

    // 获取线程编号，更新线程池信息中闲忙线程数目
    pthread_mutex_lock(&(thread_pool->info.lock));
    int idx = thread_pool->info.maxThreadIdx++;
    thread_pool->info.activeThreadNum++;
    thread_pool->info.idleThreadNum++;
    printf("worker_thread[%d]: 线程创建完成，开始接收任务.\n", idx);
    printf("-- 线程池状态：当前活跃线程数[%d]，繁忙线程数[%d]，空闲线程数[%d].\n",
           thread_pool->info.activeThreadNum, thread_pool->info.busyThreadNum, thread_pool->info.idleThreadNum);
    pthread_mutex_unlock(&(thread_pool->info.lock));

    // 工作线程开始处理任务
    while (true)
    {
        pthread_mutex_lock(&(task_queue->lock));
        printf("worker_thread[%d]: task_queue 加锁，尝试从队列中获取任务.\n", idx);
        while (task_queue->count == 0)
        {
            printf("worker_thread[%d]: task_queue 解锁，任务队列中没有任务，等待中...\n", idx);
            pthread_cond_wait(&(task_queue->not_empty), &(task_queue->lock));
            printf("worker_thread[%d]: task_queue 加锁，开始获取任务.\n", idx);
        }

        // 获取任务，更新任务队列头指针，更新任务队列中任务数目
        task.function       = task_queue->task_queue[task_queue->head].function;
        task.argument       = task_queue->task_queue[task_queue->head].argument;
        task.is_sentinel    = task_queue->task_queue[task_queue->head].is_sentinel;
        task_queue->head    = (task_queue->head + 1) % TASK_QUEUE_SIZE;
        task_queue->count--;

        printf("worker_thread[%d]: 通知任务队列未满.\n", idx);
        pthread_cond_signal(&(task_queue->not_full));
        printf("worker_thread[%d]: task_queue 解锁，取出任务成功，剩余任务数[%d].\n", idx, task_queue->count);
        pthread_mutex_unlock(&(task_queue->lock));

        // 线程运行状态出口，执行哨兵任务可以优雅地退出线程
        if (!thread_pool->info.isRunning && task.is_sentinel)
        {
            printf("worker_thread[%d]: 收到哨兵任务，线程退出.\n", idx);
            pthread_mutex_lock(&(thread_pool->info.lock));
            pthread_mutex_unlock(&(thread_pool->info.lock));
            break;
        }

        // 更新线程池信息中闲忙线程数目
        pthread_mutex_lock(&(thread_pool->info.lock));
        thread_pool->info.busyThreadNum++;
        thread_pool->info.idleThreadNum--;
        printf("-- 线程池状态：当前活跃线程数[%d]，繁忙线程数[%d]，空闲线程数[%d].\n",
               thread_pool->info.activeThreadNum, thread_pool->info.busyThreadNum, thread_pool->info.idleThreadNum);
        pthread_mutex_unlock(&(thread_pool->info.lock));

        printf("worker_thread[%d]: 开始执行任务.\n", idx);
        (*(task.function))(task.argument);

        // 更新线程池信息中闲忙线程数目
        pthread_mutex_lock(&(thread_pool->info.lock));
        thread_pool->info.busyThreadNum--;
        thread_pool->info.idleThreadNum++;
        printf("-- 线程池状态：当前活跃线程数[%d]，繁忙线程数[%d]，空闲线程数[%d].\n",
               thread_pool->info.activeThreadNum, thread_pool->info.busyThreadNum, thread_pool->info.idleThreadNum);
        pthread_mutex_unlock(&(thread_pool->info.lock));
    }

    // 工作线程退出
    printf("worker_thread[%d]: 线程退出.\n", idx);
    pthread_exit(NULL);
}

// 初始化线程池
void thread_pool_init(thread_pool_t *thread_pool)
{
    // 初始化线程池信息
    thread_pool->info.isRunning = true;
    thread_pool->task_queue.head = 0;
    thread_pool->task_queue.tail = 0;
    thread_pool->task_queue.count = 0;
    thread_pool->info.maxThreadIdx = 0;
    thread_pool->info.busyThreadNum = 0;
    thread_pool->info.idleThreadNum = 0;
    thread_pool->info.activeThreadNum = 0;
    pthread_mutex_init(&(thread_pool->info.lock), NULL);
    pthread_mutex_init(&(thread_pool->task_queue.lock), NULL);
    pthread_cond_init(&(thread_pool->task_queue.not_full), NULL);
    pthread_cond_init(&(thread_pool->task_queue.not_empty), NULL);
    printf("-- 线程池状态：当前活跃线程数[%d]，繁忙线程数[%d]，空闲线程数[%d].\n",
           thread_pool->info.activeThreadNum, thread_pool->info.busyThreadNum, thread_pool->info.idleThreadNum);

    // 创建线程池中的线程
    for (int i = 0; i < THREAD_POOL_SIZE; i++)
    {
        printf("thread_pool_init: 创建线程池中的线程: %d.\n", i);
        pthread_create(&(thread_pool->thread_pool[i]), NULL, worker_thread, thread_pool);
    }
}

// 向线程池中提交任务
void thread_pool_submit(thread_pool_t *thread_pool, task_t *task)
{
    // 获取任务队列指针
    task_queue_t *task_queue = &(thread_pool->task_queue);

    pthread_mutex_lock(&(task_queue->lock));
    printf("thread_pool_submit: task_queue 加锁，尝试向队列中添加任务.\n");
    while (task_queue->count == TASK_QUEUE_SIZE)
    {
        printf("thread_pool_submit: task_queue 解锁，任务队列已满，等待中...\n");
        pthread_cond_wait(&(task_queue->not_full), &(task_queue->lock));
        printf("thread_pool_submit: task_queue 加锁，添加任务.\n");
    }

    // 将任务添加到任务队列中，更新任务队列尾指针，更新任务队列中任务数目
    task_queue->task_queue[task_queue->tail].function = task->function;
    task_queue->task_queue[task_queue->tail].argument = task->argument;
    task_queue->task_queue[task_queue->tail].is_sentinel = task->is_sentinel;
    task_queue->tail = (task_queue->tail + 1) % TASK_QUEUE_SIZE;
    task_queue->count++;

    printf("thread_pool_submit: 通知任务队列非空.\n");
    pthread_cond_signal(&(task_queue->not_empty));
    printf("thread_pool_submit: task_queue 解锁，任务添加完成，剩余任务数[%d].\n", task_queue->count);
    pthread_mutex_unlock(&(task_queue->lock));
}

// 任务函数
void *task_function(void *arg)
{
    int *value = (int *)arg;
    printf("任务执行中 Task function executed with argument: %d\n", *value);
    free(value);
    return NULL;
}

int main()
{
    int i;
    int *value;
    task_t *task = (task_t *)malloc(sizeof(task_t));

    // 初始化线程池
    printf("初始化线程池.\n");
    thread_pool_t thread_pool;
    thread_pool_init(&thread_pool);

    // 向线程池中添加工作任务
    for (i = 0; i < TASK_QUEUE_SIZE; i++)
    {
        value = (int *)malloc(sizeof(int));
        *value = i;
        task->function = task_function;
        task->argument = value;
        task->is_sentinel = false;
        printf("向线程池中提交工作任务: %d.\n", i);
        thread_pool_submit(&thread_pool, task);
    }

    // 关闭线程池，不允许提交工作任务
    thread_pool.info.isRunning = false;
    printf("线程池即将关闭，不再接收工作任务...\n");

    // 向线程池中添加哨兵任务，通知线程退出
    for (i = 0; i < SENTINEL_TASK_NUM; i++)
    {
        task->function = NULL;
        task->argument = NULL;
        task->is_sentinel = true;
        printf("向线程池中提交哨兵任务: %d.\n", i);
        thread_pool_submit(&thread_pool, task);
    }

    // 等待所有工作线程结束
    for (i = 0; i < THREAD_POOL_SIZE; i++)
    {
        pthread_join(thread_pool.thread_pool[i], NULL);
        thread_pool.info.idleThreadNum--;
        thread_pool.info.activeThreadNum--;
        printf("工作线程[%d]已回收，剩余活跃线程数[%d]，繁忙线程数[%d]，空闲线程数[%d].\n",
               i, thread_pool.info.activeThreadNum, thread_pool.info.busyThreadNum, thread_pool.info.idleThreadNum);
    }

    // 销毁任务队列锁，销毁任务队列非空条件变量，销毁任务队列未满条件变量
    pthread_mutex_destroy(&(thread_pool.info.lock));
    pthread_mutex_destroy(&(thread_pool.task_queue.lock));
    pthread_cond_destroy(&(thread_pool.task_queue.not_full));
    pthread_cond_destroy(&(thread_pool.task_queue.not_empty));
    free(task);
    printf("线程池已销毁，程序退出.\n");

    return 0;
}
