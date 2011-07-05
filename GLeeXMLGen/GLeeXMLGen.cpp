// GLeeXMLGen.cpp : Defines the entry point for the console application.
//

#include "GLeeXMLGen.h"
#include <stdint.h>
#include <cstring>
//#include <conio.h>
#include "../Common/stdafx.h"
#include "../Common/XMLFile.h"
#ifdef _WIN32
    #include <windows.h>
#else
    #include <sys/types.h>
    #include <dirent.h>
    #undef min
    #undef max
#endif

using namespace Mirage;

/*
//void GenQueriesARB(sizei n, uint *ids);

<extensions>
    <gl>
		<extension name="ARB_occlusion_query">
			<functions>
				<function name="GenQueriesARB" returnType="void">
					<param type="sizei" name="n">
					<param type="uint *" name="ids">
				</function>
			</functions>

			<constants>
				<constant name="SAMPLES_PASSED_ARB" value="0x8914">
				<constant name="QUERY_COUNTER_BITS_ARB" value="0x8864">
			</constants>
		</extension>
	</gl>
</extensions>
*/

#define INFILE(filename) (String(DATA_DIR)+String("/GLeeXMLGenInput/")+filename).cStr()
#define OUTFILE(filename) (String(DATA_DIR)+String("/GLeeGenInput/")+filename).cStr()

void getSubstring(const char * start, const char * end, String& string_out)
{
	int len=(intptr_t)end-(intptr_t)start;
	string_out.resize(len);
	for (int a=0;a<len;a++)
		string_out[a]=start[a];
}

int main(int argc, char* argv[])
{
	ArrayList<String> ignoreList;
	ArrayList<String> includedExtensions;
	ArrayList<String> headerIgnoreList;
	readStringList(INFILE("GLExtIgnore.txt"),ignoreList);
	readStringList(INFILE("HeaderIgnore.txt"),headerIgnoreList);



    // Read the spec filenames
	ArrayList<String> specFilenames;

	if (!getSpecFilenames(String(INFILE("registry/specs")), specFilenames)) return -1;

    // Read the patched spec filenames
	ArrayList<String> patchedSpecFilenames;
	if (!getSpecFilenames(String(INFILE("PatchedSpecs")), patchedSpecFilenames)) return -1;

    uint32_t numExtensionsPatched=0;
    // Build the final filenames list
	ArrayList<String> finalSpecFilenamesList;
    for (int a=0;a<specFilenames.size(); a++ )
    {
        std::string unpatchedFilename = specFilenames[a].toLower().cStr();
        bool foundInPatchList = false;

        // Is the filename in the patched list?
        for (int b=0;b<patchedSpecFilenames.size();b++)
        {
            // Get the filename without the path
            std::string patchedFilename = patchedSpecFilenames[b].toLower().cStr();
            std::string patchedSpecsDir = "patchedspecs/";
            size_t offset = patchedFilename.find(patchedSpecsDir);
            if ( offset != std::string::npos )
            {
                patchedFilename = patchedFilename.replace( offset, patchedSpecsDir.length(), "registry/specs/" );
            }

            if ( patchedFilename == unpatchedFilename )
            {
                finalSpecFilenamesList.add( patchedSpecFilenames[b] );
                numExtensionsPatched++;
                foundInPatchList = true;
            }
        }
        if ( !foundInPatchList )
        {
            finalSpecFilenamesList.add( specFilenames[a] );
        }
    }


	XMLFile xmlFile;
	xmlFile.root.setName("extensions");
	xmlFile.root.elements.resize(3);
	xmlFile.root.elements[0].name="gl";
	xmlFile.root.elements[1].name="wgl";
	xmlFile.root.elements[2].name="glx";

	//read extensions from wglext.h, glxext.h and glext.h
	//then read any extra extensions from specs

    headerIgnoreList.append( ignoreList );

	readHeaderExtensions( 
        xmlFile.root,
        includedExtensions, 
        headerIgnoreList );

	ignoreList.append(includedExtensions);

    uint32_t numExtensionsParsed =0;
    uint32_t numExtensionsFailed =0;
    uint32_t numExtensionsIgnored =0;
	for (int a=0;a<finalSpecFilenamesList.size();a++)
	{
		FILE *file=fopen(finalSpecFilenamesList[a].cStr(),"rb");

		if (file)
		{
			String str;
			str.readFile(file);
			fclose(file);

            SpecParseResult rv = readExtensionSpec(str, xmlFile.root, ignoreList);
            switch (rv)
            {
            case SPEC_IGNORED:
                numExtensionsIgnored++;
                break;
            case SPEC_PARSED:
                numExtensionsParsed++;
                break;
            case SPEC_FAILED:
                numExtensionsFailed++;
                break;
            }
		}
	}

    printf("\n  %d extension specs parsed successfully\n",  numExtensionsParsed     );
    printf("  %d extension specs failed to parse\n",      numExtensionsFailed     );
    printf("  %d extension specs ignored\n",              numExtensionsIgnored        );
    printf("  %d extension specs overridden from patch dir (some of which may have been ignored anyway)\n\n",              numExtensionsPatched  );
	
	//next read extension XML from glext, wglext and glxext, ignoring included extensions and those in ignorelist
	xmlFile.save(OUTFILE("extensions.xml"));
	ignoreList.append(includedExtensions);
	return 0;
}

bool getSpecFilenames(const String& startDir, ArrayList<String>& specFilenamesOut)
{
#ifdef _WIN32
	//need to traverse subdirectories
	WIN32_FIND_DATA fileData;
	HANDLE hFind;
	BOOL res=TRUE; 

	String filespec=startDir+"/*";
	hFind = FindFirstFile(filespec.cStr(), &fileData);
	if (hFind == INVALID_HANDLE_VALUE) 
	{
		printf("error");
		return false;
	} 

	do 
	{
		String filenameStr=fileData.cFileName;
		if (filenameStr!=String(".") && filenameStr!=String(".."))
		{
			if (fileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
				getSpecFilenames(startDir+"/"+filenameStr,specFilenamesOut); //recursive call
			else
			{
				if (filenameStr.length()>5) //store only text files
				{
					if (filenameStr.subString(filenameStr.length()-4,filenameStr.length())==String(".txt"))
					{
						specFilenamesOut.add(startDir+"/"+filenameStr);
					}
				}
			}
		}
		res=FindNextFile(hFind, &fileData);
	} while (res!=0);
	return true;
#else
    DIR*           dir;
    struct dirent* dit;
    dir = opendir( startDir.cStr() );
    
    while( dit = readdir(dir) )
    {
        //
        // If this is a folder recurse
        //
        if( dit->d_type == 0x4 &&
            strcmp(dit->d_name, ".") != 0 && 
            strcmp(dit->d_name, "..") != 0 )
        {
            if( !getSpecFilenames( startDir + "/" + String(dit->d_name), specFilenamesOut ) )
                return false;
        }
        else if ( dit->d_type == 0x8 )
        {
            specFilenamesOut.add( startDir + String("/") + String(dit->d_name) );
        }
    }
    
    return true;
#endif
}

SpecParseResult readExtensionSpec(String& extFileString, XMLElement& extensionsXML, ArrayList<String>& ignoreList)
{
	ArrayList<String> names;

    // NOTE: certain NVIDIA extensions helpfully report (none) as their name string
    // because they're actually part of other extensions. E.g. GL_NV_GPU_program4
	if (!readNames(extFileString,names))
    {
		return SPEC_IGNORED;
    }

    /*
	if ( extFileString.search("ARB_framebuffer_sRGB") != -1 )
	{
		printf("blah");
	}
    */
	ArrayList<XMLElement> extensionElements(names.size());

	bool nameTypeDone[3]={false,false,false};

	//where there are multiple extension names, there must be only one of each type (GL, WGL, GLX)
	//extensions with duplicate types will be skipped
	if (names.size()>3) 
    {
        return SPEC_FAILED;
    }

	for (int a=0;a<names.size();a++)
	{
		int fileType;
		String& name=names[a];
		XMLElement& extXML=extensionElements[a];

		if (name.subString(0,3)==String("GL_")) fileType=FT_GLEXT;
		else if (name.subString(0,4)==String("WGL_")) fileType=FT_WGLEXT;
		else if (name.subString(0,4)==String("GLX_")) fileType=FT_GLXEXT;
		else return SPEC_FAILED; //could not determine extension type

		if (nameTypeDone[fileType]) continue; 
		nameTypeDone[fileType]=true;

		//verify the extension is not in the ignorelist
        // FIXME: shouldn't this just ignore one of the name strings?
		for (int a=0;a<ignoreList.size();a++)
		{
			if (name==ignoreList[a]) 
            {
				return SPEC_IGNORED;
            }
		}



		extXML.name="extension";
		extXML.addAttrib(XMLAttrib("name",name.trimWhitespace()));

		int typeFilter=names.size()==1?-1:fileType; //-1 signifies no filter

		if (!readConstants(extFileString,extXML,typeFilter)) 
        {
            return SPEC_FAILED;
        }
		if (!readFunctions(extFileString,extXML,typeFilter)) 
        {
            return SPEC_FAILED;
        }

		/*
		if (extXML.getElement("functions").elements.size()==0 && XMLOut.getElement("constants").elements.size()==0)
		{
			continue; //blank extension
		}*/
		if (fileType==FT_GLEXT) addPrefixes(extXML);
		extensionsXML.elements[fileType].addElement(extXML);
	}
	return SPEC_PARSED;
}

void addPrefixes(XMLElement& XMLOut)
{
	XMLElement& functionsEl=XMLOut.getElement("functions");
	XMLElement& constantsEl=XMLOut.getElement("constants");

	XMLElement *funcEl=functionsEl.findFirstElementPtr("function");
	while (funcEl)
	{
		String& funcName=funcEl->getAttrib("name");
		assert(funcName.length()>2);
		if (funcName[0]!='g' || funcName[1]!='l') 
			funcName=String("gl")+funcName;
		String& returnType=funcEl->getAttrib("returnType");
		assert(returnType.length()>0);
		convertType(returnType);
		XMLElement *paramEl=funcEl->findFirstElementPtr("param");
		while (paramEl)
		{
			String& paramType=paramEl->getAttrib("type");
			assert(paramType.length()>0);
			convertType(paramType);
			paramEl=funcEl->findNextElementPtr();
		}
		funcEl=functionsEl.findNextElementPtr();
	}

	XMLElement *constEl=constantsEl.findFirstElementPtr("constant");
	while (constEl)
	{
		String& nameStr=constEl->getAttrib("name");
		assert(nameStr.length()>0);
		if (nameStr[0]!='G' || nameStr[0]!='L' || nameStr[1]!='_')
			nameStr=String("GL_")+nameStr;
		constEl=constantsEl.findNextElementPtr();
	}
}

void convertType(String& type)
{
	//regex typeReg("(\\s)*((unsigned|const)(\\s)+)*(([a-zA-Z0-9]|_)+)(\\s)*(\\*)*(\\s)*");
	regex& typeReg=getTypeRegex();
	cmatch what;
	String temp=type+" "; //hack (type pattern requires ending space for non-pointers)
	if (!regex_match(temp.cStr(),what,typeReg))
	{
		assert(false);  //this shouldn't happen
		return;
	}

	int i=what[3].first-temp.cStr();
	if (temp[i]!='G' || temp[i+1]!='L')
		temp.insert(i,String("GL"));
	type=temp.trimWhitespace();
	//String typeStr;
	//getSubstring(what[5].first,what[5].second,typeStr);
	//typeStr="GL"+typeStr;


	//type=String("GLGLGLGLGL_________________")+type;
}

bool readConstants(String& extFileString, XMLElement& XMLOut, int typeFilter)
{
	XMLElement constantsXML;
	constantsXML.setName("constants");
	regex regStartTokens("\r?\nNew Tokens\\s+");
	regex regEndTokens("\r?\n\r?\n[a-zA-Z0-9]");
	cmatch what,what2; 
	if(regex_search(extFileString.cStr(), what, regStartTokens)) 
	{
		if (regex_search(what[0].first, what2, regEndTokens))
		{
			String tokensBlock;
			getSubstring(what[0].first, what2[0].first, tokensBlock);

			//read the tokens
			const char * pos=&tokensBlock[0];
			cmatch tokwhat;
			//regex regToken("(\r)?\n(\\s)*(([a-zA-Z0-9]|_)+)(\\s)+((0x([0-9a-fA-F])+)|(([0-9]);+))");
			regex regToken("\r?\n\\s*([A-Z0-9_]+)\\s+((?:0x[0-9a-fA-F\\?]+)|(?:[0-9]+))");
			while (regex_search(pos, tokwhat, regToken))
			{
				XMLElement constantXML;
				constantXML.setName("constant");
				String tokValue,tokName;
				getSubstring(tokwhat[1].first,tokwhat[1].second,tokName);
				getSubstring(tokwhat[1].second,tokwhat[2].second,tokValue);

				regex regUnknownValue("0x\\?+");
				cmatch temp;

				if( regex_search(tokValue.cStr(), temp, regUnknownValue) )
					tokValue = String("0x0");

				bool skip = false;

				if (typeFilter>=0)
				{
					String prefix=tokName.subString(0,3);
					if (prefix==String("WGL"))
					{
						if (typeFilter!=FT_WGLEXT) skip=true;
					}
					else if (prefix==String("GLX"))
					{
						if (typeFilter!=FT_GLXEXT) skip=true;
					}
					else if (typeFilter!=FT_GLEXT) skip=true;
				}

				if (!skip)
				{
					constantXML.addAttrib(XMLAttrib("name",tokName.trimWhitespace()));
					constantXML.addAttrib(XMLAttrib("value",tokValue.trimWhitespace()));
					constantsXML.addElement(constantXML);
				}
				pos=tokwhat[2].second;
			}
		}
	}
	XMLOut.addElement(constantsXML);
	return true;
}

bool readFunctions(String& extFileString, XMLElement& XMLOut, int typeFilter)
{
	XMLElement functionsXML;
	functionsXML.setName("functions");
	String ptrStr;

	static regex regStartFunctions("\r?\nNew Procedures and Functions");
	static regex regEndFunctions("\r?\n\r?\n[a-zA-Z0-9]");
//	static regex regStartFunc("\n(\\s)+(((const|unsigned)(\\s)+)*([a-zA-Z0-9]|_)+)(((\\s)*((\\*)+)(\\s)*)|((\\s)+))(([a-zA-Z0-9]|_)+)(\\s)*\\(");

	static regex regNewLine("\r?\n");
	static regex regStartFunc("(\\w+)\\s*\\((.*)");

	String typeStr;
	String funcName;
	cmatch what,what2; 
	if(regex_search(extFileString.cStr(), what, regStartFunctions)) 
	{
		if (regex_search(what[0].first, what2, regEndFunctions))
		{
			String functionsBlock;
			getSubstring(what[0].second, what2[0].first, functionsBlock);

			//read the tokens
			const char * pos=&functionsBlock[0];
			cmatch funcWhat;
			assert(pos!=0);
			while (regex_search(pos, funcWhat, regNewLine)) //find a newline
			{
				pos=funcWhat[0].second;
				if (pos=readType(pos,typeStr)) //read the return type
				{
					if (regex_match(pos, funcWhat, regStartFunc))
					{
						getSubstring(funcWhat[1].first,funcWhat[1].second, funcName);

						//find out if we should skip this function
						bool skip=false;
						if (typeFilter>=0)
						{
							String prefix=funcName.subString(0,3);
							if (prefix==String("wgl"))
							{
								if (typeFilter!=FT_WGLEXT) skip=true;
							}
							else if (prefix==String("glX"))
							{
								if (typeFilter!=FT_GLXEXT) skip=true;
							}
							else if (typeFilter!=FT_GLEXT) skip=true;
						}
						if (!skip)
						{
							XMLElement funcXML;
							funcXML.attribs.resize(2);
							funcXML.setName("function");
							funcXML.attribs[0].name="name";
							funcXML.attribs[1].name="returnType";
							funcXML.attribs[0].value=funcName;
							funcXML.attribs[1].value=typeStr;

							pos=funcWhat[2].first;
							if (!readParams(pos,funcXML)) return false;
							functionsXML.addElement(funcXML);
						}
						else pos=funcWhat[0].second; //skip this function
					}
				} //read return type
				else
				{
					//can't read return type: continue
					pos=funcWhat[0].second;
				}
			}
		}
	}
	XMLOut.addElement(functionsXML);
	return true;
}

regex& getTypeRegex()
{
	static String regStr=  String("\\s*")+ //Any leading spaces
					"("+ //open brackets 2 //capturing the whole type
					    "((?:const|unsigned)(?:\\s)+)*"+ //type modifiers
						"((?:[a-zA-Z0-9]|_)+)"+ //core type (6)
						"((?:\\s)+(:?const|unsigned))*"+ //more type modifiers
						"(?:\\s)*"+
						"("+
   					      "(?:\\*(?:\\s)*(const(?:\\s)+)?)+"+ //stars (each with optional :const modifier and spaces)
						  "|"+ //OR
						  "(?:\\s)+"+ //spaces
						")"+
					")"+// close brackets 2
					"(.*)";

	static regex regType(regStr.cStr());
	return regType;
}
const char * readType(const char *pos, String& typeOut)
{
	const char *posOut=NULL;
	regex& regType=getTypeRegex();
	//slight problem: consts following stars require a space after them
	cmatch what;
	if (regex_match(pos,what,regType))
	{
		getSubstring(what[1].first,what[1].second,typeOut);
		typeOut=typeOut.trimWhitespace();
		posOut=what[1].second;
	}
	return posOut;
}

bool readParams(const char *pos, XMLElement& funcXML)
{
	char voidStr[]="void)";
	String ptrStr;


	//GLenum mode, const GLsizei *count, GLenum type, const GLvoid* *indices, GLsizei primcount

	//  Begin                                                           1     11            1       11       11     
	//              1     2345               6       7                890     12            4       56       78               
	//regex regParam("(\\s)*((((const|unsigned)(\\s)+)*([a-zA-Z0-9]|_)+)(((\\s)*((\\*(\\s)*)+)(\\s)*)|((\\s)+))(([a-zA-Z0-9]|_)+))(.*)");
	//  End                                                                 1            1 1    1        1 1                1 1
	//1                   5    6 4               7 3      0            2 1    4 9      6 58               8 72
	static regex regName("(\\w+).*");
	cmatch nameWhat;

	bool hasParams=false;
	for (int a=0;a<5;a++)
	{
		if (pos[a]!=voidStr[a])
		{
			hasParams=true;
			break;
		}
	}
	if (hasParams) 
	{
		static regex regTypeAndName("\\s*([\\w\\s\\*]+?)([\\*\\s]+)(\\w+)\\s*([\\),])");
		cmatch typeAndNameWhat;
		while (regex_search(pos, typeAndNameWhat, regTypeAndName))
		{
			String type;
			String name;

			getSubstring(typeAndNameWhat[1].first, typeAndNameWhat[2].second, type);
			getSubstring(typeAndNameWhat[3].first, typeAndNameWhat[3].second, name);
			
			XMLElement paramXML;
			paramXML.setName("param");
			paramXML.attribs.resize(2);
			paramXML.attribs[0].name="type";
			paramXML.attribs[1].name="name";
			paramXML.attribs[0].value=type;
			paramXML.attribs[1].value=name;
			
			funcXML.addElement(paramXML);
			
			pos = typeAndNameWhat[0].second;

			if (*pos=='\0') 
				return false;
			if (*typeAndNameWhat[4].first==')')
				break;
		}
		/*String type;
		while (pos=readType(pos, type))
		{
			if (regex_match(pos, nameWhat, regName))
			{
				XMLElement paramXML;
				paramXML.setName("param");
				paramXML.attribs.resize(2);
				paramXML.attribs[0].name="type";
				paramXML.attribs[1].name="name";
				paramXML.attribs[0].value=type;
				getSubstring(nameWhat[1].first,nameWhat[1].second, paramXML.attribs[1].value);

				//read the ,
				pos=nameWhat[1].second;
				funcXML.addElement(paramXML);

				while (*pos==' ' || *pos=='\n' || *pos=='\t' || *pos=='\r') //skip whitespace
					pos++;

				if (*pos=='\0') 
					return false;
				if (*pos==')') break;
				if (*pos!=',') 
					return false;
				pos++;
				if (*pos=='\0') 
					return false;
			}
		}*/
	}
	return true;
}

/*
bool readParams(const char *pos, XMLElement& funcXML)
{
	char voidStr[]="void)";
	String ptrStr;


	//GLenum mode, const GLsizei *count, GLenum type, const GLvoid* *indices, GLsizei primcount

	//  Begin                                                           1     11            1       11       11     
	//              1     2345               6       7                890     12            4       56       78               
	regex regParam("(\\s)*((((const|unsigned)(\\s)+)*([a-zA-Z0-9]|_)+)(((\\s)*((\\*(\\s)*)+)(\\s)*)|((\\s)+))(([a-zA-Z0-9]|_)+))(.*)");
	//  End                                                                 1            1 1    1        1 1                1 1
	//                  1                   5    6 4               7 3      0            2 1    4 9      6 58               8 72
	cmatch paramWhat;

	bool hasParams=false;
	for (int a=0;a<5;a++)
	{
		if (pos[a]!=voidStr[a])
		{
			hasParams=true;
			break;
		}
	}
	if (hasParams) 
	{
		//read the params
		while (regex_match(pos, paramWhat, regParam))
		{
			XMLElement paramXML;
			paramXML.setName("param");
			paramXML.attribs.resize(2);
			paramXML.attribs[0].name="type";
			paramXML.attribs[1].name="name";
			getSubstring(paramWhat[3].first,paramWhat[3].second, paramXML.attribs[0].value);
			getSubstring(paramWhat[17].first,paramWhat[17].second, paramXML.attribs[1].value);
			if (paramWhat[11].first!=paramWhat[11].second) //if *
			{
				getSubstring(paramWhat[11].first,paramWhat[11].second,ptrStr);
				paramXML.attribs[0].value+=" "+ptrStr;
			}

			if (paramXML.attribs[1].value==String("GLvoid"))
				bool b=false;

			//read the ,
			pos=paramWhat[2].second;
			funcXML.addElement(paramXML);

			while (*pos==' ' || *pos=='\n' || *pos=='\t' || *pos=='\r') //skip whitespace
				pos++;

			if (*pos=='\0') 
				return false;
			if (*pos==')') break;
			if (*pos!=',') 
				return false;
			pos++;
			if (*pos=='\0') 
				return false;

		}
	}
}
*/

bool readNames(String &extFileString, ArrayList<String>& names)
{
	String name;
//	regex regName("Name\n\n    ([A-Z][a-z])+");
//	regex regName("Name\n\n    ");
	/*
	regex regName("Name Strings(\\s)+(([a-zA-Z0-9]|_)+)");
	cmatch what;
	if(regex_search(extFileString.cStr(), what, regName)) 
	{
		getSubstring(what[2].first,what[2].second,name);
	}
	return name;
	*/
	regex regName("W?GLX?_\\w+");
	regex regNamesStart("Name Strings? *\r?\n *\r?\n");
	regex regNamesEnd("\r?\n *\r?\n\\w+");
	String namesBlock;
	cmatch whatStart,whatEnd,whatName;
	if (regex_search(extFileString.cStr(),whatStart,regNamesStart))
	{
		if (regex_search(whatStart[0].second,whatEnd,regNamesEnd))
		{
			getSubstring(whatStart[0].second,whatEnd[0].first,namesBlock);
			const char *pos=namesBlock.cStr();
			int numNames=0;
			String name;
			//read the names
			while (regex_search(pos,whatName,regName))
			{
				getSubstring(whatName[0].first,whatName[0].second,name);
				pos=whatName[0].second;
				names.add(name);
			}
			if (names.size()==0) return false;
			return true;
		}
	}
	return false;
}

bool readStringList(const char * filename, ArrayList<String>& stringListOut)
{
	FILE * file=fopen(filename,"rt");
	if (!file) return false;
	String s;

	while (!feof(file))
	{
		int c=fgetc(file);
		if (c!='\n' &&  c!=EOF) s.insertCharAtEnd(c);
		else 
		{
			s=s.trimWhitespace();
			printf("Ignore : %s\n",s.cStr());
			stringListOut.push_back(s);
			s.clear();
		}
	}
	fclose(file);
	return true;
}

////////////////////////////////////////////////////////
//For parsing headers (from old GLeeGen)
////////////////////////////////////////////////////////
void readHeaderExtensions(XMLElement& XMLOut, ArrayList<String>& extensionsReadOut, ArrayList<String>& ignoreList)
{
	ArrayList<_Extension> glExtensions,wglExtensions,glxExtensions;
	getHeaderExtensions(FT_GLEXT, glExtensions, ignoreList);
	getHeaderExtensions(FT_WGLEXT, wglExtensions, ignoreList);
	getHeaderExtensions(FT_GLXEXT, glxExtensions, ignoreList);
	for (int a=0;a<3;a++)
	{
		XMLOut.elements[a].elements.allocSetShrinkable(false);
		XMLOut.elements[a].elements.allocate(1024);
	}
	convertHeaderExtensionsToXML(glExtensions, XMLOut.getElement("gl"), extensionsReadOut);
	convertHeaderExtensionsToXML(wglExtensions, XMLOut.getElement("wgl"), extensionsReadOut);
	convertHeaderExtensionsToXML(glxExtensions, XMLOut.getElement("glx"), extensionsReadOut);
}

void convertHeaderExtensionsToXML(ArrayList<_Extension> &extensions, XMLElement& xmlOut, ArrayList<String>& extensionsReadOut)
{
	for (int a=0;a<extensions.size();a++)
	{
		_Extension& ext=extensions[a];
		//find the functions and the constants
		XMLElement xmlElement;
		xmlElement.setName("extension");
		xmlElement.addAttrib(XMLAttrib("name",ext.name.trimWhitespace()));
		xmlElement.elements.resize(2);
		xmlElement.elements[0].setName("constants");
		xmlElement.elements[1].setName("functions");
		for (int b=0;b<ext.constants.size();b++)
		{
			if (!readHeaderConstant(ext.constants[b], xmlElement.elements[0])) continue;
		}
		assert(ext.functionNames.size()==ext.typedefLines.size());
		for (int b=0;b<ext.typedefLines.size();b++)
		{
			if (!readHeaderFunction(ext.typedefLines[b], ext.functionNames[b], xmlElement.elements[1])) continue;
		}
		extensionsReadOut.add(ext.name);
		xmlOut.addElement(xmlElement);
	}
}

bool readHeaderConstant(std::pair<String,String >& constPair, XMLElement& XMLOut)
{
	XMLElement el;
	el.setName("constant");
	el.addAttrib(XMLAttrib("name",constPair.first.trimWhitespace()));
	el.addAttrib(XMLAttrib("value",constPair.second.trimWhitespace()));
	XMLOut.addElement(el);
	return true;
}
bool readHeaderFunction(String& typeDefString, String& funcName, XMLElement& XMLOut)
{
	/*
	XMLElement el;
	el.setName("function");
	el.addAttrib(XMLAttrib("name",funcName));

	regex regFuncTypedefStart("((\\s)*typedef(\\s)+(((const|unsigned)(\\s)+)*(([a-zA-Z0-9]|_)+)((\\*)?(\\s)*)*)\\((\\s|[a-zA-Z0-9]|_|\\*)+\\)(\\s)*\\().*");

	cmatch what,whatParam;
	if (regex_match(typeDefString.cStr(),what,regFuncTypedefStart))
	{
		String returnType;
		getSubstring(what[4].first,what[4].second,returnType);
		el.addAttrib(XMLAttrib("returnType",returnType.trimWhitespace()));
		readParams(what[1].second,el);
		XMLOut.addElement(el);
		return true;
	}
	return false;
	*/
	XMLElement el;
	el.setName("function");
	el.addAttrib(XMLAttrib("name",funcName.trimWhitespace()));

	regex regFuncTypedefStart("\\s*typedef\\s+(.*)");
    regex regFuncTypedefNext("\\s?\\(([\\sa-zA-Z0-9_\\*])+\\)\\s*\\((.*)");
	//regex regFuncTypedefNext("\\s?\\(((?:\\s|\\w)+)       \\)\\s*\\((.*)");
	//regex regFuncTypedefNext("(?:\\s)?\\(((?:\\s|\\w)+)\\)(?:\\s)*\\((.*)");

	cmatch what;
	if (regex_match(typeDefString.cStr(),what,regFuncTypedefStart))
	{
		String returnType;
		const char *pos=what[1].first;
		if (!(pos=readType(pos,returnType)))
			return false;
		if (!regex_match(pos,what,regFuncTypedefNext))
			return false;
		pos=what[2].first;
		if (!readParams(pos,el))
			return false;
		el.addAttrib(XMLAttrib("returnType",returnType.trimWhitespace()));
		XMLOut.addElement(el);
		return true;
	}
	return false;
}


void getHeaderExtensions(int fileType, ArrayList<_Extension> &extensions, ArrayList<String>& ignoreList)
{
	String glext;
	FILE * file;

	String startString,protoStartString,extBegin,extIfndef,extAPIENTRY,extAPIENTRYPtr,extProto;

	switch (fileType)
	{
		case FT_GLEXT:
			file=fopen(INFILE("registry/api/glext.h"),"rt");

			startString="#ifndef GL_VERSION_1_2";
			protoStartString="#define GL_VERSION_1_2";
			extBegin="GL_";
			extProto="#ifdef GL_GLEXT_PROTOTYPES";
			extAPIENTRY="APIENTRY ";
			extAPIENTRYPtr="APIENTRYP";
			break;
		case FT_WGLEXT:
			file=fopen(INFILE("registry/api/wglext.h"),"rt");

			startString="#ifndef WGL_ARB_buffer_region";
			protoStartString="#define WGL_ARB_buffer_region";
			extBegin="WGL_";
			extProto="#ifdef WGL_WGLEXT_PROTOTYPES";
			extAPIENTRY="WINAPI ";
			extAPIENTRYPtr="WINAPI *";
			break;
		case FT_GLXEXT:
			file=fopen(INFILE("registry/api/glxext.h"),"rt");
			startString="#ifndef GLX_VERSION_1_3";
			protoStartString="#define GLX_VERSION_1_3";
			extBegin="GLX_";
			extProto="#ifdef GLX_GLXEXT_PROTOTYPES";
			break;
	}
	extIfndef="#ifndef "+extBegin;

	glext.readFile(file);
	fclose(file);

	String block;
	int a=0;

	int p=0;
	int end=0;

	//go the the start of the constants
	//p=glext.search(p,startString);
	BlockPos blockPos;
	blockPos.pos=p;

	String constantBlock;
	String commentLine("/*************************************************************/");
	getChunk(p,glext,startString,commentLine,constantBlock);

	while(readIfBlock(blockPos, constantBlock, extIfndef, block)) 
	{
		//get the extension name
		_Extension newext;
		_Extension *pext = &newext;
		int pos=0;
		getChunk(pos,block,extBegin,"\n",newext.name);
		if (newext.name.search("_DEPRECATED") > 0) {
			newext.name.resize(newext.name.length() - 11);
			int c=0;
			for (;c<extensions.size();c++)
			{
				if (extensions[c].name==newext.name) {
					pext = &extensions[c];
					break; 
				}
			}
		}
		_Extension &ext=*pext;

		//read the constants
		while (block.searchRep(pos,"#define"))
		{
			std::pair<String,String> constant;
			//getChunk(pos,block,extBegin,"\n",chunk);
			//int pc=0;
			getChunk(pos,block," "," ",constant.first);
			constant.first=constant.first.trimWhitespace();
			pos--;
			getChunk(pos,block,String(" "),"\n",constant.second);
			constant.second=constant.second.trimWhitespace();
			//getChunk(pos,block,String("")," ",String());
			//getChunk(pos,block,String(""),"\n",constant.second);
			ext.constants.push_back(constant);
		}
		if (!isIgnored(ext.name,ignoreList) && pext == &newext) 
			extensions.push_back(newext);
	}


	p=0;
	//go to the start of the function prototypes 
	p=glext.search(p,protoStartString)-200;
	p=glext.search(p,startString);
//	getChunk(p,glext,startString,commentLine,constantBlock);

	BlockPos bp;
	bp.pos=p;

	a=0;

	while(readIfBlock(bp, glext, extIfndef, block)) 
	{
		//get the extension name
		int pos=0;
		String ename;
		getChunk(pos,block,extBegin,"\n",ename);

		if (ename.search("_DEPRECATED") > 0)
			ename.resize(ename.length() - 11);
		//search for the matching extension
		int c=0;
		for (;c<extensions.size();c++)
		{
			if (extensions[c].name==ename) break; 
		}
		if (c==extensions.size()) 
		{
			//the extension wasn't found
			//insert it extension anyway
			_Extension ext;
			ext.name=ename;
			if (!isIgnored(ext.name,ignoreList)) 
				extensions.push_back(ext);
			else continue;
		}

		a=c;
		_Extension &ext=extensions[a];

		int cPos=0;
		String protoBlock;
		BlockPos extPos;
		int endPos;

		if (readIfBlock(extPos, block, extProto, protoBlock))
		{
			int pPos=0;
			//search for all APIENTRY tokens
			printf("%s\n",ext.name.cStr());
			ArrayList<String> functionNames;
			while (searchForFuncStart(protoBlock,pPos,endPos,fileType,extAPIENTRY,false))
			{
				//get the function name
				int end=protoBlock.search(pPos," ");
				String fname=protoBlock.subString(pPos,end);
				ext.functionNames.push_back(fname);
				//functionNames.push_back(fname);
				printf("%s\n",fname.cStr());
				pPos=endPos;
			}
			//read the typedefs from the rest of the extension block
			pPos=extPos.pos;
			int cPos=pPos;
			String tdString="typedef";
			while (block.searchRep(pPos,tdString))
			{
				//read the line
				String line;
				getChunk(cPos,block,tdString,"\n",line);
				ext.typedefLines.push_back(line);
				cPos=pPos;

				searchForFuncStart(block,pPos,endPos,fileType,extAPIENTRYPtr, true);
				//get the typedef 
				int end=block.search(pPos,")");
				String type=block.subString(pPos,end).trimWhitespace();
				ext.typedefs.push_back(type);
				printf("%s\n",type.cStr());
				pPos=endPos;
			}
		}
		a++;
	}
}


bool searchForFuncStart(String &block, int &pPos, int& lineEndPos, int fileType, const String& APIENTRYStr, bool ptr)
{
	if (fileType!=FT_GLXEXT)
	{
		bool rv=block.searchRep(pPos,APIENTRYStr);
		lineEndPos=pPos;
		return rv;
	}
	else
	{
		int p=pPos;
		int endPos=-1;
		//no apientry string, so cleverness required

		//get the position of the end of the function name/type
		p=block.search(p,String(");"));
		lineEndPos=p+2;
		int level=0;
		int a=p;
		for (;a>=0;a--)
		{
			if (block[a]==')') level++;
			if (block[a]=='(') level--;
			if (level==0)
			{
				endPos=a;
				break;
			}
		}

		if (ptr) endPos-=3; 
		else endPos-=2;

		if (endPos<0)
			return false;

		int startPos=-1;
		//find the start of the function name/type
		for (a=endPos;a>=0;a--)
		{
			if (block[a]==' ')
			{
				startPos=a+1;
				break;
			}
		}
		if (startPos<0) 
			return false;

		pPos=startPos;
	}
	return true;
}

//reads a preprocessor #if/#endif block. Pos is updated to point to the end of the block
bool readIfBlock(BlockPos &pos, String& str, const String& blockStart, String &chunkOut)
{
	int a=0,b=0;

	String sIf="#if";
	String sEndif="#endif";

	int startPos;

	bool found=false;
	int targetLevel=0; //exit when the level reaches this

	do
	{
		int iPos=str.search(pos.pos,sIf);
		int ePos=str.search(pos.pos,sEndif);

		if (ePos==-1 && iPos==-1)
		{
			return false; //no more tokens
		}

		if (iPos==-1) iPos=999999999;
		if (ePos==-1) ePos=999999999;

		if (iPos<ePos) 
		{
			//is the block the one we're looking for? 
			if (str.search(pos.pos,blockStart)==iPos) 
			{
				found=true; 
				targetLevel=pos.level; //search just this block
				startPos=iPos;
			}

			pos.pos=iPos+3;
			pos.level++;
		}
		else
		{
			pos.pos=ePos+6;
			pos.level--;
		}
	}while (pos.level>targetLevel);

	if (found)
	{
		chunkOut=str.subString(startPos,pos.pos);
	}

	return found;
}

bool getChunk(int &pos, String& str, const String &start, const String &end, String &tokenOut)
{
	if (!str.searchRep(pos,start)) return false;
	int startPos=pos-start.getLength();
	if (!str.searchRep(pos,end)) return false;
	int endPos=pos-end.getLength();
	tokenOut=str.subString(startPos,endPos);
	return true;
}

bool isIgnored(const String &extName, const ArrayList<String>& ignoreList)
{
	for (int a=0;a<ignoreList.size();a++) if (extName==ignoreList[a]) 
	{
		printf("Extension Ignored : %s\n",extName.cStr());
//		getch();
		return true;
	}
	return false;
}

