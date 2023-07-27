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

typedef struct
{
    char8  *input_filename;
    char8  *output_filename;
    void   *buf;
    u32     size;

} file_t;

long long get_time_in_microseconds();
ret_t create_test_data_set(file_t ***file_set, u32 *count);
ret_t read_one_picture_from_file(void **buffer, char8 *filename, u32 *length);
ret_t write_one_picture_to_file(void *buffer, char8 *filename, u32 length);
ret_t sync_read_one_picture_from_file(void **buffer, char8 *filename, u32 *length);
ret_t sync_write_one_picture_to_file(void *buffer, char8 *filename, u32 length);

async_file_accessor_type_t g_async_method_type;

int main(int argc, char *argv[])
{
    if ((argc < 3) || (strcmp(argv[1], "0") && strcmp(argv[1], "1")) || (strcmp(argv[2], "1") && strcmp(argv[2], "2")))
    {
        printf("Usage: %s <EN_ASYNC_IO> <ASYNC_METHOD_TYPE>\n\n"
               "       EN_ASYNC_IO = 0: disable async IO\n"
               "       EN_ASYNC_IO = 1: enable async IO\n\n"
               "       ASYNC_METHOD_TYPE = 1: use aio\n"
               "       ASYNC_METHOD_TYPE = 2: use mmap\n\n", argv[0]);
        exit(1);
    }

    BOOL en_async = strcmp(argv[1], "0") ? TRUE : FALSE;
    g_async_method_type = strcmp(argv[2], "1") ? ASYNC_FILE_ACCESSOR_MMAP : ASYNC_FILE_ACCESSOR_AIO;

    printf("- This is a test for async accessor.\n");
    if (!en_async)
    {
        printf("- Disable async IO.\n");
    }
    else
    {
        printf("- Enable async IO\n");
        if (g_async_method_type == ASYNC_FILE_ACCESSOR_MMAP)
        {
            printf("- ASYNC_FILE_ACCESSOR_MMAP.\n");
        }
        else
        {
            printf("- ASYNC_FILE_ACCESSOR_AIO.\n");
        }
    }


    ret_t res = RET_OK;

    int         i           = 0;
    u32         fileCnt     = 0;
    file_t    **file_set    = NULL;

    async_file_accessor_t *pFileAccessor = en_async ? Async_File_Accessor_Get_Instance(g_async_method_type) : NULL;

    printf("- Create test data set.\n");
    res = create_test_data_set(&file_set, &fileCnt);

    printf("- Start to read all files.\n");
    long long read_start_time = get_time_in_microseconds();

    if (en_async)
    {
        for (i = 0; i < fileCnt; i++)
        {
            read_one_picture_from_file(&(file_set[i]->buf),
                                       file_set[i]->input_filename,
                                       &(file_set[i]->size));
        }

        // sleep(2);

        pFileAccessor->waitAll(pFileAccessor, 0);
    }
    else
    {
        for (i = 0; i < fileCnt; i++)
        {
            sync_read_one_picture_from_file(&(file_set[i]->buf),
                                            file_set[i]->input_filename,
                                            &(file_set[i]->size));
        }

        // sleep(2);
    }

    long long read_end_time       = get_time_in_microseconds();
    double elapsed_read_time    = (double)(read_end_time - read_start_time) / 1000;
    printf("\n -- Read %d pictures time consumption: %f ms.\n\n", fileCnt, elapsed_read_time);

    // sleep(2);

    printf("- Start to write all files.\n");
    long long write_start_time = get_time_in_microseconds();

    if (en_async)
    {
        for (i = 0; i < fileCnt; i++)
        {
            write_one_picture_to_file(file_set[i]->buf,
                                    file_set[i]->output_filename,
                                    file_set[i]->size);
        }

        // sleep(2);

        pFileAccessor->waitAll(pFileAccessor, 0);
    }
    else
    {
        for (i = 0; i < fileCnt; i++)
        {
            sync_write_one_picture_to_file(file_set[i]->buf,
                                        file_set[i]->output_filename,
                                        file_set[i]->size);
        }

        // sleep(2);
    }

    long long write_end_time = get_time_in_microseconds();
    double elapsed_write_time = (double)(write_end_time - write_start_time) / 1000;
    printf("\n -- Write %d pictures time consumption: %f ms.\n\n", fileCnt, elapsed_write_time);

    // sleep(2);

    if (en_async)
    {
        printf("- Start to cancel and release all request...\n");
        pFileAccessor->cancelAll(pFileAccessor);
        pFileAccessor->releaseAll(pFileAccessor);
    }

    for (i = 0; i < fileCnt; i++)
    {
        free(file_set[i]->buf);
        file_set[i]->buf = NULL;
    }

    return res;
}

long long get_time_in_microseconds()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return ((long long)tv.tv_sec * 1000000) + tv.tv_usec;
}

ret_t create_test_data_set(file_t ***file_set, u32 *count)
{
    ret_t res   = RET_OK;

    *count      = 10;
    *file_set   = (file_t **)malloc(sizeof(file_t *)*(*count));

    for (int i = 0; i < *count; i++)
    {
        (*file_set)[i] = (file_t *)malloc(sizeof(file_t));
    }

    (*file_set)[0]->input_filename   = "./pictures/source_data_set/RAW_4K_0.RAW";
    (*file_set)[1]->input_filename   = "./pictures/source_data_set/RAW_4K_1.RAW";
    (*file_set)[2]->input_filename   = "./pictures/source_data_set/RAW_4K_2.RAW";
    (*file_set)[3]->input_filename   = "./pictures/source_data_set/RAW_4K_3.RAW";
    (*file_set)[4]->input_filename   = "./pictures/source_data_set/RAW_4K_4.RAW";
    (*file_set)[5]->input_filename   = "./pictures/source_data_set/RAW_4K_5.RAW";
    (*file_set)[6]->input_filename   = "./pictures/source_data_set/RAW_4K_6.RAW";
    (*file_set)[7]->input_filename   = "./pictures/source_data_set/RAW_4K_7.RAW";
    (*file_set)[8]->input_filename   = "./pictures/source_data_set/RAW_4K_8.RAW";
    (*file_set)[9]->input_filename   = "./pictures/source_data_set/RAW_4K_9.RAW";

    (*file_set)[0]->output_filename  = "./pictures/output/new_RAW_4K_0.RAW";
    (*file_set)[1]->output_filename  = "./pictures/output/new_RAW_4K_1.RAW";
    (*file_set)[2]->output_filename  = "./pictures/output/new_RAW_4K_2.RAW";
    (*file_set)[3]->output_filename  = "./pictures/output/new_RAW_4K_3.RAW";
    (*file_set)[4]->output_filename  = "./pictures/output/new_RAW_4K_4.RAW";
    (*file_set)[5]->output_filename  = "./pictures/output/new_RAW_4K_5.RAW";
    (*file_set)[6]->output_filename  = "./pictures/output/new_RAW_4K_6.RAW";
    (*file_set)[7]->output_filename  = "./pictures/output/new_RAW_4K_7.RAW";
    (*file_set)[8]->output_filename  = "./pictures/output/new_RAW_4K_8.RAW";
    (*file_set)[9]->output_filename  = "./pictures/output/new_RAW_4K_9.RAW";

    return res;
}

ret_t read_one_picture_from_file(void **buffer, char8 *filename, u32 *length)
{
    ret_t   res         = RET_OK;
    u32     timeout_ms  = 2000;

    struct stat fsb;
    s32 fd = open(filename, O_RDONLY, 0666);
    fstat(fd, &fsb);
    *length = fsb.st_size;

    async_file_accessor_t *pFileAccessor = Async_File_Accessor_Get_Instance(g_async_method_type);
    async_file_access_request_t *pNewRequest = NULL;
    async_file_access_request_info_t createInfo = 
    {
        .direction  = ASYNC_FILE_ACCESS_READ,
        .size       = *length,
        .offset     = 0,
    };
    memcpy(createInfo.fn, filename, strlen(filename));

    res = pFileAccessor->getRequest(pFileAccessor, &pNewRequest, &createInfo);

    if(RET_OK == res)
    {
        (*buffer) = malloc(*length);
        res = pFileAccessor->importReadBuf(pFileAccessor, pNewRequest, (*buffer));
        res = pFileAccessor->putRequest(pFileAccessor, pNewRequest);
    }

    printf("read_picture: [%s], size: %d, buf_addr: %p / %p.\n", createInfo.fn, *length, buffer, *buffer);

    close(fd);

    return res;
}

ret_t write_one_picture_to_file(void *buffer, char8 *filename, u32 length)
{
    ret_t res = (buffer != NULL && length != 0) ? RET_OK : RET_INVALID_OPERATION;

    u32     timeout_ms   = 2000;
    async_file_accessor_t *pFileAccessor = Async_File_Accessor_Get_Instance(g_async_method_type);
    async_file_access_request_t *pNewRequest = NULL;
    async_file_access_request_info_t createInfo = 
    {
        .direction  = ASYNC_FILE_ACCESS_WRITE,
        .size       = length,
        .offset     = 0,
    };
    memcpy(createInfo.fn, filename, strlen(filename));

    printf("write_picture: [%s], size: %d, buf_addr: %p.\n", createInfo.fn, length, buffer);

    if(RET_OK == res)
    {
        res = pFileAccessor->getRequest(pFileAccessor, &pNewRequest, &createInfo);
    }

    if(RET_OK == res)
    {
        void *buf = NULL;
        res = pFileAccessor->allocWriteBuf(pFileAccessor, pNewRequest, &buf);

        long long memcpy_start_time = get_time_in_microseconds();
        memcpy(buf, buffer, length);
        long long memcpy_end_time = get_time_in_microseconds();
        double memcpy_time = (double)(memcpy_end_time - memcpy_start_time) / 1000;
        printf("\n -- memcpy [%s] time consumption: %f ms.\n\n", createInfo.fn, memcpy_time);

        res = pFileAccessor->putRequest(pFileAccessor, pNewRequest);
    }

    return res;
}

ret_t sync_read_one_picture_from_file(void **buffer, char8 *filename, u32 *length)
{
    ret_t   res         = RET_OK;

    struct stat fsb;
    s32 fd = open(filename, O_RDONLY, 0666);
    fstat(fd, &fsb);
    *length = fsb.st_size;

    (*buffer) = malloc(*length);
    read(fd, *buffer, *length);
    printf("read_picture: [%s], size: %d, buf_addr: %p / %p.\n", filename, *length, buffer, *buffer);

    close(fd);

    return res;
}

ret_t sync_write_one_picture_to_file(void *buffer, char8 *filename, u32 length)
{
    ret_t res = (buffer != NULL && length != 0) ? RET_OK : RET_INVALID_OPERATION;

    s32 fd = open(filename, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR, 0666);

    char8 *buf = malloc(length);

    long long memcpy_start_time = get_time_in_microseconds();
    memcpy(buf, buffer, length);
    long long memcpy_end_time = get_time_in_microseconds();
    double memcpy_time = (double)(memcpy_end_time - memcpy_start_time) / 1000;
    printf("\n -- memcpy [%s] time consumption: %f ms.\n\n", filename, memcpy_time);

    write(fd, buf, length);
    printf("write_picture: [%s], size: %d, buf_addr: %p.\n", filename, length, buffer);

    return res;
}