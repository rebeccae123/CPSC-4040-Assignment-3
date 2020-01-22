#specify your compiler
CC      = g++

# auxiliary flags
CFLAGS	= -g

#first set up the platform dependent variables
ifeq ("$(shell uname)", "Darwin")
  ALDFLAGS     = -framework Foundation -lOpenImageIO -lm
else
  ifeq ("$(shell uname)", "Linux")
    ALDFLAGS     = -L /usr/lib64/ -lOpenImageIO -lm
  endif
endif

#compose
ifeq ("$(shell uname)", "Darwin")
  CLDFLAGS     = -framework Foundation -framework GLUT -framework OpenGL -lOpenImageIO -lm
else
  ifeq ("$(shell uname)", "Linux")
    CLDFLAGS     = -L /usr/lib64/ -lglut -lGL -lGLU -lOpenImageIO -lm
  endif
endif

#this does the linking step
all: alphamask compose
alphamask : alphamask.o
	${CC} ${CFLAGS} -o alphamask alphamask.o ${ALDFLAGS}
compose : compose.o
	${CC} ${CFLAGS} -o compose compose.o ${CLDFLAGS}

#this generically compiles each .cpp to a .o file
%.o: %.cpp
	${CC} -c ${CFLAGS} $<

#it does not check for .h files dependencies, but you could add that, e.g.
#somfile.o    : somefile.cpp someheader.h
#	${CC} ${CFLAGS} -c somefile.cpp


#this will clean up all temporary files created by make all
clean:
	rm -f core.* *.o *~ alphamask
	rm -f core.* *.o *~ compose
