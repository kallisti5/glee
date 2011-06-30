prefix=@CMAKE_INSTALL_PREFIX@
exec_prefix=${prefix}
libdir=${exec_prefix}/lib
includedir=${prefix}/include

Name: glee
Description: GL Easy Extension Library
Version: 5.4.1 
URL: git://lon.eso.me.uk/joemath 
Libs: -L${libdir} -lglee
Cflags: -I${includedir}
