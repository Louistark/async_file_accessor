/***************************************************************************************
 * Project      : async_file_accessor
 * File         : mmap_file_accessor.h
 * Description  : Implement abstract async file accessor interface by mmap
 * Author       : Louis Liu
 * Created Date : 2023-7-13
 * Copyright (c) 2023, [Louis.Liu]
 * All rights reserved.
 ***************************************************************************************/

#ifndef __MMAP_FILE_ACCESSOR_H__
#define __MMAP_FILE_ACCESSOR_H__

#include <sys/mman.h>
#include "common_types.h"
#include "async_file_accessor.h"

#ifdef __cplusplus
extern "C" {
#endif

/// mmap request struct (inherited from __async_file_access_request)
typedef struct __mmap_request
{
    async_file_access_request_t     parent;

    s32                             fd;         /// file descriptor
    struct stat                     fsb;        /// file state block
    void                           *buf;        /// data buffer
    u32                             nbytes;     /// data length
    u32                             offset;     /// file operate offset

    bool                            isValid;    /// check whether request valid
    bool                            isAlloced;  /// whether buffer is alloced by mmap
    bool                            submitted;  /// whether request submitted
    bool                            accessDone; /// whether access request done
    bool                            canceled;   /// whether access canceled

} mmap_request_t;


/// mmap file accessor struct (inherited from __async_file_accessor)
typedef struct __mmap_file_accessor
{
    async_file_accessor_t           parent;

    mmap_request_t                **req_list;   /// mmap request list
    u32                             req_count;  /// accumulative requests count

} mmap_file_accessor_t;


/// Acqiure single static mmap accessor
mmap_file_accessor_t* MMAP_File_Accessor_Get_Instance();


#ifdef __cplusplus
}//extern "C" {
#endif

#endif /* __MMAP_FILE_ACCESSOR_H__ */
