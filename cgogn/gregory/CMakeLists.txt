project(cgogn_gregory
	LANGUAGES CXX
)

find_package(cgogn_core REQUIRED)
find_package(cgogn_geometry REQUIRED)

# Hide symbols by default
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN YES)

add_library(${PROJECT_NAME} STATIC "")
add_library(cgogn::gregory ALIAS ${PROJECT_NAME})

target_sources(${PROJECT_NAME}	PRIVATE
        "${CMAKE_CURRENT_LIST_DIR}/algos/gregory_evaluation.cpp"
	"${CMAKE_CURRENT_LIST_DIR}/ui_modules/gregory_render_pc.h"
)

set_target_properties(${PROJECT_NAME} PROPERTIES
	DEBUG_POSTFIX "_d"
	EXPORT_NAME gregory 
	LINKER_LANGUAGE CXX
)

target_include_directories(${PROJECT_NAME} PUBLIC
	$<BUILD_INTERFACE:${CMAKE_BINARY_DIR}>
	$<BUILD_INTERFACE:${CGOGN_SOURCE_DIR}>
	$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_link_libraries(${PROJECT_NAME} cgogn::core cgogn::geometry)

# Write out cgogn_gregory_export.h to the current binary directory
generate_export_header(${PROJECT_NAME})

set(PKG_CONFIG_REQUIRES "cgogn_core cgogn_geometry")
configure_file(${PROJECT_SOURCE_DIR}/cgogn_gregory.pc.in ${CMAKE_CURRENT_BINARY_DIR}/cgogn_gregory.pc @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/cgogn_gregory.pc
	DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
	COMPONENT cgogn_gregory_libraries
)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
	DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/cgogn/gregory
	COMPONENT cgogn_gregory_headers
	FILES_MATCHING PATTERN "*.h"
	REGEX "(examples|tests)" EXCLUDE
)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/cgogn_gregory_export.h
	DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/cgogn/gregory
	COMPONENT cgogn_gregory_headers
)

cgogn_create_package("${CMAKE_CURRENT_SOURCE_DIR}")

# VisualStudio nice source organisation
set_target_properties(${PROJECT_NAME} PROPERTIES FOLDER cgogn)
