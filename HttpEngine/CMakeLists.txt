set(SRCS
    Main.cpp
    Util.cpp
    Engine.cpp
    SocketChannel.cpp
    Epoll.cpp
    HttpRequestContext.cpp
    EventsController.cpp
    EventsProcessor.cpp
)
include_directories(${PROJECT_SOURCE_DIR}/base)


add_executable(HttpEngine ${SRCS})
target_link_libraries(HttpEngine libserver_base)

add_subdirectory(base)