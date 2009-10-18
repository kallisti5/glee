//////////////////////////////////////////////////////////////////////
// ArrayList.h
// Description:		Replacement for STL's vector class
//					ie, easier to debug, safer (array index errors 
//						caught by debug assertions), more flexible
//
// By:				Ben Woodhouse 
//
// Date:			02/2004
//
// Version:			1.3
//
//////////////////////////////////////////////////////////////////////

#pragma once

#define ARRAYLIST_MIN_SIZE 16

#include <stdio.h>

template <class T>
class ArrayList 
{
public:
	ArrayList()			{
							_size=0;
							minSize=0; 
							maxSize=0; 
							buf=NULL; 
						}
	ArrayList(uint size_in)
						{
							_size=0;
							buf=NULL;
							minSize=0;
							maxSize=0;
							resize(size_in);

						}
	ArrayList(const ArrayList<T>& src)	
	{
		_size=0; buf=NULL; maxSize=0; minSize=0; *this=src;
	}
	virtual ~ArrayList()
						{
							if (buf) delete [] buf;
						}

	ArrayList<T>& operator=(const ArrayList<T>& src)	
	{
		resize(src.size());
		for (int a=0;a<_size;a++) buf[a]=src.buf[a];
		return *this;
	}
	void push_back(const T& object)	{add(object);} //for compatibility with Vector
	void add(const T &object)
	{
		if (_size==maxSize)
		{
			T objCopy=object; //create a backup in case the object gets deleted
			resize(_size+1);
			buf[_size-1]=objCopy;
		}
		else 
		{
			_size++;
			buf[_size-1]=object;
		}
	}

	//inserts an object at index I
	int insert(uint i, const T& object)
	{
		assert(i<=_size);
		if (_size==maxSize)
		{
			T objCopy=object;
			resize(_size+1);
			for (uint a=_size-1;a>i;a--)
				buf[a]=buf[a-1];
			buf[i]=objCopy;
		}
		else
		{
			resize(_size+1);
			for (uint a=_size-1;a>i;a--)
				buf[a]=buf[a-1];
			buf[i]=object;
		}
		return i+1;	//return the index of the next point
	}

	int insert(uint i, const ArrayList<T>& src, uint begin, uint end)
	{
		assert(i<=_size);
		assert(begin<=end);
		assert(end<=src.size());
		int cSize=end-begin;
		int destEnd=i+cSize-1;
		if (_size==maxSize)
		{
			ArrayList<T> srcCopy=src;
			resize(_size+cSize);
			for (uint a=_size-1;a>destEnd;a--)
				buf[a]=srcCopy.buf[a-cSize];
			for (uint a=0;a<cSize;a++)
				buf[a+i]=srcCopy.buf[begin+a];
		}
		else
		{
			resize(_size+cSize);
			for (uint a=_size-1;a>destEnd;a--)
				buf[a]=buf[a-cSize];
			for (uint a=0;a<cSize;a++)
				buf[a+i]=src.buf[begin+a];
		}
		return i+cSize;	//return the index of the next point
	}

	int erase(uint i)
	{
		assert(i<_size);
		int sizeMinus1=_size-1;
		for (uint a=i;a<sizeMinus1;a++)
			buf[a]=buf[a+1];
		resize(sizeMinus1);
		return i;
	}

	void allocate(uint newMaxSize)
	{
		if (newMaxSize!=maxSize)
		{
			T* newBuf=new T[newMaxSize];
			int n=min(newMaxSize,maxSize);
			for (int a=0;a<n;a++) newBuf[a]=buf[a];
			delete [] buf;
			buf=newBuf;
			maxSize=newMaxSize;
			minSize=newMaxSize;
		}
	}

	void resize(uint newSize)
	{
		if (newSize>maxSize)
		{
			//expand the buffer
			int newMaxSize=max(maxSize, ARRAYLIST_MIN_SIZE);
			while (newMaxSize<=newSize)
			{
				newMaxSize=newMaxSize<<1;
			}
			T * newBuf=NULL;
			newBuf=new T[newMaxSize];
			for (int a=0;a<_size;a++) newBuf[a]=buf[a];
			if (buf) delete [] buf;
			buf=newBuf;
			maxSize=newMaxSize;
			minSize=maxSize>>1;
		}
		else if (newSize<_size)
		{
			if (newSize<minSize && minSize>ARRAYLIST_MIN_SIZE)
			{
				//shrink the buffer
				int newMinSize=max(minSize,ARRAYLIST_MIN_SIZE);
				while (newMinSize>newSize)
				{
					newMinSize=newMinSize>>1;
				}
				int newMaxSize=newMinSize<<1;

				T * newBuf=new T[newMaxSize];
				for (int a=0;a<newSize;a++) newBuf[a]=buf[a];
				if (buf!=NULL) delete [] buf;
				buf=newBuf;
				maxSize=newMaxSize;
				minSize=newMinSize;
			}
		}
		_size=newSize;
	}
	const T& getConst(uint i) const 
								{
									assert(i<_size);
									return buf[i];
								}
	T& get(uint i) const		{
									assert(i<_size);
									return buf[i];
								}
	T& operator[](uint i) const	{
									assert(i<_size);
									return buf[i];
								}

	uint size() const			{return _size;}

	T& last()					{
									assert(_size>0);
									return buf[_size-1];
								}
	T& first()					{
									assert(_size>0);
									return buf[0];
								}

	void clear()				{	resize(0);}


	T sum(T initialVal)			{ T rv=initialVal; for (int a=0;a<_size;a++) rv+=buf[a]; return rv; }
	T sum()						{ T rv; for (int a=0;a<_size;a++) rv+=buf[a]; return rv; }

private:
	T *buf;
	uint _size;
	uint maxSize;
	uint minSize;
};
