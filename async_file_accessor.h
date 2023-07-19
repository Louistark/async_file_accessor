/***************************************************************************************
 * Project      : async_file_accessor
 * File         : async_file_accessor.h
 * Description  : Define an abstract asynchronous file IO framework and provide an
                  instantiation interface.
 * Author       : Louis Liu
 * Created Date : 2023-7-13
 * Copyright (c) 2023, [Louis.Liu]
 * All rights reserved.
 ***************************************************************************************/

#ifndef __ASYNC_FILE_ACCESSOR_H__
#define __ASYNC_FILE_ACCESSOR_H__

#include "common_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define REQ_LIST_BUFSIZE    1024
#define RETRY_TIMES         2


typedef enum __async_file_accessor_type
{
    ASYNC_FILE_ACCESSOR_AIO = 0,
    ASYNC_FILE_ACCESSOR_MMAP,
    ASYNC_FILE_ACCESSOR_MAX,

} async_file_accessor_type_t;

typedef enum __async_file_access_direction
{
    ASYNC_FILE_ACCESS_READ = 0,
    ASYNC_FILE_ACCESS_WRITE,
    ASYNC_FILE_ACCESS_MAX,

} async_file_access_direction_t;

/// Async file accessor request info struct
typedef struct __async_file_access_request_info
{
    async_file_access_direction_t       direction;              /// read or write
    char8                               fn[MAX_FILE_NAME_LEN];  /// file name
    u32                                 size;                   /// size of data
    u32                                 offset;                 /// file access offset

} async_file_access_request_info_t;

/// Async file accessor request struct
typedef struct __async_file_access_request
{
    async_file_access_request_info_t    info;                   /// request info

} async_file_access_request_t;


typedef struct __async_file_accessor async_file_accessor_t;

typedef ret_t (*async_file_access_get_request_func)(async_file_accessor_t* thiz,
                                                    async_file_access_request_t** pRequest,
                                                    async_file_access_request_info_t* pCreateInfo);

typedef ret_t (*async_file_access_alloc_write_buffer_func)(async_file_accessor_t* thiz,
                                                           async_file_access_request_t* pRequest,
                                                           void** buf);

typedef ret_t (*async_file_access_import_read_buffer_func)(async_file_accessor_t* thiz,
                                                           async_file_access_request_t* pRequest,
                                                           void* buf);

typedef ret_t (*async_file_access_put_request_func)(async_file_accessor_t* thiz,
                                                    async_file_access_request_t* pRequest);

typedef ret_t (*async_file_access_wait_request_func)(async_file_accessor_t* thiz,
                                                     async_file_access_request_t* pRequest,
                                                     u32 timeout_ms);

typedef ret_t (*async_file_access_cancel_request_func)(async_file_accessor_t* thiz,
                                                       async_file_access_request_t* pRequest);

typedef ret_t (*async_file_access_wait_all_request_func)(async_file_accessor_t* thiz,
                                                         u32 timeout_ms);

typedef ret_t (*async_file_access_cancel_all_requests_func)(async_file_accessor_t* thiz);

typedef ret_t (*async_file_access_release_all_requests_func)(async_file_accessor_t* thiz);

struct __async_file_accessor
{
    async_file_accessor_type_t                      type;

    async_file_access_get_request_func              getRequest;
    async_file_access_alloc_write_buffer_func       allocWriteBuf;
    async_file_access_import_read_buffer_func       importReadBuf;
    async_file_access_put_request_func              putRequest;
    async_file_access_wait_request_func             waitRequest;
    async_file_access_cancel_request_func           cancelRequest;
    async_file_access_wait_all_request_func         waitAll;
    async_file_access_cancel_all_requests_func      cancelAll;
    async_file_access_release_all_requests_func     releaseAll;
};


async_file_accessor_t* Async_File_Accessor_Get_Instance(async_file_accessor_type_t type);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __ASYNC_FILE_ACCESSOR_H__ */
