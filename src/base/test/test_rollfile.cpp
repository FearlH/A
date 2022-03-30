#include "RollFile.h"
#include <thread>

int main()
{
    using namespace m2;
    RollFile f("roll__", 10, true, 3, 20);
    while (1)
    {
        f.append("ccc", 4);
    }
}