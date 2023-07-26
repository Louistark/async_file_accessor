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

#define ASYNC_IO_METHOD     ASYNC_FILE_ACCESSOR_AIO
// #define ASYNC_IO_METHOD     ASYNC_FILE_ACCESSOR_MMAP

typedef struct
{
    char8  *input_filename;
    char8  *output_filename;
    void   *buf;
    u32     size;

} file_t;

ret_t create_test_data_set(file_t ***file_set, u32 *count);
ret_t read_one_picture_from_file(void **buffer, char8 *filename, u32 *length);
ret_t write_one_picture_to_file(void *buffer, char8 *filename, u32 length);

int main()
{
    printf("This is a test for async accessor.\n");

    ret_t res = RET_OK;

    int         i           = 0;
    u32         fileCnt     = 0;
    file_t    **file_set    = NULL;

    async_file_accessor_t *pFileAccessor = Async_File_Accessor_Get_Instance(ASYNC_IO_METHOD);

    printf("Create test data set.\n");
    res = create_test_data_set(&file_set, &fileCnt);

    printf("Start to read all files.\n");
    for (i = 0; i < fileCnt; i++)
    {
        read_one_picture_from_file(&(file_set[i]->buf),
                                   file_set[i]->input_filename,
                                   &(file_set[i]->size));
    }

    pFileAccessor->waitAll(pFileAccessor, 0);

    sleep(1);

    printf("Start to write all files.\n");
    for (i = 0; i < fileCnt; i++)
    {
        write_one_picture_to_file(file_set[i]->buf,
                                  file_set[i]->output_filename,
                                  file_set[i]->size);
    }

    pFileAccessor->waitAll(pFileAccessor, 0);

    sleep(1);

    printf("Start to cancel and release all request...\n");
    pFileAccessor->cancelAll(pFileAccessor);
    pFileAccessor->releaseAll(pFileAccessor);


    for (i = 0; i < fileCnt; i++)
    {
        free(file_set[i]->buf);
        file_set[i]->buf = NULL;
    }

    return res;
}

ret_t create_test_data_set(file_t ***file_set, u32 *count)
{
    ret_t   res         = RET_OK;

    *count      = 10;
    *file_set   = (file_t **)malloc(sizeof(file_t *)*(*count));

    printf("Creating test file set.\n");

    for (int i = 0; i < *count; i++)
    {
        (*file_set)[i] = (file_t *)malloc(sizeof(file_t));
    }

    (*file_set)[0]->input_filename   = "./pictures/source_data_set/cat_0.jpg";
    (*file_set)[1]->input_filename   = "./pictures/source_data_set/cat_1.jpg";
    (*file_set)[2]->input_filename   = "./pictures/source_data_set/cat_2.jpg";
    (*file_set)[3]->input_filename   = "./pictures/source_data_set/cat_3.jpg";
    (*file_set)[4]->input_filename   = "./pictures/source_data_set/cat_4.jpg";
    (*file_set)[5]->input_filename   = "./pictures/source_data_set/cat_5.jpg";
    (*file_set)[6]->input_filename   = "./pictures/source_data_set/cat_6.jpg";
    (*file_set)[7]->input_filename   = "./pictures/source_data_set/cat_7.jpg";
    (*file_set)[8]->input_filename   = "./pictures/source_data_set/cat_8.jpg";
    (*file_set)[9]->input_filename   = "./pictures/source_data_set/cat_9.jpg";

    printf("Creating test file set.\n");

    (*file_set)[0]->output_filename  = "./pictures/output/new_cat_0.jpg";
    (*file_set)[1]->output_filename  = "./pictures/output/new_cat_1.jpg";
    (*file_set)[2]->output_filename  = "./pictures/output/new_cat_2.jpg";
    (*file_set)[3]->output_filename  = "./pictures/output/new_cat_3.jpg";
    (*file_set)[4]->output_filename  = "./pictures/output/new_cat_4.jpg";
    (*file_set)[5]->output_filename  = "./pictures/output/new_cat_5.jpg";
    (*file_set)[6]->output_filename  = "./pictures/output/new_cat_6.jpg";
    (*file_set)[7]->output_filename  = "./pictures/output/new_cat_7.jpg";
    (*file_set)[8]->output_filename  = "./pictures/output/new_cat_8.jpg";
    (*file_set)[9]->output_filename  = "./pictures/output/new_cat_9.jpg";
}

ret_t read_one_picture_from_file(void **buffer, char8 *filename, u32 *length)
{
    ret_t   res         = RET_OK;
    u32     timeout_ms  = 2000;

    struct stat fsb;
    s32 fd = open(filename, O_RDONLY, 0666);
    fstat(fd, &fsb);
    *length = fsb.st_size;
printf("read_one_picture_from_file: 1, file length = %d.\n", *length);

    async_file_accessor_t *pFileAccessor = Async_File_Accessor_Get_Instance(ASYNC_IO_METHOD);
// printf("read_one_picture_from_file: 2.\n");
    async_file_access_request_t *pNewRequest = NULL;
    async_file_access_request_info_t createInfo = 
    {
        .direction  = ASYNC_FILE_ACCESS_READ,
        .size       = *length,
        .offset     = 0,
    };
printf("read_one_picture_from_file: 3, request length = %d.\n", createInfo.size);
    memcpy(createInfo.fn, filename, strlen(filename));

// printf("read_one_picture_from_file: 4.\n");
    res = pFileAccessor->getRequest(pFileAccessor, &pNewRequest, &createInfo);

    if(RET_OK == res)
    {
        (*buffer) = malloc(*length);
printf("read_one_picture_from_file: 5, buffer length = %d.\n", *length);
        res = pFileAccessor->importReadBuf(pFileAccessor, pNewRequest, (*buffer));
// printf("read_one_picture_from_file: 6.\n");
        res = pFileAccessor->putRequest(pFileAccessor, pNewRequest);
// printf("read_one_picture_from_file: 7.\n");
    }

    close(fd);

    return res;
}

ret_t write_one_picture_to_file(void *buffer, char8 *filename, u32 length)
{
    ret_t   res          = RET_OK;
    u32     timeout_ms   = 2000;
printf("write_one_picture_to_file: 1, buffer length = %d.\n", length);
    async_file_accessor_t *pFileAccessor = Async_File_Accessor_Get_Instance(ASYNC_IO_METHOD);
    async_file_access_request_t *pNewRequest = NULL;
    async_file_access_request_info_t createInfo = 
    {
        .direction  = ASYNC_FILE_ACCESS_WRITE,
        .size       = length,
        .offset     = 0,
    };
printf("write_one_picture_to_file: 2.\n");
    memcpy(createInfo.fn, filename, strlen(filename));
printf("write_one_picture_to_file: 3, filename[%s], length = %d.\n", createInfo.fn, length);

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

    return res;
}