# cmake_minimum_required(VERSION 3.5)


# project(LOG)

# include_directories(${PROJECT_SOURCE_DIR}/include)

# add_library(appendlog STATIC
#     src/base/AppendFile.cpp)


# target_include_directories(appendlog
#     PUBLIC 
#         include)

# add_executable(test_appendFile
#     src/base/test/test_appendfile.cpp)

# target_link_libraries(test_appendFile
#     PRIVATE
#         appendlog)

# add_executable(test_logstream
#     src/base/test/test_logstream.cpp src/base/LogStream.cpp)


# target_include_directories(test_logstream
#     PRIVATE
#         include)


# add_executable(test_asynclog
#     src/base/test/test_asynclog.cpp src/base/AsyncLog.cpp src/base/AppendFile.cpp src/base/CountDownLatch.cpp src/base/LogStream.cpp 
#     src/base/ProcessInfo.cpp src/base/RollFile.cpp src/base/Timestamp.cpp)

# target_include_directories(test_asynclog
#     PRIVATE
#         include)

# target_link_libraries(test_asynclog
#     PRIVATE
#         pthread)   

# add_executable(test_rollfile
#     src/base/test/test_rollfile.cpp src/base/AppendFile.cpp src/base/RollFile.cpp src/base/ProcessInfo.cpp)

# target_include_directories(test_rollfile
#     PRIVATE
#         include)

# target_link_libraries(test_rollfile
#     PRIVATE
#         pthread)   


# add_executable(test_appendfile
#     src/base/test/test_appendfile.cpp src/base/AppendFile.cpp)
# target_include_directories(test_appendfile
#     PRIVATE
#         include)
# target_link_libraries(test_appendfile
#     PRIVATE
#         pthread)

# add_library(TimeZone STATIC
#     src/base/TimeZone.cpp src/base/Date.cpp)
# target_include_directories(TimeZone
#     PUBLIC
#         include)

# add_library(Logging STATIC
#     src/base/Logging.cpp src/base/Timestamp.cpp src/base/LogStream.cpp)

# target_include_directories(Logging
#     PUBLIC
#         include)


# set(BASE_TEST_CPPS
#     src/base/test/test_logging.cpp
#     src/base/test/test_asynclog.cpp
#     src/base/test/test_logstream.cpp
#     src/base/test/test_rollfile.cpp
#     src/base/test/test_appendfile.cpp)





# add_executable(help_header
#     src/base/test/help_header.cpp ${BASE_TEST_CPPS})

# target_include_directories(help_header
#     PRIVATE
#         ${PROJECT_SOURCE_DIR}/include)



cmake_minimum_required(VERSION 3.5)


project(m2)

include_directories(${PROJECT_SOURCE_DIR}/include)

set(BASE_LIB_CPP
        src/base/AsyncLog.cpp 
        src/base/AppendFile.cpp 
        src/base/CountDownLatch.cpp 
        src/base/CurrentThread.cpp
        src/base/LogStream.cpp 
        src/base/ProcessInfo.cpp 
        src/base/RollFile.cpp 
        src/base/Timestamp.cpp 
        src/base/Date.cpp 
        src/base/Logging.cpp
        src/base/ThreadPool.cpp
        src/base/TimeZone.cpp
        )




add_library(baselib STATIC
    ${BASE_LIB_CPP})

target_include_directories(baselib
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include)

target_link_libraries(baselib
    PUBLIC
        pthread)



# add_executable(test_logging
#     src/base/test/test_logging.cpp ${BASE_LIB_CPP})

# target_include_directories(test_logging
#     PRIVATE
#     ${PROJECT_SOURCE_DIR}/include)

# target_link_libraries(test_logging
#     PRIVATE
#         pthread)


##########################################################################################OLD
SET(OLD_DIR_HEADER
    ${PROJECT_SOURCE_DIR}/include/net)
set(OLD_DIR_CPP
    ${PROJECT_SOURCE_DIR}/src/net)

set(OLD_CPPS
    ${OLD_DIR_CPP}/EventLoop.cpp
    ${OLD_DIR_CPP}/Channel.cpp
    ${OLD_DIR_CPP}/Callbacks.cpp
    ${OLD_DIR_CPP}/Poller.cpp
    ${OLD_DIR_CPP}/EpollPoller.cpp
    ${OLD_DIR_CPP}/defaultPoller.cpp
    ${OLD_DIR_CPP}/Timer.cpp
    ${OLD_DIR_CPP}/TimerQueue.cpp
    ${OLD_DIR_CPP}/EventLoopThread.cpp
    ${OLD_DIR_CPP}/SocketOps.cpp
    ${OLD_DIR_CPP}/Socket.cpp
    ${OLD_DIR_CPP}/InetAddress.cpp
    ${OLD_DIR_CPP}/Acceptor.cpp
    ${OLD_DIR_CPP}/TcpConnection.cpp
    ${OLD_DIR_CPP}/TcpServer.cpp
    ${OLD_DIR_CPP}/Buffer.cpp
    ${OLD_DIR_CPP}/EventLoopThreadPool.cpp
    ${OLD_DIR_CPP}/TcpClient.cpp
    ${OLD_DIR_CPP}/Connector.cpp
    )

set(OLD_HEADERS
    ${OLD_DIR_HEADER}/EventLoop.h
    ${OLD_DIR_HEADER}/Channel.h
    ${OLD_DIR_HEADER}/Callbacks.h
    ${OLD_DIR_HEADER}/Poller.h
    ${OLD_DIR_HEADER}/EpollPoller.h
    ${OLD_DIR_HEADER}/Timer.h
    ${OLD_DIR_HEADER}/TimerId.h
    ${OLD_DIR_HEADER}/TimerQueue.h
    ${OLD_DIR_HEADER}/EventLoopThread.h
    ${OLD_DIR_HEADER}/Endian.h
    ${OLD_DIR_HEADER}/SocketOps.h
    ${OLD_DIR_HEADER}/Socket.h
    ${OLD_DIR_HEADER}/InetAddress.h
    ${OLD_DIR_HEADER}/Acceptor.h
    ${OLD_DIR_HEADER}/TcpConnection.h
    ${OLD_DIR_HEADER}/TcpServer.h
    ${OLD_DIR_HEADER}/Buffer.h
    ${OLD_DIR_HEADER}/EventLoopThreadPool.h
    ${OLD_DIR_HEADER}/TcpCLient.h
    ${OLD_DIR_HEADER}/Connector.h
    )

add_library(netlib
    ${OLD_CPPS})

target_include_directories(netlib
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include)

target_include_directories(netlib
    PUBLIC
        ${OLD_DIR_HEADER})

target_link_libraries(netlib
    PUBLIC
        baselib)






# set(OLD_INCLUDE
#     ${PROJECT_SOURCE_DIR}/include ${OLD_DIR_HEADER})

# add_executable(test_eventloop
#     ${OLD_DIR_HEADER}/test/test_eventloop.cpp ${OLD_DIR_HEADER}/EventLoop.cpp)
# target_include_directories(test_eventloop
#     PUBLIC
#         ${PROJECT_SOURCE_DIR}/include ${OLD_DIR_HEADER})
# target_link_libraries(test_eventloop
#     PRIVATE
#         pthread baselib)

add_executable(test_timer
    ${OLD_DIR_CPP}/test/test_timer.cpp)
target_include_directories(test_timer
    PRIVATE
        ${OLD_DIR_HEADER})
target_link_libraries(test_timer
    PUBLIC
        pthread baselib netlib)


add_executable(test_eventloopthread
    ${OLD_DIR_CPP}/test/test_eventloopthread.cpp)
target_include_directories(test_eventloopthread
    PRIVATE
        ${OLD_DIR_HEADER})
target_link_libraries(test_eventloopthread
    PUBLIC
        pthread baselib netlib)



set(INCLUDE_DIR
    ${PROJECT_SOURCE_DIR}/include)
set(EXAMPLE_DIR
    ${PROJECT_SOURCE_DIR}/src/examples)


add_executable(echo_server
    ${EXAMPLE_DIR}/echo/server/echo_server.cpp ${EXAMPLE_DIR}/echo/server/main.cpp)
target_link_libraries(echo_server
    PRIVATE
        baselib netlib pthread)
target_include_directories(echo_server
    PRIVATE
        ${INCLUDE_DIR} ${EXAMPLE_DIR}/echo/server)



add_executable(echo_client
    ${EXAMPLE_DIR}/echo/client/echo_client.cpp ${EXAMPLE_DIR}/echo/client/main.cpp)
target_link_libraries(echo_client
    PRIVATE
        baselib netlib pthread)
target_include_directories(echo_client
    PRIVATE
        ${INCLUDE_DIR} ${EXAMPLE_DIR}/echo/client)

add_executable(finger
    ${EXAMPLE_DIR}/finger/finger.cpp)
target_include_directories(finger
    PRIVATE
        ${INCLUDE_DIR} ${EXAMPLE_DIR}/finger/)
target_link_libraries(finger
    PRIVATE
        baselib netlib)


########################################################################################OLD
