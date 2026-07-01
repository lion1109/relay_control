cmake_minimum_required(VERSION 3.16)

#if(UNIT_TEST)
#    list(APPEND EXTRA_COMPONENT_DIRS test)
#endif()

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(relay_control)

if(DEFINED UNIT_TEST)
    set(EXTRA_COMPONENT_DIRS test)
endif()

#set(EXTRA_COMPONENT_DIRS components/dw3000)

#if(UNIT_TEST)
#    set(TEST_COMPONENTS "test")
#endif()
