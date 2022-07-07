set(SRCS
    Main.cpp
    Engine.cpp
)
include_directories(${PROJECT_SOURCE_DIR}/base)


add_executable(WebEngine ${SRCS})
target_link_libraries(WebEngine libserver_base)