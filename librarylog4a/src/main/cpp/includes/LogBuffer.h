//
// Created by pqpo on 2017/11/16.
//

#ifndef LOG4A_LOGBUFFER_H
#define LOG4A_LOGBUFFER_H

#include <string>
#include <math.h>
#include <unistd.h>
#include <sys/mman.h>
#include <thread>
#include<vector>
#include <mutex>
#include <condition_variable>
#include "AsyncFileFlush.h"
#include "FlushBuffer.h"
#include "LogBufferHeader.h"
#include <zlib.h>

using namespace log_header;

class LogBuffer {
public:
    LogBuffer(char* ptr, long file_max_size, size_t capacity);
    ~LogBuffer();

    void initData(char *log_path, size_t log_path_len, bool is_compress);
    size_t length();
    size_t append(const char* log, size_t len);
    void release();
    size_t emptySize();
    char *getLogPath();
    void setAsyncFileFlush(AsyncFileFlush *fileFlush);
    void async_flush();
    void async_flush(AsyncFileFlush *fileFlush);
    void async_flush(AsyncFileFlush *fileFlush, LogBuffer *releaseThis);
    void changeLogPath(char *log_path);

    void checkFileSize();
public:
    bool map_buffer = true; // 是否使用的是 mmap，true 是，false mmap创建失败，使用内存

private:
    void clear();
    void setLength(size_t len);
    bool initCompress(bool compress);
    bool openSetLogFile(const char *log_path);

    FILE* log_file = nullptr;
    long file_max_size = 10 * 1024 * 1024; // 日志文件最大大小 单位 byte, 默认大小 10 m
    AsyncFileFlush *fileFlush = nullptr;
    char* const buffer_ptr = nullptr; // 缓存文件指针
    char* data_ptr = nullptr;
    char* write_ptr = nullptr;

    size_t buffer_size = 0;
    std::recursive_mutex log_mtx;

    LogBufferHeader logHeader;
    z_stream zStream;
    bool is_compress = false;

};


#endif //LOG4A_LOGBUFFER_H
