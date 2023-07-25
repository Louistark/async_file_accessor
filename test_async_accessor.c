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

ret_t read_one_picture_from_file(void **buffer, char8 *filename, u32 *length);
ret_t write_one_picture_to_file(void *buffer, char8 *filename, u32 length);

int main()
{
    printf("This is a test for async accessor.\n");

    ret_t res = RET_OK;

    // res = aio_write_once();
    // res = aio_read_once();

    // res = mmap_write_once();
    // res = mmap_read_once();

    u32   length        = 0;
    void  *pic_buffer   = NULL;
    // char8 *src_filename = "./pictures/one_cat.jpg";
    // char8 *des_filename = "./pictures/another_cat.jpg";

    char8 *src_filename = "./test_aio_write_1.txt";
    res = read_one_picture_from_file(&pic_buffer, src_filename, &length);
    // res = write_one_picture_to_file(pic_buffer, des_filename, length);

printf("read buffer: %s\n", (char8 *)pic_buffer);

    printf("Start to cancel and release all aio request...\n");
    // async_file_accessor_t *pAioFileAccessor = Async_File_Accessor_Get_Instance(ASYNC_FILE_ACCESSOR_AIO);
    // pAioFileAccessor->cancelAll(pAioFileAccessor);
    // pAioFileAccessor->releaseAll(pAioFileAccessor);

    // printf("Start to cancel and release all mmap request...\n");
    // async_file_accessor_t *pMmapFileAccessor = Async_File_Accessor_Get_Instance(ASYNC_FILE_ACCESSOR_MMAP);
    // pMmapFileAccessor->cancelAll(pMmapFileAccessor);
    // pMmapFileAccessor->releaseAll(pMmapFileAccessor);

    free(pic_buffer);
    pic_buffer = NULL;

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

ret_t mmap_read_once()
{
    ret_t   res          = RET_OK;
    u32     data_size    = 10*sizeof(char8);
    u32     timeout_ms   = 1000;
    async_file_accessor_t *pFileAccessor = Async_File_Accessor_Get_Instance(ASYNC_FILE_ACCESSOR_MMAP);
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

ret_t read_one_picture_from_file(void **buffer, char8 *filename, u32 *length)
{
    ret_t   res         = RET_OK;
    u32     timeout_ms  = 2000;

    struct stat fsb;
    s32 fd = open(filename, O_RDONLY, 0666);
    fstat(fd, &fsb);
    *length = fsb.st_blksize;
printf("read_one_picture_from_file: 1.\n");

    async_file_accessor_t *pFileAccessor = Async_File_Accessor_Get_Instance(ASYNC_FILE_ACCESSOR_AIO);
printf("read_one_picture_from_file: 2.\n");
    async_file_access_request_t *pNewRequest = NULL;
    async_file_access_request_info_t createInfo = 
    {
        .direction  = ASYNC_FILE_ACCESS_READ,
        .size       = *length,
        .offset     = 0,
    };
printf("read_one_picture_from_file: 3.\n");
    memcpy(createInfo.fn, filename, strlen(filename));

printf("read_one_picture_from_file: 4.\n");
    res = pFileAccessor->getRequest(pFileAccessor, &pNewRequest, &createInfo);

    if(RET_OK == res)
    {
        (*buffer) = malloc(*length + 1);
printf("read_one_picture_from_file: 5.\n");
        res = pFileAccessor->importReadBuf(pFileAccessor, pNewRequest, (*buffer));
printf("read_one_picture_from_file: 6.\n");
        res = pFileAccessor->putRequest(pFileAccessor, pNewRequest);
printf("read_one_picture_from_file: 7.\n");
    }

    if(RET_OK == res)
    {
printf("read_one_picture_from_file: 8.\n");
        res = pFileAccessor->waitRequest(pFileAccessor, pNewRequest, NULL);
printf("read_one_picture_from_file: 9.read buffer: %s\n", (char8 *)(*buffer));

        if (res != RET_OK)
        {
            printf("Cancel request to file: %s", pNewRequest->info.fn);
            res = pFileAccessor->cancelRequest(pFileAccessor, pNewRequest);
        }
    }
sleep(1);
printf("read_one_picture_from_file: 10.read buffer: %s\n", (char8 *)(*buffer));

    close(fd);

    return res;
}

ret_t write_one_picture_to_file(void *buffer, char8 *filename, u32 length)
{
    ret_t   res          = RET_OK;
    u32     timeout_ms   = 2000;
printf("write_one_picture_to_file: 1.\n");
    async_file_accessor_t *pFileAccessor = Async_File_Accessor_Get_Instance(ASYNC_FILE_ACCESSOR_AIO);
    async_file_access_request_t *pNewRequest = NULL;
    async_file_access_request_info_t createInfo = 
    {
        .direction  = ASYNC_FILE_ACCESS_WRITE,
        .size       = length,
        .offset     = 0,
    };
printf("write_one_picture_to_file: 2.\n");
    memcpy(createInfo.fn, filename, strlen(filename));
printf("write_one_picture_to_file: 3.\n");

    res = pFileAccessor->getRequest(pFileAccessor, &pNewRequest, &createInfo);
printf("write_one_picture_to_file: 4.\n");

    if(RET_OK == res)
    {
        void *buf = NULL;
printf("write_one_picture_to_file: 5.\n");
        res = pFileAccessor->allocWriteBuf(pFileAccessor, pNewRequest, &buf);
printf("write_one_picture_to_file: 6.\n");

        memcpy(buf, buffer, length);
printf("write_one_picture_to_file: 7.\n");
        res = pFileAccessor->putRequest(pFileAccessor, pNewRequest);
printf("write_one_picture_to_file: 8.\n");
    }

    if(RET_OK == res)
    {
printf("write_one_picture_to_file: 9.\n");
        res = pFileAccessor->waitRequest(pFileAccessor, pNewRequest, timeout_ms);

        if (res != RET_OK)
        {
            printf("Cancel request to file: %s\n", pNewRequest->info.fn);
            res = pFileAccessor->cancelRequest(pFileAccessor, pNewRequest);
        }
    }
printf("write_one_picture_to_file: 10.\n");

    return res;
}