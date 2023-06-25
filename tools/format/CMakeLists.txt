include(ExternalProject)

add_executable(
        json_formatter
        format.cpp
        format_main.cpp
        ${CMAKE_SOURCE_DIR}/src/json.cpp
        ${CMAKE_SOURCE_DIR}/src/wcwidth.cpp)

target_link_libraries(json_formatter PRIVATE flatbuffers)

add_definitions(-DCATA_IN_TOOL)

include_directories(${CMAKE_SOURCE_DIR}/src)


if(NOT WIN32)
    set_property(TARGET json_formatter PROPERTY OUTPUT_NAME json_formatter.cgi)
endif()
install(TARGETS json_formatter RUNTIME)
