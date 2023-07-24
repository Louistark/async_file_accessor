/***************************************************************************************
 * Project      : async_file_accessor
 * File         : mmap_file_accessor.c
 * Description  : Implement abstract async file accessor interface by mmap
 * Author       : Louis Liu
 * Created Date : 2023-7-13
 * Copyright (c) 2023, [Louis.Liu]
 * All rights reserved.
 ***************************************************************************************/

#include "mmap_file_accessor.h"
#include "async_file_accessor.h"
#include "common_types.h"

/// Timeout thread funtion
static void *timeout_thread(void *args)
{
    timeout_args_t *t_args = (timeout_args_t *)args;
    struct timespec timeout =
    {
        .tv_sec     = t_args->timeout_ms / 1000,
        .tv_nsec    = (t_args->timeout_ms % 1000) * 1000000,
    };

    if (0 == *(t_args->finished))
    {
        nanosleep(&timeout, NULL);
    }

    pthread_mutex_lock(t_args->lock);
    *(t_args->finished) = (0 == *(t_args->finished)) ? -1 : *(t_args->finished);
    pthread_cond_signal(t_args->cond);
    pthread_mutex_unlock(t_args->lock);

    pthread_exit(NULL);
}

/// Read request task process function
static void *mmapRead(void *param)
{
    mmap_request_t *pRequest    = (mmap_request_t *)param;
    void           *mmapAddr    = NULL;

    if (memcpy(pRequest->buf, mmapAddr, pRequest->nbytes) != NULL)
    {
        munmap(mmapAddr, pRequest->nbytes);
        pthread_mutex_lock(&(pRequest->lock));
        pRequest->status = REQUEST_STAT_IOSUCCESS;
        pthread_cond_signal(&(pRequest->isFinished));
        pthread_mutex_unlock(&(pRequest->lock));
    }
    else
    {
        pthread_mutex_lock(&(pRequest->lock));
        pRequest->status = REQUEST_STAT_IOFAIL;
        pthread_cond_signal(&(pRequest->isFinished));
        pthread_mutex_unlock(&(pRequest->lock));
        printf("ERROR: file [%s] read fail! error: %d - %s.\n", pRequest->parent.info.fn, errno, strerror(errno));
    }

    return NULL;
}

/// Write request task process function
static void *mmapWrite(void *param)
{
    mmap_request_t *pRequest    = (mmap_request_t *)param;
    void           *mmapAddr    = NULL;

    if (msync(pRequest->buf, pRequest->nbytes, MS_SYNC) != -1)
    {
        pthread_mutex_lock(&(pRequest->lock));
        pRequest->status = REQUEST_STAT_IOSUCCESS;
        pthread_cond_signal(&(pRequest->isFinished));
        pthread_mutex_unlock(&(pRequest->lock));
        munmap(pRequest->buf, pRequest->nbytes);
    }
    else
    {
        pthread_mutex_lock(&(pRequest->lock));
        pRequest->status = REQUEST_STAT_IOFAIL;
        pthread_cond_signal(&(pRequest->isFinished));
        pthread_mutex_unlock(&(pRequest->lock));
        printf("ERROR: file [%s] write fail! error: %d - %s.\n", pRequest->parent.info.fn, errno, strerror(errno));
    }

    return NULL;
}

/// Worker thread funtion
static void *worker_thread(void *arg)
{
    thread_pool_t  *thread_pool = (thread_pool_t *)arg;
    task_queue_t   *task_queue  = &(thread_pool->task_queue);
    task_t         *request_task;
    u32             idx;

    /// Acquire thread index
    pthread_mutex_lock(&(thread_pool->info.lock));
    idx = thread_pool->info.maxThreadIdx++;
    thread_pool->info.aliveThreadNum++;
    thread_pool->info.idleThreadNum++;
    printf("worker_thread[%d]: initialize done, start to process task.\n", idx);
    printf("-- Thread Pool State: aliveThreadNum[%d], busyThreadNum[%d], idleThreadNum[%d].\n",
           thread_pool->info.aliveThreadNum, thread_pool->info.busyThreadNum, thread_pool->info.idleThreadNum);
    pthread_mutex_unlock(&(thread_pool->info.lock));

    while (true)
    {
        /// Acquire request task
        pthread_mutex_lock(&(task_queue->lock));
        while (task_queue->todoCnt == 0)
        {
            pthread_cond_wait(&(task_queue->not_empty), &(task_queue->lock));
        }

        request_task = task_queue->request_queue[task_queue->head];
        task_queue->head++;
        task_queue->todoCnt--;
        printf("worker_thread[%d]: Acquire task success! Total[%d], Todo[%d], Processed[%d].\n",
               idx, task_queue->totoalCnt, task_queue->todoCnt, task_queue->totoalCnt - task_queue->todoCnt);
        pthread_mutex_unlock(&(task_queue->lock));

        /// If acquire sentinel task, thread exit.
        if (!thread_pool->info.isRunning && request_task->is_sentinel)
        {
            printf("worker_thread[%d]: Acquire sentinel task, exit.\n", idx);
            pthread_mutex_lock(&(thread_pool->info.lock));
            thread_pool->info.aliveThreadNum--;
            thread_pool->info.idleThreadNum--;
            pthread_mutex_unlock(&(thread_pool->info.lock));
            break;
        }

        pthread_mutex_lock(&(thread_pool->info.lock));
        thread_pool->info.busyThreadNum++;
        thread_pool->info.idleThreadNum--;
        printf("worker_thread[%d]: Acquire task success! Total[%d], Todo[%d], Processed[%d].\n",
               idx, task_queue->totoalCnt, task_queue->todoCnt, task_queue->totoalCnt - task_queue->todoCnt);
        pthread_mutex_unlock(&(thread_pool->info.lock));

        (*(request_task->function))(request_task->argument);

        pthread_mutex_lock(&(thread_pool->info.lock));
        thread_pool->info.busyThreadNum--;
        thread_pool->info.idleThreadNum++;
        printf("worker_thread[%d]: Acquire task success! Total[%d], Todo[%d], Processed[%d].\n",
               idx, task_queue->totoalCnt, task_queue->todoCnt, task_queue->totoalCnt - task_queue->todoCnt);
        pthread_mutex_unlock(&(thread_pool->info.lock));
    }

    printf("worker_thread[%d]: exit.\n", idx);
    pthread_exit(NULL);
}


// Initiaize thread pool
static void thread_pool_init(thread_pool_t *thread_pool)
{
    /// Initialize mutex and cond
    pthread_mutex_init(&(thread_pool->info.lock), NULL);
    pthread_mutex_init(&(thread_pool->task_queue.lock), NULL);
    pthread_cond_init(&(thread_pool->task_queue.not_empty), NULL);
    printf("-- Thread Pool Init State: aliveThreadNum[%d], busyThreadNum[%d], idleThreadNum[%d].\n",
           thread_pool->info.aliveThreadNum, thread_pool->info.busyThreadNum, thread_pool->info.idleThreadNum);

    /// Initialize request task queue
    thread_pool->task_queue.request_queue = (task_t **)malloc(sizeof(task_t *) * REQ_LIST_BUFSIZE);

    /// Create threads in thread pool
    for (int i = 0; i < THREAD_POOL_SIZE; i++)
    {
        printf("thread_pool_init: Create thread: %d.\n", i);
        pthread_create(&(thread_pool->thread_pool[i]), NULL, worker_thread, thread_pool);
    }
}

/// Submit request task to thread pool
static ret_t thread_pool_submit(thread_pool_t *thread_pool, task_t *task)
{
    ret_t           res         = RET_OK;
    task_queue_t   *task_queue  = &(thread_pool->task_queue);

    if ((thread_pool->info.isRunning && !task->is_sentinel) ||
        (!thread_pool->info.isRunning && task->is_sentinel))
    {
        pthread_mutex_lock(&(task_queue->lock));

        if (task_queue->totoalCnt % REQ_LIST_BUFSIZE == 0)
        {
            u32 newListBufSize = ((task_queue->totoalCnt + REQ_LIST_BUFSIZE) / REQ_LIST_BUFSIZE)
                                    * REQ_LIST_BUFSIZE * sizeof(task_t *);
            task_t **new_queue = (task_t **)realloc(task_queue->request_queue, newListBufSize);

            if (NULL == new_queue)
            {
                res = RET_NO_MEMORY;
                printf("ERROR: fail to realloc request list! res = %d.\n", res);
            }
            else
            {
                task_queue->request_queue = new_queue;
            }
        }

        task_queue->request_queue[task_queue->tail] = task;
        task_queue->tail++;
        task_queue->todoCnt++;
        task_queue->totoalCnt++;

        printf("Task submit success! Total[%d], Todo[%d], Processed[%d].\n",
            task_queue->totoalCnt, task_queue->todoCnt, task_queue->totoalCnt - task_queue->todoCnt);
        pthread_cond_signal(&(task_queue->not_empty));
        pthread_mutex_unlock(&(task_queue->lock));
    }
    else if(!thread_pool->info.isRunning && !task->is_sentinel)
    {
        res = RET_ALREADY_EXISTS;
        printf("Warning: thread pool is closing, task rejected!\n");
    }

    return res;
}

/// Ckeck whether mmap request valid
static ret_t mmap_check_request_valid(mmap_request_t *pRequest)
{
    ret_t res = RET_OK;



    return res;
}

/// Get mmap request
static ret_t mmap_get_request(async_file_accessor_t            *thiz,
                              async_file_access_request_t     **pAsyncRequest,
                              async_file_access_request_info_t *pCreateInfo)
{
    mmap_file_accessor_t *pMmapAccessor = (mmap_file_accessor_t *)thiz;
    mmap_request_t      **pRequest      = (mmap_request_t **)pAsyncRequest;

    ret_t   res         = RET_OK;
    u32     retry_times = 0;
    *pRequest = (mmap_request_t *)malloc(sizeof(mmap_request_t));
    memset(*pRequest, 0, sizeof(mmap_request_t));

    (*pRequest)->parent.info.direction  = pCreateInfo->direction;
    (*pRequest)->parent.info.size       = pCreateInfo->size;
    (*pRequest)->parent.info.offset     = pCreateInfo->offset;
    memcpy((*pRequest)->parent.info.fn, pCreateInfo->fn, MAX_FILE_NAME_LEN);
    res = mmap_check_request_valid(*pRequest);

    if (RET_OK == res)
    {
        do {
            (*pRequest)->fd = pCreateInfo->direction == ASYNC_FILE_ACCESS_READ
                              ? open((char8 *)(pCreateInfo->fn), O_RDONLY, 0666)
                              : open((char8 *)(pCreateInfo->fn), O_WRONLY | O_CREAT | O_TRUNC, 0666);
        }
        while ((*pRequest)->fd==-1 && retry_times++ < MAX_RETRY_TIMES);

        if ((*pRequest)->fd >= 0)
        {
            (*pRequest)->buf            = NULL;
            (*pRequest)->isAlloced      = FALSE;
            (*pRequest)->status         = REQUEST_STAT_INIT;
            (*pRequest)->nbytes         = pCreateInfo->size;
            (*pRequest)->offset         = lseek((*pRequest)->fd, pCreateInfo->offset, SEEK_CUR);

            if ((*pRequest)->parent.info.direction == ASYNC_FILE_ACCESS_WRITE)
            {
                ftruncate((*pRequest)->fd, (*pRequest)->nbytes);
            }
            fstat((*pRequest)->fd, &(*pRequest)->fsb);
        }
        else
        {
            res = RET_BAD_VALUE;
            printf("ERROR: file [%s] open fail! error: %d - %s.\n",
                   (*pRequest)->parent.info.fn, errno, strerror(errno));
        }

    }

    printf("file = %s: req_addr = %p.\n", (*pRequest)->parent.info.fn, (*pRequest));

    return res;
}

/// Alloc mmap write request buffer
static ret_t mmap_request_alloc_write_buffer(async_file_accessor_t       *thiz,
                                             async_file_access_request_t *pAsyncRequest,
                                             void                       **buffer)
{
    mmap_file_accessor_t *pMmapAccessor = (mmap_file_accessor_t *)thiz;
    mmap_request_t       *pRequest      = (mmap_request_t *)pAsyncRequest;

    u32     retry_times = 0;
    ret_t   res         = mmap_check_request_valid(pRequest);

    if (pRequest->nbytes <= 0)
    {
        res = RET_BAD_VALUE;
        pRequest->status = REQUEST_STAT_IOFAIL;
        printf("ERROR: invalid malloc buffer size! res = %d.\n", res);
    }

    if (RET_OK == res)
    {
        do {
            (*buffer) = mmap(NULL, pRequest->nbytes, PROT_READ | PROT_WRITE,
                                MAP_SHARED, pRequest->fd, pRequest->nbytes);
        }
        while (MAP_FAILED == (*buffer) && retry_times++ < MAX_RETRY_TIMES);

        if (MAP_FAILED != (*buffer))
        {
            pRequest->buf       = *buffer;
            pRequest->isAlloced = TRUE;
            printf("file = %s: req_addr = %p, buf_addr = %p.\n",
                    pRequest->parent.info.fn, pRequest, pRequest->buf);
        }
        else
        {
            res = RET_BAD_VALUE;
            pRequest->status = REQUEST_STAT_IOFAIL;
            printf("ERROR: file [%s] write buffer alloc fail! error: %d - %s.\n",
                    pRequest->parent.info.fn, errno, strerror(errno));
        }
    }

    return res;
}

/// Import mmap read only request buffer
static ret_t mmap_request_import_read_buffer(async_file_accessor_t       *thiz,
                                             async_file_access_request_t *pAsyncRequest,
                                             void                        *buffer)
{
    mmap_file_accessor_t *pMmapAccessor = (mmap_file_accessor_t *)thiz;
    mmap_request_t       *pRequest      = (mmap_request_t *)pAsyncRequest;

    u32     retry_times = 0;
    ret_t   res         = mmap_check_request_valid(pRequest);

    if (NULL == buffer)
    {
        res = RET_BAD_VALUE;
        pRequest->status = REQUEST_STAT_IOFAIL;
        printf("ERROR: invalid import buffer empty! res = %d.\n", res);
    }

    if (RET_OK == res)
    {
        do {
            buffer = mmap(NULL, pRequest->nbytes, PROT_READ, MAP_PRIVATE, pRequest->fd, pRequest->offset);
        }
        while (MAP_FAILED == buffer && retry_times++ < MAX_RETRY_TIMES);

        if (MAP_FAILED != buffer)
        {
            pRequest->buf = buffer;
            printf("file = %s: req_addr = %p, buf_addr = %p.\n",
                    pRequest->parent.info.fn, pRequest, pRequest->buf);
        }
        else
        {
            res = RET_BAD_VALUE;
            pRequest->status = REQUEST_STAT_IOFAIL;
            printf("ERROR: file [%s] read buffer import fail! error: %d - %s.\n",
                   pRequest->parent.info.fn, errno, strerror(errno));
        }
    }

    return res;
}

/// Put mmap request
static ret_t mmap_put_request(async_file_accessor_t       *thiz,
                              async_file_access_request_t *pAsyncRequest)
{
    mmap_file_accessor_t *pMmapAccessor = (mmap_file_accessor_t *)thiz;
    mmap_request_t       *pRequest      = (mmap_request_t *)pAsyncRequest;

    ret_t res = mmap_check_request_valid(pRequest);

    if (RET_OK == res)
    {
        task_t *pRequestTask        = (task_t *)malloc(sizeof(task_t));
        pRequestTask->is_sentinel   = false;
        pRequestTask->argument      = pRequest;
        pRequestTask->function      = ASYNC_FILE_ACCESS_WRITE == pRequest->parent.info.direction
                                      ? mmapWrite : mmapRead;

        res = thread_pool_submit(&(pMmapAccessor->distributor), pRequestTask);
        if (res != RET_OK)
        {
            if (TRUE == pRequest->isAlloced)
            {
                free((void*)pRequest->buf);
                pRequest->buf           = NULL;
            }
            printf("ERROR: failed to initiate the async IO operation! error: %d - %s.\n", errno, strerror(errno));
        }
        pRequest->status = REQUEST_STAT_SUBMITTED;
    }

    return res;
}

/// Wait for an mmap request finish
static ret_t mmap_wait_request(async_file_accessor_t       *thiz,
                               async_file_access_request_t *pAsyncRequest,
                               u32                          timeout_ms)
{
    mmap_file_accessor_t *pMmapAccessor = (mmap_file_accessor_t *)thiz;
    mmap_request_t       *pRequest      = (mmap_request_t *)pAsyncRequest;

    ret_t res = mmap_check_request_valid(pRequest);

    if (RET_OK != res                           ||
        pRequest->status <= REQUEST_STAT_INIT   ||
        pRequest->status >= REQUEST_STAT_CANCEL)
    {
        res = RET_INVALID_OPERATION;
        printf("Error: cannot wait an invalid request! res = %d.\n", res);
    }
    else if (RET_OK == res && 0 == pRequest->accessDone)
    {
        if (timeout_ms > 0)
        {
            pthread_t timeoutThread;
            timeout_args_t t_args =
            {
                .timeout_ms = timeout_ms,
                .finished   = &(pRequest->accessDone),
                .lock       = &(pRequest->lock),
                .cond       = &(pRequest->isFinished),
            };
            pthread_create(&timeoutThread, NULL, timeout_thread, (void*)&t_args);
            while (0 == pRequest->accessDone)
            {
                pthread_cond_wait(&(pRequest->isFinished), &(pRequest->lock));
            }
            pthread_cancel(timeoutThread);
            pthread_join(timeoutThread, NULL);
        }
        else
        {
            while (0 == pRequest->accessDone)
            {
                pthread_cond_wait(&(pRequest->isFinished), &(pRequest->lock));
            }
        }
    }

    return res;
}

/// Cancel an mmap request
static ret_t mmap_cancel_request(async_file_accessor_t       *thiz,
                                 async_file_access_request_t *pAsyncRequest)
{
    mmap_file_accessor_t *pMmapAccessor = (mmap_file_accessor_t *)thiz;
    mmap_request_t       *pRequest      = (mmap_request_t *)pAsyncRequest;

    ret_t res = mmap_check_request_valid(pRequest);

    if (RET_OK == res && pRequest->accessDone)
    {
        printf("Warning: cannot cancel a finished request!\n");
    }
    else if (RET_OK == res)
    {
    }

    return res;
}

/// Wait for all mmap request finish, after this func release all requests
static ret_t mmap_wait_all_request(async_file_accessor_t *thiz,
                                  u32                    timeout_ms)
{
    mmap_file_accessor_t *pMmapAccessor = (mmap_file_accessor_t *)thiz;

    ret_t res = RET_OK;

    if (NULL == pMmapAccessor || NULL == pMmapAccessor->req_list || 0 == pMmapAccessor->req_count)
    {
        res = RET_BAD_VALUE;
        printf("ERROR: invalid mmap file accessor! res = %d.\n", res);
    }
    else
    {
    }

    return res;
}

/// Cancel all mmap request, after this func release all requests
static ret_t mmap_cancel_all_requests(async_file_accessor_t *thiz)
{
    mmap_file_accessor_t *pMmapAccessor = (mmap_file_accessor_t *)thiz;

    ret_t res = RET_OK;

    if (NULL == pMmapAccessor || NULL == pMmapAccessor->req_list || 0 == pMmapAccessor->req_count)
    {
        res = RET_BAD_VALUE;
        printf("ERROR: invalid mmap file accessor! res = %d.\n", res);
    }
    else
    {
    }

    return res;
}

// Cancel all MMAP operations and release resources
static ret_t mmap_release_all_resources(async_file_accessor_t *thiz)
{
    mmap_file_accessor_t *pMmapAccessor = (mmap_file_accessor_t *)thiz;

    ret_t res = RET_OK;

    if (NULL == pMmapAccessor || NULL == pMmapAccessor->req_list || 0 == pMmapAccessor->req_count)
    {
        res = RET_BAD_VALUE;
        printf("ERROR: invalid mmap file accessor! res = %d.\n", res);
    }
    else
    {
    }

    return res;
}

/// Singleton static mmap accessor
static mmap_file_accessor_t g_mmapFileAccessor =
{
    .parent =
    {
        .type               = ASYNC_FILE_ACCESSOR_MMAP,

        .getRequest         = mmap_get_request,
        .allocWriteBuf      = mmap_request_alloc_write_buffer,
        .importReadBuf      = mmap_request_import_read_buffer,
        .putRequest         = mmap_put_request,
        .waitRequest        = mmap_wait_request,
        .cancelRequest      = mmap_cancel_request,
        .waitAll            = mmap_wait_all_request,
        .cancelAll          = mmap_cancel_all_requests,
        .releaseAll         = mmap_release_all_resources,
    },

    .distributor =
    {
        .info =
        {
            .maxThreadIdx   = 0,
            .busyThreadNum  = 0,
            .idleThreadNum  = 0,
            .aliveThreadNum = 0,
            .isRunning      = false,
        },
        .task_queue =
        {
            .request_queue  = NULL,
            .head           = 0,
            .tail           = 0,
            .todoCnt        = 0,
            .queueSize      = 0,
        },
    },
};

/// Acqiure single static mmap accessor
mmap_file_accessor_t* MMAP_File_Accessor_Get_Instance()
{
    return &g_mmapFileAccessor;
}
