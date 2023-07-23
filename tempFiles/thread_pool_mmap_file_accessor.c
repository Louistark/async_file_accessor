#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define REQ_LIST_BUFSIZE    1024
#define RETRY_TIMES         2
#define STR_NAME_MAX_LEN    64
#define MAX_FILE_NAME_LEN   511

typedef struct __mmap_request mmap_request_t;

mmap_request_t    **req_list;   /// mmap request list
int                 req_count;  /// accumulative requests count


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
    char                                fn[MAX_FILE_NAME_LEN];  /// file name
    int                                 size;                   /// size of data
    int                                 offset;                 /// file access offset

} async_file_access_request_info_t;

/// Async file accessor request struct
typedef struct __async_file_access_request
{
    async_file_access_request_info_t    info;                   /// request info

} async_file_access_request_t;

/// mmap request struct (inherited from __async_file_access_request)
struct __mmap_request
{
    async_file_access_request_t     parent;

    int                             fd;         /// file descriptor
    struct stat                     fsb;        /// file state block
    void                           *buf;        /// data buffer
    int                             nbytes;     /// data length
    int                             offset;     /// file operate offset

    bool                            isValid;    /// check whether request valid
    bool                            isAlloced;  /// whether buffer is alloced by mmap
    bool                            submitted;  /// whether request submitted
    bool                            accessDone; /// whether access request done
    bool                            canceled;   /// whether access canceled

};

// mmap 异步写入测试
void mmap_write_once();
// mmap 异步读取测试
void mmap_read_once();


int main(int argc, char *argv[])
{
    if (argc < 3)
    {
        printf("Usage: %s <input_file> <output_file>\n", argv[0]);
        exit(1);
    }

    mmap_read_once();
    mmap_write_once();

    return 0;
}

void mmap_write_once()
{
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