#include "AppendFile.h"
#include <iostream>
int main()
{
    using namespace m2;
    FileDetail::AppendFile a("aaa"), b("bbb");
    for (int i = 0; i < 10; ++i)
    {
        a.append("ccc", 4);
        b.append("ccc", 4);
        std::cout << a.writtenBytes() << std::endl;
        std::cout << b.writtenBytes() << std::endl;
    }
}