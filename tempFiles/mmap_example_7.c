#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>

typedef struct
{
    int fd;
    size_t length;
    off_t offset;
    void *buffer;
    int completed; // 标记读写是否完成
    pthread_mutex_t mutex;
    pthread_cond_t cond;
} AsyncIORequest;

// 异步读取线程函数
void *asyncReadThread(void *param)
{
    AsyncIORequest *request = (AsyncIORequest *)param;

    // 映射文件到内存
    printf("异步读取线程函数: 映射文件到内存.\n");
    void *mmapAddr = mmap(NULL, request->length, PROT_READ, MAP_PRIVATE, request->fd, request->offset);
    if (mmapAddr == MAP_FAILED)
    {
        pthread_mutex_lock(&request->mutex);
        request->completed = -1; // 读取失败
        pthread_mutex_unlock(&request->mutex);

        pthread_cond_signal(&request->cond);

        return NULL;
    }

    // 复制数据到缓冲区
    printf("异步读取线程函数: 复制数据到缓冲区.\n");
    memcpy(request->buffer, mmapAddr, request->length);

    // 解除文件内存映射
    printf("异步读取线程函数: 解除文件内存映射.\n");
    munmap(mmapAddr, request->length);

    pthread_mutex_lock(&request->mutex);
    request->completed = 1; // 读取完成
    printf("异步读取线程函数: 读取完成.\n");
    printf("读取内容： %s\n", request->buffer);
    pthread_mutex_unlock(&request->mutex);

    pthread_cond_signal(&request->cond);
    printf("异步读取线程函数: pthread_cond_signal.\n");

    return NULL;
}

// 异步写入线程函数
void *asyncWriteThread(void *param)
{
    AsyncIORequest *request = (AsyncIORequest *)param;

    // 映射文件到内存
    printf("异步写入线程函数: 映射文件到内存.\n");
    void *mmapAddr = mmap(NULL, request->length, PROT_WRITE, MAP_SHARED, request->fd, request->offset);
    if (mmapAddr == MAP_FAILED)
    {
        pthread_mutex_lock(&request->mutex);
        request->completed = -1; // 写入失败
        pthread_mutex_unlock(&request->mutex);

        pthread_cond_signal(&request->cond);

        return NULL;
    }

    // 复制数据到缓冲区
    printf("异步写入线程函数: 复制数据到缓冲区.\n");
    memcpy(mmapAddr, request->buffer, request->length);

    // 刷新文件到磁盘
    printf("异步写入线程函数: 刷新文件到磁盘.\n");
    if (msync(mmapAddr, request->length, MS_SYNC) == -1)
    {
        pthread_mutex_lock(&request->mutex);
        request->completed = -1; // 写入失败
        pthread_mutex_unlock(&request->mutex);

        pthread_cond_signal(&request->cond);

        return NULL;
    }

    // 解除文件内存映射
    printf("异步写入线程函数: 解除文件内存映射.\n");
    munmap(mmapAddr, request->length);

    pthread_mutex_lock(&request->mutex);
    request->completed = 1; // 写入完成
    printf("异步写入线程函数: 写入完成.\n");
    pthread_mutex_unlock(&request->mutex);

    pthread_cond_signal(&request->cond);
    printf("异步写入线程函数: pthread_cond_signal.\n");

    return NULL;
}

// 异步读取函数
void asyncRead(int fd, size_t length, off_t offset, void *buffer)
{
    // 创建异步读取请求
    AsyncIORequest *request = (AsyncIORequest *)malloc(sizeof(AsyncIORequest));
    if (request == NULL)
    {
        perror("malloc");
        return;
    }

    // 初始化请求结构体
    request->fd = fd;
    request->length = length;
    request->offset = offset;
    request->buffer = buffer;
    request->completed = 0;

    // 创建互斥锁和条件变量
    pthread_mutex_init(&request->mutex, NULL);
    pthread_cond_init(&request->cond, NULL);

    // 创建异步读取线程并启动
    pthread_t thread;
    printf("创建异步读取线程并启动.\n");
    int result = pthread_create(&thread, NULL, asyncReadThread, request);
    printf("创建异步读取线程并启动. ---- done\n");
    if (result != 0)
    {
        perror("pthread_create");
        free(request);
        return;
    }

    pthread_detach(thread);
}

// 异步写入函数
void asyncWrite(int fd, size_t length, off_t offset, void *buffer)
{
    // 创建异步写入请求
    AsyncIORequest *request = (AsyncIORequest *)malloc(sizeof(AsyncIORequest));
    if (request == NULL)
    {
        perror("malloc");
        return;
    }

    // 初始化请求结构体
    request->fd = fd;
    request->length = length;
    request->offset = offset;
    request->buffer = buffer;
    request->completed = 0;

    // 创建互斥锁和条件变量
    pthread_mutex_init(&request->mutex, NULL);
    pthread_cond_init(&request->cond, NULL);

    // 创建异步写入线程并启动
    pthread_t thread;
    printf("创建异步写入线程并启动.\n");
    int result = pthread_create(&thread, NULL, asyncWriteThread, request);
    printf("创建异步写入线程并启动. ---- done\n");
    if (result != 0)
    {
        perror("pthread_create");
        free(request);
        return;
    }

    pthread_detach(thread);
}

// 等待异步 IO 请求完成
void waitForAsyncIO(AsyncIORequest *request)
{
    pthread_mutex_lock(&request->mutex);
    while (request->completed == 0)
    {
        pthread_cond_wait(&request->cond, &request->mutex);
    }
    pthread_mutex_unlock(&request->mutex);
}

// 释放异步 IO 请求
void freeAsyncIORequest(AsyncIORequest *request)
{
    pthread_mutex_destroy(&request->mutex);
    pthread_cond_destroy(&request->cond);
    free(request);
}

int main(int argc, char *argv[])
{
    if (argc < 3)
    {
        printf("Usage: %s <input_file> <output_file>\n", argv[0]);
        exit(1);
    }

    // 打开输入文件和输出文件
    printf("打开输入文件和输出文件.\n");
    int inputFd = open(argv[1], O_RDONLY);
    if (inputFd < 0)
    {
        perror("open input file");
        exit(1);
    }

    int outputFd = open(argv[2], O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
    if (outputFd < 0)
    {
        perror("open output file");
        exit(1);
    }

    // 获取输入文件长度
    printf("获取输入文件长度.\n");
    off_t fileSize = lseek(inputFd, 0, SEEK_END);
    lseek(inputFd, 0, SEEK_SET);

    // 创建输出文件
    printf("创建输出文件.\n");
    if (ftruncate(outputFd, fileSize) == -1)
    {
        perror("ftruncate");
        exit(1);
    }

    // 映射输出文件到内存
    printf("映射输出文件到内存.\n");
    void *outputBuffer = mmap(NULL, fileSize, PROT_WRITE, MAP_SHARED, outputFd, 0);
    if (outputBuffer == MAP_FAILED)
    {
        perror("mmap");
        exit(1);
    }

    // 异步读取输入文件到内存
    printf("异步读取输入文件到内存.\n");
    void *inputBuffer = malloc(fileSize);
    if (inputBuffer == NULL)
    {
        perror("malloc");
        exit(1);
    }

    AsyncIORequest *readRequest = (AsyncIORequest *)malloc(sizeof(AsyncIORequest));
    if (readRequest == NULL)
    {
        perror("malloc");
        exit(1);
    }

    readRequest->fd = inputFd;
    readRequest->length = fileSize;
    readRequest->offset = 0;
    readRequest->buffer = inputBuffer;
    readRequest->completed = 0;

    pthread_mutex_init(&readRequest->mutex, NULL);
    pthread_cond_init(&readRequest->cond, NULL);

    pthread_t readThread;
    printf("创建异步读取线程并启动. ---- start\n");
    int result = pthread_create(&readThread, NULL, asyncReadThread, readRequest);
    printf("创建异步读取线程并启动. ---- done\n");
    if (result != 0)
    {
        perror("pthread_create");
        free(inputBuffer);
        free(readRequest);
        exit(1);
    }

    // 异步写入输出文件
    printf("异步写入输出文件.\n");
    AsyncIORequest *writeRequest = (AsyncIORequest *)malloc(sizeof(AsyncIORequest));
    if (writeRequest == NULL)
    {
        perror("malloc");
        exit(1);
    }

    writeRequest->fd = outputFd;
    writeRequest->length = fileSize;
    writeRequest->offset = 0;
    writeRequest->buffer = outputBuffer;
    writeRequest->completed = 0;

    pthread_mutex_init(&writeRequest->mutex, NULL);
    pthread_cond_init(&writeRequest->cond, NULL);

    pthread_t writeThread;
    printf("创建异步写入线程并启动. ---- start\n");
    result = pthread_create(&writeThread, NULL, asyncWriteThread, writeRequest);
    printf("创建异步写入线程并启动. ---- done\n");
    if (result != 0)
    {
        perror("pthread_create");
        free(inputBuffer);
        free(readRequest);
        free(writeRequest);
        exit(1);
    }

    // 等待异步 IO 请求完成
    printf("等待异步 IO 请求完成 waitForAsyncIO.\n");
    waitForAsyncIO(readRequest);
    printf("等待异步 IO 请求完成 waitForAsyncIO. ---- done\n");
    printf("等待异步 IO 请求完成. waitForAsyncIO.\n");
    waitForAsyncIO(writeRequest);
    printf("等待异步 IO 请求完成. waitForAsyncIO. ---- done\n");

    // 检查异步 IO 请求是否成功
    printf("检查异步 IO 请求是否成功.\n");
    if (readRequest->completed == -1)
    {
        perror("asyncRead");
        exit(1);
    }

    if (writeRequest->completed == -1)
    {
        perror("asyncWrite");
        exit(1);
    }

    // 释放异步 IO 请求
    printf("释放异步 IO 请求.\n");
    freeAsyncIORequest(readRequest);
    freeAsyncIORequest(writeRequest);

    // 解除内存映射和关闭文件
    printf("解除内存映射和关闭文件.\n");
    munmap(outputBuffer, fileSize);
    free(inputBuffer);
    close(inputFd);
    close(outputFd);

    return 0;
}
