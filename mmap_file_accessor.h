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

#include "common_types.h"
#include "async_file_accessor.h"

#ifdef __cplusplus
extern "C" {
#endif

#define THREAD_POOL_SIZE 10

typedef enum __request_stat
{
    REQUEST_STAT_INIT               = 0,                    /// request initialized
    REQUEST_STAT_SUBMITTED,                                 /// request submitted to task queue
    REQUEST_STAT_IOSUCCESS,                                 /// request read/write success
    REQUEST_STAT_IOFAIL,                                    /// request read/write fail
    REQUEST_STAT_CANCEL,                                    /// request canceled
    REQUEST_STAT_MAX,

} request_stat_t;

/// mmap request struct (inherited from __async_file_access_request)
typedef struct __mmap_request
{
    async_file_access_request_t     parent;

    s32                             fd;                     /// file descriptor
    struct stat                     fsb;                    /// file state block
    void                           *buf;                    /// data buffer
    u32                             nbytes;                 /// data length
    u32                             offset;                 /// file operate offset

    bool                            isValid;                /// check whether request valid
    bool                            isAlloced;              /// whether buffer is alloced by mmap
    request_stat_t                  status;                 /// request status
    pthread_mutex_t                 lock;                   /// accessDone status lock
    pthread_cond_t                  isFinished;             /// request done or timeout

} mmap_request_t;

/// struct define a task
typedef struct __task
{
    void                           *(*function)(void *);    /// pointer to task function
    void                           *argument;               /// pointer to task arguments
    bool                            is_sentinel;            /// whether current task is sentinel

} task_t;

/// struct define a task queue
typedef struct __task_queue
{
    task_t                        **request_queue;          /// pointer to task queue
    u32                             head;                   /// index of oldest unexecuted task
    u32                             tail;                   /// index if latest task
    u32                             todoCnt;                /// count of all unexecutd tasks
    u32                             totoalCnt;              /// count of all tasks
    pthread_mutex_t                 lock;                   /// task queue lock
    pthread_cond_t                  not_empty;              /// task queue not empty condition

} task_queue_t;

/// struct define a thread pool information
typedef struct __thread_pool_info
{
    u32                             maxThreadIdx;           /// max thread index
    u32                             busyThreadNum;          /// count of busy threads
    u32                             idleThreadNum;          /// count of idle threads
    u32                             aliveThreadNum;         /// count of alive threads
    bool                            isInitialized;          /// whether thread pool is initialized
    bool                            isRunning;              /// whether thread pool is running
    pthread_mutex_t                 lock;                   /// thread pool infomation lock

} thread_pool_info_t;

/// struct define a thread pool
typedef struct __thread_pool
{
    pthread_t                       thread_pool[THREAD_POOL_SIZE];
    task_queue_t                    task_queue;             /// thread pool task queue
    thread_pool_info_t              info;                   /// thread pool information

} thread_pool_t;

/// mmap file accessor struct (inherited from __async_file_accessor)
typedef struct __mmap_file_accessor
{
    async_file_accessor_t           parent;

    thread_pool_t                   distributor;            /// distributor to process mmap requests

} mmap_file_accessor_t;

/// Struct define timeout params
typedef struct __timeout_args
{
    u32                             timeout_ms;             /// timeout millisecond
    request_stat_t                 *status;                 /// pointer to request status
    pthread_mutex_t                *lock;                   /// pointer to status lock
    pthread_cond_t                 *cond;                   /// pointer to status condition

} timeout_args_t;

/// Acqiure single static mmap accessor
mmap_file_accessor_t* MMAP_File_Accessor_Get_Instance();


#ifdef __cplusplus
}//extern "C" {
#endif

#endif /* __MMAP_FILE_ACCESSOR_H__ */
