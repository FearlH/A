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
SET(OLD_DIR
    ${PROJECT_SOURCE_DIR}/src/net/old)

set(OLD_CPPS
    ${OLD_DIR}/EventLoop.cpp
    ${OLD_DIR}/Channel.cpp
    ${OLD_DIR}/Callbacks.cpp
    ${OLD_DIR}/Poller.cpp
    ${OLD_DIR}/EpollPoller.cpp
    ${OLD_DIR}/defaultPoller.cpp
    )

set(OLD_HEADERS
    ${OLD_DIR}/EventLoop.h
    ${OLD_DIR}/Channel.h
    ${OLD_DIR}/Callbacks.h
    ${OLD_DIR}/Poller.h
    ${OLD_DIR}/EpollPoller.h
    )

add_library(oldlib
    ${OLD_CPPS})

target_include_directories(oldlib
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include)

target_include_directories(oldlib
    PUBLIC
        ${OLD_DIR})

target_link_libraries(oldlib
    PUBLIC
        baselib)




set(OLD_INCLUDE
    ${PROJECT_SOURCE_DIR}/include ${OLD_DIR})

# add_executable(test_eventloop
#     ${OLD_DIR}/test/test_eventloop.cpp ${OLD_DIR}/EventLoop.cpp)
# target_include_directories(test_eventloop
#     PUBLIC
#         ${PROJECT_SOURCE_DIR}/include ${OLD_DIR})
# target_link_libraries(test_eventloop
#     PRIVATE
#         pthread baselib)

add_executable(test_eventloop2
    ${OLD_DIR}/test/test_eventloop2.cpp)
target_include_directories(test_eventloop2
    PRIVATE
        ${OLD_INCLUDE})
target_link_libraries(test_eventloop2
    PUBLIC
        pthread baselib oldlib)
########################################################################################OLD
