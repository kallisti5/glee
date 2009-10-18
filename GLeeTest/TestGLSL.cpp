////////////////////////////////////////////////////////////////////////
//
//  TestGLSL.cpp 
//	By Ben Woodhouse 
//  http://elf-stone.com
//
//	Description:
//  Program to reproduce bug with ATi OpenGL implementation
//  which causes a crash when a GLSL vertex shader is used in 
//	combination with VBO and generic attributes.
//
//  Both VBO and generic vertex attributes must be used in order to 
//  cause the crash, and vertex attributes must be used in the vertex 
//  shader.  
//
//  To disable VBO, set the value of gUseVBO to false. To
//	disable the use of generic vertex attibutes in the shader, comment
//  out the line #define USEVERTEXATTRIBS
//
//  Tested on Radeon 9700 Pro, Radeon 9600 Pro Mobile Turbo (both with 
//  the Catylist 4.1 OpenGL DLL).
//
////////////////////////////////////////////////////////////////////////

#include ".\testglsl.h"

bool gUseVBO=false;//disabling this prevents the program from crashing

GLhandleARB program;
GLhandleARB vs;
VertexArray gVertexArray;

bool init()
{
	if (!GLeeInit()) 
	{
		MessageBox(NULL,GLeeGetErrorString(),"GLee Initialisation Error",MB_OK|MB_ICONEXCLAMATION);
		return false;
	}
	//check for extensions
	if (!GLEE_NV_gpu_program4)
    {
		MessageBox(NULL,"GLEE_NV_gpu_program4 not supported","Initialisation Error",MB_OK|MB_ICONEXCLAMATION);
    }
	if (!GLEE_ARB_vertex_buffer_object)
	{
		MessageBox(NULL,"VBO not supported","Initialisation Error",MB_OK|MB_ICONEXCLAMATION);
		return false;
	}
	if (!(GLEE_ARB_shader_objects && GLEE_ARB_vertex_shader))
	{
		MessageBox(NULL,"GLSL extensions not supported","Initialisation Error",MB_OK|MB_ICONEXCLAMATION);
		return false;
	}
	//load, compile, attach and link the vertex shader
	if (!loadShader("simple.vert"))
	{
		MessageBox(NULL,"Error loading shader","Initialisation Error",MB_OK|MB_ICONEXCLAMATION);
		return false;
	}
	//get the attrib location
	int attribLocation=glGetAttribLocationARB(program, "attrib");
	if (attribLocation==-1)
	{
		//can't find the attrib location in the shader, so use location 6 instead
		attribLocation=6;
	}
	//initialise the VA
	if (!initVertexArray(gVertexArray, attribLocation))
	{
		MessageBox(NULL,"Vertex Array initialisation error","Initialisation Error",MB_OK|MB_ICONEXCLAMATION);
		return false;
	}

    char * versionString = (char*)glGetString(GL_VERSION);
    printf("GL Version : %s\n", versionString );

	return true;
}

bool render()
{
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);	
	glUseProgramObjectARB(program);
	glLoadIdentity();
	glTranslatef(-1.5f,0.0f,-6.0f);	
	renderVertexArray(gVertexArray);
	if (glGetError()) 
	{
		MessageBox(NULL,"An OpenGL error occurred in Render()","ERROR",MB_OK|MB_ICONEXCLAMATION);
		return false;
	}
	glUseProgramObjectARB(0);
	return true;
}

bool shutdown()
{
	destroyVertexArray(gVertexArray);
	glDeleteObjectARB(program); 
	glDeleteObjectARB(vs);	
	return true;
}

//shader initialisation
bool loadShader(char *filename)
{
	FILE *file=fopen(filename,"rt");
	if (!file) return false;
	String source;
	source.readFile(file);
	fclose(file);
	int len=source.length();

	//compile the shader
	program=glCreateProgramObjectARB();
	if (glGetError()!=GL_NO_ERROR) return false;
	GLhandleARB vs=glCreateShaderObjectARB(GL_VERTEX_SHADER_ARB);
	if (glGetError()!=GL_NO_ERROR) return false;
	const char *sourceStr=source.cStr();
	glShaderSourceARB(vs, 1, &sourceStr, &len);
	if (glGetError()!=GL_NO_ERROR) return false;
	glCompileShaderARB(vs);
	if (glGetError()!=GL_NO_ERROR) return false;

	int blen=0,slen=0;
	glGetObjectParameterivARB(vs, GL_OBJECT_INFO_LOG_LENGTH_ARB , &blen);
	if (blen > 1) 
	{
		String iLog;
		iLog.resize(blen);
		glGetInfoLogARB(vs, blen, &slen, &iLog[0]);
		MessageBox(NULL,iLog.cStr(),"Shader Compilation Error",MB_OK|MB_ICONEXCLAMATION);
		return false;
	}

	//attach the shader and link the program
	glAttachObjectARB(program, vs);
	if (glGetError()!=GL_NO_ERROR) return false;
	glLinkProgramARB(program);
	if (glGetError()!=GL_NO_ERROR) return false;

	return true;
}


//Vertex Array functions
bool initVertexArray(VertexArray &va, int attribLocation)
{
	float vArray[]={0,1,0,  //vertices
					-1,-1,0,
					1,-1,0,

					1,1,1,1, //colours (starts at [9])
					0,1,1,1,
					1,0,0,1,
					
					1,1,1,1, //attributes (starts at [21])
					1,1,1,1,
					1,1,1,1};
	uint indices[]={0,1,2};

	va.numVerts=3;
	va.numIndices=3;
	uint bufSize=va.numVerts*(4+4+3)*sizeof(float);
	va.attribLocation=attribLocation;

	if (gUseVBO)
	{
		va.useVBO=true;
		//create a vertex array buffer
		glGenBuffersARB(1,&va.bufName);
		glBindBufferARB(GL_ARRAY_BUFFER_ARB, va.bufName);
		glBufferDataARB(GL_ARRAY_BUFFER_ARB, bufSize, vArray, GL_STATIC_DRAW_ARB);
		va.pVert=VBO_BUFFER_OFFSET(0);
		va.pColour=VBO_BUFFER_OFFSET(9*sizeof(float));
		va.pAttrib=VBO_BUFFER_OFFSET(21*sizeof(float));

		//create an index array buffer
		glGenBuffersARB(1,&va.indexBufName);
		glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, va.indexBufName);
		glBufferDataARB(GL_ELEMENT_ARRAY_BUFFER_ARB, va.numIndices*sizeof(uint), indices, GL_STATIC_DRAW_ARB);
		va.indexBuffer=VBO_BUFFER_OFFSET(0);
		if (glGetError()!=GL_NO_ERROR) return false;
	}
	else //keep the buffer in memory
	{
		va.useVBO=false;
		va.buffer=(char *)new float[bufSize]; 
		memcpy(va.buffer,vArray,bufSize);
		va.pVert=va.buffer;
		va.pColour=&va.buffer[9*sizeof(float)];
		va.pAttrib=&va.buffer[21*sizeof(float)];
		va.indexBuffer=(char *)new int[va.numIndices];
		va.pIndices=va.indexBuffer;
		memcpy(va.indexBuffer,indices,sizeof(int)*3);
	}

	return true;
}

void renderVertexArray(VertexArray &va)
{
	enableVertexArray(va);
	if (va.useVBO) glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB,va.indexBufName);
	glDrawElements(GL_TRIANGLES, va.numIndices, GL_UNSIGNED_INT, va.pIndices);
	disableVertexArray(va);
}

void enableVertexArray(VertexArray &va)
{
	if (va.useVBO) glBindBufferARB(GL_ARRAY_BUFFER_ARB,va.bufName);
	glVertexPointer(3, GL_FLOAT, 0, va.pVert);
	glEnableClientState(GL_VERTEX_ARRAY);     
	glColorPointer(4,GL_FLOAT, 0, va.pColour); 
	glEnableClientState(GL_COLOR_ARRAY);     
	glVertexAttribPointerARB(va.attribLocation, 4, GL_FLOAT, GL_FALSE, 0, va.pAttrib);
	glEnableVertexAttribArrayARB(va.attribLocation);
}
void disableVertexArray(VertexArray &va)
{
	glDisableClientState(GL_VERTEX_ARRAY);     
	glDisableClientState(GL_COLOR_ARRAY);      
	glDisableVertexAttribArrayARB(va.attribLocation);
}

bool destroyVertexArray(VertexArray& va)
{
	if (va.useVBO)
	{
		glDeleteBuffersARB(1,&va.bufName);
		glDeleteBuffersARB(1,&va.indexBufName);
		if (glGetError()!=GL_NO_ERROR) return false;
	}
	else
	{
		delete [] va.buffer;
		delete [] va.indexBuffer;
	}
	return true;
}