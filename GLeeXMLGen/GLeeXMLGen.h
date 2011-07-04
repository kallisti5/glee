// GLeeXMLGen.cpp : Defines the entry point for the console application.
//
#include "../Common/Config.h"
#include "../Common/stdafx.h"
#include "../Common/XMLFile.h"
#include "regexp_wrapper.h"

using namespace Mirage;

enum FileType
{
	FT_GLEXT,
	FT_WGLEXT,
	FT_GLXEXT
};

enum SpecParseResult
{
    SPEC_IGNORED,
    SPEC_PARSED,
    SPEC_FAILED
};


int main(int argc, char* argv[]);
bool readNames(String &extFileString, ArrayList<String>& names);
bool readConstants(String& extFileString, XMLElement& XMLOut, int typeFilter);
bool readFunctions(String& extFileString, XMLElement& XMLOut, int typeFilter);
const char *readType(const char *pos, String& typeOut);
bool readParams(const char *pos, XMLElement& funcXML);
SpecParseResult readExtensionSpec(String& extFileString, XMLElement& extensionsXML, ArrayList<String>& ignoreList);
bool getSpecFilenames(const String& startDir, ArrayList<String>& specFilenamesOut);
bool readStringList(const char * filename, ArrayList<String>& stringListOut);

regex& getTypeRegex();

void addPrefixes(XMLElement& XMLOut);
void convertType(String& type);

////////////////////////////////////////////////////////
//For parsing headers (from old GLeeGen)
////////////////////////////////////////////////////////
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
	ArrayList<Mirage::String> functionNames;
	ArrayList<Mirage::String> typedefs;
	ArrayList<Mirage::String> typedefLines;

	ArrayList<std::pair<Mirage::String,Mirage::String> > constants;
};

struct BlockPos
{
	BlockPos(){pos=level=0;}
	int pos;
	int level;
};

void readHeaderExtensions(XMLElement& XMLOut, ArrayList<String>& extensionsReadOut, ArrayList<String>& ignoreList);
void getHeaderExtensions(int fileType, ArrayList<_Extension> &extensions, ArrayList<String>& ignoreList);
void convertHeaderExtensionsToXML(ArrayList<_Extension> &extensions, XMLElement& xmlOut, ArrayList<String>& extensionsReadOut);
bool readHeaderConstant(std::pair<String,String >& constPair, XMLElement& XMLOut);
bool readHeaderFunction(String& typeDefString, String& funcName, XMLElement& XMLOut);
bool searchForFuncStart(String &block, int &pPos, int& lineEndPos, int fileType, const String& APIENTRYStr, bool ptr);
bool readIfBlock(BlockPos &pos, String& str, const String& blockStart, String &chunkOut);
bool getChunk(int &pos, String& str, const String &start, const String &end, String &tokenOut);
bool isIgnored(const String &extName, const ArrayList<String>& ignoreList);

