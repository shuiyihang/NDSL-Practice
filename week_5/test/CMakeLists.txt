set(SRC_LIST bplusTest.c cmd.c)
add_executable(${PROJECT_NAME} ${SRC_LIST})

set(CMAKE_C_FLAGS "-O2 -Werror -Wextra")

# message("===========")
include_directories(${PROJECT_SOURCE_DIR}/lib)
# message(${PROJECT_SOURCE_DIR})
target_link_libraries(${PROJECT_NAME} ${LIB_BPLUSTREE_NAME})
target_link_libraries(${PROJECT_NAME} readline)