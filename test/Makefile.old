PATH_TO_RETRO68=$(HOME)/Retro68-build/toolchain/powerpc-apple-macos

.SUFFIXES:	.xcoff


TARGETS = checkkeys.xcoff graywin.xcoff loopwave.xcoff testalpha.xcoff testbitmap.xcoff \
testblitspeed.xcoff testcdrom.xcoff testcursor.xcoff testdyngl.xcoff \
testerror.xcoff testfile.xcoff testgamma.xcoff testgl.xcoff testthread.xcoff \
testiconv.xcoff testjoystick.xcoff testkeys.xcoff testlock.xcoff \
testoverlay2.xcoff testoverlay.xcoff testpalette.xcoff testplatform.xcoff \
testsem.xcoff testsprite.xcoff testtimer.xcoff testver.xcoff testvidinfo.xcoff \
testwin.xcoff testwm.xcoff threadwin.xcoff torturethread.xcoff testloadso.xcoff

OBJS=$(subst .c,.o,$(wildcard *.c))



BINPATH = .
INCPATH+= -I../include -I$(PATH_TO_RETRO68)/include/GL
CFLAGS+= $(INCPATH) -D__MACOSCLASSIC__ -DHAVE_OPENGL=1

LIBPATH = -L..
LIBS = -lSDL -lm -lInputSprocketLib -lDrawSprocketLib -lOpenGLLibrary -lOpenGLUtility -lThreadsLib
#LIBS = -lSDL -lm -lInputSprocketLib -lDrawSprocketLib 
#LIBS+=-L$(HOME)/tinygl/lib -lTinyGL
LIBF = SDLmain.lib

CC=powerpc-apple-macos-gcc
CXX=powerpc-apple-macos-g++


.o.xcoff:
	$(CXX) -o $@ $< ../obj/SDL_main.c.o $(LIBPATH) $(LIBS)
	mv $@ $*
	../xcoff2app.sh $*
	
.c.o:
	$(CC) -o $@ -c $< $(CFLAGS)


all: $(TARGETS)

clean:
	rm -rf *.bin *.APPL *.dsk *.pef *.PPC *.PPC.xSYM *.gdb
	rm -rf checkkeys graywin loopwave testalpha testbitmap \
testblitspeed testcdrom testcursor testdyngl \
testerror testfile testgamma testgl testthread \
testiconv testjoystick testkeys testlock \
testoverlay2 testoverlay testpalette testplatform \
testsem testsprite testtimer testver testvidinfo \
testwin testwm threadwin torturethread testloadso

distclean:
