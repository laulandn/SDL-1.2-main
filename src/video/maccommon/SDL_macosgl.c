/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 1997-2012 Sam Lantinga

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

    Sam Lantinga
    slouken@libsdl.org
*/
#include "SDL_config.h"


#ifdef SDL_VIDEO_OSGL


/* OS implementation of SDL OpenGL support */

#include "SDL_lowvideo.h"
#include "SDL_macgl_c.h"
#include "SDL_loadso.h"


#ifdef SDL_MACOSCLASSIC_TINYGL
#include "GL/ososgl.h"
#else
#include <GL/osmesa.h>
#endif


#ifdef MESA
#undef MESA
#endif

/* This supports both real Apple dynamic OpenGL, and static Mesa */

//#ifdef __POWERPC__
#define OPENGL_IS_DYNAMIC 1
//#endif


typedef struct OSGLPixelFormatRec
{
  int fake;
} OSGLPixelFormatRec;

#ifdef SDL_MACOSCLASSIC_TINYGL
typedef ostgl_context_t * OSGLContext;
#else
typedef OSMesaContext OSGLContext;
#endif


static struct OSGLPixelFormatRec osPixelFormat;
typedef void * OSGLPixelFormat;
static OSGLPixelFormat osPixelFormatPtr=(OSGLPixelFormat)&osPixelFormat;


static int osError=0;
char *osBuffer=NULL;
OSGLContext osglContext=NULL;


OSGLContext osglCreateContext(OSGLPixelFormat pix, OSGLContext share)
{
/*
  int winSizeX=640,winSizeY=480;
  SDL_GetWindowSize(theOnlyWindow,&winSizeX,&winSizeY);
#ifdef SDL_MACOSCLASSIC_TINYGL
  	osglContext = ostgl_create_context(winSizeX,winSizeY,myDepth);
	ostgl_make_current(osglContext);
	osBuffer=osglContext->pixels;
#else
    int format=OSMESA_RGB;
    if(myDepth==32) format=OSMESA_ARGB;
    int factor=2;
    if(myDepth==32) factor=4;
    fprintf(stderr,"myDepth is %d\n",myDepth); fflush(stderr);
    // TODO: The context is obviously not QUITE right...
  	osglContext=OSMesaCreateContext(format,NULL);
  	osBuffer=(char *)malloc(winSizeX*winSizeY*factor);
    if(!osBuffer) {
      fprintf(stderr,"osBuffer failed!\n"); fflush(stderr); osError=1;
      return NULL;
    }  
    else { fprintf(stderr,"got osBuffer\n"); fflush(stderr); } 
    GLboolean res=OSMesaMakeCurrent(osglContext,osBuffer,GL_UNSIGNED_BYTE,winSizeX,winSizeY);
    if(!res) {
      fprintf(stderr,"OSMesaMakeCurrent failed!\n"); fflush(stderr); osError=1;
      return NULL;
  }  
  else { fprintf(stderr,"got OSMesaMakeCurrent\n"); fflush(stderr); } 
#endif
  if(!osglContext) {
    fprintf(stderr,"osglContext failed!\n"); fflush(stderr); osError=1;
    return NULL;
  }
  else { fprintf(stderr,"got osglContext\n"); fflush(stderr); } 
  renderer = SDL_CreateRenderer(theOnlyWindow, -1, SDL_RENDERER_PRESENTVSYNC);	
  if(!renderer) {
    fprintf(stderr,"SDL_CreateRenderer failed!\n"); fflush(stderr); osError=1;
    return NULL;
  }  
  else { fprintf(stderr,"got renderer\n"); fflush(stderr); } 
  texture = SDL_CreateTexture(renderer, SDL_GetWindowPixelFormat(theOnlyWindow), SDL_TEXTUREACCESS_STREAMING, winSizeX,winSizeY);
  if(!texture) {
    fprintf(stderr,"SDL_CreateTexture failed!\n"); fflush(stderr); osError=1;
    return NULL;
  }  
  else { fprintf(stderr,"got texture\n"); fflush(stderr); } 
*/
  fprintf(stderr,"osglCreateContext done\n"); fflush(stderr);
  return (OSGLContext)osglContext;
}


GLboolean osglSwapBuffers(OSGLContext ctx)
{
/*
    SDL_Surface *src=NULL;
    SDL_Surface *dst=NULL;
    int winSizeX=640,winSizeY=480;
#ifdef SDL_MACOSCLASSIC_TINYGL
    winSizeX=((ostgl_context_t *)ctx)->width;
    winSizeY=((ostgl_context_t *)ctx)->height;
    myDepth=((ostgl_context_t *)ctx)->depth;
#else
    SDL_GetWindowSize(theOnlyWindow,&winSizeX,&winSizeY);
#endif
	src = SDL_CreateRGBSurfaceWithFormatFrom(osBuffer, winSizeX, winSizeY, myDepth, (winSizeX * myDepth / 8), SDL_PIXELFORMAT_RGB565);
	dst = SDL_CreateRGBSurfaceWithFormatFrom(NULL, winSizeX, winSizeY, 0, 0, SDL_GetWindowPixelFormat(theOnlyWindow));
	if (SDL_LockTexture(texture, NULL, &dst->pixels, &dst->pitch) == 0)
	{
		if (SDL_BlitSurface(src, NULL, dst, NULL) != 0)
		{ SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "surface blit failed: %s", SDL_GetError()); }
		SDL_UnlockTexture(texture);
	}
	else
	{ SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "texture lock failed: %s", SDL_GetError()); }
	if(dst) SDL_FreeSurface(dst);
	if(src) SDL_FreeSurface(src);
	SDL_RenderClear(renderer);
	SDL_RenderCopy(renderer, texture, NULL, NULL);
	SDL_RenderPresent(renderer);
*/
  return true;
}


GLenum osglGetError()
{
  return osError;
}


struct OSGLpixelfmtRec *osglChoosePixelFormat(void *gdevs, GLint ndev,  GLint *v)
{
  // We don't choose anything, there's only a single static one
  return (struct OSGLpixelfmtRec *)osPixelFormatPtr;
}


void osglDestroyPixelFormat(OSGLPixelFormat pix)
{
  // We don't destroy anything, there's only a single static one
}


GLboolean osglSetDrawable(OSGLContext ctx, AGLDrawable draw)
{
  //fprintf(stderr,"fake osglSetDrawable!\n"); fflush(stderr);
  return true;
}


GLboolean osglSetCurrentContext(OSGLContext ctx)
{
  //fprintf(stderr,"fake osglSetCurrentContext!\n"); fflush(stderr);
  return true;
}


GLboolean osglDestroyContext(OSGLContext ctx)
{
#ifdef SDL_MACOSCLASSIC_TINYGL
  ostgl_delete_context((ostgl_context_t *)ctx);
#else
#endif
  return true;
}


OSGLContext osglGetCurrentContext(void)
{
  return osglContext;
}


GLboolean osglUpdateContext(OSGLContext ctx)
{
  //fprintf(stderr,"fake osglUpdateContext!\n"); fflush(stderr);
  return true;
}


GLboolean osglSetInteger(OSGLContext ctx, GLenum pname,const GLint *params)
{
  fprintf(stderr,"fake osglSetInteger %d %d!\n",pname,*params); fflush(stderr);
  return true;
}


const GLubyte * osglErrorString(GLenum code)
{
  if(osError==1) return (GLubyte *)"Problem getting something!";
  else return (GLubyte *)"Who knows";
}


GLboolean osglDescribePixelFormat(OSGLPixelFormatRec *fmt,GLint what,GLint *value)
{
  // This is all fake, but plausible
  switch(what) {
    case AGL_RGBA:
      *value=1;
      return true;
      break;
    case AGL_ACCELERATED:
      *value=true;
      return true;
      break;
    case AGL_RENDERER_ID:
      *value=1;
      return true;
      break;
    case AGL_DEPTH_SIZE:
      *value=20;  // Random
      return true;
      break;
    case AGL_STENCIL_SIZE:
      *value=20;  // Random
      return true;
      break;
    case AGL_DOUBLEBUFFER:
      *value=true;
      return true;
      break;
    case AGL_PIXEL_SIZE:
      *value=32;  // This should be taken from actual display
      return true;
      break;
    default:
      fprintf(stderr,"osglDescribePixelFormat...what=%d implemented\n",what); fflush(stderr);
      return false;
      break;
  }
  return false;
}


/* krat: adding OpenGL support */
int Mac_GL_Init(_THIS)
{
#if SDL_VIDEO_OPENGL
#ifdef MESA
    OSPixelFmt format;
#else
	OSGLPixelFormat format;
#endif
   	int i = 0;
	GLint attributes [ 26 ]; /* 26 is max possible in this setup */
	GLboolean noerr;
   
	/* load the gl driver from a default path */
	if ( ! this->gl_config.driver_loaded ) {
		/* no driver has been loaded, use default (ourselves) */
		if ( Mac_GL_LoadLibrary(this, NULL) < 0 ) {
			return(-1);
		}
	}

	attributes[i++] = AGL_RGBA;
	if ( this->gl_config.red_size   != 0 &&
	     this->gl_config.blue_size  != 0 &&
	     this->gl_config.green_size != 0 ) {
		attributes[i++] = AGL_RED_SIZE;
		attributes[i++] = this->gl_config.red_size;
		attributes[i++] = AGL_GREEN_SIZE;
		attributes[i++] = this->gl_config.green_size;
		attributes[i++] = AGL_BLUE_SIZE;
		attributes[i++] = this->gl_config.blue_size;
		attributes[i++] = AGL_ALPHA_SIZE;
		attributes[i++] = this->gl_config.alpha_size;
	}
	if ( this->gl_config.double_buffer ) {
		attributes[i++] = AGL_DOUBLEBUFFER;
	}
	if ( this->gl_config.depth_size != 0 ) {
		attributes[i++] = AGL_DEPTH_SIZE;
		attributes[i++] = this->gl_config.depth_size;
	}	
	if ( this->gl_config.stencil_size != 0 ) {
		attributes[i++] = AGL_STENCIL_SIZE;
		attributes[i++] = this->gl_config.stencil_size;
	}
	if ( this->gl_config.accum_red_size   != 0 &&
	     this->gl_config.accum_blue_size  != 0 &&
	     this->gl_config.accum_green_size != 0 ) {
		
		attributes[i++] = AGL_ACCUM_RED_SIZE;
		attributes[i++] = this->gl_config.accum_red_size;
		attributes[i++] = AGL_ACCUM_GREEN_SIZE;
		attributes[i++] = this->gl_config.accum_green_size;
		attributes[i++] = AGL_ACCUM_BLUE_SIZE;
		attributes[i++] = this->gl_config.accum_blue_size;
		attributes[i++] = AGL_ACCUM_ALPHA_SIZE;
		attributes[i++] = this->gl_config.accum_alpha_size;
	}
	if ( this->gl_config.stereo ) {
		attributes[i++] = AGL_STEREO;
	}
#if defined(AGL_SAMPLE_BUFFERS_ARB) && defined(AGL_SAMPLES_ARB)
	if ( this->gl_config.multisamplebuffers != 0 ) {
		attributes[i++] = AGL_SAMPLE_BUFFERS_ARB;
		attributes[i++] = this->gl_config.multisamplebuffers;
	}	
	if ( this->gl_config.multisamplesamples != 0 ) {
		attributes[i++] = AGL_SAMPLES_ARB;
		attributes[i++] = this->gl_config.multisamplesamples;
	}	
#endif
	if ( this->gl_config.accelerated > 0 ) {
#ifdef MESA
#else
		attributes[i++] = AGL_ACCELERATED;
		attributes[i++] = AGL_NO_RECOVERY;
#endif
	}

#ifdef MESA
#else
	attributes[i++] = AGL_ALL_RENDERERS;
#endif
	attributes[i]	= AGL_NONE;

	format =
#ifdef MESA
    (OSPixelFmt *)
#endif
	osglChoosePixelFormat(NULL, 0, attributes);
	if ( format == NULL ) {
		SDL_SetError("Couldn't match OpenGL desired format");
		return(-1);
	}

	glContext = (AGLContext)osglCreateContext(format, NULL);
	if ( glContext == NULL ) {
		SDL_SetError("Couldn't create OpenGL context");
		return(-1);
	}
	osglDestroyPixelFormat(format);

    #if  TARGET_API_MAC_CARBON
	noerr = osglSetDrawable((OSGLContext)glContext, GetWindowPort(SDL_Window));
    #else
	noerr = osglSetDrawable((OSGLContext)glContext, (AGLDrawable)SDL_Window);
    #endif
    
	if(!noerr) {
		SDL_SetError("Unable to bind GL context to window");
		return(-1);
	}
	return(0);
#else
	SDL_SetError("OpenGL support not configured");
	return(-1);
#endif
}

void Mac_GL_Quit(_THIS)
{
#if SDL_VIDEO_OPENGL
	if ( glContext != NULL ) {
		osglSetCurrentContext(NULL);
		osglSetDrawable((OSGLContext)glContext, NULL);
		osglDestroyContext((OSGLContext)glContext);		
		glContext = NULL;
	}
#endif
}

#if SDL_VIDEO_OPENGL

/* Make the current context active */
int Mac_GL_MakeCurrent(_THIS)
{
	int retval;

	retval = 0;
	if( ! osglSetCurrentContext((OSGLContext)glContext) ) {
		SDL_SetError("Unable to make GL context current");
		retval = -1;
	}
	return(retval);
}

void Mac_GL_SwapBuffers(_THIS)
{
	osglSwapBuffers(
#ifdef MESA
#else
	(OSGLContext)glContext
#endif
	);
}

int Mac_GL_LoadLibrary(_THIS, const char *location)
{
	if (location == NULL)
#if __MACH__
		location = "/System/Library/Frameworks/OpenGL.framework/OpenGL";
#else
		location = "OpenGLLibrary";
#endif

#ifdef OPENGL_IS_DYNAMIC
	this->hidden->libraryHandle = SDL_LoadObject(location);
#endif

	this->gl_config.driver_loaded = 1;
#ifdef OPENGL_IS_DYNAMIC
	return (this->hidden->libraryHandle != NULL) ? 0 : -1;
#else
    return 0;
#endif
}

void Mac_GL_UnloadLibrary(_THIS)
{
#ifdef OPENGL_IS_DYNAMIC
	SDL_UnloadObject(this->hidden->libraryHandle);
#endif

	this->hidden->libraryHandle = NULL;
	this->gl_config.driver_loaded = 0;
}

void* Mac_GL_GetProcAddress(_THIS, const char *proc)
{
#ifdef OPENGL_IS_DYNAMIC
	return SDL_LoadFunction( this->hidden->libraryHandle, proc );
#else
    void *symbol=NULL;
    /* There's a better way to do this...but this works for now... */
    if(!strcmp("glBegin",proc)) symbol=(void *)glBegin;
    if(!strcmp("glBindTexture",proc)) symbol=(void *)glBindTexture;
    if(!strcmp("glClear",proc)) symbol=(void *)glClear;
    if(!strcmp("glClearColor",proc)) symbol=(void *)glClear;
    if(!strcmp("glColor3fv",proc)) symbol=(void *)glColor3fv;
    if(!strcmp("glColor4f",proc)) symbol=(void *)glColor4f;
    if(!strcmp("glColorPointer",proc)) symbol=(void *)glColorPointer;
    if(!strcmp("glDeleteTextures",proc)) symbol=(void *)glDeleteTextures;
    if(!strcmp("glDisable",proc)) symbol=(void *)glDisable;
    if(!strcmp("glDisableClientState",proc)) symbol=(void *)glDisableClientState;
    if(!strcmp("glDrawArrays",proc)) symbol=(void *)glDrawArrays;
    if(!strcmp("glDrawPixels",proc)) symbol=(void *)glDrawPixels;
    if(!strcmp("glEnable",proc)) symbol=(void *)glEnable;
    if(!strcmp("glEnableClientState",proc)) symbol=(void *)glEnableClientState;
    if(!strcmp("glEnd",proc)) symbol=(void *)glEnd;
    if(!strcmp("glGenTextures",proc)) symbol=(void *)glGenTextures;
    if(!strcmp("glGetError",proc)) symbol=(void *)glGetError;
    if(!strcmp("glGetFloatv",proc)) symbol=(void *)glGetFloatv;
    if(!strcmp("glGetIntegerv",proc)) symbol=(void *)glGetIntegerv;
    if(!strcmp("glGetString",proc)) symbol=(void *)glGetString;
    if(!strcmp("glLoadIdentity",proc)) symbol=(void *)glLoadIdentity;
    if(!strcmp("glMatrixMode",proc)) symbol=(void *)glMatrixMode;
    if(!strcmp("glPointSize",proc)) symbol=(void *)glPointSize;
    if(!strcmp("glReadBuffer",proc)) symbol=(void *)glReadBuffer;
    if(!strcmp("glReadPixels",proc)) symbol=(void *)glReadPixels;
    if(!strcmp("glRectf",proc)) symbol=(void *)glRectf;
    if(!strcmp("glRotatef",proc)) symbol=(void *)glRotatef;
    if(!strcmp("glShadeModel",proc)) symbol=(void *)glShadeModel;
    if(!strcmp("glTexCoord2f",proc)) symbol=(void *)glTexCoord2f;
    if(!strcmp("glTexCoordPointer",proc)) symbol=(void *)glTexCoordPointer;
    if(!strcmp("glTexParameteri",proc)) symbol=(void *)glTexParameteri;
    if(!strcmp("glVertex2f",proc)) symbol=(void *)glVertex2f;
    if(!strcmp("glVertex3fv",proc)) symbol=(void *)glVertex3fv;
    if(!strcmp("glVertexPointer",proc)) symbol=(void *)glVertexPointer;
    if(!strcmp("glViewport",proc)) symbol=(void *)glViewport;
    //
    if(!strcmp("glBlendFuncSeparate",proc)) symbol=(void *)glBlendFuncSeparate;
    if(!strcmp("glBlendEquation",proc)) symbol=(void *)glBlendEquation;
    //
    if(!strcmp("glOrtho",proc)) symbol=(void *)glOrtho;
    if(!strcmp("glTexEnvf",proc)) symbol=(void *)glTexEnvf;
    if(!strcmp("glDepthFunc",proc)) symbol=(void *)glDepthFunc;
    if(!strcmp("glScissor",proc)) symbol=(void *)glScissor;
    if(!strcmp("glTexSubImage2D",proc)) symbol=(void *)glTexSubImage2D;
    if(!strcmp("glGetPointerv",proc)) symbol=(void *)glGetPointerv;
    if(!strcmp("glLineWidth",proc)) symbol=(void *)glLineWidth;
    if(!strcmp("glTexImage2D",proc)) symbol=(void *)glTexImage2D;
    if(!strcmp("glRasterPos2i",proc)) symbol=(void *)glRasterPos2i;
    if(!strcmp("glPixelStorei",proc)) symbol=(void *)glPixelStorei;
    if(!strcmp("glColor4ub",proc)) symbol=(void *)glColor4ub;
    //
    if(!symbol) {
    /*
#ifdef MAC_DEBUG
      fprintf(stderr,"Returning fake pointer for %s...this will cause a crash when called!\n",proc); fflush(stderr);
#endif
      symbol=(void *)0x8000;
      */
      fprintf(stderr,"Func for %s...does not exist\n",proc); fflush(stderr);
      SDL_Quit();
      exit(5);
    }
#ifdef MAC_DEBUG
      fprintf(stderr,"Returning 0x%x\n",(int)symbol); fflush(stderr);
#endif
  return symbol;
#endif
}

#endif /* SDL_VIDEO_OPENGL */


#endif