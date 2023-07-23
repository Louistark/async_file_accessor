/***************************************************************************************
 * Project      : async_file_accessor
 * File         : test_async_accessor.c
 * Description  : Provide main function to test async file accessor lib.
 * Author       : Louis Liu
 * Created Date : 2023-7-13
 * Copyright (c) 2023, [Louis.Liu]
 * All rights reserved.
 ***************************************************************************************/

#include "async_file_accessor.h"
#include "aio_file_accessor.h"

ret_t aio_write_once();
ret_t aio_read_once();

ret_t mmap_write_once();
ret_t mmap_read_once();

int main()
{
   printf("This is a test for async accessor.\n");

   ret_t res = RET_OK;

   res = aio_write_once();
   res = aio_read_once();

   printf("Start to cancel and release all aio request...\n");
   async_file_accessor_t *pAioFileAccessor = Async_File_Accessor_Get_Instance(ASYNC_FILE_ACCESSOR_AIO);
   pAioFileAccessor->cancelAll(pAioFileAccessor);
   pAioFileAccessor->releaseAll(pAioFileAccessor);

   printf("Start to cancel and release all mmap request...\n");
   async_file_accessor_t *pMmapFileAccessor = Async_File_Accessor_Get_Instance(ASYNC_FILE_ACCESSOR_AIO);
   pMmapFileAccessor->cancelAll(pMmapFileAccessor);
   pMmapFileAccessor->releaseAll(pMmapFileAccessor);

   return res;
}

ret_t aio_write_once()
{
    ret_t   res          = RET_OK;
    u32     data_size    = 1024;
    u32     timeout_ms   = 1000;
    async_file_accessor_t *pFileAccessor = Async_File_Accessor_Get_Instance(ASYNC_FILE_ACCESSOR_AIO);
    async_file_access_request_t *pNewRequest = NULL;
    async_file_access_request_info_t createInfo = 
    {
        .direction  = ASYNC_FILE_ACCESS_WRITE,
        .fn         = "./test_aio_write_1.txt",
        .size       = data_size,
        .offset     = 0,
    };

    res = pFileAccessor->getRequest(pFileAccessor, &pNewRequest, &createInfo);

    if(RET_OK == res)
    {
        void *buf = NULL;
        res = pFileAccessor->allocWriteBuf(pFileAccessor, pNewRequest, &buf);

        memset(buf, 'a', data_size);
        res = pFileAccessor->putRequest(pFileAccessor, pNewRequest);
    }

    if(RET_OK == res)
    {
        res = pFileAccessor->waitRequest(pFileAccessor, pNewRequest, timeout_ms);

        if (res != RET_OK)
        {
            printf("Cancel request to file: %s\n", pNewRequest->info.fn);
            res = pFileAccessor->cancelRequest(pFileAccessor, pNewRequest);
        }
    }

    return res;
}

ret_t aio_read_once()
{
    ret_t   res          = RET_OK;
    u32     data_size    = 10*sizeof(char8);
    u32     timeout_ms   = 1000;
    async_file_accessor_t *pFileAccessor = Async_File_Accessor_Get_Instance(ASYNC_FILE_ACCESSOR_AIO);
    async_file_access_request_t *pNewRequest = NULL;
    async_file_access_request_info_t createInfo = 
    {
        .direction  = ASYNC_FILE_ACCESS_READ,
        .fn         = "./test_aio_write_1.txt",
        .size       = data_size,
        .offset     = 0,
    };

    void *buf = malloc(data_size + 1);
    res = pFileAccessor->getRequest(pFileAccessor, &pNewRequest, &createInfo);

    if(RET_OK == res)
    {
        res = pFileAccessor->importReadBuf(pFileAccessor, pNewRequest, buf);
        res = pFileAccessor->putRequest(pFileAccessor, pNewRequest);
    }

    if(RET_OK == res)
    {
        res = pFileAccessor->waitRequest(pFileAccessor, pNewRequest, timeout_ms);

        if (res != RET_OK)
        {
            printf("Cancel request to file: %s", pNewRequest->info.fn);
            res = pFileAccessor->cancelRequest(pFileAccessor, pNewRequest);
        }
    }

    printf("Read from file[%s]: %s.\n", createInfo.fn, (char8 *)buf);

    free(buf);
    buf = NULL;

    return res;
}

ret_t mmap_write_once()
{
    ret_t   res          = RET_OK;
    u32     data_size    = 1024;
    u32     timeout_ms   = 1000;
    async_file_accessor_t *pFileAccessor = Async_File_Accessor_Get_Instance(ASYNC_FILE_ACCESSOR_MMAP);
    async_file_access_request_t *pNewRequest = NULL;
    async_file_access_request_info_t createInfo = 
    {
        .direction  = ASYNC_FILE_ACCESS_WRITE,
        .fn         = "./test_aio_write_1.txt",
        .size       = data_size,
        .offset     = 0,
    };

    res = pFileAccessor->getRequest(pFileAccessor, &pNewRequest, &createInfo);

    if(RET_OK == res)
    {
        void *buf = NULL;
        res = pFileAccessor->allocRequestBuf(pFileAccessor, pNewRequest, &buf);

        memset(buf, 'a', data_size);
        res = pFileAccessor->putRequest(pFileAccessor, pNewRequest);
    }

    if(RET_OK == res)
    {
        res = pFileAccessor->waitRequest(pFileAccessor, pNewRequest, timeout_ms);

        if (res != RET_OK)
        {
            printf("Cancel request to file: %s\n", pNewRequest->info.fn);
            res = pFileAccessor->cancelRequest(pFileAccessor, pNewRequest);
        }
    }

    return res;
}
