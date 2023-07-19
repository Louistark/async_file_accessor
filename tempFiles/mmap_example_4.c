#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

typedef struct {
    char* buffer;
    size_t length;
    off_t offset;
    int completed; // 标记读取是否完成
} AsyncReadRequest;

void readComplete(AsyncReadRequest* request) {
    printf("Read complete: %.*s\n", (int)request->length, request->buffer);
    free(request->buffer);
    free(request);
}

void asyncRead(int fd, size_t length, off_t offset) {
    struct stat sb;
    fstat(fd, &sb);

    // 检查请求是否超过文件大小
    if (offset + length > sb.st_size) {
        fprintf(stderr, "Invalid read request\n");
        return;
    }

    // 分配内存
    char* buffer = malloc(length);
    if (buffer == NULL) {
        perror("malloc");
        return;
    }

    // 创建异步读取请求
    AsyncReadRequest* request = malloc(sizeof(AsyncReadRequest));
    if (request == NULL) {
        perror("malloc");
        free(buffer);
        return;
    }

    request->buffer = buffer;
    request->length = length;
    request->offset = offset;
    request->completed = 0;

    // 映射文件到内存
    void* mmapAddr = mmap(NULL, length, PROT_READ, MAP_PRIVATE, fd, offset);
    if (mmapAddr == MAP_FAILED) {
        perror("mmap");
        free(buffer);
        free(request);
        return;
    }

    // 异步读取：将数据复制到分配的缓冲区中
    memcpy(buffer, mmapAddr, length);

    // 解除文件内存映射
    munmap(mmapAddr, length);

    // 启动异步读取完成回调
    readComplete(request);
}

int main() {
    int fd = open("file.txt", O_RDONLY);
    if (fd == -1) {
        perror("open");
        return 1;
    }

    // 异步读取文件
    asyncRead(fd, 10, 0);

    // 主线程的其他操作

    close(fd);

    return 0;
}