if (CMAKE_CURRENT_LIST_DIR MATCHES "-src/sources$")
    include(${XRN_BUILD_DIR}/_deps/${XRN_BIN_NAME}/conanbuildinfo.cmake)
else ()
    include(${XRN_BUILD_DIR}/conanbuildinfo.cmake)
endif ()
conan_basic_setup(TARGETS)

include(CMakeFindDependencyMacro)
foreach(library IN LISTS XRN_LIBRARIES_REQUIREMENTS)
    find_dependency(${library} REQUIRED)
endforeach()

add_library(XRN_${XRN_BIN_NAME}_BIN_OBJECTS OBJECT ${XRN_MAIN} ${XRN_SOURCES} ${XRN_FRAGMENT} ${XRN_VERTEX} ${XRN_HEADERS})

if (ENABLE_PCH)
    target_precompile_headers(
        XRN_${XRN_BIN_NAME}_BIN_OBJECTS
        PRIVATE
            <pch.hpp>
    )
endif ()

if (ENABLE_BINARY)
    target_link_libraries(
        XRN_${XRN_BIN_NAME}_BIN_OBJECTS
        PUBLIC
            ${XRN_BIN_NAME}_project_options
            ${XRN_BIN_NAME}_project_warnings
            ${XRN_BIN_NAME}_project_dependencies
            ${XRN_LIBRARIES_DEPENDENCIES}
    )
    add_executable(${XRN_BIN_NAME} $<TARGET_OBJECTS:XRN_${XRN_BIN_NAME}_BIN_OBJECTS>)

    target_link_libraries(
        ${XRN_BIN_NAME}
        PRIVATE
            XRN_${XRN_BIN_NAME}_BIN_OBJECTS
    )
endif ()

set_property(TARGET XRN_${XRN_BIN_NAME}_BIN_OBJECTS PROPERTY POSITION_INDEPENDENT_CODE 1)
if (ENABLE_STATIC_LIBRARY)
    add_library(${XRN_BIN_NAME}_static STATIC $<TARGET_OBJECTS:XRN_${XRN_BIN_NAME}_BIN_OBJECTS>)
    set_target_properties(${XRN_BIN_NAME}_static PROPERTIES OUTPUT_NAME ${XRN_BIN_NAME})
endif ()

if (ENABLE_SHARED_LIBRARY)
    add_library(${XRN_BIN_NAME}_shared SHARED $<TARGET_OBJECTS:XRN_${XRN_BIN_NAME}_BIN_OBJECTS>)
    set_target_properties(${XRN_BIN_NAME}_shared PROPERTIES OUTPUT_NAME ${XRN_BIN_NAME})
endif ()

foreach(library IN LISTS XRN_LIBRARIES_HEADERS)
    include_directories(
        ${${library}_INCLUDE_DIRS}
    )
endforeach()
