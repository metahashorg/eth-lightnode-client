#set(BOOST_LIBS_LIBS ../../3rd_party/boost/lib)
#find_library(BOOST1 NAMES libboost_filesystem.a HINTS ${BOOST_LIBS_LIBS})
#find_library(BOOST2 NAMES libboost_program_options.a HINTS ${BOOST_LIBS_LIBS})
#find_library(BOOST3 NAMES libboost_system.a HINTS ${BOOST_LIBS_LIBS})

#include_directories(SYSTEM
#    ${OPENSSL_INCLUDE_DIRS}
#    "../../3rd_party/boost/include"
#    "../../3rd_party"
#    )

#find_package(OpenSSL REQUIRED)
#include_directories(${OPENSSL_INCLUDE_DIR})

#find_package(GTest REQUIRED)
#include_directories(SYSTEM ${GTEST_INCLUDE_DIR})

#find_library(LIB_CRYPTO NAMES libcrypto.so HINTS /usr/local/lib/)
#find_library(LIB_SSL NAMES libssl.so HINTS /usr/local/lib/)

#set (CMAKE_CXX_FLAGS "-fno-builtin-malloc -fno-builtin-calloc -fno-builtin-realloc -fno-builtin-free -pthread -Ofast -Wall -Wextra -Werror -Wno-unused-parameter")

#find_library(SECP secp256k1 HINTS ../3rd_party/secp256k1/lib/linux/)
#set(PROJECT_LIBS
#    stdc++fs.a
#    uuid
#    ssl
#    crypto
#    cryptopp
#    ${SECP}
#    gmp
#    gmpxx
#)

add_executable(task_handlers_test
#    ../exception/except.cpp
#    ../settings/settings.cpp
#    ../http_json_rpc_request.cpp
#    ../json_rpc.cpp
#    ../log/log.cpp
#    ../task_handlers/utils.cpp
#    ../task_handlers/generate_handler.cpp
#    ../wallet_storage/wallet_storage.cpp
    task_handlers_test.cpp)

target_link_libraries(task_handlers_test eth)

target_link_libraries(task_handlers_test ${PROJECT_LIBS} ${LIB_CRYPTO} ${LIB_SSL})
target_link_libraries(task_handlers_test ${BOOST1} ${BOOST2} ${BOOST3})
target_link_libraries(task_handlers_test ${GTEST_BOTH_LIBRARIES})
target_compile_options(task_handlers_test PRIVATE -no-pie -D_GLIBCXX_USE_CXX11_ABI=0)
set_target_properties(task_handlers_test PROPERTIES LINK_FLAGS "-no-pie -D_GLIBCXX_USE_CXX11_ABI=0")
