AR=lib /nologo
ARFLAGS=/out:cppre.lib
CC=cl /nologo
CCOPTS=-Ox
# CCOPTS=-ZI
CXXFLAGS=-I. -c $(CCOPTS) -EHsc  -D_CRT_SECURE_NO_DEPRECATE=1

SRC=          \
Matcher.cpp   \
Pattern.cpp   \
WCMatcher.cpp \
WCPattern.cpp \

OBJ=$(SRC:.cpp=.obj)

all: cppre.lib test test2

cppre.lib: $(OBJ)
	$(AR) $(ARFLAGS) $(OBJ)

test: cppre.lib test.cpp
	$(CC) $(CXXFLAGS) test.cpp
	$(CC) -Fetest.exe -Zi -EHsc test.obj cppre.lib

test2: cppre.lib test2.cpp
	$(CC) $(CXXFLAGS) test2.cpp
	$(CC) -Fetest2.exe -Zi -EHsc test2.obj cppre.lib

install:
	xcopy /Y /F cppre.lib "$(MSVCDIR)\lib"
	xcopy /Y /E /I /F regexp "$(MSVCDIR)\include\regexp"

.cpp.obj:
	$(CC) $(CXXFLAGS) $<

clean:
	del $(OBJ) *~ cppre.lib *.ilk *.pdb *.obj test.exe test2.exe
