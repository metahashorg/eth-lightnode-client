set(BOOST_LIBS_INCLUDE ../3rd_party/boost/include)
set(BOOST_LIBS_LIBS ../3rd_party/boost/lib)
find_library(BOOST1 NAMES libboost_filesystem.a HINTS ${BOOST_LIBS_LIBS})
find_library(BOOST2 NAMES libboost_program_options.a HINTS ${BOOST_LIBS_LIBS})
find_library(BOOST3 NAMES libboost_system.a HINTS ${BOOST_LIBS_LIBS})

set(ETH_SOURCES 
    ethtx/crossguid/Guid.cpp
    ethtx/scrypt/crypto_scrypt-nosse.cpp
    ethtx/scrypt/crypto_scrypt_saltgen.cpp
    ethtx/scrypt/sha256.cpp
    ethtx/cert.cpp
    ethtx/cert2.cpp
    ethtx/ethtx.cpp
    ethtx/rlp.cpp
    ethtx/utils2.cpp
    eth_wallet/EthWallet.cpp
    eth_wallet/FileSystem.cpp
    eth_wallet/TypedException.cpp
    eth_wallet/jsonUtils.cpp
)

add_executable(${PROJECT_NAME}
    common/string_utils.cpp
    common/filesystem_utils.cpp
    log/log.cpp
    main.cpp
    exception/except.cpp
    settings/settings.cpp
    http_server.cpp
    #http_json_rpc_request_impl.cpp
    http_json_rpc_request.cpp
    json_rpc.cpp
    http_session.cpp
    data_storage/data_address.cpp
    data_storage/data_storage.cpp
    data_storage/data_updater.cpp
    task_handlers/utils.cpp
    task_handlers/base_handler.cpp
    task_handlers/network_handler.cpp
    task_handlers/create_tx_base_handler.cpp
    task_handlers/create_tx_handler.cpp
    task_handlers/send_tx_handler.cpp
    task_handlers/generate_handler.cpp
    task_handlers/fetch_balance_handler.cpp
    task_handlers/fetch_history_handler.cpp
    task_handlers/add_addresses_to_batch.cpp
    task_handlers/del_addresses_to_batch.cpp
    task_handlers/get_addresses_to_batch.cpp
    task_handlers/batch_balance.cpp
    task_handlers/batch_history.cpp
    task_handlers/create_tx_token_base_handler.cpp
    task_handlers/create_tx_token_handler.cpp
    task_handlers/send_tx_token_handler.cpp
    task_handlers/add_addresses_to_batch_tkn.cpp
    task_handlers/del_addresses_to_batch_tkn.cpp
    task_handlers/get_addresses_to_batch_tkn.cpp
    task_handlers/batch_balance_tkn.cpp
    task_handlers/batch_history_tkn.cpp
    task_handlers/fetch_balance_tkn.cpp
    task_handlers/fetch_history_tkn.cpp
    task_handlers/get_transaction_params.cpp
    task_handlers/get_transaction_params_tkn.cpp
    task_handlers/add_to_tracking_handler.cpp
    task_handlers/del_from_tracking_handler.cpp
    task_handlers/get_tracking_handler.cpp
    task_handlers/status_handler.cpp

    ${ETH_SOURCES}
    wallet_storage/wallet_storage.cpp
    task_handlers/task_handlers.cpp
    )

include_directories(SYSTEM
    #${Boost_LIBRARY_DIRS} /
    ${OPENSSL_INCLUDE_DIRS} 
    ${BOOST_LIBS_INCLUDE}
    "../3rd_party"
    "../3rd_party/gmp"
    "../3rd_party/leveldb/include"
    "../3rd_party/p7logger/include"
    ./
    )


#include_directories(${PROJECT_NAME} ../3rd_party/leveldb/lib)
#find_library(LIB_LEVEL_DB leveldb HINTS ../3rd_party/leveldb/lib)
#target_link_libraries(${PROJECT_NAME} ${LIB_LEVEL_DB})

find_package(Threads)
target_link_libraries(${PROJECT_NAME} ${CMAKE_THREAD_LIBS_INIT})

#find_package(Boost COMPONENTS system filesystem program_options REQUIRED)
target_link_libraries(${PROJECT_NAME} ${BOOST1} ${BOOST2} ${BOOST3})

link_directories(${PROJECT_NAME} ${Boost_LIBRARY_DIRS})
target_link_libraries(${PROJECT_NAME} ${Boost_LIBRARIES})

find_package(OpenSSL REQUIRED)
#target_link_libraries(${PROJECT_NAME} ${OPENSSL_LIBRARIES})
include_directories(${OPENSSL_INCLUDE_DIR})

find_library(LIB_CRYPTO NAMES libcrypto.so HINTS /usr/local/lib/)
find_library(LIB_SSL NAMES libssl.so HINTS /usr/local/lib/)
find_library(LIB_LEVELDB NAMES libleveldb.a HINTS ../3rd_party/leveldb/lib)
find_library(LIB_P7 NAMES libP7.a HINTS ../3rd_party/p7logger/lib)
find_library(LIB_GMP NAMES libgmp.a HINTS /../3rd_party/gmp/lib)
find_library(LIB_GMPXX NAMES libgmpxx.a HINTS ../3rd_party/gmp/lib)

target_link_libraries(${PROJECT_NAME} ${LIB_CRYPTO} ${LIB_SSL} ${LIB_LEVELDB} ${LIB_P7} ${LIB_GMP} ${LIB_GMPXX})

#include_directories(../3rd_party/leveldb/include/)

set (CMAKE_CXX_FLAGS "-fno-builtin-malloc -fno-builtin-calloc -fno-builtin-realloc -fno-builtin-free -pthread -Ofast -Wall -Wextra -Werror -Wno-unused-parameter -Wfloat-equal")

target_compile_options(${PROJECT_NAME} PRIVATE -no-pie -D_GLIBCXX_USE_CXX11_ABI=0)
set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS "-no-pie -D_GLIBCXX_USE_CXX11_ABI=0")
#set (CMAKE_CXX_FLAGS "-g -rdynamic")

find_library(SECP secp256k1 HINTS ../3rd_party/secp256k1/lib/linux/)
target_link_libraries(${PROJECT_NAME} ${SECP})

set(PROJECT_LIBS
    stdc++fs.a
    uuid
    ssl
    crypto
    cryptopp
    dl
    rt
)

target_link_libraries(${PROJECT_NAME} ${PROJECT_LIBS})
