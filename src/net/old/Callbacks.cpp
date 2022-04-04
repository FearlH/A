#include "Callbacks.h"
using namespace m2;
using namespace net;

Callbacks::Callbacks() = default;
Callbacks::~Callbacks() = default;
void Callbacks::eventCallback() {}
void Callbacks::readCallback(Timestamp){};
void Callbacks::writeCallback(){};
void Callbacks::timeCallback(){};
void Callbacks::errorCallback(){};
void Callbacks::closeCalback(){};

using namespace CallbackNum;

NumCall callNumPair(const int64_t todo, std::shared_ptr<Callbacks> callback)
{
    return {todo, callback};
}
void doCallNumPair(NumCall &pCalls, Timestamp time)
{
    int64_t type = pCalls.first;
    std::shared_ptr<Callbacks> callback = pCalls.second;
    if (type & KClose)
    {
        callback->closeCalback();
    }
    if (type & KError)
    {
        callback->errorCallback();
    }
    if (type & KEvent)
    {
        callback->eventCallback();
    }
    if ((type & KRead) && Timestamp::invalid() != time)
    {
        callback->readCallback(time);
    }
    if (type & KTime)
    {
        callback->timeCallback();
    }
    if (type & KWrite)
    {
        callback->writeCallback();
    }
    if (type & KRead)
    {
        callback->readCallback();
    }
    if (type & KVoid)
    {
        callback->someVoidCallback();
    }
}