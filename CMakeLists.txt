cmake_minimum_required(VERSION 3.10)
project(CollaborativeML)
set(CMAKE_CXX_STANDARD 11)

set(SOURCE_FILES main.cpp
        src/client/Client.cpp
        src/client/Client.h
        src/utils/util.h
        src/utils/util.cpp
        src/models/logistic_regression.h
        src/models/logistic_regression.cpp
        src/utils/encoder.h
        src/utils/encoder.cpp
        src/utils/djcs_t_aux.h
        src/utils/djcs_t_aux.cpp
        tests/test_encoder.cpp
        tests/test_djcs_t_aux.cpp tests/test_encoder.h tests/test_djcs_t_aux.h tests/test_logistic_regression.h tests/test_logistic_regression.cpp)

include_directories($ENV{HOME}/libscapi/include $ENV{HOME}/Documents/projects/boost_1_64_0/stage/lib)

link_directories(/usr/ssl/lib $ENV{HOME}/libscapi/install/lib)

add_executable(CollaborativeML ${SOURCE_FILES})
target_link_libraries(CollaborativeML
        /usr/lib/x86_64-linux-gnu/libgmp.so
        /usr/lib/x86_64-linux-gnu/libgmpxx.so
        $ENV{HOME}/Documents/projects/libhcs/lib/libhcs.so
        $ENV{HOME}/libscapi/libscapi.a
        libboost_thread.a libboost_system.a pthread crypto)

#set(TEST_DIR "tests/")
#set(BINARY_DIR "bin/")

#set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${BINARY_DIR})

# BEGIN: Test commands
#enable_testing()
#set(CMAKE_CTEST_COMMAND "${CMAKE_CTEST_COMMAND} --verbose")

#file(GLOB test_srcs "${TEST_DIR}/*.cpp")
#foreach(f ${test_srcs})
#    get_filename_component(test_prog ${f} NAME_WE)
#    add_executable(${test_prog} "${TEST_DIR}/${test_prog}.cpp")
#    target_link_libraries(${test_prog} $ENV{HOME}/Documents/projects/libhcs/lib/libhcs.so
#            /usr/lib/x86_64-linux-gnu/libgmp.so /usr/lib/x86_64-linux-gnu/libgmpxx.so)
#    add_test(${test_prog} "${BINARY_DIR}/${test_prog}")
#endforeach()
# EN: Test commands
