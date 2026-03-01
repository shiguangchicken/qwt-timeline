set(TIMELINE_QWT_QMAKE "/home/meng/Qt/6.9.3/gcc_64/bin/qmake" CACHE FILEPATH "Path to qmake used for building timeline/qwt")

set(TIMELINE_QWT_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/qwt)
set(TIMELINE_QWT_BUILD_DIR ${CMAKE_CURRENT_BINARY_DIR}/qwt-build)
set(TIMELINE_QWT_STAMP_FILE ${TIMELINE_QWT_BUILD_DIR}/.qwt_built.stamp)

if(NOT EXISTS ${TIMELINE_QWT_SOURCE_DIR}/qwt.pro)
    message(FATAL_ERROR "Qwt submodule is missing. Run: git submodule update --init --recursive")
endif()

if(NOT EXISTS ${TIMELINE_QWT_QMAKE})
    message(FATAL_ERROR "Configured qmake not found: ${TIMELINE_QWT_QMAKE}")
endif()

find_program(TIMELINE_QWT_MAKE_EXECUTABLE NAMES make gmake REQUIRED)

add_custom_command(
    OUTPUT ${TIMELINE_QWT_STAMP_FILE}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${TIMELINE_QWT_BUILD_DIR}
    COMMAND ${TIMELINE_QWT_QMAKE} ${TIMELINE_QWT_SOURCE_DIR}/qwt.pro "QWT_CONFIG+=QwtOpenGL"
    COMMAND ${TIMELINE_QWT_MAKE_EXECUTABLE}
    COMMAND ${CMAKE_COMMAND} -E touch ${TIMELINE_QWT_STAMP_FILE}
    WORKING_DIRECTORY ${TIMELINE_QWT_BUILD_DIR}
    COMMENT "Building qwt submodule with qmake (${TIMELINE_QWT_QMAKE})"
    VERBATIM
)

add_custom_target(timeline_qwt_build DEPENDS ${TIMELINE_QWT_STAMP_FILE})

add_library(timeline_widgets STATIC
    theme.h
    timeline_event.h
    timeline_node.h
    timeline_node.cpp
    timeline_node_model.h
    timeline_node_model.cpp
    timeline_widget.h
    timeline_widget.cpp
    timeline_view.h
    timeline_view.cpp
)

target_include_directories(timeline_widgets
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${TIMELINE_QWT_SOURCE_DIR}/src
)

target_link_directories(timeline_widgets
    PUBLIC
        ${TIMELINE_QWT_BUILD_DIR}/lib
)

target_link_libraries(timeline_widgets
    PUBLIC
        Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::OpenGLWidgets
        qwt
)

add_dependencies(timeline_widgets timeline_qwt_build)
