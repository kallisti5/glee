set( gleexmlgen_sources GLeeXMLGen.cpp
                        GLeeXMLGen.h
                        regexp_wrapper.h
                        regexp/Matcher.cpp
                        regexp/Pattern.cpp
                        regexp/regexp/Matcher.h
                        regexp/regexp/Matcher.h )

include_directories( regexp )
                        
add_executable( gleexmlgen ${gleexmlgen_sources} ${glee_common_sources} ) 

