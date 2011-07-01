if( CMAKE_COMPILER_IS_GNUCXX )
    find_package( Boost REQUIRED )
endif()

set( gleexmlgen_sources GLeeXMLGen.cpp
                        GLeeXMLGen.h )
                        
add_executable( gleexmlgen ${gleexmlgen_sources} ${glee_common_sources} ) 

if( CMAKE_COMPILER_IS_GNUCXX )
    target_link_libraries( gleexmlgen ${BOOST_REGEX_LIBRARY} )
endif()

