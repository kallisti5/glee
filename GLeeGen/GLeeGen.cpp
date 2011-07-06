/////////////////////////////////////////////////////////////////////
//
// GLeeGen.cpp
//
// Processes GLEXT.h and WGLEXT.h to generate CPP source code to the 
// GLEE library
//
//  Version : 6.0
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

#include "GLeeGen.h"
#include <cstring>
using namespace Mirage;
using namespace std;

// TODO: make this data driven
String versionString;//="5.33"; YAY, no longer need to recompile GLEEGEN!!! (see GLeeVersion.txt)
String gleeGenVersionString="7.0"; 

#define INFILE(filename)         (String(DATA_DIR)+String("/GLeeGenInput/")+filename).cStr()
#define OUTFILE(filename)        (String(DATA_DIR)+String("/output/")+filename).cStr()
#define OUTINCLUDEFILE(filename) (String(INCLUDE_DIR)+String("/GL/")+filename).cStr()

int main(int argc, char* argv[])
{
	XMLFile xmlFile;
	xmlFile.load(INFILE("extensions.xml"));

    // Load the version string
	FILE * versionFile=fopen(INFILE("GLeeVersion.txt"),"rt");
	versionString.readFile(versionFile);
	fclose(versionFile);

	generateCode(	OUTFILE("GLee.c"),
					OUTINCLUDEFILE("GLee.h"), xmlFile.root);

	generateExtensionList(OUTFILE("extensionList.txt"),xmlFile.root);
	return 0;
}

void writeFuncPtrs(String &cpp, String &header, XMLElement& xml)
{

	XMLElement *extXML=xml.findFirstElementPtr("extension");
    while(extXML)
	{
		String& extName=extXML->getAttrib("name");

		//add name comment
		cpp+="\n/* ";
		cpp+=extName;
		cpp+=" */\n\n";
		header+="\n/* "+extName+" */\n\n";

		//define the extension
		header+="#ifndef "+extName+"\n";
		header+="#define "+extName+" 1\n";

		//define that it's to be linked with GLee
		header+="#define __GLEE_"+extName+" 1\n";

		cpp+="#ifdef __GLEE_"+extName+"\n";

		//write the constants
		XMLElement& xmlConstants=extXML->getElement("constants");
		XMLElement *xmlConst=xmlConstants.findFirstElementPtr("constant");
		header+="/* Constants */\n";
		while (xmlConst)
		{
			int numSpaces=50-xmlConst->getAttrib("name").length();
			String spaces;
			for (int d=0;d<numSpaces;d++) spaces+=" ";
			header+="#define "+xmlConst->getAttrib("name")+" "+spaces+xmlConst->getAttrib("value")+"\n";
			xmlConst=xmlConstants.findNextElementPtr();
		}

		XMLElement& xmlFunctions=extXML->getElement("functions");
		XMLElement* xmlFunc=xmlFunctions.findFirstElementPtr("function");

        // Write the typedefs in the header
		while (xmlFunc)
		{
			String& funcName=xmlFunc->getAttrib("name");

            // preprocesor guard so we don't define this function twice in two extensions
            header+="#ifndef GLEE_H_DEFINED_" + funcName + "\n";
            header+="#define GLEE_H_DEFINED_" + funcName + "\n";

			header+="  typedef "+xmlFunc->getAttrib("returnType")+" (APIENTRYP "+getFType(funcName)+") (";
			//write the params
			XMLElement *xmlParam=xmlFunc->findFirstElementPtr("param");
			while (xmlParam)
			{
				header+=xmlParam->getAttrib("type")+" "+xmlParam->getAttrib("name");
				xmlParam=xmlFunc->findNextElementPtr();
				if (xmlParam) header+=", ";
			}
			header+=");\n";

            // Write the extern for the func ptr
			header+="  GLEE_EXTERN " + getFType(funcName) + " GLeeFuncPtr_" + funcName + ";\n";

            // Write the alias macro
			header+="  #define " + funcName + " GLeeFuncPtr_" + funcName + "\n";
            header+="#endif\n";

			xmlFunc=xmlFunctions.findNextElementPtr();
		}

	
		//write lazy loaders
		xmlFunc=xmlFunctions.findFirstElementPtr("function");
		while (xmlFunc)
		{
			//write function def
			int lineLength=cpp.length();

			String& funcName=xmlFunc->getAttrib("name");
			String& returnType=xmlFunc->getAttrib("returnType");

            // preprocesor guard so we don't define this function twice in two extensions
            cpp+="#ifndef GLEE_C_DEFINED_" + funcName + "\n";
            cpp+="#define GLEE_C_DEFINED_" + funcName + "\n";
			cpp+="  " + returnType + " __stdcall GLee_Lazy_" + funcName + "(";
			XMLElement *xmlParam=xmlFunc->findFirstElementPtr("param");
			bool bVoid=(!xmlParam);
			while (xmlParam)
			{
				cpp+=xmlParam->getAttrib("type")+" "+xmlParam->getAttrib("name");
				xmlParam=xmlFunc->findNextElementPtr();
				if (xmlParam) cpp+=", ";
			}
			if (bVoid) cpp+="void";
			cpp+=") ";
			//align the function body
			lineLength=cpp.length()-lineLength;
			int filler=max(1,90-lineLength);
			for (int a=0;a<filler;a++) cpp.insertCharAtEnd(' ');
			
			cpp+="{if (GLeeInit()) ";
			bool retVoid=(returnType==String("void") || returnType==String("GLvoid") || returnType==String("VOID"));
			if (!retVoid) cpp+="return ";
			cpp+=funcName+"(";
			xmlParam=xmlFunc->findFirstElementPtr("param");
			while (xmlParam)
			{
				cpp+=xmlParam->getAttrib("name");
				xmlParam=xmlFunc->findNextElementPtr();
				if (xmlParam) cpp+=", ";
			}
			cpp+=");";
			if (!retVoid)
			{
				cpp+=" return ("+returnType+")0;";
			}
			cpp+="}\n";

            // Write the function definition
			cpp+="  " + getFType(funcName) + " GLeeFuncPtr_" + funcName + "=GLee_Lazy_" + funcName +";\n";

            cpp+="#endif\n";
			xmlFunc=xmlFunctions.findNextElementPtr();
		}

		cpp+="#endif \n";//__GLEE_" + ext.name + " defined\n";
		header+="#endif \n";//__GLEE_" + ext.name + " defined\n";
		
		extXML=xml.findNextElementPtr();
	}
}

String getFType(const String& s)
{
	String rv="GLEEPFN"+s+"PROC";
	for (int a=3;a<rv.length()-4;a++)
	{
		rv[a]=toupper(rv[a]);
	}
	return rv;
}

void writeFunctionLinkFunctions(String& cpp, XMLElement& xml, int fileType)
{
	String prefix[3]={"GL","WGL","GLX"};
	GLVersion glVersion;
	int nExtensions=0;

	XMLElement *extXML=xml.findFirstElementPtr("extension");
	while (extXML)
	{
		String& extName=extXML->getAttrib("name");
		cpp+="GLuint __GLeeLink_"+extName+"(void)";

		//link the functions
		XMLElement xmlFunctions=extXML->getElement("functions");
		XMLElement *xmlFunc=xmlFunctions.findFirstElementPtr("function");
		bool bFunctions=(xmlFunc!=NULL);
		if (!bFunctions) 
		{
			cpp+=" {return GLEE_LINK_COMPLETE;}\n\n";
		}
		else
		{
			int nFunctions=0;
			cpp+="\n{\n";
			cpp+="    GLint nLinked=0;\n";
			cpp+="#ifdef __GLEE_" + extName + "\n";
			while (xmlFunc)
			{
				String& funcName=xmlFunc->getAttrib("name");
				//cpp+="    if ((GLeeFuncPtr_" + extName + "_" + funcName + " = (" + getFType(funcName) + ") __GLeeGetProcAddress(\"" + 
				cpp+="    if ((GLeeFuncPtr_" + funcName + " = (" + getFType(funcName) + ") __GLeeGetProcAddress(\"" + 
								funcName + "\"))!=0) nLinked++;\n";
				nFunctions++;
				xmlFunc=xmlFunctions.findNextElementPtr();
			}
			if (bFunctions) cpp+="#endif\n";//__GLEE_" + ext.name + " defined\n";
			String nFuncStr;
			nFuncStr.printf("%d",nFunctions);

			cpp+="    if (nLinked=="+nFuncStr+") return GLEE_LINK_COMPLETE;\n";
			cpp+="    if (nLinked==0) return GLEE_LINK_FAIL;\n";
			cpp+="    return GLEE_LINK_PARTIAL;\n";
			cpp+="}\n\n";
		}
		nExtensions++;
		extXML=xml.findNextElementPtr();
	}

	//write function pointer array for the load functions
	cpp.printf("GLEE_LINK_FUNCTION __GLee%sLoadFunction[%d];\n\n",prefix[fileType].cStr(),nExtensions);
	cpp+="void init"+prefix[fileType]+"LoadFunctions(void)\n{\n";
	extXML=xml.findFirstElementPtr("extension");
	int a=0;
	while (extXML)
	{
		String funcName=String("__GLeeLink_")+extXML->getAttrib("name");
		cpp.printf("    __GLee%sLoadFunction[%d]=%s;\n",prefix[fileType].cStr(),a,funcName.cStr());
		a++;
		extXML=xml.findNextElementPtr();
	}
	cpp+="}\n\n";
}

void writeFunctionLinkCode(String& cpp, XMLElement& xml, int fileType)
{
	GLVersion glVersion;

	XMLElement *extXML=xml.findFirstElementPtr("extension");

	while (extXML)
	{
		String& extName=extXML->getAttrib("name");

		if (fileType==FT_GLEXT && extName.search("GL_VERSION_")!=-1) //version extension
		{
			glVersion.parse(extName.cStr());
			cpp.printf("    if (version>=%d)\n",glVersion.getIntValue());
		}
		else
		{
			cpp+="    if (__GLeeCheckExtension(\"" + extName + "\", &extensionNames) )\n";
		}

		cpp+="    {\n";
		//set the querying variable
		cpp+="        _GLEE_";
		if (fileType==FT_GLEXT) cpp+=extName.trimStart(3); //trim the "GL"
		else cpp+=extName; //WGL/GLX variables have full name following GLEE 
		cpp+=" = GL_TRUE;\n";

		//link the functions
		cpp+="        __GLeeLink_"+extName+"();\n";

		cpp+="    }\n";
		extXML=xml.findNextElementPtr();
	}
}


void writeBools(String& cpp, String& header, XMLElement& xml, int fileType)
{
	String defineBlock;
	cpp+="\n/* Extension querying variables */\n\n";
	header+="\n/* Extension querying variables */\n\n";

	XMLElement *extXML=xml.findFirstElementPtr("extension");
	while (extXML)
	{
		String &name=extXML->getAttrib("name");
		assert(name.length()>4);
		String shortName;
		if (fileType==FT_GLEXT) shortName=name.trimStart(3);
		else shortName=name;
		String extVar="GLboolean _GLEE_";
		extVar+=shortName;
		header+="GLEE_EXTERN " + extVar + ";\n";

		defineBlock+="#define GLEE_"+shortName+"     GLeeEnabled(&_GLEE_"+shortName+")\n";
		cpp+=extVar + " = GL_FALSE;\n";
		extXML=xml.findNextElementPtr();
	}
	header+="\n/* Aliases for extension querying variables */\n\n";
	header+=defineBlock;

}

void generateExtensionList(const char * filename, XMLElement &xml)
{
	GLVersion glVersion;
    String fileString;
	fileString+="--------------------------------------------------------------\n";
	fileString+="GLee "+versionString+" Supported Extensions and Core Functions\n";
	fileString+="--------------------------------------------------------------\n\n";
	String extString;
	int nExtensions[3]={0,0,0};

	for (int a=0;a<3;a++)
	{
		if (a==0)		extString+="\nGL Extensions\n-------------------\n\n";
		else if (a==1)	extString+="\nWGL Extensions\n------------------\n\n";
		else if (a==2)	extString+="\nGLX Extensions\n------------------\n\n";
		XMLElement &xmlExtensions=xml.elements[a];
		XMLElement *xmlExt=xmlExtensions.findFirstElementPtr("extension");
		if (a==0)
		{
			while (xmlExt)
			{
				String& extName=xmlExt->getAttrib("name");
				if (extName.search("GL_VERSION_")==0)
				{
					GLVersion tempVersion;
					tempVersion.parse(extName.cStr());
					if (tempVersion>glVersion) glVersion=tempVersion;
				}
				else extString+=extName+"\n"; 
				nExtensions[a]++;
				xmlExt=xmlExtensions.findNextElementPtr();
			}
		}
		else
		{
			while (xmlExt)
			{
				extString+=xmlExt->getAttrib("name")+"\n"; 
				nExtensions[a]++;
				xmlExt=xmlExtensions.findNextElementPtr();
			}
		}
	}
	fileString.printf("Core OpenGL Version: %d.%d\n",glVersion.major,glVersion.minor);
	fileString.printf("%d extensions supported (%d GL | %d WGL | %d GLX)\n",nExtensions[0]+nExtensions[1]+nExtensions[2],nExtensions[0],nExtensions[1],nExtensions[2]);
	fileString+=extString;

	FILE * file=fopen(filename,"wt");
    fileString.writeFile(file);
	fclose(file);
}

void generateCode(const char * cppFilename,
				  const char * headerFilename, 
				  XMLElement &xml)
{
	const char * elseIfAppleString = "#elif defined(__APPLE__) || defined(__APPLE_CC__)\n";

	FILE * licenseFile=fopen(INFILE("License.txt"),"rt");
	String license;
	license.readFile(licenseFile);
	fclose(licenseFile);

	FILE * exceptionsFile=fopen(INFILE("exceptions.txt"),"rt");
	String exceptions;
	exceptions.readFile(exceptionsFile);
	fclose(exceptionsFile);

	FILE * headerTopFile=fopen(INFILE("gleehtop.txt"),"rt");
	String headerTop;
	headerTop.readFile(headerTopFile);
	fclose(headerTopFile);

	FILE * cppTopFile=fopen(INFILE("GLeeCTop.txt"),"rt");
	String cppTop;
	cppTop.readFile(cppTopFile);
	fclose(cppTopFile);

	FILE * headerFile=fopen(headerFilename,"wt");
	FILE * cppFile=fopen(cppFilename,"wt");

	String topBorder
			=	"/***************************************************************************\n";
	topBorder+=	"*\n";

	String description
		=			"* GLee (OpenGL Easy Extension library)        \n";
	description+=	"* Version : "; description+=versionString; 
	description+=	"\n*\n";
	description+=	license + "\n\n";
	description+=   "* Web: http://elf-stone.com/glee.php\n";
	description+="*\n";
	description+=	"* [This file was automatically generated by GLeeGen "+gleeGenVersionString+"\n" ;
	description+="*\n";
	description+="***************************************************************************/\n\n";

	//write the header file description
	String header=topBorder;
	header+="* GLee.h \n";
	header+=description;
	header+=headerTop;

	//write the cpp file description
	String cpp;
	cpp+=topBorder;
	cpp+="* GLee.c\n" ;
	cpp+=description;
	cpp+=cppTop;

	writeBools(cpp,header,xml.elements[FT_GLEXT],FT_GLEXT);
	writeExtensionNames(cpp,xml.elements[FT_GLEXT],FT_GLEXT);

	header+="\n";
	header+=exceptions+"\n";

	writeFuncPtrs(cpp,header,xml.elements[FT_GLEXT]);
    
	//win32 specific
	cpp+="\n/* WGL */\n\n";
	cpp+="#ifdef _WIN32\n";
	header+="\n/* WGL  */\n\n";
	header+="#ifdef _WIN32\n";

	writeBools(cpp,header,xml.elements[FT_WGLEXT],FT_WGLEXT);
	writeExtensionNames(cpp,xml.elements[FT_WGLEXT],FT_WGLEXT);
	writeFuncPtrs(cpp,header,xml.elements[FT_WGLEXT]);

	cpp+=elseIfAppleString;
	cpp+="#else /* GLX */\n";
	header+=elseIfAppleString;
	header+="#else /* GLX */\n";
	writeBools(cpp,header,xml.elements[FT_GLXEXT],FT_GLXEXT);
	writeExtensionNames(cpp,xml.elements[FT_GLXEXT],FT_GLXEXT);
	writeFuncPtrs(cpp,header,xml.elements[FT_GLXEXT]);
	cpp+="#endif /* end GLX */\n";
	header+="#endif /*end GLX */\n";

	cpp+="/*****************************************************************\n"
		 "* Extension link functions\n"
		 "*****************************************************************/\n\n";

	writeFunctionLinkFunctions(cpp,xml.elements[FT_GLEXT],FT_GLEXT);
	cpp+="#ifdef _WIN32\n";
	writeFunctionLinkFunctions(cpp,xml.elements[FT_WGLEXT],FT_WGLEXT);
	cpp+=elseIfAppleString;
	cpp+="#else /* Linux */\n";
	writeFunctionLinkFunctions(cpp,xml.elements[FT_GLXEXT],FT_GLXEXT);
	cpp+="#endif /* end Linux */\n\n";

	//add the fixed code functions to the cpp file
	FILE * functionsFile=fopen(INFILE("GLeeFunctions.txt"),"rt");
	if (functionsFile)
	{
		String functions;
		functions.readFile(functionsFile);
		cpp+=functions;
	}
	fclose(functionsFile);

	writeFunctionLinkCode(cpp,xml.elements[FT_GLEXT],FT_GLEXT);
	cpp+="#ifdef _WIN32\n";
	writeFunctionLinkCode(cpp,xml.elements[FT_WGLEXT],FT_WGLEXT);
	cpp+=elseIfAppleString;
	cpp+="#else /* GLX */\n";
	writeFunctionLinkCode(cpp,xml.elements[FT_GLXEXT],FT_GLXEXT);
	cpp+="#endif /* end GLX */\n\n";

	//add the cleanup code
	cpp+="    __GLeeExtList_clean(&extensionNames);\n";
	cpp+="    return GL_TRUE;\n}\n";

	//add the external function declarations
	FILE * hBottomFile=fopen(INFILE("GLeeHBottom.txt"),"rt");
	String hBottom;
	hBottom.readFile(hBottomFile);
	fclose(hBottomFile);

	header+=hBottom;
	cpp.writeFile(cppFile);
	fclose(cppFile);
	header.writeFile(headerFile);
	fclose(headerFile);
}

void writeExtensionNames(String &cpp, XMLElement& xml, int fileType)
{
	String prefixStr[3]={"GL","WGL","GLX"};
	cpp+="\n";
	cpp+="/*  "+prefixStr[fileType]+" Extension names */\n\n";

	//write the array string
	int nExtensions=0;
	int maxWidth=0;
	String arrayStr;
	XMLElement* xmlExt=xml.findFirstElementPtr("extension");
	while (xmlExt)
	{
		String& name=xmlExt->getAttrib("name");
		maxWidth=max(maxWidth, name.length());
		arrayStr+="    \""+name+"\"";
		nExtensions++;
		xmlExt=xml.findNextElementPtr();
		if (xmlExt) arrayStr+=",";
		arrayStr+="\n";
	}
	cpp+="char __GLee"+prefixStr[fileType]+"ExtensionNames";
	cpp.printf("[%d][%d]={\n",nExtensions,maxWidth+1);
	cpp+=arrayStr;
	cpp+="};\n";
	cpp.printf("int __GLee%sNumExtensions=%d;\n",prefixStr[fileType].cStr(),nExtensions);
}

bool GLVersion::parse(const char *verString)
{

	char verPrefix[]="GL_VERSION_";
	int n=strlen(verPrefix);
	int a=0;
	for (a=0;a<n;a++)
	{
		if (verString[a]!=verPrefix[a]) return false;
	}
	unsigned int lmajor=(int)verString[a++]-(int)'0';
	if (lmajor>9) return false;

	if (verString[a++]!='_') return false;

	unsigned int lminor=(int)verString[a++]-(int)'0';
	if (lminor>9) return false;

	this->major=lmajor;
	this->minor=lminor;

	return true;
}
