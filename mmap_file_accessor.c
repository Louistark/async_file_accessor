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
        printf("Warning: thread pool is closing, task rejected!\n");
    }

    return res;
}


// Called when MMAP operation is done, check result and free control block
static void mmap_callback(sigval_t sv)
{
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

    ret_t res = RET_OK;
    *pRequest = (mmap_request_t *)malloc(sizeof(mmap_request_t));
    memset(*pRequest, 0, sizeof(mmap_request_t));

    (*pRequest)->parent.info.direction  = pCreateInfo->direction;
    (*pRequest)->parent.info.size       = pCreateInfo->size;
    (*pRequest)->parent.info.offset     = pCreateInfo->offset;
    memcpy((*pRequest)->parent.info.fn, pCreateInfo->fn, MAX_FILE_NAME_LEN);
    res = mmap_check_request_valid(*pRequest);

    if (RET_OK == res)
    {
        (*pRequest)->fd = pCreateInfo->direction == ASYNC_FILE_ACCESS_READ
                              ? open((char8 *)(pCreateInfo->fn), O_RDONLY, 0666)
                              : open((char8 *)(pCreateInfo->fn), O_WRONLY | O_CREAT | O_TRUNC, 0666);
        for (int i=0; (*pRequest)->fd==-1 && i<RETRY_TIMES; i++)
        {
            printf("ERROR: file [%s] open fail! error: %d - %s. Retrying[%d] ...\n",
                            (*pRequest)->parent.info.fn, errno, strerror(errno), i);
            (*pRequest)->fd = pCreateInfo->direction == ASYNC_FILE_ACCESS_READ
                              ? open((char8 *)(pCreateInfo->fn), O_RDONLY, 0666)
                              : open((char8 *)(pCreateInfo->fn), O_WRONLY | O_CREAT | O_TRUNC, 0666);
        }

        (*pRequest)->buf            = NULL;
        (*pRequest)->isAlloced      = FALSE;
        (*pRequest)->submitted      = FALSE;
        (*pRequest)->accessDone     = FALSE;
        (*pRequest)->canceled       = FALSE;
        (*pRequest)->nbytes         = pCreateInfo->size;
        (*pRequest)->offset         = lseek((*pRequest)->fd, pCreateInfo->offset, SEEK_CUR);

        if ((*pRequest)->parent.info.direction == ASYNC_FILE_ACCESS_WRITE && (*pRequest)->fd >= 0)
        {
            ftruncate((*pRequest)->fd, (*pRequest)->nbytes);
        }
        fstat((*pRequest)->fd, &(*pRequest)->fsb);
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

    ret_t res = mmap_check_request_valid(pRequest);

    if (RET_OK == res)
    {
        if (pRequest->nbytes > 0)
        {
            (*buffer)                   = mmap(NULL, pRequest->nbytes, PROT_READ | PROT_WRITE,
                                         MAP_SHARED, pRequest->fd, pRequest->nbytes);
            for (int i=0; MAP_FAILED==(*buffer) && i<RETRY_TIMES; i++)
            {
                printf("ERROR: file [%s] mmap fail! error: %d - %s. Retrying[%d] ...\n",
                                        pRequest->parent.info.fn, errno, strerror(errno), i);
                (*buffer)               = mmap(NULL, pRequest->nbytes, PROT_READ | PROT_WRITE,
                                         MAP_SHARED, pRequest->fd, pRequest->nbytes);
            }
            pRequest->buf               = *buffer;
            pRequest->isAlloced         = TRUE;

            printf("file = %s: req_addr = %p, buf_addr = %p.\n",
                    pRequest->parent.info.fn, pRequest, pRequest->buf);
        }
        else
        {
            res = RET_BAD_VALUE;
            printf("ERROR: invalid malloc buffer size! res = %d.\n", res);
        }
    }

    return res;
}

/**********************************************************************************************************************/
/// Import mmap read only request buffer
static ret_t mmap_request_import_read_buffer(async_file_accessor_t       *thiz,
                                             async_file_access_request_t *pAsyncRequest,
                                             void                        *buffer)
{
    mmap_file_accessor_t *pMmapAccessor = (mmap_file_accessor_t *)thiz;
    mmap_request_t       *pRequest      = (mmap_request_t *)pAsyncRequest;

    ret_t res = mmap_check_request_valid(pRequest);

    if (RET_OK == res)
    {
        if (buffer != NULL)
        {
            buffer = mmap(NULL, pRequest->nbytes, PROT_READ | PROT_WRITE,
                    MAP_SHARED, pRequest->fd, pRequest->nbytes);
            for (int i=0; MAP_FAILED==buffer && i<RETRY_TIMES; i++)
            {
                printf("ERROR: file [%s] mmap fail! error: %d - %s. Retrying[%d] ...\n",
                        pRequest->parent.info.fn, errno, strerror(errno), i);
                buffer = mmap(NULL, pRequest->nbytes, PROT_READ,
                        MAP_SHARED, pRequest->fd, pRequest->nbytes);
            }
            pRequest->buf = buffer;

            printf("file = %s: req_addr = %p, buf_addr = %p.\n",
                    pRequest->parent.info.fn, pRequest, pRequest->buf);
        }
        else
        {
            res = RET_BAD_VALUE;
            printf("ERROR: invalid import buffer empty! res = %d.\n", res);
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
        res = ASYNC_FILE_ACCESS_WRITE == pRequest->parent.info.direction ? aio_write(&pRequest->cb)
                                                                         : aio_read(&pRequest->cb);

        if (res != RET_OK)
        {
            if (TRUE == pRequest->isAlloced)
            {
                free((void*)pRequest->cb.aio_buf);
                pRequest->buf           = NULL;
                pRequest->cb.aio_buf    = NULL;
            }
            printf("ERROR: failed to initiate the async IO operation! error: %d - %s.\n", errno, strerror(errno));
        }
        pRequest->submitted = TRUE;
    }

    if (RET_OK == res && pMmapAccessor->req_count % REQ_LIST_BUFSIZE == 0)
    {
    }

    if (RET_OK == res)
    {
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

    if (RET_OK != res || !pRequest->submitted || pRequest->canceled)
    {
        res = RET_INVALID_OPERATION;
        printf("Error: invalid request! res = %d.\n", res);
    }
    else if (RET_OK == res && !pRequest->accessDone)
    {
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
