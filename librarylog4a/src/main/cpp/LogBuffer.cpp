//
// Created by pqpo on 2017/11/16.
//

#include "includes/LogBuffer.h"
#include "includes/log.h"

LogBuffer::LogBuffer(char *ptr, long file_max_size, size_t buffer_size):
        buffer_ptr(ptr),
        buffer_size(buffer_size),
        logHeader(buffer_ptr, buffer_size) {
    if (logHeader.isAvailable()) {
        data_ptr = (char *) logHeader.ptr();
        write_ptr = (char *) logHeader.write_ptr();
        if(logHeader.getIsCompress()) {
            initCompress(true);
        }
        char* log_path = getLogPath();
        if(log_path != nullptr) {
            openSetLogFile(log_path);
            delete[] log_path;
        }
        // 设置日志文件最大的大小，文件最小需要大于buffer的大小
        if (file_max_size < buffer_size) {
            file_max_size = (long) buffer_size;
        }
        this->file_max_size = file_max_size;
    }
    memset(&zStream, 0, sizeof(zStream));
}

LogBuffer::~LogBuffer() {
    release();
}

size_t LogBuffer::length() {
    return write_ptr - data_ptr;
}

void LogBuffer::setLength(size_t len) {
    logHeader.setLogLen(len);
}

size_t LogBuffer::append(const char *log, size_t len) {
    std::lock_guard<std::recursive_mutex> lck_append(log_mtx);

    if (length() == 0) {
        initCompress(is_compress);
    }

    size_t freeSize = emptySize();
    size_t writeSize = 0;
    if (is_compress) {
        zStream.avail_in = (uInt)len;
        zStream.next_in = (Bytef*)log;

        zStream.avail_out = (uInt)freeSize;
        zStream.next_out = (Bytef*)write_ptr;

        if (Z_OK != deflate(&zStream, Z_SYNC_FLUSH)) {
            return 0;
        }

        writeSize = freeSize - zStream.avail_out;
    } else {
        writeSize = len <= freeSize ? len : freeSize;
        memcpy(write_ptr, log, writeSize);
    }
    write_ptr += writeSize;
    setLength(length());
    return writeSize;
}

void LogBuffer::setAsyncFileFlush(AsyncFileFlush *_fileFlush) {
    fileFlush = _fileFlush;
}

void LogBuffer::async_flush() {
    async_flush(fileFlush);
}

void LogBuffer::async_flush(AsyncFileFlush *fileFlush) {
    async_flush(fileFlush, nullptr);
}

void LogBuffer::async_flush(AsyncFileFlush *fileFlush, LogBuffer *releaseThis) {
    if(fileFlush == nullptr) {
        if (releaseThis != nullptr) {
            delete releaseThis;
        }
        LOGE("asyncFileFlush is null.");
        return;
    }
    std::lock_guard<std::recursive_mutex> lck_clear(log_mtx);
    if (length() > 0) {
        if (is_compress && Z_NULL != zStream.state) {
            deflateEnd(&zStream);
        }

        checkFileSize();

        FlushBuffer* flushBuffer = new FlushBuffer(log_file);
        LOGD("==##async_flush data_ptr %s", data_ptr);
        flushBuffer->write(data_ptr, length());
        flushBuffer->releaseThis(releaseThis);
        clear();
        fileFlush->async_flush(flushBuffer);
    } else if (releaseThis != nullptr) {
        delete releaseThis;
    }
}

void LogBuffer::checkFileSize() {
    // 将文件指针移动到文件末尾
    fseek(log_file, 0, SEEK_END);
    // 求出当前文件指针距离文件开始的字节数 (对于超过4G大小的文件，这个方法获取文件长度就会溢出了)
    long size = ftell(log_file);

    LOGW("size of file %ld, max is %ld", size, file_max_size);
    if (size > file_max_size && file_max_size > 0) {
        // 清空文件
        FILE *pFile = fopen(logHeader.getLogPath(), "wb");
        char *clear = const_cast<char *>("清空文件中...\r\n");
        fwrite(clear, sizeof(char), strlen(clear), pFile);
        fclose(pFile);

        LOGE("清空文件 %s", logHeader.getLogPath());
    }
}

void LogBuffer::clear() {
    std::lock_guard<std::recursive_mutex> lck_clear(log_mtx);
    write_ptr = data_ptr;
    memset(write_ptr, '\0', emptySize());
    setLength(length());
}

void LogBuffer::release() {
    std::lock_guard<std::recursive_mutex> lck_release(log_mtx);
    if (is_compress && Z_NULL != zStream.state) {
        deflateEnd(&zStream);
    }
    if(map_buffer) {
        munmap(buffer_ptr, buffer_size);
    } else {
        delete[] buffer_ptr;
    }
    if(log_file != nullptr) {
        fclose(log_file);
    }
}

size_t LogBuffer::emptySize() {
    return buffer_size - (write_ptr - buffer_ptr);
}

void LogBuffer::initData(char *log_path, size_t log_path_len, bool is_compress) {
    std::lock_guard<std::recursive_mutex> lck_release(log_mtx);
    memset(buffer_ptr, '\0', buffer_size);

    log_header::Header header;
    header.magic = kMagicHeader;
    header.log_path_len = log_path_len;
    header.log_path = log_path;
    header.log_len = 0;
    header.isCompress = is_compress;

    logHeader.initHeader(header);
    initCompress(is_compress);

    data_ptr = (char *) logHeader.ptr();
    write_ptr = (char *) logHeader.write_ptr();

    bool rst = openSetLogFile(log_path);
    LOGD("创建日志文件 %d", rst);
}

char *LogBuffer::getLogPath() {
    return logHeader.getLogPath();
}

bool LogBuffer::initCompress(bool compress) {
    is_compress = compress;
    if (is_compress) {
        zStream.zalloc = Z_NULL;
        zStream.zfree = Z_NULL;
        zStream.opaque = Z_NULL;
        return Z_OK == deflateInit2(&zStream, Z_BEST_COMPRESSION, Z_DEFLATED, -MAX_WBITS, MAX_MEM_LEVEL, Z_DEFAULT_STRATEGY);
    }
    return false;
}

bool LogBuffer::openSetLogFile(const char *log_path) {
    if (log_path != nullptr) {
        // ab+：向二进制文件末添加数据，允许读；
        FILE* _file_log = fopen(log_path, "ab+");
        if(_file_log != NULL) {
            log_file = _file_log;
            return true;
        }
    }
    return false;
}

void LogBuffer::changeLogPath(char *log_path) {
    if(log_file != nullptr) {
        async_flush();
    }
    initData(log_path, strlen(log_path), is_compress);
}






