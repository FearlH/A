#include "AppendFile.h"

#include <string.h>
using namespace m2;

FileDetail::AppendFile::AppendFile(const std::string &fileName) : fp_(fopen(fileName.c_str(), "ae")), writtenBytes_(0)
{
    ::setbuffer(fp_, IOBuffer_, sizeof(IOBuffer_));
}

void FileDetail::AppendFile::flush()
{
    fflush(fp_);
}

void FileDetail::AppendFile::append(const char *buf, size_t len)
{
    size_t nLeft = len;
    ssize_t nWrite = 0;
    while (nLeft != 0)
    {
        ssize_t thisWrite = ::fwrite_unlocked(buf + nWrite, 1, nLeft, fp_);

        if (ferror(fp_))
        {
            fprintf(stderr, "ERROR_IN_FILEAPPEND_WRITE AppendFile::append() %s\n", strerror(errno));
            break;
        }
        nWrite += thisWrite;
        nLeft -= thisWrite;
        writtenBytes_ += thisWrite;
    }
}

FileDetail::AppendFile::~AppendFile()
{
    ::fclose(fp_);
}
