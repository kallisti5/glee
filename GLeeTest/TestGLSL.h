#pragma once
#include "stdafx.h"
#include "..\DATA\output\GLee.h"			// Extension loading library (includes gl\gl.h)
#include "String.h"

//types, #defines
#define VBO_BUFFER_OFFSET(i) ((char *)NULL + (i))

struct VertexArray
{
	bool useVBO;

	//vertex array stuff
	uint bufName; 
	uint numVerts;    
	char *pVert;
	char *pColour;
	char *pAttrib;
	int attribLocation;

	//index array stuff
	uint indexBufName;
	uint numIndices;
	char *pIndices;

	//local buffers (used if VBO is disabled)
	char *buffer;
	char *indexBuffer;
};

//functions
bool init();
bool loadShader(char *filename);

bool render();
bool shutdown();

bool initVertexArray(VertexArray &va, int attribIndex);
void enableVertexArray(VertexArray &va);
void renderVertexArray(VertexArray &va);
void disableVertexArray(VertexArray &va);
bool destroyVertexArray(VertexArray &va);

