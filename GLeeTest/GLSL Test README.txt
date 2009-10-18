GLSL Test
By Ben Woodhouse
Reproduces a possible bug with ATi's GLSL implementation

The program crashes VBO, generic attributes and a GLSL vertex shader are used together. An access violation trying to read location NULL (0x00000000) is reported. 

Notes:
To save time, I used nehe's base code to set up the window. I also used my GLee library to link the extensions. However, all the interesting stuff is in  TestGLSL.cpp and TestGLSL.h, and the vertex shader, simple.vert. The functions init(), shutdown() and render() should be pretty self-explanatory; the vertex shader just multiplies the current colour by a vertex attribute.

Both VBO and generic vertex attributes must be used in order to cause the crash, and vertex attributes must be used in the vertex shader.  To disable VBO, set the value of gUseVBO in TestGLSL.cpp to false. To disable the use of generic vertex attibutes in the shader, comment out the line #define USEVERTEXATTRIBS.
