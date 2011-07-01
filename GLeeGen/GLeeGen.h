/////////////////////////////////////////////////////////////////////
//
// GLeeGen.h
//
// Processes GLEXT.h and WGLEXT.h to generate CPP source code to the 
// GLEE library
//
//  Version : 3.05
//
//  Copyright (c)2003  Ben Woodhouse  All rights reserved.
//
//  Redistribution and use in source and binary forms, with or without
//  modification, are permitted provided that the following conditions are 
//  met:
//  1. Redistributions of source code must retain the above copyright
//  notice, this list of conditions and the following disclaimer as
//  the first lines of this file unmodified.
//  2. Redistributions in binary form must reproduce the above copyright
//  notice, this list of conditions and the following disclaimer in the
//  documentation and/or other materials provided with the distribution.
//
//  THIS SOFTWARE IS PROVIDED BY BEN WOODHOUSE ``AS IS'' AND ANY EXPRESS OR
//  IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
//  OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
//  IN NO EVENT SHALL BEN WOODHOUSE BE LIABLE FOR ANY DIRECT, INDIRECT,
//  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
//  NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
//  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
//  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
//  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
//  THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.


//#include <conio.h>
#include <utility>
#include "../Common/stdafx.h"
#include "../Common/XMLFile.h"
#include "../Common/Config.h"

//CLASSES
struct GLVersion
{
	GLVersion()							{major=0; minor=0;}
	bool parse(const char *verString);
	bool operator==(const GLVersion& b) {return major==b.major && minor==b.minor;}
	bool operator!=(const GLVersion& b) {return major!=b.major || minor!=b.minor;}
	bool operator<(const GLVersion& b)  {return (major<b.major) || (major==b.major && minor<b.minor);}
	bool operator>(const GLVersion& b)  {return (major>b.major) || (major==b.major && minor>b.minor);}
	int getIntValue()					{return major<<8 | minor;}
	unsigned int major;
	unsigned int minor;
};

enum FileType
{
	FT_GLEXT,
	FT_WGLEXT,
	FT_GLXEXT
};
/*
struct _Extension
{
	_Extension(){}
	_Extension(const _Extension& src){*this=src;}

	_Extension& operator =(const _Extension& src)
	{
		name=src.name;
		assert(src.functionNames.size()==src.typedefs.size());
		functionNames.clear();
		typedefs.clear();
		int numFunctions=src.typedefs.size();
		functionNames=src.functionNames;
		typedefs=src.typedefs;
		constants=src.constants;
		typedefLines=src.typedefLines;
		return *this;
	}
	Mirage::String name;
	std::vector<Mirage::String> functionNames;
	std::vector<Mirage::String> typedefs;
	std::vector<Mirage::String> typedefLines;

	std::vector<std::pair<Mirage::String,Mirage::String> > constants;
};

typedef std::vector<_Extension> ExtensionList;
*/

//functions 
int main(int argc, char* argv[]);
void writeFuncPtrs(Mirage::String &cpp, Mirage::String &header, Mirage::XMLElement& xml);
void writeFunctionLinkFunctions(Mirage::String& cpp, Mirage::XMLElement& xml, int fileType);
void writeFunctionLinkCode(Mirage::String& cpp, Mirage::XMLElement& xml, int fileType);
void writeBools(Mirage::String& cpp, Mirage::String& header, Mirage::XMLElement& xml, int fileType);
void writeExtensionNames(Mirage::String &cpp, Mirage::XMLElement& xml, int fileType);


void generateCode(const char * cppFilename,
				  const char * headerFilename, 
				  Mirage::XMLElement &xml);

Mirage::String getFType(const Mirage::String& s);
void generateExtensionList(const char * filename, Mirage::XMLElement &xml);

