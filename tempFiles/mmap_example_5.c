#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <pthread.h>

typedef struct {
    int fd;
    size_t length;
    off_t offset;
    void* buffer;
    void (*callback)(int result, void* buffer, size_t length);
} AsyncReadRequest;

// 异步读取线程函数
void* asyncReadThread(void* param) {
    AsyncReadRequest* request = (AsyncReadRequest*)param;

    // 映射文件到内存
    void* mmapAddr = mmap(NULL, request->length, PROT_READ, MAP_PRIVATE, request->fd, request->offset);
    if (mmapAddr == MAP_FAILED) {
        request->callback(-1, NULL, 0);  // 读取失败
        return NULL;
    }

    // 复制数据到缓冲区
    memcpy(request->buffer, mmapAddr, request->length);

    // 解除文件内存映射
    munmap(mmapAddr, request->length);

    request->callback(0, request->buffer, request->length);  // 读取完成

    return NULL;
}

// 异步读取函数
void asyncRead(int fd, size_t length, off_t offset, void* buffer, void (*callback)(int result, void* buffer, size_t length)) {
    // 创建异步读取请求
    AsyncReadRequest* request = (AsyncReadRequest*)malloc(sizeof(AsyncReadRequest));
    if (request == NULL) {
        callback(-1, NULL, 0);
        return;
    }

    request->fd = fd;
    request->length = length;
    request->offset = offset;
    request->buffer = buffer;
    request->callback = callback;

    // 创建异步读取线程并启动
    pthread_t thread;
    int result = pthread_create(&thread, NULL, asyncReadThread, request);
    if (result != 0) {
        callback(-1, NULL, 0);
        free(request);
        return;
    }

    pthread_detach(thread);
}

// 示例回调函数
void readCallback(int result, void* buffer, size_t length) {
    if (result == 0) {
        printf("Read successful: %.*s\n", (int)length, (char*)buffer);
    } else {
        fprintf(stderr, "Read failed\n");
    }

    // 释放缓冲区
    free(buffer);
}

int main() {
    int fd = open("file.txt", O_RDONLY);
    if (fd == -1) {
        perror("open");
        return 1;
    }

    char* buffer = (char*)malloc(4096);
    if (buffer == NULL) {
        perror("malloc");
        close(fd);
        return 1;
    }

    asyncRead(fd, 10, 0, buffer, readCallback);

    // 主线程的其他操作

    close(fd);

    // 延时等待异步读取完成

    return 0;
}