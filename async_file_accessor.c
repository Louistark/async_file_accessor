/***************************************************************************************
 * Project      : async_file_accessor
 * File         : async_file_accessor.c
 * Description  : Define an abstract asynchronous file IO framework and provide an
                  instantiation interface.
 * Author       : Louis Liu
 * Created Date : 2023-7-13
 * Copyright (c) 2023, [Louis.Liu]
 * All rights reserved.
 ***************************************************************************************/

#include "async_file_accessor.h"
#include "aio_file_accessor.h"
#include "mmap_file_accessor.h"
// #include "mmap_file_accessor.h"

async_file_accessor_t* Async_File_Accessor_Get_Instance(async_file_accessor_type_t type)
{
    async_file_accessor_t* pFileAcessor = NULL;

    switch (type)
    {
        case ASYNC_FILE_ACCESSOR_AIO:
        {
            pFileAcessor = (async_file_accessor_t *)AIO_File_Accessor_Get_Instance();
            break;
        }
        case ASYNC_FILE_ACCESSOR_MMAP:
        {
            pFileAcessor = (async_file_accessor_t *)MMAP_File_Accessor_Get_Instance();
            break;
        }
        default:
        {
            break;
        }
    }

    return pFileAcessor;
}