/***************************************************************************************
 * Project      : async_file_accessor
 * File         : aio_file_accessor.h
 * Description  : Implement abstract async file accessor interface by aio
 * Author       : Louis Liu
 * Created Date : 2023-7-13
 * Copyright (c) 2023, [Louis.Liu]
 * All rights reserved.
 ***************************************************************************************/

#ifndef __AIO_FILE_ACCESSOR_H__
#define __AIO_FILE_ACCESSOR_H__

#include <aio.h>
#include "common_types.h"
#include "async_file_accessor.h"

#ifdef __cplusplus
extern "C" {
#endif

/// aio request struct (inherited from __async_file_access_request)
typedef struct __aio_request
{
    async_file_access_request_t     parent;

    s32                             fd;         /// file descriptor
    struct stat                     fsb;        /// file state block
    struct aiocb                    cb;         /// AIO control block
    void                           *buf;        /// data buffer

    bool                            isValid;    /// check whether request valid
    bool                            isAlloced;  /// whether buffer is alloced by aio
    bool                            submitted;  /// whether request submitted
    bool                            accessDone; /// whether access request done
    bool                            canceled;   /// whether access canceled

} aio_request_t;


/// aio file accessor struct (inherited from __async_file_accessor)
typedef struct __aio_file_accessor
{
    async_file_accessor_t           parent;

    aio_request_t                 **req_list;   /// aio request list
    u32                             req_count;  /// accumulative requests count

} aio_file_accessor_t;


/// Acqiure single static aio accessor
aio_file_accessor_t* AIO_File_Accessor_Get_Instance();


#ifdef __cplusplus
}//extern "C" {
#endif

#endif /* __AIO_FILE_ACCESSOR_H__ */
