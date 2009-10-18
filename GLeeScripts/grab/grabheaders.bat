cd ..\..\DATA\GLeeXMLGenInput\specs

wget http://www.opengl.org/registry/api/glext.h
wget http://www.opengl.org/registry/api/wglext.h
wget http://www.opengl.org/registry/api/glxext.h

md extspecs
md headers

xcopy /S oss.sgi.com\projects\ogl-sample\registry\*.* extspecs
rd /s /q oss.sgi.com
move *.h headers

