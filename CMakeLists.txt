cmake_minimum_required(VERSION 3.8)

if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project("NetworkingServer")

# Add the root directory as an include directory
include_directories(${CMAKE_SOURCE_DIR})

# Define the first executable with its sources
add_executable(NetworkingServer "server/server.cpp" "server/server.h" "common/defines.h" "server/main.cpp" "common/packet.h" "server/input_observer.h" "server/input_observer.cpp")
target_link_libraries(NetworkingServer ws2_32)
if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET NetworkingServer PROPERTY CXX_STANDARD 20)
endif()

# Define the second executable with its own sources
add_executable(NetworkingClient "client/client.cpp" "client/client.h" "common/defines.h" "client/main.cpp" "common/packet.h" "client/input_provider.cpp" "client/input_provider.h")
target_link_libraries(NetworkingClient ws2_32)
if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET NetworkingClient PROPERTY CXX_STANDARD 20)
endif()
