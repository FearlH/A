#ifndef BUFFER_H
#define BUFFER_H

#include "base/StringPiece.h"
#include "Endian.h"
#include "base/Types.h"

#include <vector>
#include <algorithm>

#include "Endian.h"
#include "base/StringPiece.h"
#include "base/Types.h"

#include <assert.h>
#include <string.h>
#include <unistd.h>

#include <vector>
#include <algorithm>
namespace m2
{
    namespace net
    {
        class Buffer
        {
        public:
            static const size_t kCheapPrepend = 8;   //初始化的prepend大小
            static const size_t kInitialSize = 1024; //初始化的容量大小(size=kCheapPrepend+kInitialSize)

            explicit Buffer(int initSize = kInitialSize)
                : buffer_(kCheapPrepend + kInitialSize),
                  readerIndex_(kCheapPrepend),
                  writerIndex_(kCheapPrepend)
            {
            }

            void swap(Buffer &other)
            {
                std::swap(buffer_, other.buffer_);
                std::swap(readerIndex_, other.readerIndex_);
                std::swap(writerIndex_, other.writerIndex_);
            }

            size_t readableBytes() const { return writerIndex_ - readerIndex_; }
            size_t writableBytes() const { return buffer_.size() - writerIndex_; }
            size_t prependableBytes() const { return readerIndex_; };

            const char *peek() const
            {
                return begin() + readerIndex_;
            }

            const char *findCRLF() const
            {
                const char *posCRLF = std::search(peek(), beginWrite(), KCRLF, KCRLF + 2);
                return posCRLF == beginWrite() ? nullptr : posCRLF; //没找到返回的是尾后指针
            }

            const char *findCRLF(const char *start) const
            {
                assert(start >= peek());
                assert(start <= beginWrite());
                const char *posCRLF = std::search(start, beginWrite(), KCRLF, KCRLF + 2);
                return posCRLF == beginWrite() ? nullptr : posCRLF;
            }

            const char *findEOL() const
            {
                const void *eol = memchr(peek(), '\n', readableBytes());
                return static_cast<const char *>(eol);
            }

            const char *findEOL(const char *start) const
            {
                assert(peek() <= start);
                assert(start <= beginWrite());
                const void *eol = memchr(start, '\n', beginWrite() - start);
                return static_cast<const char *>(eol);
            }

            char *beginWrite() { return begin() + writerIndex_; }

            const char *beginWrite() const { return begin() + writerIndex_; }

            void hasWriten(size_t len)
            {
                assert(len <= writableBytes());
                writerIndex_ += len;
            }

            // retrieve系列相当于是读取了一些数据之后需要移动readerIndex_的位置
            void retrieve(size_t len)
            {
                assert(len <= readableBytes());
                if (len < readableBytes())
                {
                    readerIndex_ += len;
                }
                else
                {
                    retrieveAll();
                }
            }

            void retrieveUntil(const char *end)
            {
                assert(peek() <= end);
                assert(end <= beginWrite());
                retrieve(end - peek());
            }

            void retrieveInt64()
            {
                retrieve(sizeof(int64_t));
            }

            void retrieveInt32()
            {
                retrieve(sizeof(int32_t));
            }

            void retrieveInt16()
            {
                retrieve(sizeof(int16_t));
            }

            void retrieveInt8()
            {
                retrieve(sizeof(int8_t));
            }

            void retrieveAll()
            {
                readerIndex_ = kCheapPrepend;
                writerIndex_ = kCheapPrepend;
            }

            std::string retrieveAllAsString()
            {
                return retrieveAsString(readableBytes());
            }

            std::string retrieveAsString(size_t len)
            {
                assert(len <= readableBytes());
                std::string res(peek(), len); //从一个地方开始，多长
                retrieve(len);
                return res;
            }

            StringPiece toStringPiece() const { return StringPiece(peek(), static_cast<int>(readableBytes())); }

            void append(const StringPiece &str)
            {
                append(str.data(), str.size());
            }

            void append(const char *data, size_t len)
            {
                ensureWritableBytes(len);
                std::copy(data, data + len, beginWrite());
                hasWriten(len);
            }

            void append(const void *data, size_t len)
            {
                append(static_cast<const char *>(data), len);
            }

            void ensureWritableBytes(size_t len)
            {
                if (writableBytes() < len)
                {
                    makeSpace(len);
                }
                assert(writableBytes() >= len);
            }

            void unwrite(size_t len)
            {
                assert(len < readableBytes());
                writerIndex_ -= len;
            }

            ///
            /// Append int64_t using network endian
            ///
            void appendInt64(int64_t x)
            {
                int64_t be64 = socket::hostToNetwork64(x);
                append(&be64, sizeof be64);
            }

            ///
            /// Append int32_t using network endian
            ///
            void appendInt32(int32_t x)
            {
                int32_t be32 = socket::hostToNetwork32(x);
                append(&be32, sizeof be32);
            }

            void appendInt16(int16_t x)
            {
                int16_t be16 = socket::hostToNetwork16(x);
                append(&be16, sizeof be16);
            }

            void appendInt8(int8_t x)
            {
                append(&x, sizeof x);
            }

            ///
            /// Read int64_t from network endian
            ///
            /// Require: buf->readableBytes() >= sizeof(int32_t)
            int64_t readInt64()
            {
                int64_t result = peekInt64();
                retrieveInt64();
                return result;
            }

            ///
            /// Read int32_t from network endian
            ///
            /// Require: buf->readableBytes() >= sizeof(int32_t)
            int32_t readInt32()
            {
                int32_t result = peekInt32();
                retrieveInt32();
                return result;
            }

            int16_t readInt16()
            {
                int16_t result = peekInt16();
                retrieveInt16();
                return result;
            }

            int8_t readInt8()
            {
                int8_t result = peekInt8();
                retrieveInt8();
                return result;
            }

            ///
            /// Peek int64_t from network endian
            ///
            /// Require: buf->readableBytes() >= sizeof(int64_t)
            int64_t peekInt64() const
            {
                assert(readableBytes() >= sizeof(int64_t));
                int64_t be64 = 0;
                ::memcpy(&be64, peek(), sizeof be64);
                return socket::networkToHost64(be64);
            }

            ///
            /// Peek int32_t from network endian
            ///
            /// Require: buf->readableBytes() >= sizeof(int32_t)
            int32_t peekInt32() const
            {
                assert(readableBytes() >= sizeof(int32_t));
                int32_t be32 = 0;
                ::memcpy(&be32, peek(), sizeof be32);
                return socket::networkToHost32(be32);
            }

            int16_t peekInt16() const
            {
                assert(readableBytes() >= sizeof(int16_t));
                int16_t be16 = 0;
                ::memcpy(&be16, peek(), sizeof be16);
                return socket::networkToHost16(be16);
            }

            int8_t peekInt8() const
            {
                assert(readableBytes() >= sizeof(int8_t));
                int8_t x = *peek();
                return x;
            }

            ///
            /// Prepend int64_t using network endian
            ///
            void prependInt64(int64_t x)
            {
                int64_t be64 = socket::hostToNetwork64(x);
                prepend(&be64, sizeof be64);
            }

            ///
            /// Prepend int32_t using network endian
            ///
            void prependInt32(int32_t x)
            {
                int32_t be32 = socket::hostToNetwork32(x);
                prepend(&be32, sizeof be32);
            }

            void prependInt16(int16_t x)
            {
                int16_t be16 = socket::hostToNetwork16(x);
                prepend(&be16, sizeof be16);
            }

            void prependInt8(int8_t x)
            {
                prepend(&x, sizeof x);
            }

            void prepend(const void *data, size_t len)
            {
                assert(len < prependableBytes());
                readerIndex_ -= len;
                const char *cData = static_cast<const char *>(data);
                std::copy(cData, cData + len, begin() + readerIndex_);
            }

            void shrink(size_t reserve) //!和muduo不一样，这里直接忽略了参数
            {
                std::copy(begin() + readerIndex_, beginWrite(), begin() + kCheapPrepend);
                buffer_.shrink_to_fit(); //使得capacity和size一样
            }

            size_t capacity() const { return buffer_.capacity(); }

            ssize_t readFd(int fd, int *saveErrno);

        private:
            char *begin() { return buffer_.data(); }
            const char *begin() const { return buffer_.data(); }

            void makeSpace(size_t len) //不是总长度，而是需要增加的长度
            {
                if (writableBytes() + prependableBytes() < len + kCheapPrepend)
                {
                    buffer_.resize(writerIndex_ + len);
                }
                else
                {
                    assert(kCheapPrepend < readerIndex_);
                    size_t loadBytes = readableBytes();
                    std::copy(buffer_.begin() + readerIndex_, buffer_.begin() + writerIndex_, buffer_.begin() + kCheapPrepend);
                    readerIndex_ = kCheapPrepend;
                    writerIndex_ = readerIndex_ + loadBytes;
                    assert(loadBytes = readableBytes());
                }
            }

            std::vector<char> buffer_;
            size_t readerIndex_;
            size_t writerIndex_;
            static const char KCRLF[]; //结束的/r/n
        };
    }
}

#endif // BUFFER_H