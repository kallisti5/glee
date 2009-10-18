cd ..\..\DATA\GLeeXMLGenInput\specs

wget -A txt -r -l 1 -I registry/specs -X registry/doc --user-agent="Mozilla/4.0 (compatible; MSIE 6.0; Windows NT 5.1)" http://www.opengl.org/registry/

wget http://www.opengl.org/registry/api/glext.h
wget http://www.opengl.org/registry/api/wglext.h
wget http://www.opengl.org/registry/api/glxext.h

md extspecs
md headers

xcopy /S www.opengl.org\registry\*.* extspecs
rd /s /q www.opengl.org
move *.h headers
