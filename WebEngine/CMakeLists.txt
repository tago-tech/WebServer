set(SRCS
    SocketChannel.cpp
    HttpRequestContext.cpp
    Epoll.cpp
    EventsLoop.cpp
    EventsLoopProcessor.cpp
    Engine.cpp
    Util.cpp
    Main.cpp
)
include_directories(${PROJECT_SOURCE_DIR}/base)


add_executable(WebEngine ${SRCS})
target_link_libraries(WebEngine libserver_base)

add_subdirectory(base)