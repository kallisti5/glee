//To disable generic vertex attributes in this shader, comment out the following line
#define USEVERTEXATTRIBS

#ifdef USEVERTEXATTRIBS
attribute vec4 attrib;
#endif

void main(void)
{
#ifdef USEVERTEXATTRIBS
	gl_FrontColor = gl_Color * attrib;
#else
	gl_FrontColor = gl_Color;                //uncomment this line to enable the shader   
#endif
	gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
	
}
