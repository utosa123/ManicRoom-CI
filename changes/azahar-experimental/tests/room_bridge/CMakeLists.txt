cmake_minimum_required(VERSION 3.25)
project(room_bridge_tests LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 20)
get_filename_component(AZAHAR "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
add_subdirectory("${AZAHAR}/externals/enet" enet)
add_subdirectory("${AZAHAR}/externals/fmt" fmt EXCLUDE_FROM_ALL)
add_executable(room_bridge_tests
    integration.cpp test_logging.cpp
    "${AZAHAR}/src/citra_libretro/room_session.cpp"
    "${AZAHAR}/src/network/network.cpp"
    "${AZAHAR}/src/network/room.cpp"
    "${AZAHAR}/src/network/room_member.cpp"
    "${AZAHAR}/src/network/packet.cpp"
    "${AZAHAR}/src/network/verify_user.cpp")
target_include_directories(room_bridge_tests PRIVATE
    "${AZAHAR}/src" "${AZAHAR}/externals/boost" "${AZAHAR}/externals/enet/include")
target_link_libraries(room_bridge_tests PRIVATE enet fmt::fmt)
if(WIN32)
    target_compile_definitions(room_bridge_tests PRIVATE NOMINMAX _WIN32_WINNT=0x0601)
    target_link_libraries(room_bridge_tests PRIVATE ws2_32 winmm)
    target_link_libraries(enet PUBLIC ws2_32 winmm)
endif()
enable_testing()
add_test(NAME actual_enet_room_bridge COMMAND room_bridge_tests)
set_tests_properties(actual_enet_room_bridge PROPERTIES TIMEOUT 60)

add_executable(keyboard_abi_layout abi_layout.cpp)
target_include_directories(keyboard_abi_layout PRIVATE "${AZAHAR}/src" "${AZAHAR}/externals/libretro-common/libretro-common/include")
add_test(NAME keyboard_abi_layout COMMAND keyboard_abi_layout)
add_library(manic_adapter_compile OBJECT "${AZAHAR}/src/citra_libretro/manic_compat.cpp")
target_compile_definitions(manic_adapter_compile PRIVATE MANIC_ROOM_EXPERIMENT=1)
target_include_directories(manic_adapter_compile PRIVATE "${AZAHAR}/src" "${AZAHAR}/externals/libretro-common/libretro-common/include")
