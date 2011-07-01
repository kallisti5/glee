#
# CMakeLists.txt for glee
# Adapted from glfw (http://glfw.org)
#

project( glee )
cmake_minimum_required(VERSION 2.8)

set( glee_common_sources    ${glee_SOURCE_DIR}/Common/ArrayList.h
                            ${glee_SOURCE_DIR}/Common/Exception.h
                            ${glee_SOURCE_DIR}/Common/String.cpp
                            ${glee_SOURCE_DIR}/Common/String.h
                            ${glee_SOURCE_DIR}/Common/XMLFile.cpp
                            ${glee_SOURCE_DIR}/Common/XMLFile.h 
                            ${glee_SOURCE_DIR}/Common/stdafx.h )
                            
#
# Create shared configuration header
#

configure_file( ${glee_SOURCE_DIR}/libglee.pc.cmake 
                ${glee_BINARY_DIR}/libglee.pc @ONLY )

set( DATA_DIR ${glee_SOURCE_DIR}/DATA )

configure_file( ${glee_SOURCE_DIR}/Common/Config.h.in 
                ${glee_SOURCE_DIR}/Common/Config.h
                ESCAPE_QUOTES )

#
# 
#
add_subdirectory( GLeeXMLGen )
add_subdirectory( GLeeGen )

#
# Create library
#

set( glee_sources ${DATA_DIR}/output/glee.c
                  ${DATA_DIR}/output/glee.h )

set_source_files_properties( ${glee_sources}
                             PROPERTIES GENERATED TRUE )

add_library( libgleeStatic    STATIC    ${glee_sources} )
add_library( libgleeShared    SHARED    ${glee_sources} )

target_link_libraries( libgleeShared opengl32 )

add_custom_target ( update_specs
                    wget --recursive --level=1 --timestamping --directory-prefix=${DATA_DIR}/GLeeXMLGenInput/ --no-host-directories --accept=txt,h --include-directories=registry/specs,registry/api --exclude-directories=registry/doc --user-agent=\"Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/534.30 (KHTML, like Gecko) Chrome/12.0.742.100 Safari/534.30\" http://www.opengl.org/registry/
                    COMMENT "Getting latest specs from http://www.opengl.org/registry/" )

file( GLOB_RECURSE spec_files
      ${DATA_DIR}/GLeeXMLGenInput/registry/*.h
      ${DATA_DIR}/GLeeXMLGenInput/registry/*.txt )

add_custom_command( OUTPUT ${DATA_DIR}/GleeGenInput/extensions.xml
                    COMMAND gleexmlgen
                    MAIN_DEPENDENCY gleexmlgen
                    DEPENDS ${DATA_DIR}/GLeeXMLGenInput/GLExtIgnore.txt
                            ${DATA_DIR}/GLeeXMLGenInput/HeaderIgnore.txt
                            ${DATA_DIR}/GLeeXMLGenInput/registry
                            ${spec_files}
                    COMMENT "Generating GLeeXMLGen inputs" )

add_custom_command( OUTPUT ${DATA_DIR}/output/glee.c
                           ${DATA_DIR}/output/glee.h
                    COMMAND gleegen
                    MAIN_DEPENDENCY gleegen
                    DEPENDS ${DATA_DIR}/GleeGenInput/GLeeVersion.txt
                            ${DATA_DIR}/GleeGenInput/extensions.xml
                    COMMENT "Generating glee.h and glee.c" )

#
# Set up glee on windows
#
if (WIN32)
   # set_target_properties(libjoemathShared PROPERTIES 
   #                         DEFINE_SYMBOL GLEE_BUILD_DLL
   #                         PREFIX ""
   #                         IMPORT_PREFIX ""
   #                         IMPORT_SUFFIX "dll.lib")
endif (WIN32)

set_target_properties( libgleeStatic libgleeShared PROPERTIES 
		               CLEAN_DIRECT_OUTPUT 1
		               OUTPUT_NAME glee )

install( TARGETS libgleeStatic libgleeShared DESTINATION lib )
install( FILES ${glee_BINARY_DIR}/libglee.pc DESTINATION lib/pkgconfig )
  
#
# Install standard files 
#

install( DIRECTORY include/GL DESTINATION include 
         FILES_MATCHING REGEX ".*\\.h")

install( FILES license.txt readme.txt 
         DESTINATION share/doc/glee/)

#
# Uninstall operation
#
configure_file( ${glee_SOURCE_DIR}/cmake_uninstall.cmake.in
                ${glee_BINARY_DIR}/cmake_uninstall.cmake IMMEDIATE @ONLY )

add_custom_target( uninstall
                   ${CMAKE_COMMAND} -P
                   ${uninstall_BINARY_DIR}/cmake_uninstall.cmake )

