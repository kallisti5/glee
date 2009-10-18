/////////////////////////////////////////////////////////////////////
//
// String.cpp
// Efficient String buffer class (uses array doubling to resize)
// Version :1.4
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

#include "stdafx.h"
#include "String.h"
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

String::String()
{
	buf.resize(16);
	buf[0]='\0';
	_length=0;
}

String::String(const String &src)
{
	_length=0;
	copyFrom(src);
}

String::String(const char * src)
{
	_length=0;
	copyFrom(src);
}

String::~String()
{
}

void String::copyFrom(const String &src)
{
	buf=src.buf;
	_length=src._length;
}

void String::copyFrom(const char * src)
{
	int len=strlen(src);
	resize(len);
	memcpy(&buf.first(),src,len*sizeof(char));
}

void String::copyFrom(const char * src, int srcLength)
{
	resize(srcLength);
	memcpy(&buf.first(),src,srcLength);
}


const String& String::operator=(const String& src)
{
	copyFrom(src);
	return (*this);
}

const String& String::operator=(const char * src)
{
	copyFrom(src);
	return (*this);
}


const String& String::operator+=(const String& appendix)
{
	append(appendix);
	return *this;
}

const String& String::operator+=(const char * appendix)
{
	int appLen=strlen(appendix);
	append(appendix,appLen);
	return *this;
}


const String String::operator+(const String& appendix)
{
	String newString;
	newString.copyFrom(*this);
	newString+=appendix;
	return newString;
}


void String::append(const char * appendix, int appLength)
{
	int oldLength=_length;
	resize(appLength+_length);
	memcpy(&buf[oldLength],appendix,appLength*sizeof(char));
}

void String::append(const char * appendix)
{
	append(appendix,strlen(appendix));
}
void String::append(const String &appendix)
{
	int oldLength=_length;
	resize(appendix._length+_length);
	memcpy(&buf[oldLength],&appendix.buf.getConst(0),appendix._length);
}

/*
void String::resize(int newLength)
{
	int realNewLength=newLength+1; //allow for NULL character
	if (realNewLength>maxSize)
	{
		//expand the buffer
		int newMaxSize=max(maxSize,16);
		while (newMaxSize<=realNewLength)
		{
			newMaxSize=newMaxSize<<1;
		}
		char * newBuf=new char[newMaxSize];
		memcpy(newBuf,buf,sizeof(char)*length);
		if (buf!=NULL) delete [] buf;
		buf=newBuf;
		maxSize=newMaxSize;
		minSize=maxSize>>1;
	}
	else if (realNewLength<length)
	{
		if (minSize>16 && realNewLength<minSize)
		{
			//shrink the buffer
			int newMinSize=max(minSize,16);
			while (newMinSize>realNewLength)
			{
				newMinSize=newMinSize>>1;
			}
			int newMaxSize=newMinSize<<1;

			char * newBuf=new char[newMaxSize];
			memcpy(newBuf,buf,sizeof(char)*newLength);
			if (buf!=NULL) delete [] buf;
			buf=newBuf;
			maxSize=newMaxSize;
			minSize=newMinSize;
		}
	}
	length=newLength;
	buf[length]=0; //null-terminate the string
}
*/

/*
void String::allocate(int newMaxSize)
{
	if (buf!=NULL) delete [] buf;
	maxSize=newMaxSize;
	buf=new char[maxSize];
}*/


String String::subString(int start, int end) const
{
	String sub;
	assert(end>=start && end<=_length && start>=0);

	//sub._length=end-start;
	//sub.allocate(max(16,sub._length+1));
	sub.resize(end-start);
	memcpy(&sub.buf[0],&buf.getConst(start),sub._length);
	return sub;
}

void String::insert(int pos, String &src)
{
	//int newLength=length+src.length;
	int oldLength=_length;
	resize(_length+src._length);

	//shift chars after pos forwards by src.length
	for (int i=_length-1;i>=pos;i--)
	{
		buf[i+src._length]=buf[i];
	}
	//copy in src
	memcpy(&buf[pos],&src.buf[0],src._length);
}
void String::remove(int pos, int len)
{
	assert(len>=0);
	if (len>_length-pos)
	{
		len=_length-pos;
	}
	for (int i=pos;i<_length-len;i++)
	{
		buf[i]=buf[i+len];
	}
	resize(_length-len);
}
void String::insertChar(int pos, char c)
{
	//int newLength=length+1;
	int oldLength=_length;
	resize(_length+1);

	//shift chars after pos forwards 
	for (int i=oldLength-1;i>=pos;i--)
	{
		buf[i+1]=buf[i];
	}
	buf[pos]=c; //write the character
}

void String::removeChar(int pos)
{
	if (_length>0)
	{
		for (int i=pos;i<_length-1;i++)
		{
			buf[i]=buf[i+1];
		}
		resize(_length-1);
	}
}

void String::printf(const char * format, ...)
{
	char msg[1000];
	int len;
	va_list args;

	va_start(args,format);
	len=_vsnprintf( msg, 999, format, args);
	va_end( args );
	append(msg,len);
}

bool String::operator==(const String& sb) const
{
	if (_length!=sb._length) return false;

	for (int a=0;a<_length;a++)
	{
		if (buf.getConst(a)!=sb.buf.getConst(a)) return false;
	}
	return true;
}
bool String::operator!=(const String& sb) const
{
	return !((*this)==sb);
}

bool String::readFile(FILE *file)
{
	while (!feof(file))
	{
		if (!feof(file)) insertCharAtEnd(fgetc(file));
	}
	resize(_length-1);
	return true;
}

bool String::writeFile(FILE *file)
{
	if (fwrite(&buf.first(),1,_length,file)==_length) return true;
	return false;
}

bool String::writeToFile(FILE *file)
{
	_putw(_length,file);
	return writeFile(file);
}

bool String::readFromFile(FILE *file)
{
	int flen=_getw(file);
	if (flen<0) return false;
	resize(flen);
	for (int a=0;a<flen;a++)
	{
		buf[a]=fgetc(file);
	}
	return true;
}




int String::search(int startOffset, const String &text) const
{
	assert(startOffset>=0);
	if (startOffset>=_length) return -1;
	//if (length==0) return -1;
	//assert(startOffset>=0 && startOffset<length);

	int textLen=text.getLength();
	const char *textBuf=&text.buf.getConst(0);

	if (startOffset+textLen>_length) return -1;

	int b=0; //text index
	for (int a=startOffset;a<_length;a++)
	{
		if (buf.getConst(a)==textBuf[b]) 
		{
			if (b==textLen-1) 
			{
				int rv=a-b;
				return a-b;
			}
			b++;
		} 
		else b=0;
	}
	return -1;
}

//continuable search
bool String::searchRep(int &pos, const String &text) const
{
	int r=search(pos,text);
	if (r==-1) return false;
	pos=r+text._length;
	return true;
}


int String::parseInt() const
{
	const char *cstr=&buf.getConst(0);
	char *endPtr=0;
	return strtol(cstr,&endPtr,10);
}

String String::toUpper() const
{
	String s;
	s.resize(_length);
	for (int a=0;a<_length;a++)
	{
		char c=buf.getConst(a);
		if (c>='a' && c<='z') s[a]=c-32;
		else s[a]=c;
	}
	return s;
}
String String::toLower() const
{
	String s;
	int offset='A';
	s.resize(_length);
	for (int a=0;a<_length;a++)
	{
		char c=buf.getConst(a);
		if (c>='A' && c<='Z') s[a]=c+32;
		else s[a]=c;
	}
	return s;
}
