cmake_minimum_required(VERSION 3.14)
project(CollaborativeML)
set(CMAKE_CXX_STANDARD 11)

set(SOURCE_FILES main.cpp src/client/Client.cpp src/client/Client.h src/utils/util.h src/utils/util.cpp src/models/logistic_regression.h src/models/logistic_regression.cpp src/utils/encoder.h src/utils/encoder.cpp src/utils/djcs_aux.h src/utils/djcs_aux.cpp)

include_directories($ENV{HOME}/libscapi/include $ENV{HOME}/Documents/projects/boost_1_64_0/stage/lib)

link_directories(/usr/ssl/lib $ENV{HOME}/libscapi/install/lib)

add_executable(CollaborativeML ${SOURCE_FILES})
target_link_libraries(CollaborativeML
        /usr/lib/x86_64-linux-gnu/libgmp.so
        $ENV{HOME}/Documents/projects/libhcs/lib/libhcs.so
        $ENV{HOME}/libscapi/libscapi.a
        libboost_thread.a libboost_system.a pthread crypto)