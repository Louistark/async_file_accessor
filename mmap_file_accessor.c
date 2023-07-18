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

    


    return res;
}

/// Alloc mmap request buffer
static ret_t mmap_request_alloc_buffer(async_file_accessor_t       *thiz,
                                      async_file_access_request_t *pAsyncRequest,
                                      void                       **buf)
{
    mmap_file_accessor_t *pMmapAccessor = (mmap_file_accessor_t *)thiz;
    mmap_request_t       *pRequest      = (mmap_request_t *)pAsyncRequest;

    ret_t res = mmap_check_request_valid(pRequest);

    if (RET_OK == res)
    {
    }

    return res;
}

/// Import mmap request buffer
static ret_t mmap_request_import_buffer(async_file_accessor_t       *thiz,
                                       async_file_access_request_t *pAsyncRequest,
                                       void                       **buf)
{
    mmap_file_accessor_t *pMmapAccessor = (mmap_file_accessor_t *)thiz;
    mmap_request_t       *pRequest      = (mmap_request_t *)pAsyncRequest;

    ret_t res = mmap_check_request_valid(pRequest);

    if (RET_OK == res)
    {
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
        .allocRequestBuf    = mmap_request_alloc_buffer,
        .importRequestBuf   = mmap_request_import_buffer,
        .putRequest         = mmap_put_request,
        .waitRequest        = mmap_wait_request,
        .cancelRequest      = mmap_cancel_request,
        .waitAll            = mmap_wait_all_request,
        .cancelAll          = mmap_cancel_all_requests,
        .releaseAll         = mmap_release_all_resources,
    },

    .req_list = NULL,
    .req_count = 0,
};

/// Acqiure single static mmap accessor
mmap_file_accessor_t* MMAP_File_Accessor_Get_Instance()
{
    return &g_mmapFileAccessor;
}
