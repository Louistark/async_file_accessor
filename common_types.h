/***************************************************************************************
 * Project      : async_file_accessor
 * File         : common_types.h
 * Description  : Define common types.
 * Author       : Louis Liu
 * Created Date : 2023-7-13
 * Copyright (c) 2023, [Louis.Liu]
 * All rights reserved.
 ***************************************************************************************/

#ifndef __COMMON_TYPES_H__
#define __COMMON_TYPES_H__

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>


#ifdef __cplusplus
extern "C" {
#endif

typedef char     char8;
typedef int8_t   s8;
typedef int16_t  s16;
typedef int32_t  s32;
typedef int64_t  s64;
typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef float    f32;
typedef double   f64;
typedef unsigned long ulong;

#ifndef BOOL
#define BOOL u8
#endif

#ifndef TRUE
#define TRUE  1U
#endif

#ifndef FALSE
#define FALSE 0U
#endif

#ifndef NULL
#define NULL  0U
#endif

typedef s32 ret_t;

enum {
    RET_OK                  = 0,
    RET_UNKNOWN_ERROR       = (-2147483647-1), // INT32_MIN value

    RET_BUSY                = -EBUSY,
    RET_MAX_USERS           = -EUSERS,
    RET_NO_MEMORY           = -ENOMEM,
    RET_INVALID_OPERATION   = -ENOSYS,
    RET_BAD_VALUE           = -EINVAL,
    RET_NAME_NOT_FOUND      = -ENOENT,
    RET_PERMISSION_DENIED   = -EPERM,
    RET_NO_INIT             = -ENODEV,
    RET_ALREADY_EXISTS      = -EEXIST,
    RET_DEAD_OBJECT         = -EPIPE,
    RET_BAD_INDEX           = -EOVERFLOW,
    RET_NOT_ENOUGH_DATA     = -ENODATA,
    RET_WOULD_BLOCK         = -EWOULDBLOCK,
    RET_TIMED_OUT           = -ETIMEDOUT,
    RET_UNKNOWN_TRANSACTION = -EBADMSG,
};

#define STR_NAME_MAX_LEN    (64)
#define MAX_FILE_NAME_LEN   (511)

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(a)      (sizeof(a) / sizeof((a)[0]))
#endif

#ifndef offsetof
#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)
#endif

#ifndef container_of
#define container_of(ptr, type, member) ({                                  \
                     const typeof( ((type *)0)->member) *__mptr = (ptr);    \
                     (type *) ( (char *)__mptr - offsetof(type, member));})

#endif

#define UNUSED(x) ((void)(x))

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __COMMON_TYPES_H__ */