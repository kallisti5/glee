/*
 *  main.c
 *  Carbon OpenGL
 *
 *  Created by ggs on 13 Nov 2002

	Copyright:	Copyright © 2002-2004 Apple Computer, Inc., All Rights Reserved

	Disclaimer:	IMPORTANT:  This Apple software is supplied to you by Apple Computer, Inc.
			("Apple") in consideration of your agreement to the following terms, and your
			use, installation, modification or redistribution of this Apple software
			constitutes acceptance of these terms.  If you do not agree with these terms,
			please do not use, install, modify or redistribute this Apple software.

			In consideration of your agreement to abide by the following terms, and subject
			to these terms, Apple grants you a personal, non-exclusive license, under Apple’s
			copyrights in this original Apple software (the "Apple Software"), to use,
			reproduce, modify and redistribute the Apple Software, with or without
			modifications, in source and/or binary forms; provided that if you redistribute
			the Apple Software in its entirety and without modifications, you must retain
			this notice and the following text and disclaimers in all such redistributions of
			the Apple Software.  Neither the name, trademarks, service marks or logos of
			Apple Computer, Inc. may be used to endorse or promote products derived from the
			Apple Software without specific prior written permission from Apple.  Except as
			expressly stated in this notice, no other rights or licenses, express or implied,
			are granted by Apple herein, including but not limited to any patent rights that
			may be infringed by your derivative works or by other works in which the Apple
			Software may be incorporated.

			The Apple Software is provided by Apple on an "AS IS" basis.  APPLE MAKES NO
			WARRANTIES, EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION THE IMPLIED
			WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY AND FITNESS FOR A PARTICULAR
			PURPOSE, REGARDING THE APPLE SOFTWARE OR ITS USE AND OPERATION ALONE OR IN
			COMBINATION WITH YOUR PRODUCTS.

			IN NO EVENT SHALL APPLE BE LIABLE FOR ANY SPECIAL, INDIRECT, INCIDENTAL OR
			CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
			GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
			ARISING IN ANY WAY OUT OF THE USE, REPRODUCTION, MODIFICATION AND/OR DISTRIBUTION
			OF THE APPLE SOFTWARE, HOWEVER CAUSED AND WHETHER UNDER THEORY OF CONTRACT, TORT
			(INCLUDING NEGLIGENCE), STRICT LIABILITY OR OTHERWISE, EVEN IF APPLE HAS BEEN
			ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */
  
#define DEBUG 1 // define this to 1 to enable debugging output
#define kUseMultiSample 0 // define this to 1 to enable multi-sample

#include "../DATA/output/GLee.h"

#include <OpenGL/glu.h>
//#include <OpenGL/glext.h>

#include "HIDSupport.h"
#include "glCheck.h"
#include "trackball.h"
#include "SurfaceGeometry.h"
#include "main.h"

// ==================================

recVec gOrigin = {0.0, 0.0, 0.0};

// single set of interaction flags and states
GLint gDollyPanStartPoint[2] = {0, 0};
GLfloat gTrackBallRotation [4] = {0.0f, 0.0f, 0.0f, 0.0f};
GLboolean gDolly = GL_FALSE;
GLboolean gPan = GL_FALSE;
GLboolean gTrackball = GL_FALSE;
AGLContext gTrackingContextInfo = NULL;

enum
{
    kMainMenu = 500,
	kCloseMenuItem = 2,
	kInfoMenuItem = 4,
	kAnimateMenuItem = 5,
	kInfoState = 1,
	kAnimateState = 1
};

EventHandlerUPP gEvtHandler;			// main event handler
EventHandlerUPP gWinEvtHandler;			// window event handler

IBNibRef nibRef = NULL;

AbsoluteTime gStartTime;

char gErrorMessage[256] = ""; // buffer for error message output
float gErrorTime = 0.0;

#pragma mark ---- OpenGL Capabilities ----

// GL configuration info globals
// see glcheck.h for more info
GLCaps * gDisplayCaps = NULL; // array of GLCaps
CGDisplayCount gNumDisplays = 0;
// related DM change notification:
DMExtendedNotificationUPP gConfigEDMUPP = NULL;

static void getCurrentCaps (void)
{
 	// Check for existing opengl caps here
	// This can be called again with same display caps array when display configurations are changed and
	//   your info needs to be updated.  Note, if you are doing dynmaic allocation, the number of displays
	//   may change and thus you should always reallocate your display caps array.
	if (gDisplayCaps && HaveOpenGLCapsChanged (gDisplayCaps, gNumDisplays)) { // see if caps have changed
		free (gDisplayCaps);
		gDisplayCaps = NULL;
	}
	if (!gDisplayCaps) { // if we do not have caps
		CheckOpenGLCaps (0, NULL, &gNumDisplays); // will just update number of displays
		gDisplayCaps = (GLCaps*) malloc (sizeof (GLCaps) * gNumDisplays);
		CheckOpenGLCaps (gNumDisplays, gDisplayCaps, &gNumDisplays);
	}
	
}

#pragma mark ---- Utilities ----

// return float elpased time in seconds since app start
static float getElapsedTime (void)
{	
	float deltaTime = (float) AbsoluteDeltaToDuration (UpTime(), gStartTime);
    if (0 > deltaTime)	// if negative microseconds
		deltaTime /= -1000000.0;
    else				// else milliseconds
		deltaTime /= 1000.0;
	return deltaTime;
}

#pragma mark ---- Error Reporting ----

// C string to Pascal string
static void cstr2pstr (StringPtr outString, const char *inString)
{	
	unsigned short x = 0;
	do {
	    outString [x + 1] = (unsigned char) inString [x];
		x++;
	} while ((inString [x] != 0)  && (x < 256));
	outString [0] = x;									
}

// ---------------------------------

// error reporting as both window message and debugger string
void reportError (char * strError)
{
	Str255 strErr = "\p";

	gErrorTime = getElapsedTime ();
	sprintf (gErrorMessage, "Error: %s (at time: %0.1f secs)", strError, gErrorTime);
	 
	// out as debug string
	cstr2pstr (strErr, gErrorMessage);
	DebugStr (strErr);
	fflush (stderr);
}

// ---------------------------------

// if error dump agl errors to debugger string, return error
OSStatus aglReportError (void)
{
	GLenum err = aglGetError();
	if (AGL_NO_ERROR != err) {
		char errStr[256];
		sprintf (errStr, "AGL: %s",(char *) aglErrorString(err));
		reportError (errStr);
	}
	// ensure we are returning an OSStatus noErr if no error condition
	if (err == AGL_NO_ERROR)
		return noErr;
	else
		return (OSStatus) err;
}

// ---------------------------------

// if error dump gl errors to debugger string, return error
OSStatus glReportError (void)
{
	GLenum err = glGetError();
	if (GL_NO_ERROR != err) {
		char errStr[256];
		sprintf (errStr, "GL: %s",(char *) gluErrorString(err));
		reportError (errStr);
	}
	// ensure we are returning an OSStatus noErr if no error condition
	if (err == GL_NO_ERROR)
		return noErr;
	else
		return (OSStatus) err;
}

#pragma mark ---- OpenGL Utilities ----

// C string drawing function
void drawCStringGL (char * cstrOut, GLuint fontList)
{
	GLint i = 0;
	while (cstrOut [i])
		glCallList (fontList + cstrOut[i++]);
}

// ---------------------------------

// AGL bitmpp font setup
GLuint buildFontGL (AGLContext ctx, GLint fontID, Style face, GLint size)
{
	GLuint listBase = glGenLists (256);
	if (aglUseFont (ctx, fontID , face, size, 0, 256, (long) listBase)) {
		glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
		glReportError ();
		return listBase;
	} else {
		reportError ("aglUseFont failed\n" );
		glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
		glDeleteLists (listBase, 256);
		return 0;
	}
}

// ---------------------------------

// delete font list passed in
void deleteFontGL (GLuint fontList)
{
	if (fontList)
		glDeleteLists (fontList, 256);
}

// ---------------------------------

// given a delta time in seconds and current roatation accel, velocity and position, update overall object rotation
void updateRotation (double deltaTime, GLfloat * fRot, GLfloat * fVel, GLfloat * fAccel, GLfloat * objectRotation )
{
	// update rotation based on vel and accel
	float rotation[4] = {0.0f, 0.0f, 0.0f, 0.0f};
	GLfloat fVMax = 2.0f;
	short i;
	// do velocities
	for (i = 0; i < 3; i++) {
		fVel[i] += fAccel[i] * deltaTime * 30.0f;
		
		if (fVel[i] > fVMax) {
			fAccel[i] *= -1.0f;
			fVel[i] = fVMax;
		} else if (fVel[i] < -fVMax) {
			fAccel[i] *= -1.0f;
			fVel[i] = -fVMax;
		}
		
		fRot[i] += fVel[i] * deltaTime * 30.0f;
		
		while (fRot[i] > 360.0f)
			fRot[i] -= 360.0f;
		while (fRot[i] < -360.0f)
			fRot[i] += 360.0f;
	}
	rotation[0] = fRot[0];
	
	rotation[1] = 1.0f;
	addToRotationTrackball (rotation, objectRotation);
	rotation[0] = fRot[1];
	rotation[1] = 0.0f; rotation[2] = 1.0f;
	addToRotationTrackball (rotation, objectRotation);
	rotation[0] = fRot[2];
	rotation[2] = 0.0f; rotation[3] = 1.0f;
	addToRotationTrackball (rotation, objectRotation);
}

// ---------------------------------

#ifndef DTOR
	#define DTOR 0.0174532925
#endif

#ifndef MIN
	#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

// update the projection matrix based on camera and view info
// should be called when viewport size, eye z position, or camera aperture changes
// also call if far or near changes which is determined by shape size in this case
// current context should be set before calling
void updateProjection (GLdouble width, GLdouble height, GLdouble zPos, GLdouble aperture, GLdouble shapeSize)
{
	GLdouble xmin, xmax, ymin, ymax;
	// far frustum plane
	GLdouble zFar = -zPos + shapeSize * 0.5;
	// near frustum plane (clamped at 1.0)
	GLdouble zNear = MIN (-zPos - shapeSize * 0.5, 1.0);
	// view aspect ratio
	GLdouble aspect = width / height; 

	glMatrixMode (GL_PROJECTION);
	glLoadIdentity ();

	if (aspect > 1.0) {
		ymax = zNear * tan (aperture * 0.5 * DTOR);
		ymin = -ymax;
		xmin = ymin * aspect;
		xmax = ymax * aspect;
	} else {
		xmax = zNear * tan (aperture * 0.5 * DTOR);
		xmin = -xmax;
		ymin = xmin / aspect;
		ymax = xmax / aspect;
	}
	glFrustum(xmin, xmax, ymin, ymax, zNear, zFar);
}

// ---------------------------------

// updates the contexts model view matrix for object and camera moves
// we will call this every draw loop for simplicity
// current context should be set before calling
void updateModelView (AGLContext aglContext, recCamera * pCamera, GLfloat * pSpinRot, GLfloat * pObjectRot, GLfloat * pWorldRot)
{
	// move view
	glMatrixMode (GL_MODELVIEW);
	glLoadIdentity ();
	gluLookAt (pCamera->viewPos.x, pCamera->viewPos.y, pCamera->viewPos.z,
			   pCamera->viewPos.x + pCamera->viewDir.x,
			   pCamera->viewPos.y + pCamera->viewDir.y,
			   pCamera->viewPos.z + pCamera->viewDir.z,
			   pCamera->viewUp.x, pCamera->viewUp.y ,pCamera->viewUp.z);
		
	if (pWorldRot) { // if we have a world rotation eval the track ball
		// if we have trackball rotation to map (this IS the test I want as it can be explicitly 0.0f)
		if ((gTrackingContextInfo == aglContext) && gTrackBallRotation[0] != 0.0f) 
		glRotatef (gTrackBallRotation[0], gTrackBallRotation[1], gTrackBallRotation[2], gTrackBallRotation[3]);
	else {
	}
	// accumlated world rotation via trackball
		glRotatef (pWorldRot[0], pWorldRot[1], pWorldRot[2], pWorldRot[3]);
	}
	if (pObjectRot && pSpinRot) {
	// object itself rotating applied after camera rotation
		glRotatef (pObjectRot[0], pObjectRot[1], pObjectRot[2], pObjectRot[3]);
		pSpinRot[0] = 0.0f; // reset animation rotations (do in all cases to prevent rotating while moving with trackball)
		pSpinRot[1] = 0.0f;
		pSpinRot[2] = 0.0f;
	}
}

// ---------------------------------

// handles resizing of GL need context update and if the window dimensions change, a
// a window dimension update, reseting of viewport and an update of the projection matrix
// this is a windowing system level routine so it handles setting context, etc.
// returns error if resize fails

OSStatus resizeGL (AGLContext aglContext, recCamera * pCamera, GLfloat shapeSize, CGRect viewRect)
{
    OSStatus err = noErr;

    if (!aglContext)
        return paramErr;

	// re-attach drawable to ensure context is updated
	if (!aglSetCurrentContext (aglContext))
		err = aglReportError ();
	if ((noErr == err) && !aglUpdateContext (aglContext))
		err = aglReportError ();
	
// Must go directly to the stored value to proper check for differences with window size
// Note, it is likely a toss up as to whether blinding setting the viewport is faster than
// testing for changes
	if (noErr == err) {
		GLint aViewportDims[4];
		glGetIntegerv (GL_VIEWPORT, aViewportDims);
		if ((viewRect.size.width != aViewportDims[2]) ||
			(viewRect.size.height != aViewportDims[3])) {
			pCamera->viewOriginX = viewRect.origin.x;
			pCamera->viewOriginY = viewRect.origin.y;
			pCamera->viewWidth = viewRect.size.width;
			pCamera->viewHeight = viewRect.size.height;
			glViewport (0, 0, pCamera->viewWidth, pCamera->viewHeight);
		}
	}
    return err;
}

// ---------------------------------

// sets the camera data to initial conditions
static void resetCamera (recCamera * pCamera)
{
   pCamera->aperture = 40;
   pCamera->rotPoint = gOrigin;

   pCamera->viewPos.x = 0.0;
   pCamera->viewPos.y = 0.0;
   pCamera->viewPos.z = -12.0;
   pCamera->viewDir.x = -pCamera->viewPos.x; 
   pCamera->viewDir.y = -pCamera->viewPos.y; 
   pCamera->viewDir.z = -pCamera->viewPos.z;

   pCamera->viewUp.x = 0;  
   pCamera->viewUp.y = 1; 
   pCamera->viewUp.z = 0;

	// will be set in resize once the target view size and position is known
	pCamera->viewOriginY = 0;
	pCamera->viewOriginX = 0;
	pCamera->viewHeight = 0;
	pCamera->viewWidth = 0;
}

// ---------------------------------

void SetLighting(unsigned int mode)
{
	GLfloat mat_specular[] = {1.0, 1.0, 1.0, 1.0};
	GLfloat mat_shininess[] = {90.0};

	GLfloat position[4] = {7.0,-7.0,12.0,0.0};
	GLfloat ambient[4]  = {0.2,0.2,0.2,1.0};
	GLfloat diffuse[4]  = {1.0,1.0,1.0,1.0};
	GLfloat specular[4] = {1.0,1.0,1.0,1.0};
	
	glMaterialfv (GL_FRONT_AND_BACK, GL_SPECULAR, mat_specular);
	glMaterialfv (GL_FRONT_AND_BACK, GL_SHININESS, mat_shininess);
	
	glEnable(GL_COLOR_MATERIAL);
	glColorMaterial(GL_FRONT_AND_BACK,GL_AMBIENT_AND_DIFFUSE);

	switch (mode) {
		case 0:
			break;
		case 1:
			glLightModeli(GL_LIGHT_MODEL_TWO_SIDE,GL_FALSE);
			glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER,GL_FALSE);
			break;
		case 2:
			glLightModeli(GL_LIGHT_MODEL_TWO_SIDE,GL_FALSE);
			glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER,GL_TRUE);
			break;
		case 3:
			glLightModeli(GL_LIGHT_MODEL_TWO_SIDE,GL_TRUE);
			glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER,GL_FALSE);
			break;
		case 4:
			glLightModeli(GL_LIGHT_MODEL_TWO_SIDE,GL_TRUE);
			glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER,GL_TRUE);
			break;
	}
	
	glLightfv(GL_LIGHT0,GL_POSITION,position);
	glLightfv(GL_LIGHT0,GL_AMBIENT,ambient);
	glLightfv(GL_LIGHT0,GL_DIFFUSE,diffuse);
	glLightfv(GL_LIGHT0,GL_SPECULAR,specular);
	glEnable(GL_LIGHT0);
}

#pragma mark ---- OpenGL Minimize Handler (thanks to Dan Herman) ----

void InvertGLImage( char *imageData, size_t imageSize, size_t rowBytes )
{
	long i, j;
	char *tBuffer = (char*) malloc (rowBytes);
	if (NULL == tBuffer) return;
		
	// Copy by rows through temp buffer
	for (i = 0, j = imageSize - rowBytes; i < imageSize >> 1; i += rowBytes, j -= rowBytes) {
		memcpy( tBuffer, &imageData[i], rowBytes );
		memcpy( &imageData[i], &imageData[j], rowBytes );
		memcpy( &imageData[j], tBuffer, rowBytes );
	}
	free(tBuffer);
}

// ---------------------------------

void CompositeGLBufferIntoWindow (AGLContext ctx, Rect *bufferRect, WindowRef win)
{
	GWorldPtr pGWorld;
	QDErr err;
	// blit OpenGL content into window backing store
	// allocate buffer to hold pane image
	long width  = (bufferRect->right - bufferRect->left);
	long height  = (bufferRect->bottom - bufferRect->top);
	Rect src_rect = {0, 0, height, width};
	long rowBytes = width * 4;
	long imageSize = rowBytes * height;
	char *image = (char *) NewPtr (imageSize);
	if (!image) {
		printf("Out of memory in CompositeGLBufferIntoWindow()!");
		return;		// no harm in continuing
	}
	
	// pull GL content down to our image buffer
	aglSetCurrentContext( ctx );
	glReadPixels (0, 0, width, height, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, image);

	// GL buffers are upside-down relative to QD buffers, so we need to flip it
	InvertGLImage( image, imageSize, rowBytes );

	// create a GWorld containing our image
	err = NewGWorldFromPtr (&pGWorld, k32ARGBPixelFormat, &src_rect, 0, 0, 0, image, rowBytes);
	if (err != noErr) {
		printf("WARNING: error in NewGWorldFromPtr, called from CompositeGLBufferIntoWindow()");
		free( image );
		return;
	}
	
	SetPort( GetWindowPort (win));
	CopyBits( GetPortBitMapForCopyBits (pGWorld), GetPortBitMapForCopyBits (GetWindowPort (win)), &src_rect, bufferRect, srcCopy, 0 );

	DisposeGWorld( pGWorld );
	DisposePtr ( image );
}

#pragma mark ---- Display Manager Event Handling ----

// update our GL configuration info based on display change notification
void handleConfigDMEvent (void *userData, short theMessage, void *notifyData)
{
	if (kDMNotifyEvent == theMessage) { // post change notifications only
		getCurrentCaps ();
	}
}

// ---------------------------------

// handle display config changes meaing we need to update the GL context via the resize function and check for windwo dimension changes
// also note we redraw the content here as it could be lost in a display config change
void handleWindowDMEvent (void *userData, short theMessage, void *notifyData)
{
	if (kDMNotifyEvent == theMessage) { // post change notifications only
		pRecContext pContextInfo = NULL;
		WindowRef window = (WindowRef) userData;
		if (window)
			pContextInfo = GetCurrentContextInfo (window);
		if (pContextInfo) { // have a valid OpenGl window
			Rect rectPort;
			CGRect viewRect = {{0.0f, 0.0f}, {0.0f, 0.0f}};
#if DEBUG
			sprintf (pContextInfo->message, "Event: Display Change at %0.1f secs", getElapsedTime ());
			pContextInfo->msgTime = getElapsedTime ();
#endif
			GetWindowPortBounds (window, &rectPort);
			viewRect.size.width = (float) (rectPort.right - rectPort.left);
			viewRect.size.height = (float) (rectPort.bottom - rectPort.top);
			resizeGL (pContextInfo->aglContext, &pContextInfo->camera, pContextInfo->shapeSize, viewRect);
			InvalWindowRect (window,  &rectPort); // force redrow
		}
	}
}

#pragma mark ---- OpenGL Window ----

// intializes context conditions
static void initialConditions (pRecContext pContextInfo)
{
	BlockZero (pContextInfo, sizeof (recContext));
	resetCamera (&pContextInfo->camera);
	pContextInfo->fVel[0] = 0.3; pContextInfo->fVel[1] = 0.1; pContextInfo->fVel[2] = 0.2; 
	pContextInfo->fAccel[0] = 0.003; pContextInfo->fAccel[1] = -0.005; pContextInfo->fAccel[2] = 0.004;
	pContextInfo->info = kInfoState;
	pContextInfo->animate = kAnimateState;
	pContextInfo->drawHelp = 1;
	pContextInfo->polygons = 1;
	pContextInfo->showCredits = 1;
	pContextInfo->lighting = 4;
	pContextInfo->timer = NULL;

	pContextInfo->surface = 1; 
	pContextInfo->colorScheme = 4;
	pContextInfo->subdivisions = 128;
	pContextInfo->xyRatio = 3;
}

// ---------------------------------

OSStatus buildGL (WindowRef window)
{
	OSStatus err = noErr;
	Rect rectPort;
#if kUseMultiSample
	GLint attrib[] = { AGL_RGBA, AGL_DOUBLEBUFFER, AGL_DEPTH_SIZE, 16, AGL_NO_RECOVERY, 
	                   AGL_SAMPLE_BUFFERS_ARB, 1, AGL_SAMPLES_ARB, kSamples, AGL_NONE };
#else
	GLint attrib[] = { AGL_RGBA, AGL_DOUBLEBUFFER, AGL_DEPTH_SIZE, 16, AGL_NONE };
#endif
    pRecContext pContextInfo = GetCurrentContextInfo(window);
	ProcessSerialNumber psn = { 0, kCurrentProcess };
    
    if (NULL == pContextInfo)
        return paramErr;
	if (pContextInfo->aglContext)
		return noErr; // already built
		
	// build context
	pContextInfo->aglContext = NULL;
	pContextInfo->aglPixFmt = aglChoosePixelFormat(NULL, 0, attrib);
	err = aglReportError ();
	if (pContextInfo->aglPixFmt) {
		pContextInfo->aglContext = aglCreateContext(pContextInfo->aglPixFmt, NULL);
		err = aglReportError ();
	}
#if kUseMultiSample
	// if multi-sample pixel format or context fails allocate a non-multi-sample one
	if ((noErr != err) || (NULL == pContextInfo->aglContext)) { // try non-multisample
		attrib[5] = attrib[6] = attrib[7] = attrib[8] = 0;
		if (pContextInfo->aglPixFmt)
			aglDestroyPixelFormat (pContextInfo->aglPixFmt);
		pContextInfo->aglPixFmt = aglChoosePixelFormat(NULL, 0, attrib);
		err = aglReportError ();
		if (pContextInfo->aglPixFmt) {
			pContextInfo->aglContext = aglCreateContext (pContextInfo->aglPixFmt, NULL);
			err = aglReportError ();
		}
	}
#endif
	if (pContextInfo->aglContext) {
		short fNum;
		GLint swap = 1;
		CGRect viewRect = {{0.0f, 0.0f}, {0.0f, 0.0f}};

        GrafPtr portSave = NULL;
        GetPort (&portSave);
        SetPort ((GrafPtr) GetWindowPort (window));

		if(!aglSetDrawable(pContextInfo->aglContext, GetWindowPort (window)))
			err = aglReportError ();
		if (!aglSetCurrentContext(pContextInfo->aglContext))
			err = aglReportError ();
		pContextInfo->currentVS = aglGetVirtualScreen (pContextInfo->aglContext); // sync renderer
		
		// ensure we know when display configs are changed
		pContextInfo->windowEDMUPP = NewDMExtendedNotificationUPP (handleWindowDMEvent); // for display change notification
		DMRegisterExtendedNotifyProc (pContextInfo->windowEDMUPP, (void *) window, NULL, &psn);

		// VBL SYNC
		if (!aglSetInteger (pContextInfo->aglContext, AGL_SWAP_INTERVAL, &swap))
			aglReportError ();

		// init GL stuff here
#if kUseMultiSample
		switch (pContextInfo->modeFSAA) {
			case kFSAAOff:
				glDisable (GL_MULTISAMPLE_ARB);
				break;
			case kFSAAFast:
				glEnable (GL_MULTISAMPLE_ARB);
				glHint (GL_MULTISAMPLE_FILTER_HINT_NV, GL_FASTEST);
				break;
			case kFSAANice:
				glEnable (GL_MULTISAMPLE_ARB);
				glHint (GL_MULTISAMPLE_FILTER_HINT_NV, GL_NICEST);
				break;
		}
#endif
		glEnable(GL_DEPTH_TEST);
		glShadeModel(GL_SMOOTH);    
		glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);
		glFrontFace(GL_CCW);
		glPolygonOffset (1.0, 1.0);
		glClearColor(0.0,0.0,0.0,0.0);

		pContextInfo->shapeSize = 11.0f; // max radius of of objects

		GetFNum ("\pGeneva", &fNum); // build font
		pContextInfo->boldFontList = buildFontGL (pContextInfo->aglContext, fNum, bold, 9);
		pContextInfo->regFontList = buildFontGL (pContextInfo->aglContext, fNum, normal, 9);

		// setup viewport and prespective
		GetWindowPortBounds (window, &rectPort);
		viewRect.size.width = (float) (rectPort.right - rectPort.left);
		viewRect.size.height = (float) (rectPort.bottom - rectPort.top);
		pContextInfo->camera.viewOriginX = viewRect.origin.x;
		pContextInfo->camera.viewOriginY = viewRect.origin.y;
		pContextInfo->camera.viewWidth = viewRect.size.width;
		pContextInfo->camera.viewHeight = viewRect.size.height;
		glViewport (0, 0, pContextInfo->camera.viewWidth, pContextInfo->camera.viewHeight);
		
		SetLighting (4);
		BuildGeometry (pContextInfo->surface, pContextInfo->colorScheme, pContextInfo->subdivisions, pContextInfo->xyRatio,
					   &(pContextInfo->polyList), &(pContextInfo->lineList), &(pContextInfo->pointList));
		
        SetPort (portSave);
    }
    return err;
}

// ---------------------------------

// dump window data structures and OpenGL context and related data structures
OSStatus disposeGL (pRecContext pContextInfo)
{
	// release our data
    if ( pContextInfo != NULL )
    {
		if (pContextInfo->windowEDMUPP) { // dispose UPP for DM notifications
			DisposeDMExtendedNotificationUPP (pContextInfo->windowEDMUPP);
			pContextInfo->windowEDMUPP = NULL;
		}
		aglSetDrawable (pContextInfo->aglContext, NULL);
		aglSetCurrentContext (NULL);
		if (pContextInfo->aglContext) {
			aglDestroyContext (pContextInfo->aglContext);
			pContextInfo->aglContext = NULL;
		}
		if (pContextInfo->aglPixFmt) {
			aglDestroyPixelFormat (pContextInfo->aglPixFmt);
			pContextInfo->aglPixFmt = NULL;
		}
		if (pContextInfo->timer) {
			RemoveEventLoopTimer(pContextInfo->timer);
			pContextInfo->timer = NULL;
		}
		// dump font display list
		if (pContextInfo->boldFontList) {
			deleteFontGL (pContextInfo->boldFontList);
			pContextInfo->boldFontList = 0;
		}
		if (pContextInfo->regFontList) {
			deleteFontGL (pContextInfo->regFontList);
			pContextInfo->regFontList = 0;
		}
    }
    
    return noErr;
}

// ---------------------------------

#include "drawInfo.h" // source info for info output

// ---------------------------------

// draw text info
// note: this bitmap technique is not the speediest and one should use textures for fonts in more proformance critical code
static void drawInfo (pRecContext pContextInfo)
{	
	static float msgPresistance = 10.0f;
	char cstr [256];
	GLint matrixMode, line = 1;
	GLboolean depthTest = glIsEnabled (GL_DEPTH_TEST);
	GLfloat height, width;
	
	if (!pContextInfo)
		return;
	
	height = pContextInfo->camera.viewHeight;
	width = pContextInfo->camera.viewWidth;

	glDisable (GL_DEPTH_TEST); // ensure text is not remove by deoth buffer test.
	glEnable (GL_BLEND); // for text fading
	glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // ditto
	
	// set orthograhic 1:1  pixel transform in local view coords
	glGetIntegerv (GL_MATRIX_MODE, &matrixMode);
	glMatrixMode (GL_PROJECTION);
	glPushMatrix();
		glLoadIdentity ();
		glMatrixMode (GL_MODELVIEW);
		glPushMatrix();
			glLoadIdentity ();
			glScalef (2.0 / width, -2.0 /  height, 1.0);
			glTranslatef (-width / 2.0, -height / 2.0, 0.0);
			// output strings
			glColor3f (1.0, 1.0, 1.0);
			sprintf (cstr, "Camera at (%0.1f, %0.1f, %0.1f) looking at (%0.1f, %0.1f, %0.1f) with %0.1f aperture", 
					pContextInfo->camera.viewPos.x, pContextInfo->camera.viewPos.y, pContextInfo->camera.viewPos.z,
					pContextInfo->camera.viewDir.x, pContextInfo->camera.viewDir.y, pContextInfo->camera.viewDir.z,
					pContextInfo->camera.aperture);
			glRasterPos3d (10, line++ * 12, 0); 
			drawCStringGL (cstr, pContextInfo->boldFontList);
			sprintf (cstr, "Trackball Rotation: (%0.1f, %0.2f, %0.2f, %0.2f)", 
			         gTrackBallRotation[0], gTrackBallRotation[1], gTrackBallRotation[2], gTrackBallRotation[3]);
			glRasterPos3d (10, line++ * 12, 0); 
			drawCStringGL (cstr, pContextInfo->regFontList);
			sprintf (cstr, "World Rotation: (%0.1f, %0.2f, %0.2f, %0.2f)", 
			         pContextInfo->worldRotation[0], pContextInfo->worldRotation[1], pContextInfo->worldRotation[2], pContextInfo->worldRotation[3]);
			glRasterPos3d (10, line++ * 12, 0); 
			drawCStringGL (cstr, pContextInfo->regFontList);
			sprintf (cstr, "Vertices: %lu, Color Scheme: %lu", 
			         pContextInfo->subdivisions * pContextInfo->xyRatio * pContextInfo->subdivisions, pContextInfo->colorScheme);
			glRasterPos3d (10, line++ * 12, 0); 
			drawCStringGL (cstr, pContextInfo->regFontList);
			{
				GLboolean twoSidedLighting, localViewer;
				glGetBooleanv (GL_LIGHT_MODEL_LOCAL_VIEWER, &localViewer);
				glGetBooleanv (GL_LIGHT_MODEL_TWO_SIDE, &twoSidedLighting);
				if (!pContextInfo->lighting) {
					sprintf (cstr, "-- Lighting off");
				} else {
					if (!twoSidedLighting)
						sprintf (cstr, "-- Single Sided Lighting");
					else
						sprintf (cstr, "-- Two Sided Lighting");
					if (localViewer)
						sprintf (cstr, "%s: Local Viewer", cstr);
				}	
				glRasterPos3d (10, line++ * 12, 0); 
				drawCStringGL (cstr, pContextInfo->regFontList);
			}

			glRasterPos3d (10, line++ * 12, 0); 
#if kUseMultiSample
			switch (pContextInfo->modeFSAA) {
				case kFSAAOff:
					sprintf (cstr, "-- %d x FSAA: Disabled", kSamples);
					break;
				case kFSAAFast:
					sprintf (cstr, "-- %d x FSAA: Fastest hint", kSamples);
					break;
				case kFSAANice:
					sprintf (cstr, "-- %d x FSAA: Nicest hint", kSamples);
					break;
			}
#else
			sprintf (cstr, "-- FSAA: Off");
#endif
			drawCStringGL (cstr, pContextInfo->regFontList);

			// message string
			if (pContextInfo->message[0]) {
				float currDelta = getElapsedTime () - pContextInfo->msgTime;
				glColor4f (1.0, 1.0, 1.0, (msgPresistance - currDelta) * 0.1);
				glRasterPos3d (10, line++ * 12, 0); 
				drawCStringGL (pContextInfo->message, pContextInfo->boldFontList);
				if (currDelta > msgPresistance)
					pContextInfo->message[0] = 0;
			}
			// global error message
			if (gErrorMessage[0]) {
				float currDelta = getElapsedTime () - gErrorTime;
				glColor4f (1.0, 0.2, 0.2, (msgPresistance - currDelta) * 0.1);
				glRasterPos3d (10, line++ * 12, 0); 
				drawCStringGL (gErrorMessage, pContextInfo->boldFontList);
				if (currDelta > msgPresistance)
					gErrorMessage[0] = 0;
			}
			if (pContextInfo->showCredits) {
				char *strName, *strAuthor, *strX, *strY, *strZ, *strRange;
				GetStrings (pContextInfo->surface, &strName, &strAuthor, &strX, &strY, &strZ, &strRange);
				line = 10;
				glColor3f (1.0f, 1.0f, 0.0f);
				glRasterPos3d (10, line++ * 12, 0); 
				drawCStringGL (strName, pContextInfo->boldFontList);
				glRasterPos3d (10, line++ * 12, 0); 
				drawCStringGL (strAuthor, pContextInfo->regFontList);
				glColor3f (0.7f, 0.7f, 0.0f);
				glRasterPos3d (10, line++ * 12, 0); 
				drawCStringGL (strX, pContextInfo->regFontList);
				glRasterPos3d (10, line++ * 12, 0); 
				drawCStringGL (strY, pContextInfo->regFontList);
				glRasterPos3d (10, line++ * 12, 0); 
				drawCStringGL (strZ, pContextInfo->regFontList);
				glRasterPos3d (10, line++ * 12, 0); 
				drawCStringGL (strRange, pContextInfo->regFontList);
			}
			if (pContextInfo->drawHelp) {
				line = 4;
				glColor3f (0.8f, 0.8f, 0.8f);
				glRasterPos3d (10, height - line++ * 12, 0); 
				drawCStringGL ("\\: cycle surface type", pContextInfo->regFontList);
				glRasterPos3d (10, height - line++ * 12, 0); 
				drawCStringGL ("; & ': decrease/increase subdivisions", pContextInfo->regFontList);
				glRasterPos3d (10, height - line++ * 12, 0); 
				drawCStringGL ("[ & ]: cycle color scheme", pContextInfo->regFontList);
				glRasterPos3d (10, height - line++ * 12, 0); 
				drawCStringGL ("'l': cycle lighting  'm': cycle full scene anti-aliasing", pContextInfo->regFontList);
				glRasterPos3d (10, height - line++ * 12, 0); 
				drawCStringGL ("'p': toggle points   'w': toggle wireframe   'f': toggle fill", pContextInfo->regFontList);
				glRasterPos3d (10, height - line++ * 12, 0); 
				drawCStringGL ("'?': toggle credits  'c': toggle OpenGL caps", pContextInfo->regFontList);
				glRasterPos3d (10, height - line++ * 12, 0); 
				drawCStringGL ("Cmd-A: animate       Cmd-I: show info", pContextInfo->regFontList);
				glRasterPos3d (10, height - line++ * 12, 0); 
				drawCStringGL ("Wheel: zoom camera", pContextInfo->boldFontList);
				glRasterPos3d (10, height - line++ * 12, 0); 
				drawCStringGL ("Middle Button Drag: dolly object", pContextInfo->boldFontList);
				glRasterPos3d (10, height - line++ * 12, 0); 
				drawCStringGL ("Right Button Drag: pan object", pContextInfo->boldFontList);
				glRasterPos3d (10, height - line++ * 12, 0); 
				drawCStringGL ("Left Button Drag: rotate object", pContextInfo->boldFontList);
				glRasterPos3d (10, height - line++ * 12, 0); 
				drawCStringGL ("-- Help ('h') --", pContextInfo->boldFontList);
			}

			glColor3f (1.0, 1.0, 1.0);
			glRasterPos3d (10, height - 27, 0); 
			sprintf (cstr, "(%0.0f x %0.0f)", width, height);
			drawCStringGL (cstr, pContextInfo->boldFontList);
			// render and vendor info
			glRasterPos3d (10, height - 15, 0); 
			drawCStringGL ((char*) glGetString (GL_RENDERER), pContextInfo->boldFontList);
			glRasterPos3d (10, height - 3, 0); 
			drawCStringGL ((char*) glGetString (GL_VERSION), pContextInfo->regFontList);
			if (pContextInfo->drawCaps)
				drawCaps (pContextInfo);
		// reset orginal martices
		glPopMatrix(); // GL_MODELVIEW
		glMatrixMode (GL_PROJECTION);
	glPopMatrix();
	glMatrixMode (matrixMode);
	if (depthTest)
		glEnable (GL_DEPTH_TEST);
	glReportError ();
}

// ---------------------------------

// main OpenGL drawing function
void drawGL (WindowRef window, Boolean swap)
{
	static bool GLeeHasBeenInited = false;
	
    pRecContext pContextInfo = NULL;
	Rect rectPort;
	CGRect viewRect = {{0.0f, 0.0f}, {0.0f, 0.0f}};
	
	if (window)
		pContextInfo = GetCurrentContextInfo (window);
	if (!pContextInfo)
        return;
	if (!pContextInfo->aglContext)
		buildGL(window);
   
	// ensure the context is current	
	if (!aglSetCurrentContext (pContextInfo->aglContext))
		aglReportError ();

	if ( !GLeeHasBeenInited )
	{
		GLeeHasBeenInited = true;
		// Lazy load test
		glUseProgramObjectARB(0);
		
//		if ( !GLeeInit() )
//		{
//			aglReportError();
//		}
		if (GLEE_VERSION_1_2)
		{
			printf("OpenGL 1.5 supported!");
		}
		if ( GLEE_ARB_shader_objects )
		{
			GLhandleARB program;
			glUseProgramObjectARB(0);
		}
	}
	

	GetWindowPortBounds (window, &rectPort);
	viewRect.size.width = (float) (rectPort.right - rectPort.left);
	viewRect.size.height = (float) (rectPort.bottom - rectPort.top);
	resizeGL (pContextInfo->aglContext, &pContextInfo->camera, pContextInfo->shapeSize, viewRect);
	updateProjection (pContextInfo->camera.viewWidth, pContextInfo->camera.viewHeight, pContextInfo->camera.viewPos.z,
					  pContextInfo->camera.aperture, pContextInfo->shapeSize);
	updateModelView (pContextInfo->aglContext, &pContextInfo->camera, 
	                 pContextInfo->fRot, pContextInfo->objectRotation, pContextInfo->worldRotation);

	// clear our drawable
	glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	if (pContextInfo->polygons) {
		if (pContextInfo->lighting)
			glEnable(GL_LIGHTING);
		else 
			glDisable(GL_LIGHTING);
		glCallList (pContextInfo->polyList);
		glDisable(GL_LIGHTING);
	} else if (pContextInfo->lines) {
		glDisable(GL_LIGHTING);
		glCallList (pContextInfo->lineList);
	} else if (pContextInfo->points) {
		glDisable(GL_LIGHTING);
		glCallList (pContextInfo->pointList);
	}

    if (pContextInfo->info) {
		drawInfo (pContextInfo);
	}
	if (swap)
		aglSwapBuffers (pContextInfo->aglContext);
	else
		glFlush();
}

#pragma mark ---- Carbon Timer ----

// per-window timer function, basic time based animation preformed here
static void timerContextCB (WindowRef window)
{
    pRecContext pContextInfo = NULL;
	if (window)
		pContextInfo = GetCurrentContextInfo (window);
	if (pContextInfo) {
		AbsoluteTime currTime = UpTime ();
		double deltaTime = (float) AbsoluteDeltaToDuration (currTime, pContextInfo->time);
		pContextInfo->time = currTime;	// reset for next time interval
		if (0 > deltaTime)	// if negative microseconds
			deltaTime /= -1000000.0;
		else				// else milliseconds
			deltaTime /= 1000.0;
		if (deltaTime < 10.0) { // skip large pauses
			if (!gTrackball || (gTrackingContextInfo != pContextInfo->aglContext))
				updateRotation (deltaTime, pContextInfo->fRot, pContextInfo->fVel, pContextInfo->fAccel, pContextInfo->objectRotation);
			drawGL (window, true); // required to do this directly to get animation during resize and drags
		}
	}
}

// ---------------------------------

// timer callback bottleneck
static pascal void timerCB (EventLoopTimerRef inTimer, void* userData)
{
	#pragma unused (inTimer)
    timerContextCB ((WindowRef) userData); // timer based update
}

// ---------------------------------

// get UPP for timer
static EventLoopTimerUPP getTimerUPP (void)
{
	static EventLoopTimerUPP	sTimerUPP = NULL;
	
	if (sTimerUPP == NULL)
		sTimerUPP = NewEventLoopTimerUPP (timerCB);
	
	return sTimerUPP;
}

#pragma mark ---- Carbon Event Handling ----

pRecContext GetCurrentContextInfo (WindowRef window)
{
	if (NULL == window)  // HID use this path
		window = FrontWindow ();
	if (window)
		return (pRecContext) GetWRefCon (window);
	else
		return NULL;
}

// ---------------------------------

// move camera in z axis
static void mouseDolly (HIPoint location, pRecContext pContextInfo)
{
	GLfloat dolly = (gDollyPanStartPoint[1] - location.y) * -pContextInfo->camera.viewPos.z / 300.0f;
	pContextInfo->camera.viewPos.z += dolly;
	if (pContextInfo->camera.viewPos.z == 0.0) // do not let z = 0.0
		pContextInfo->camera.viewPos.z = 0.0001;
	gDollyPanStartPoint[0] = location.x;
	gDollyPanStartPoint[1] = location.y;
}
	
// ---------------------------------
	
// move camera in x/y plane
static void mousePan (HIPoint location, pRecContext pContextInfo)
{
	GLfloat panX = (gDollyPanStartPoint[0] - location.x) / (900.0f / -pContextInfo->camera.viewPos.z);
	GLfloat panY = (gDollyPanStartPoint[1] - location.y) / (900.0f / -pContextInfo->camera.viewPos.z);
	pContextInfo->camera.viewPos.x -= panX;
	pContextInfo->camera.viewPos.y -= panY;
	gDollyPanStartPoint[0] = location.x;
	gDollyPanStartPoint[1] = location.y;
}

// ---------------------------------

// Handles global non-system mouse events for GL windows as follows:
// primary button = object tumble (trackball)
// secondary button (or cntl-primary) = pan
// tertiary button (or option-primary) = dolly
// wheel = aperture
	
static OSStatus handleWindowMouseEvents (EventHandlerCallRef myHandler, EventRef event)
{
    WindowRef			window = NULL;
    pRecContext 		pContextInfo = NULL;
	OSStatus			result = eventNotHandledErr;
    UInt32 				kind = GetEventKind (event);
	EventMouseButton	button = 0;
	HIPoint				location = {0.0f, 0.0f};
	UInt32				modifiers = 0;	
	long				wheelDelta = 0;		
	Rect 				rectPort;

	// Mac OS X v10.1 and later
	// should this be front window???
	GetEventParameter(event, kEventParamWindowRef, typeWindowRef, NULL, sizeof(WindowRef), NULL, &window);
	if (window)
		pContextInfo = GetCurrentContextInfo (window);
	if (!pContextInfo)
		return result; // not an application GLWindow so do not process (there is an exception)
	GetWindowPortBounds (window, &rectPort);
		
	result = CallNextEventHandler(myHandler, event);	
	if (eventNotHandledErr == result) 
	{ // only handle events not already handled (prevents wierd resize interaction)
		switch (kind) {
			// start trackball, pan, or dolly
			case kEventMouseDown:
				GetEventParameter(event, kEventParamMouseButton, typeMouseButton, NULL, sizeof(EventMouseButton), NULL, &button);
				GetEventParameter(event, kEventParamWindowMouseLocation, typeHIPoint, NULL, sizeof(HIPoint), NULL, &location);	// Mac OS X v10.1 and later
				GetEventParameter(event, kEventParamKeyModifiers, typeUInt32, NULL, sizeof(UInt32), NULL, &modifiers);
				if ((button == kEventMouseButtonSecondary) || ((button == kEventMouseButtonPrimary) && (modifiers & controlKey))) { // pan
					if (gTrackball) { // if we are currently tracking, end trackball
						gTrackball = GL_FALSE;
						if (gTrackBallRotation[0] != 0.0)
							addToRotationTrackball (gTrackBallRotation, pContextInfo->worldRotation);
						gTrackBallRotation [0] = gTrackBallRotation [1] = gTrackBallRotation [2] = gTrackBallRotation [3] = 0.0f;
					} else if (gDolly) { // if we are currently dollying, end dolly
						gDolly = GL_FALSE;
					}
					gDollyPanStartPoint[0] = location.x;
					gDollyPanStartPoint[1] = location.y;
					gPan = GL_TRUE;
					gTrackingContextInfo = pContextInfo->aglContext;
				} else if ((button == kEventMouseButtonTertiary) || ((button == kEventMouseButtonPrimary) && (modifiers & optionKey))) { // dolly
					if (gTrackball) { // if we are currently tracking, end trackball
						gTrackball = GL_FALSE;
						if (gTrackBallRotation[0] != 0.0)
							addToRotationTrackball (gTrackBallRotation, pContextInfo->worldRotation);
						gTrackBallRotation [0] = gTrackBallRotation [1] = gTrackBallRotation [2] = gTrackBallRotation [3] = 0.0f;
					} else if (gPan) { // if we are currently panning, end pan
						gPan = GL_FALSE;
					}
					gDollyPanStartPoint[0] = location.x;
					gDollyPanStartPoint[1] = location.y;
					gDolly = GL_TRUE;
					gTrackingContextInfo = pContextInfo->aglContext;
				} else if (button == kEventMouseButtonPrimary)  { // trackball
					if (gDolly) { // if we are currently dollying, end dolly
						gDolly = GL_FALSE;
						gTrackingContextInfo = NULL;
					} else if (gPan) { // if we are currently panning, end pan
						gPan = GL_FALSE;
						gTrackingContextInfo = NULL;
					}
					startTrackball (location.x, location.y, 
					                pContextInfo->camera.viewOriginX, pContextInfo->camera.viewOriginY, 
									pContextInfo->camera.viewWidth, pContextInfo->camera.viewHeight);
					gTrackball = GL_TRUE;
					gTrackingContextInfo = pContextInfo->aglContext;
				} 
				break;
			// stop trackball, pan, or dolly
			case kEventMouseUp:
				if (gDolly) { // end dolly
					gDolly = GL_FALSE;
				} else if (gPan) { // end pan
					gPan = GL_FALSE;
				} else if (gTrackball) { // end trackball
					gTrackball = GL_FALSE;
					if (gTrackBallRotation[0] != 0.0)
						addToRotationTrackball (gTrackBallRotation, pContextInfo->worldRotation);
					gTrackBallRotation [0] = gTrackBallRotation [1] = gTrackBallRotation [2] = gTrackBallRotation [3] = 0.0f;
				} 
				gTrackingContextInfo = NULL;
				break;
			// trackball, pan, or dolly
			case kEventMouseDragged:
				GetEventParameter(event, kEventParamWindowMouseLocation, typeHIPoint, NULL, sizeof(HIPoint), NULL, &location);	// Mac OS X v10.1 and later
				if (gTrackball) {
					rollToTrackball (location.x, location.y, gTrackBallRotation);
					InvalWindowRect (window, &rectPort);
				} else if (gDolly) {
					mouseDolly (location, pContextInfo);
					InvalWindowRect (window, &rectPort);
				} else if (gPan) {
					mousePan (location, pContextInfo);
					InvalWindowRect (window, &rectPort);
				}
				break;
			// aperature change
			case kEventMouseWheelMoved: 
				GetEventParameter(event, kEventParamMouseWheelDelta, typeLongInteger, NULL, sizeof(long), NULL, &wheelDelta);
				if (wheelDelta)
				{
					GLfloat deltaAperture = wheelDelta * -pContextInfo->camera.aperture / 200.0f;
					pContextInfo->camera.aperture += deltaAperture;
					if (pContextInfo->camera.aperture < 0.1) // do not let aperture <= 0.1
						pContextInfo->camera.aperture = 0.1;
					if (pContextInfo->camera.aperture > 179.9) // do not let aperture >= 180
						pContextInfo->camera.aperture = 179.9;
					InvalWindowRect (window, &rectPort);
				}
				break;
		}
		result = noErr;
	}	
	return result;
}

// ---------------------------------

// key input handler
static OSStatus handleKeyInput (EventHandlerCallRef myHandler, EventRef event, Boolean keyDown, void* userData)
{
	enum { kH = 0x04,   // toggle draw help
		   kC = 0x08,   // toggle draw caps
		   kLfSqBracket = 0x21,   // color scheme down
		   kRtSqBracket = 0x1E,   // color scheme up
		   kSemi = 0x29,   // decrease subdivisions
		   kQuote = 0x27,   // increase subdivisions
		   kF = 0x03,   // toggle fill
		   kP = 0x23,   // toggle points
		   kW = 0x0D,   // toggle wire
		   kL = 0x25,   // next lighting model (wraps)
		   kBackSlash = 0x2A,   // next shape (wraps)
		   kQuestion = 0x2c,   // toggle credits
		   kM = 0x2E	// cycle multi-sample
	};
	WindowRef window = (WindowRef) userData;
	OSStatus result = eventNotHandledErr;

	result = CallNextEventHandler(myHandler, event);	
	if (eventNotHandledErr == result) { 
		pRecContext pContextInfo = GetCurrentContextInfo (window);
		if (pContextInfo) {
			UInt32 keyCode;
			Rect rectPort = {0,0,0,0};
			GetEventParameter (event, kEventParamKeyCode, typeUInt32, NULL, sizeof(UInt32), NULL, &keyCode);
#ifdef DEBUG
			sprintf (pContextInfo->message, "KeyCode: %lu (0x%lX)", keyCode, keyCode);
			pContextInfo->msgTime = getElapsedTime ();
#endif
			// handle keyboard input here
			if (keyDown) {
				switch (keyCode) {
					case kH:
						// toggle help
						pContextInfo->drawHelp = 1 - pContextInfo->drawHelp;
						GetWindowPortBounds (window, &rectPort);
						InvalWindowRect (window, &rectPort);
						result = noErr;
						break;
					case kC:
						// toggle caps
						pContextInfo->drawCaps = 1 - pContextInfo->drawCaps;
						GetWindowPortBounds (window, &rectPort);
						InvalWindowRect (window, &rectPort);
						result = noErr;
						break;
					case kQuestion: // credits
						pContextInfo->showCredits =  1 - pContextInfo->showCredits;
						GetWindowPortBounds (window, &rectPort);
						InvalWindowRect (window, &rectPort);
						break;
					case kF: // fill
						pContextInfo->polygons =  1;
						pContextInfo->lines = 0;
						pContextInfo->points = 0;
						GetWindowPortBounds (window, &rectPort);
						InvalWindowRect (window, &rectPort);
						break;
					case kW: // lines
						pContextInfo->polygons =  0;
						pContextInfo->lines = 1;
						pContextInfo->points = 0;
						GetWindowPortBounds (window, &rectPort);
						InvalWindowRect (window, &rectPort);
						break;
					case kP: // points
						pContextInfo->polygons =  0;
						pContextInfo->lines = 0;
						pContextInfo->points = 1;
						GetWindowPortBounds (window, &rectPort);
						InvalWindowRect (window, &rectPort);
						break;
					case kL: // lighting
						pContextInfo->lighting++;
						if (pContextInfo->lighting > 4)
							pContextInfo->lighting = 0;
						aglSetCurrentContext (pContextInfo->aglContext);
						SetLighting (pContextInfo->lighting);
						GetWindowPortBounds (window, &rectPort);
						InvalWindowRect (window, &rectPort);
						break;
					case kLfSqBracket: // next lower color scheme
						pContextInfo->colorScheme -= 1;
						if (pContextInfo->colorScheme < 0)
							pContextInfo->colorScheme = kColorSchemes - 1;
						aglSetCurrentContext (pContextInfo->aglContext);
						BuildGeometry (pContextInfo->surface, pContextInfo->colorScheme, pContextInfo->subdivisions, pContextInfo->xyRatio,
					                   &(pContextInfo->polyList), &(pContextInfo->lineList), &(pContextInfo->pointList));
						GetWindowPortBounds (window, &rectPort);
						InvalWindowRect (window, &rectPort);
						break;
					case kRtSqBracket: // next higher color scheme
						pContextInfo->colorScheme += 1;
						if (pContextInfo->colorScheme > kColorSchemes)
							pContextInfo->colorScheme = 0;
						aglSetCurrentContext (pContextInfo->aglContext);
						BuildGeometry (pContextInfo->surface, pContextInfo->colorScheme, pContextInfo->subdivisions, pContextInfo->xyRatio,
									   &(pContextInfo->polyList), &(pContextInfo->lineList), &(pContextInfo->pointList));
						GetWindowPortBounds (window, &rectPort);
						InvalWindowRect (window, &rectPort);
						break;
					case kSemi: // next lower subdivision setting
						pContextInfo->subdivisions = (GLuint) (pContextInfo->subdivisions / 1.414213);
						if (pContextInfo->subdivisions < 8)
							pContextInfo->subdivisions = 8;
						aglSetCurrentContext (pContextInfo->aglContext);
						BuildGeometry (pContextInfo->surface, pContextInfo->colorScheme, pContextInfo->subdivisions, pContextInfo->xyRatio,
					                   &(pContextInfo->polyList), &(pContextInfo->lineList), &(pContextInfo->pointList));
						GetWindowPortBounds (window, &rectPort);
						InvalWindowRect (window, &rectPort);
						break;
					case kQuote:
						pContextInfo->subdivisions = (GLuint) (pContextInfo->subdivisions * 1.414213);
						aglSetCurrentContext (pContextInfo->aglContext);
						BuildGeometry (pContextInfo->surface, pContextInfo->colorScheme, pContextInfo->subdivisions, pContextInfo->xyRatio,
					                   &(pContextInfo->polyList), &(pContextInfo->lineList), &(pContextInfo->pointList));
						GetWindowPortBounds (window, &rectPort);
						InvalWindowRect (window, &rectPort);
						break;
					case kBackSlash: // next higher surface
						pContextInfo->surface += 1;
						pContextInfo->surface %= kSurfaces;
						aglSetCurrentContext (pContextInfo->aglContext);
						BuildGeometry (pContextInfo->surface, pContextInfo->colorScheme, pContextInfo->subdivisions, pContextInfo->xyRatio,
					                   &(pContextInfo->polyList), &(pContextInfo->lineList), &(pContextInfo->pointList));
						GetWindowPortBounds (window, &rectPort);
						InvalWindowRect (window, &rectPort);
						break;
#if kUseMultiSample
					case kM: // multisample rotate
						pContextInfo->modeFSAA = (pContextInfo->modeFSAA + 1) % (kFSAANice + 1);
						switch (pContextInfo->modeFSAA) {
							case kFSAAOff:
								glDisable (GL_MULTISAMPLE_ARB);
								break;
							case kFSAAFast:
								glEnable (GL_MULTISAMPLE_ARB);
								glHint (GL_MULTISAMPLE_FILTER_HINT_NV, GL_FASTEST);
								break;
							case kFSAANice:
								glEnable (GL_MULTISAMPLE_ARB);
								glHint (GL_MULTISAMPLE_FILTER_HINT_NV, GL_NICEST);
								break;
						}
						GetWindowPortBounds (window, &rectPort);
						InvalWindowRect (window, &rectPort);
#endif
						break;
				}
			}
		}
	}
	return result;
}
	
// ---------------------------------

void createNewWindow (void)
{
    EventHandlerRef ref;
    EventTypeSpec list[] = { { kEventClassWindow, kEventWindowCollapsing },
							 { kEventClassWindow, kEventWindowShown },
                             { kEventClassWindow, kEventWindowActivated },
                             { kEventClassWindow, kEventWindowClose },
                             { kEventClassWindow, kEventWindowDrawContent },
                             { kEventClassWindow, kEventWindowBoundsChanged },
                             { kEventClassWindow, kEventWindowZoomed },
                             { kEventClassKeyboard, kEventRawKeyDown },
                             { kEventClassKeyboard, kEventRawKeyUp } };

    WindowRef window = NULL;
	pRecContext pContextInfo = (pRecContext) NewPtrClear (sizeof (recContext)); // memory for window record
	initialConditions (pContextInfo);
	CreateWindowFromNib (nibRef, CFSTR("MainWindow"), &window); // build window
	if (window)
	{
		SetWRefCon (window, (long) pContextInfo); // point to the window record in the ref con of the window
		InstallWindowEventHandler (window, gWinEvtHandler, GetEventTypeCount (list), list, (void*)window, &ref); // add event handler
		ShowWindow (window);
		if (pContextInfo->animate && !pContextInfo->timer) {
			pContextInfo->time = UpTime ();
			InstallEventLoopTimer (GetCurrentEventLoop(), 0, 0.01, getTimerUPP (), (void *) window, &pContextInfo->timer);
		}
	}
}
	
// ---------------------------------

// window event handler
static pascal OSStatus windowEvtHndlr (EventHandlerCallRef myHandler, EventRef event, void* userData)
{
#pragma unused (userData)
	WindowRef			window = NULL;
    pRecContext			pContextInfo = NULL;
	Rect				rectPort = {0,0,0,0};
	CGRect 				viewRect = {{0.0f, 0.0f}, {0.0f, 0.0f}};
    OSStatus			result = eventNotHandledErr;
    UInt32 				class = GetEventClass (event);
    UInt32 				kind = GetEventKind (event);

	switch (class) {
		case kEventClassKeyboard:
			switch (kind) {
				case kEventRawKeyDown:
					result = handleKeyInput  (myHandler, event, true, userData);
					break;
				case kEventRawKeyUp:
					result = handleKeyInput  (myHandler, event, false, userData);
					break;
			}
			break;
		case kEventClassWindow:
			GetEventParameter(event, kEventParamDirectObject, typeWindowRef, NULL, sizeof(WindowRef), NULL, &window);
			switch (kind) {
				case kEventWindowCollapsing:
					pContextInfo = GetCurrentContextInfo (window);
					GetWindowPortBounds (window, &rectPort);
					drawGL (window, false); // in this case just draw content
					CompositeGLBufferIntoWindow( pContextInfo->aglContext, &rectPort, window);
					result = UpdateCollapsedWindowDockTile (window);
					break;
				case kEventWindowActivated: // called on click activation and initially
					// handle any differences betwen activate and not here
					break;
				case kEventWindowDrawContent: // will receive one prior to being shown...
					drawGL (window, true); // in this case just draw content
					break;
				case kEventWindowClose: // called when window is being closed (close box)
					HideWindow (window);
					pContextInfo = GetCurrentContextInfo (window);
					disposeGL (pContextInfo);
					DisposePtr ((Ptr) pContextInfo);
        			SetWRefCon (window, NULL);
					break;
				case kEventWindowShown: // called on initial show (not on un-minimize)
					// draw content is received prior to this so if you handle that then not drawing required here
					if (window == FrontWindow ())
						SetUserFocusWindow (window);
					break;
				case kEventWindowBoundsChanged: // called for resize and moves (drag)
					pContextInfo = GetCurrentContextInfo (window);	
					if (pContextInfo) { // if we have a vlaid context
						GetWindowPortBounds (window, &rectPort); // get the bounds and set the view rect
						viewRect.size.width = (float) (rectPort.right - rectPort.left);
						viewRect.size.height = (float) (rectPort.bottom - rectPort.top);
						if ((viewRect.size.height < pContextInfo->camera.viewHeight) || // if we are shrinking the window
						    (viewRect.size.width < pContextInfo->camera.viewWidth)) {
							// resize prior to update
							resizeGL (pContextInfo->aglContext, &pContextInfo->camera, pContextInfo->shapeSize, viewRect); // forces updateContext & viewport update if required
							drawGL (window, true);// force immediate update to get live resize on shrink
						} else {
							// resize to update context virtual screen
							resizeGL (pContextInfo->aglContext, &pContextInfo->camera, pContextInfo->shapeSize, viewRect); // forces updateContext & viewport update if required
							if (pContextInfo->currentVS != aglGetVirtualScreen (pContextInfo->aglContext)) { // if virtual screen (i.e. renderer) changes
								pContextInfo->currentVS = aglGetVirtualScreen (pContextInfo->aglContext); // sync renderer
								drawGL (window, true); // force immediate update to get update on screen (renderer) switch
							}
						}
					}
					break;
				case kEventWindowZoomed: // called when user clicks on zoom button (occurs after the window has been zoomed)
					// use this if you need to some special here as you always get a kEventWindowBoundsChanged event
					break;
			}
			break;
	}
    return result;
}

// ---------------------------------

// application level event handler
static pascal OSStatus appEvtHndlr (EventHandlerCallRef myHandler, EventRef event, void* userData)
{
#pragma unused (myHandler, userData)
    OSStatus result = eventNotHandledErr;
    Rect rectPort;
    pRecContext pContextInfo = NULL;
    WindowRef window = FrontWindow ();
    UInt32 class = GetEventClass (event);
    UInt32 kind = GetEventKind (event);
    HICommand command;

    if (window) { // do we have a window?
        GetWindowPortBounds (window, &rectPort); // get bounds for later inval
        pContextInfo = GetCurrentContextInfo (window);
    }
	
	switch (class) {
		case kEventClassMouse:
			handleWindowMouseEvents (myHandler, event);
			break;
		case kEventClassCommand:
			switch (kind) {
				case kEventProcessCommand:
					GetEventParameter (event, kEventParamDirectObject, kEventParamHICommand, NULL, sizeof(command), NULL, &command); // get command
					switch (command.commandID) {
						case kHICommandNew:
							createNewWindow ();
							result = noErr;
							break;
						case kHICommandClose:
							if (window) {
								HideWindow (window);
								disposeGL (GetCurrentContextInfo(window));  // dump our structures
								DisposeWindow (window); // if it exists dump it
							}
							break;
						case kHICommandQuit:
							//memory will get dumped on exit, other callbacks/timers are for this process only
							break;
						case 'gogo':
							if (pContextInfo) {  // have window info
								pContextInfo->animate = 1 - pContextInfo->animate;
								if (pContextInfo->animate && !pContextInfo->timer) {
									pContextInfo->time = UpTime ();
									InstallEventLoopTimer (GetCurrentEventLoop(), 0, 0.001, getTimerUPP (), (void *) window, &pContextInfo->timer);
								} else if (!pContextInfo->animate && pContextInfo->timer) {
									RemoveEventLoopTimer(pContextInfo->timer);
									pContextInfo->timer = NULL;
								}
							}
							break;
						case 'info':
							if (pContextInfo) {  // have window info
								pContextInfo->info = 1 - pContextInfo->info;
								GetWindowPortBounds (window, &rectPort);
								InvalWindowRect (window, &rectPort);
							}
							break;
					}
					break;
				case kEventCommandUpdateStatus: // menu updates
					GetEventParameter (event, kEventParamDirectObject, kEventParamHICommand, NULL, sizeof(command), NULL, &command); // get command
					switch (command.commandID) {
						case kHICommandClose:
							if (pContextInfo)
								EnableMenuItem (GetMenuHandle (kMainMenu), kCloseMenuItem);
							else 
								DisableMenuItem (GetMenuHandle (kMainMenu), kCloseMenuItem);
							break;
						case 'gogo':
							if (pContextInfo) {
								EnableMenuItem (GetMenuHandle (kMainMenu), kAnimateMenuItem);
								CheckMenuItem (GetMenuHandle (kMainMenu), kAnimateMenuItem, pContextInfo->animate);
							} else { 
								DisableMenuItem (GetMenuHandle (kMainMenu), kAnimateMenuItem);
								CheckMenuItem (GetMenuHandle (kMainMenu), kAnimateMenuItem, kAnimateState);
							}
							break;
						case 'info':
							if (pContextInfo) {
								EnableMenuItem (GetMenuHandle (kMainMenu), kInfoMenuItem);
								CheckMenuItem (GetMenuHandle (kMainMenu), kInfoMenuItem, pContextInfo->info);
							} else { 
								DisableMenuItem (GetMenuHandle (kMainMenu), kInfoMenuItem);
								CheckMenuItem (GetMenuHandle (kMainMenu), kInfoMenuItem, kInfoState);
							}
							break;
					}
					result = noErr;
					break;
			}
			break;
	}
    return result;
}

#pragma mark ==== main ====

int main(int argc, char* argv[])
{
    OSStatus		err;
    EventHandlerRef	ref;
    EventTypeSpec	list[] = { { kEventClassCommand,  kEventProcessCommand },
							   { kEventClassCommand,  kEventCommandUpdateStatus },
							   { kEventClassMouse, kEventMouseDown },// handle trackball functionality globaly because there is only a single user
							   { kEventClassMouse, kEventMouseUp }, 
							   { kEventClassMouse, kEventMouseDragged },
							   { kEventClassMouse, kEventMouseWheelMoved } };
	ProcessSerialNumber psn = { 0, kCurrentProcess };
	EventRef windowEvent;

	gStartTime = UpTime (); // get app start time
	
    // Create a Nib reference passing the name of the nib file (without the .nib extension)
    // CreateNibReference only searches into the application bundle.
    err = CreateNibReference(CFSTR("main"), &nibRef);
    require_noerr( err, CantGetNibRef );
    err = SetMenuBarFromNib(nibRef, CFSTR("MainMenu"));
    require_noerr( err, CantSetMenuBar );
	gEvtHandler = NewEventHandlerUPP(appEvtHndlr);
    err = InstallApplicationEventHandler (gEvtHandler, GetEventTypeCount (list), list, 0, &ref);
	gWinEvtHandler = NewEventHandlerUPP(windowEvtHndlr); 

	getCurrentCaps ();

	// ensure we know when display configs are changed
	gConfigEDMUPP = NewDMExtendedNotificationUPP (handleConfigDMEvent); // for display change notification
	DMRegisterExtendedNotifyProc (gConfigEDMUPP, NULL, NULL, &psn);
	
	StartHIDInput ();
	
    if (err == noErr) { // post new window event
	   err = MacCreateEvent (nil, kEventClassCommand, kEventProcessCommand, 0, kEventAttributeNone, &windowEvent);
		if (err == noErr) {
			HICommand command; // set HI command parameter...
			command.commandID = kHICommandNew; command.attributes = 0; 
			command.menu.menuRef = 0; command.menu.menuItemIndex = 0;
			err = SetEventParameter(windowEvent, kEventParamDirectObject, kEventParamHICommand, sizeof(command), &command);
			if (err == noErr)
				err = PostEventToQueue (GetCurrentEventQueue(), windowEvent, kEventPriorityHigh);
			ReleaseEvent (windowEvent);
		}
	}

    // Call the event loop
    RunApplicationEventLoop();
	
	// Exiting...

	if (gConfigEDMUPP) { // dispose UPP for DM notifications
		DisposeDMExtendedNotificationUPP (gConfigEDMUPP);
		gConfigEDMUPP = NULL;
	}

	if (gDisplayCaps) { // dispose diaplsy capabilities info
		free (gDisplayCaps);
		gDisplayCaps = NULL;
	}

//CantCreateWindow:
CantSetMenuBar:
CantGetNibRef:

    // We don't need the nib reference anymore.
    DisposeNibReference (nibRef);

	return err;
}

