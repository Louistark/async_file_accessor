/***************************************************************************************
 * Project      : async_file_accessor
 * File         : aio_file_accessor.c
 * Description  : Implement abstract async file accessor interface by aio
 * Author       : Louis Liu
 * Created Date : 2023-7-13
 * Copyright (c) 2023, [Louis.Liu]
 * All rights reserved.
 ***************************************************************************************/

#include "aio_file_accessor.h"
#include "async_file_accessor.h"

// Called when AIO operation is done, check result and free control block
static void aio_callback(sigval_t sv)
{
    aio_request_t* pRequest = (aio_request_t *)sv.sival_ptr;
    pthread_mutex_lock(&(pRequest->lock));

    if (aio_error(&pRequest->cb) == 0)
    {
        pRequest->status = REQUEST_STAT_IOSUCCESS;
        // printf("Request to file '%s' done. req_addr = %p, buf_addr = %p\n",
        //        pRequest->parent.info.fn, pRequest, pRequest->buf);
    }
    else
    {
        printf("Error: async IO operation fail! error: %d - %s.\n", errno, strerror(errno));
    }

    if (TRUE == pRequest->isAlloced && pRequest->buf != NULL)
    {
        free((void*)pRequest->cb.aio_buf);
        pRequest->buf           = NULL;
        pRequest->cb.aio_buf    = NULL;
    }

    if (pRequest->fd > 0)
    {
        close(pRequest->fd);
        pRequest->fd = -1;
    }
    
    if (pRequest->cb.aio_fildes > 0)
    {
        close(pRequest->cb.aio_fildes);
        pRequest->cb.aio_fildes = -1;
    }

    pthread_cond_signal(&(pRequest->isFinished));
    pthread_mutex_unlock(&(pRequest->lock));
}

/// Ckeck whether aio request valid
static ret_t aio_check_request_valid(aio_request_t *pRequest)
{
    ret_t res = RET_OK;

    if (NULL == pRequest)
    {
        res = RET_BAD_VALUE;
        printf("Error: invalid request detected: empty request! res = %d.\n", res);
    }

    else if (RET_OK == res && !pRequest->isValid)
    {
        async_file_access_request_info_t *pCreateInfo = &(pRequest->parent.info);

        if (NULL == pCreateInfo || pCreateInfo->size <= 0||
            (pCreateInfo->direction < 0 || pCreateInfo->direction >= ASYNC_FILE_ACCESS_MAX))
        {
            res = RET_BAD_VALUE;
            printf("Error: invalid request detected: invalid info! res = %d.\n", res);
        }
        else
        {
            s32 fd = pCreateInfo->direction == ASYNC_FILE_ACCESS_READ
                         ? open((char8 *)(pCreateInfo->fn), O_RDONLY, 0666)
                         : open((char8 *)(pCreateInfo->fn), O_WRONLY | O_CREAT, 0666);
            if (fd < 0)
            {
                res = RET_BAD_VALUE;
                printf("Error: invalid request detected: file [%s] not exist! res = %d.\n",
                       pCreateInfo->fn, res);
            }
            else
            {
                close(fd);
                pRequest->isValid = TRUE;
            }
        }
    }

    return res;
}

/// Get aio request
static ret_t aio_get_request(async_file_accessor_t            *thiz,
                             async_file_access_request_t     **pAsyncRequest,
                             async_file_access_request_info_t *pCreateInfo)
{
    aio_file_accessor_t *pAioAccessor   = (aio_file_accessor_t *)thiz;
    aio_request_t      **pRequest       = (aio_request_t **)pAsyncRequest;

    ret_t res = RET_OK;
    *pRequest = (aio_request_t *)malloc(sizeof(aio_request_t));
    memset(*pRequest, 0, sizeof(aio_request_t));

    (*pRequest)->parent.info.direction  = pCreateInfo->direction;
    (*pRequest)->parent.info.size       = pCreateInfo->size;
    (*pRequest)->parent.info.offset     = pCreateInfo->offset;
    memcpy((*pRequest)->parent.info.fn, pCreateInfo->fn, MAX_FILE_NAME_LEN);
    res = aio_check_request_valid(*pRequest);

    if (RET_OK == res)
    {
        (*pRequest)->fd = pCreateInfo->direction == ASYNC_FILE_ACCESS_READ
                              ? open((char8 *)(pCreateInfo->fn), O_RDONLY, 0666)
                              : open((char8 *)(pCreateInfo->fn), O_WRONLY | O_CREAT, 0666);
        for (int i=0; (*pRequest)->fd==-1 && i<MAX_RETRY_TIMES; i++)
        {
            printf("Error: file [%s] open fail! error: %d - %s. Retrying[%d] ...\n",
                            (*pRequest)->parent.info.fn, errno, strerror(errno), i);
            (*pRequest)->fd = pCreateInfo->direction == ASYNC_FILE_ACCESS_READ
                              ? open((char8 *)(pCreateInfo->fn), O_RDONLY, 0666)
                              : open((char8 *)(pCreateInfo->fn), O_WRONLY | O_CREAT, 0666);
        }

        (*pRequest)->buf            = NULL;
        (*pRequest)->isAlloced      = FALSE;
        (*pRequest)->status         = REQUEST_STAT_INIT;
        (*pRequest)->cb.aio_buf     = NULL;
        (*pRequest)->cb.aio_fildes  = (*pRequest)->fd;
        (*pRequest)->cb.aio_nbytes  = pCreateInfo->size;
        (*pRequest)->cb.aio_offset  = lseek((*pRequest)->fd, pCreateInfo->offset, SEEK_CUR);
        fstat((*pRequest)->fd, &(*pRequest)->fsb);
        pthread_mutex_init(&((*pRequest)->lock), NULL);
        pthread_cond_init(&((*pRequest)->isFinished), NULL);
    }

    // printf("file = %s: req_addr = %p.\n", (*pRequest)->parent.info.fn, (*pRequest));

    return res;
}

/// Alloc aio write request buffer
static ret_t aio_request_alloc_write_buffer(async_file_accessor_t       *thiz,
                                            async_file_access_request_t *pAsyncRequest,
                                            void                       **buffer)
{
    aio_file_accessor_t *pAioAccessor   = (aio_file_accessor_t *)thiz;
    aio_request_t       *pRequest       = (aio_request_t *)pAsyncRequest;

    ret_t res = aio_check_request_valid(pRequest);

    if (RET_OK == res)
    {
        if (pRequest->cb.aio_nbytes > 0)
        {
            (*buffer)                   = malloc(pRequest->cb.aio_nbytes);
            for (int i=0; NULL==(*buffer) && i<MAX_RETRY_TIMES; i++)
            {
                printf("Error: file [%s] buffer malloc fail! Retrying[%d] ...\n", pRequest->parent.info.fn, i);
                (*buffer)               = malloc(pRequest->cb.aio_nbytes);
            }
            pRequest->buf               = *buffer;
            pRequest->cb.aio_buf        = *buffer;
            pRequest->isAlloced         = TRUE;

            // printf("file = %s: req_addr = %p, buf_addr = %p.\n", pRequest->parent.info.fn, pRequest, pRequest->buf);
        }
        else
        {
            res = RET_BAD_VALUE;
            printf("Error: invalid buffer size! res = %d.\n", res);
        }
    }

    return res;
}

/// Import aio read only request buffer
static ret_t aio_request_import_read_buffer(async_file_accessor_t       *thiz,
                                            async_file_access_request_t *pAsyncRequest,
                                            void                        *buffer)
{
    aio_file_accessor_t *pAioAccessor   = (aio_file_accessor_t *)thiz;
    aio_request_t       *pRequest       = (aio_request_t *)pAsyncRequest;

    ret_t res = aio_check_request_valid(pRequest);

    if (RET_OK == res)
    {
        if (buffer != NULL)
        {
            pRequest->buf               = buffer;
            pRequest->cb.aio_buf        = buffer;

            // printf("file = %s: req_addr = %p, buf_addr = %p.\n", pRequest->parent.info.fn, pRequest, pRequest->buf);
        }
        else
        {
            res = RET_BAD_VALUE;
            printf("Error: invalid buffer empty! res = %d.\n", res);
        }
    }

    return res;
}

/// Put aio request
static ret_t aio_put_request(async_file_accessor_t       *thiz,
                             async_file_access_request_t *pAsyncRequest)
{
    aio_file_accessor_t *pAioAccessor   = (aio_file_accessor_t *)thiz;
    aio_request_t       *pRequest       = (aio_request_t *)pAsyncRequest;

    ret_t res = aio_check_request_valid(pRequest);

    if (RET_OK == res)
    {
        pRequest->cb.aio_sigevent.sigev_notify              = SIGEV_THREAD;
        pRequest->cb.aio_sigevent.sigev_notify_function     = aio_callback;
        pRequest->cb.aio_sigevent.sigev_notify_attributes   = NULL;
        pRequest->cb.aio_sigevent.sigev_value.sival_ptr     = pRequest;

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
            printf("Error: failed to initiate the async IO operation! error: %d - %s.\n", errno, strerror(errno));
        }
        pRequest->status = REQUEST_STAT_SUBMITTED;
    }

    if (RET_OK == res && pAioAccessor->req_count % REQ_LIST_BUFSIZE == 0)
    {
        u32 newListBufSize = ((pAioAccessor->req_count + REQ_LIST_BUFSIZE) / REQ_LIST_BUFSIZE)
                                * REQ_LIST_BUFSIZE * sizeof(aio_request_t *);
        aio_request_t **new_list = (aio_request_t **)realloc(pAioAccessor->req_list, newListBufSize);

        if (NULL == new_list)
        {
            res = RET_NO_MEMORY;
            printf("Error: fail to realloc request list! res = %d.\n", res);
        }
        else
        {
            pAioAccessor->req_list = new_list;
        }
    }

    if (RET_OK == res)
    {
        pAioAccessor->req_list[pAioAccessor->req_count] = pRequest;
        pAioAccessor->req_count++;
    }

    // printf("put request: file = %s: req_addr = %p, buf_addr = %p.\n", pRequest->parent.info.fn, pRequest, pRequest->buf);

    return res;
}

/// Wait for an aio request finish
static ret_t aio_wait_request(async_file_accessor_t       *thiz,
                              async_file_access_request_t *pAsyncRequest,
                              u32                          timeout_ms)
{
    aio_file_accessor_t *pAioAccessor   = (aio_file_accessor_t *)thiz;
    aio_request_t       *pRequest       = (aio_request_t *)pAsyncRequest;

    ret_t res = aio_check_request_valid(pRequest);

    if (RET_OK != res                           ||
        pRequest->status <= REQUEST_STAT_INIT   ||
        pRequest->status >= REQUEST_STAT_CANCEL)
    {
        res = RET_INVALID_OPERATION;
        printf("Error: invalid request! res = %d.\n", res);
    }
    else if (REQUEST_STAT_SUBMITTED == pRequest->status)
    {
        struct aiocb   *aiocb_list[1];
        struct timespec timeout =
        {
            .tv_sec     = timeout_ms / 1000,
            .tv_nsec    = (timeout_ms % 1000) * 1000000,
        };

        aiocb_list[0] = &(pRequest->cb);
        res = aio_suspend((const struct aiocb *const *)aiocb_list, 1, (timeout_ms > 0) ? &timeout : NULL);
        res = (RET_OK == res) ? aio_error(&(pRequest->cb)) : res;
    }

    return res;
}

/// Cancel an aio request
static ret_t aio_cancel_request(async_file_accessor_t       *thiz,
                                async_file_access_request_t *pAsyncRequest)
{
    aio_file_accessor_t *pAioAccessor   = (aio_file_accessor_t *)thiz;
    aio_request_t       *pRequest       = (aio_request_t *)pAsyncRequest;

    ret_t res = aio_check_request_valid(pRequest);
    pthread_mutex_lock(&(pRequest->lock));

    if (RET_OK != res                           ||
        pRequest->status <= REQUEST_STAT_INIT   ||
        pRequest->status >= REQUEST_STAT_CANCEL)
    {
        printf("Warning: cannot cancel a finished request!\n");
    }
    else if (RET_OK == res)
    {
        res = aio_cancel(pRequest->fd, &pRequest->cb);

        if (TRUE == pRequest->isAlloced)
        {
            free((void*)pRequest->cb.aio_buf);
            pRequest->buf           = NULL;
            pRequest->cb.aio_buf    = NULL;
        }
        pRequest->status = REQUEST_STAT_CANCEL;
    }

    pthread_mutex_unlock(&(pRequest->lock));

    return res;
}

/// Wait for all aio request finish, after this func release all requests
static ret_t aio_wait_all_requests(async_file_accessor_t *thiz)
{
    aio_file_accessor_t *pAioAccessor = (aio_file_accessor_t *)thiz;

    ret_t res = RET_OK;

    if (NULL == pAioAccessor || NULL == pAioAccessor->req_list || 0 == pAioAccessor->req_count)
    {
        printf("Empty aio accessor, no need to wait.\n");
    }
    else
    {
        struct aiocb   *aiocb_list[1];

        for (int i = 0; i < pAioAccessor->req_count; i++)
        {
            aio_request_t *pRequest = pAioAccessor->req_list[i];
            if (pRequest)
            {
                // while (pRequest->submitted && !pRequest->canceled && !pRequest->accessDone ||
                //        pRequest->fd >= 0 || (TRUE == pRequest->isAlloced && pRequest->buf != NULL))
                // {
                //     if (pRequest->accessDone && pRequest->fd < 0 &&
                //         (TRUE == pRequest->isAlloced) ^ (pRequest->buf == NULL))
                //     {
                //         printf("wait request: file = %s: req_addr = %p, buf_addr = %p.\n",
                //                 pRequest->parent.info.fn, pRequest, pRequest->buf);
                //         break;
                //     }
                // }
                
                pthread_mutex_lock(&(pRequest->lock));
                while (REQUEST_STAT_SUBMITTED == pRequest->status)
                {
                    pthread_cond_wait(&(pRequest->isFinished), &(pRequest->lock));
                }
                pthread_mutex_unlock(&(pRequest->lock));
                res |= aio_error(&(pRequest->cb));
            }
        }
        printf("Wait all request done.\n");
    }

    return res;
}

/// Cancel all aio request, after this func release all requests
static ret_t aio_cancel_all_requests(async_file_accessor_t *thiz)
{
    aio_file_accessor_t *pAioAccessor = (aio_file_accessor_t *)thiz;

    ret_t res = RET_OK;

    if (NULL == pAioAccessor || NULL == pAioAccessor->req_list || 0 == pAioAccessor->req_count)
    {
        printf("Empty aio accessor, no need to cancel.\n");
    }
    else
    {
        for (int i = 0; i < pAioAccessor->req_count; i++)
        {
            aio_request_t *pRequest = pAioAccessor->req_list[i];
            aio_cancel(pRequest->fd, &pRequest->cb);
            // printf("cancel request: file = %s: req_addr = %p, buf_addr = %p.\n", pRequest->parent.info.fn, pRequest, pRequest->buf);
        }
    }

    return res;
}

// Cancel all AIO operations and release resources
static ret_t aio_release_all_resources(async_file_accessor_t *thiz)
{
    aio_file_accessor_t *pAioAccessor = (aio_file_accessor_t *)thiz;

    ret_t res = RET_OK;

    if (NULL == pAioAccessor || NULL == pAioAccessor->req_list || 0 == pAioAccessor->req_count)
    {
        printf("Empty aio accessor, no need to release.\n");
    }
    else
    {
        for (int i = 0; i < pAioAccessor->req_count; i++)
        {
            aio_request_t *pRequest = pAioAccessor->req_list[i];
            if (pRequest)
            {
                pthread_mutex_lock(&(pRequest->lock));
                if (REQUEST_STAT_SUBMITTED == pRequest->status)
                {
                    aio_cancel(pRequest->fd, &pRequest->cb);
                    printf("cancel request: file = %s: req_addr = %p, buf_addr = %p.\n",
                           pRequest->parent.info.fn, pRequest, pRequest->buf);
                }
                if (pRequest->cb.aio_fildes != -1)
                {
                    printf("close request fd: file = %s: req_addr = %p, buf_addr = %p.\n",
                           pRequest->parent.info.fn, pRequest, pRequest->buf);
                    close(pRequest->cb.aio_fildes);
                    pRequest->cb.aio_fildes = -1;
                }
                if (TRUE == pRequest->isAlloced && pRequest->buf != NULL)
                {
                    printf("free request buffer: file = %s: req_addr = %p, buf_addr = %p.\n",
                           pRequest->parent.info.fn, pRequest, pRequest->buf);
                    free(pRequest->buf);
                    pRequest->buf = NULL;
                }

                pthread_mutex_unlock(&(pRequest->lock));
                pthread_mutex_destroy(&(pRequest->lock));
                free(pRequest);

                pAioAccessor->req_list[i] = NULL;
            }
        }
    }

    return res;
}

/// Singleton static aio accessor
static aio_file_accessor_t g_aioFileAccessor =
{
    .parent =
    {
        .type               = ASYNC_FILE_ACCESSOR_AIO,

        .getRequest         = aio_get_request,
        .allocWriteBuf      = aio_request_alloc_write_buffer,
        .importReadBuf      = aio_request_import_read_buffer,
        .putRequest         = aio_put_request,
        .waitRequest        = aio_wait_request,
        .cancelRequest      = aio_cancel_request,
        .waitAll            = aio_wait_all_requests,
        .cancelAll          = aio_cancel_all_requests,
        .releaseAll         = aio_release_all_resources,
    },

    .req_list = NULL,
    .req_count = 0,
};

/// Acqiure single static aio accessor
aio_file_accessor_t* AIO_File_Accessor_Get_Instance()
{
    return &g_aioFileAccessor;
}
