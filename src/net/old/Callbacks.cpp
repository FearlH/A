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