# Copyright 2016 Proyectos y Sistemas de Mantenimiento SL (eProsima).
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

cmake_minimum_required(VERSION 3.20)

project("HelloWorldExampleTCP" VERSION 1 LANGUAGES CXX)

# Find requirements
if(NOT fastcdr_FOUND)
    find_package(fastcdr REQUIRED)
endif()

if(NOT foonathan_memory_FOUND)
    find_package(foonathan_memory REQUIRED)
endif()

if(NOT fastrtps_FOUND)
    find_package(fastrtps 2.12 REQUIRED)
endif()

#Check C++11
include(CheckCXXCompilerFlag)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    check_cxx_compiler_flag(-std=c++11 SUPPORTS_CXX11)
    if(NOT SUPPORTS_CXX11)
        message(FATAL_ERROR "Compiler doesn't support C++11")
    endif()
endif()

message(STATUS "Configuring DDSHelloWorldTCP example...")
file(GLOB DDS_TCP_HELLOWORLD_EXAMPLE_SOURCES_CXX "*.cxx")
file(GLOB DDS_TCP_HELLOWORLD_EXAMPLE_SOURCES_CPP "*.cpp")
#file(GLOB XML_CONFIG_FILES "*.xml")
#message(STATUS "XML Files: " ${XML_CONFIG_FILES})
configure_file("HelloWorldSubscriber_static_disc.xml" "HelloWorldSubscriber_static_disc.xml" COPYONLY)
configure_file("HelloWorldPublisher_static_disc.xml" "HelloWorldPublisher_static_disc.xml" COPYONLY)
configure_file("dh2048.pem" "dh2048.pem" COPYONLY)
configure_file("serverkey.pem" "serverkey.pem" COPYONLY)
configure_file("servercert.pem" "servercert.pem" COPYONLY)
configure_file("cacert.pem" "cacert.pem" COPYONLY)

add_executable(DDSHelloWorldExampleTCP ${DDS_TCP_HELLOWORLD_EXAMPLE_SOURCES_CXX} ${DDS_TCP_HELLOWORLD_EXAMPLE_SOURCES_CPP})
target_compile_definitions(DDSHelloWorldExampleTCP PRIVATE
    $<$<AND:$<NOT:$<BOOL:${WIN32}>>,$<STREQUAL:"${CMAKE_BUILD_TYPE}","Debug">>:__DEBUG>
    $<$<BOOL:${INTERNAL_DEBUG}>:__INTERNALDEBUG> # Internal debug activated.
    )
target_link_libraries(DDSHelloWorldExampleTCP fastrtps fastcdr foonathan_memory fastdds::optionparser)
install(TARGETS DDSHelloWorldExampleTCP
    RUNTIME DESTINATION examples/cpp/dds/HelloWorldExampleTCP/${BIN_INSTALL_DIR})
