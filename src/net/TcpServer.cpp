#include "net/TcpServer.h"
#include "base/Logging.h"
#include "net/Callbacks.h"
#include "net/Acceptor.h"
#include "net/EventLoop.h"
#include "net/EventLoopThreadPool.h"
#include "net/TcpConnection.h"
#include "net/SocketOps.h"
#include <functional>
#include <assert.h>

using namespace m2;
using namespace m2::net;

using std::placeholders::_1;
using std::placeholders::_2;

TcpServer::TcpServer(EventLoop *loop,
                     const InetAddress &listenAddr,
                     const std::string nameArg,
                     Opinion op)
    : loop_(loop),
      ipPort_(listenAddr.toIpPort()),
      name_(nameArg),
      acceptor_(new Acceptor(loop, listenAddr, op == KReusePort)),
      threadPool_(new EventLoopThreadPool(loop, nameArg)),
      connectionCallback_(defaultConnectionCallback),
      messageCallback_(defaultMessageCallback)

{
    acceptor_->setNewConnectionCallBack(
        std::bind(&TcpServer::newConnection, this, _1, _2));
    //传入的就是fd和地址
}

TcpServer::~TcpServer()
{
    loop_->assertInLoopThread();
    for (auto &item : connections_)
    {
        TcpConnectionPtr connPtr_ToAlive = item.second;
        item.second.reset();
        connPtr_ToAlive->getLoop()->runInLoop(std::bind(&TcpConnection::connectDestroyed, connPtr_ToAlive));
        //相当于是把智能指针当作this去传递
        //延长这个TcpConnection的生命

        /*
        对象的关系是:
        TcpServer的map含有TcpConnection的智能指针(TcpServer设置TcpConnection的一个callback)
        TcpConnection拥有Channel(TcpConnection设置Channel的一个callback)

        回调函数的调用是:
        Channel->TcpConnection->TcpServer
        TcpServer函数调用结束之后，TcpConnection可能会析构，导致里面包含的Channel析构
        此时正在调用Channel的函数，析构Channel就会引发问题

        所以在这里发出一个新的任务，
        TcpConnectionPtr connPtr_ToAlive = item.second;
        item.second.reset();
        connPtr_ToAlive->getLoop()->runInLoop(std::bind(&TcpConnection::connectDestroyed, connPtr_ToAlive));
        runInLoop就是会在和Channel同一个线程里面运行
        因为runInLoop的任务是会在最后才去执行的，那么就会使得执行任务的时候,可能引起Channel析构的那个函数一定执行完成了
        发出之后就会从Channel的函数里面返回，保证不是在Channel的函数执行的里面析构Channel

        同时这个任务会延长TcpConnection的生命期
        这个任务执行完成之后
        会正向的析构TcpConnection然后是Channel

        */
    }
}

void TcpServer::setThreadNum(int numThreads)
{
    assert(numThreads >= 0);
    threadPool_->setThreadNum(numThreads);
}

void TcpServer::start()
{
    if (started_.fetch_and(1) == 0)
    {
        threadPool_->start(threadInitCallback_);
        assert(!acceptor_->listening());
        loop_->runInLoop(std::bind(&Acceptor::listen, acceptor_.get()));
        //每一个需要在里面调用的成员都需要把loop放在自己的对象里面,调用的时候才能和loop->runInLoop对应上去
    }
}

void TcpServer::newConnection(int sockfd, const InetAddress &peerAddr)
{

    //这个函数是在acceptor_里面调用的
    //需要改变一下自己的状态，所以还会去bind自己
    loop_->assertInLoopThread();
    //!会从ioLoop里面挑选一个然后把TcpConnection放进去(构造进去)
    EventLoop *ioLoop = threadPool_->getNextLoop();
    char buf[64];
    snprintf(buf, sizeof buf, "-%s#%d", ipPort_.c_str(), nextConnId_);
    ++nextConnId_;
    string connName = name_ + buf;

    LOG_INFO << "TcpServer::newConnection [" << name_
             << "] - new connection [" << connName
             << "] from " << peerAddr.toIpPort();
    InetAddress localAddr(socket::getLocalAddr(sockfd));
    // FIXME poll with zero timeout to double confirm the new connection
    // FIXME use make_shared if necessary
    TcpConnectionPtr conn(new TcpConnection(ioLoop,
                                            connName,
                                            sockfd,
                                            localAddr,
                                            peerAddr));
    connections_[connName] = conn;
    conn->setConnectionCallback(connectionCallback_);
    conn->setMessageCallback(messageCallback_);
    conn->setWriteCompleteCallback(writeCompleteCallback_);
    //! CloseCallback这里设置，然后把TcpConnection从这个map里面移除
    //! 因为不知道什么时候会关闭连接，所以需要回调函数
    conn->setCloseCallback(
        std::bind(&TcpServer::removeConnection, this, _1)); // FIXME: unsafe
    ioLoop->runInLoop(std::bind(&TcpConnection::connectEstablished, conn));
    //构造好了之后连接建立，然后就会调用这个函数
    //!因为是listen的loop创建一个TcpConnection到一个ioLoop的里面
    //!构造函数是在listen线程里面执行的
    //!所以不会去使用有关loop_的一些函数(不在一个线程里面)
    //!会在后面调用runInLoop完成初始化
}

void TcpServer::removeConnection(const TcpConnectionPtr &conn)
{
    // FIXME: unsafe
    //都会去在Loop里面调用自己的删除函数
    loop_->runInLoop(std::bind(&TcpServer::removeConnectionInLoop, this, conn));
    //调用到此，channel的handleClose结束，会发送一个新的函数到Loop里面执行
}

void TcpServer::removeConnectionInLoop(const TcpConnectionPtr &conn)
{
    loop_->assertInLoopThread();
    LOG_INFO << "TcpServer::removeConnectionInLoop [" << name_
             << "] - connection " << conn->name();
    size_t n = connections_.erase(conn->name());
    (void)n;
    assert(n == 1);
    EventLoop *ioLoop = conn->getLoop();
    ioLoop->queueInLoop(
        std::bind(&TcpConnection::connectDestroyed, conn));
}
