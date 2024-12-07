#define _GNU_SOURCE
#include "Buffer.h"
#include <strings.h>
#include <string.h>

struct Buffer* bufferInit(int capacity) {
    struct Buffer *buffer = (struct Buffer *)malloc(sizeof (struct Buffer));
    buffer->data = (char *) malloc(capacity);
    buffer->capacity = capacity;
    buffer->readPos = buffer->writePos = 0;
    memset(buffer->data, 0, capacity);
    return buffer; 
}

void bufferDestroy(struct Buffer *buffer) {
    if (buffer != NULL) {
        if (buffer->data != NULL) {
            free(buffer->data);
        }
        free(buffer);
    }
}

int bufferReadableSize(struct Buffer *buffer) {
    return buffer->writePos - buffer->readPos;
}

int bufferWriteableSize(struct Buffer *buffer) {
    return buffer->capacity - buffer->writePos;
}

void bufferExtendRoom(struct Buffer *buffer, int size) {
    if (bufferWriteableSize(buffer) >= size) {
        return;
    } else if (buffer->readPos + bufferWriteableSize(buffer) >= size) {
        int readable = bufferReadableSize(buffer);
        memcpy(buffer->data, buffer->data + buffer->readPos, readable);
        buffer->readPos = 0;
        buffer->writePos = readable;
    } else {
        buffer->data = (char *)realloc(buffer->data, buffer->capacity + size);
        memset(buffer->data + buffer->capacity, 0, size);
        buffer->capacity += size;
    }
}

int bufferAppendData(struct Buffer *buffer, const char *data, int size) {
    if (buffer == NULL || data == NULL || size <= 0) {
        return -1;
    }
    bufferExtendRoom(buffer, size);
    memcpy(buffer->data + buffer->writePos, data, size);
    buffer->writePos += size;
    return 0;
}
 
int bufferAppendString(struct Buffer *buffer, const char *data) {
    return bufferAppendData(buffer, data, strlen(data));
}

int bufferSocketRead(struct Buffer *buffer, int fd) {
    struct iovec vec[2];

    int writeable = bufferWriteableSize(buffer);
    vec[0].iov_base = buffer->data + buffer->writePos;
    vec[0].iov_len = writeable;
    vec[1].iov_base = (char *)malloc(40960);
    vec[1].iov_len = 40960;
    int result = readv(fd, vec, 2);
    if (result == -1) {
        return -1;
    }
    if (result <= writeable) {
        buffer->writePos += result;
    } else {
        buffer->writePos = buffer->capacity;
        bufferAppendData(buffer, vec[1].iov_base, result - writeable);
    }
    free(vec[1].iov_base);
    return result;
}

char* bufferFindCRLF(struct Buffer *buffer) {
    // strstr  - 大字符串匹配小字符串，遇到\0结束
    // memmem  - 大数据快匹配子数据块，指定数据块大小
    char *ptr = memmem(buffer->data + buffer->readPos, bufferReadableSize(buffer), "\r\n", 2);
    return ptr;
}

int bufferSendData(struct Buffer *buffer, int socket) {
    int readable = bufferReadableSize(buffer);
    if (readable <= 0) {
        return 0;
    }
    int count = send(socket, buffer->data + buffer->readPos, readable, MSG_NOSIGNAL);
    if (count > 0) {
        buffer->readPos += count;
        usleep(1);
    }
    return count;
}