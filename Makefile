# Header file for OGVCore test thingy

.FAKE : all clean


all : ogvcoretest

clean :
	rm -f libskeleton.so
	rm -f ogvcoretest


# ogvcoretest

CFLAGS=-std=c++11 `pkg-config --cflags ogg vorbis theora` -Ilibskeleton/include
LDFLAGS=`pkg-config --libs ogg vorbis theora`

SOURCES=OGVCoreTest.cpp \
        OGVCoreDecoder.cpp

HEADERS=OGVCore.hpp

ogvcoretest : $(SOURCES) $(HEADERS) libskeleton.so
	c++ $(CFLAGS) $(SOURCES) libskeleton.so -o ogvcoretest $(LDFLAGS)



# libskeleton

SKELETON_CFLAGS=-Ilibskeleton/include
SKELETON_SOURCES=libskeleton/src/skeleton.c \
                 libskeleton/src/skeleton_query.c \
                 libskeleton/src/skeleton_vector.c

libskeleton.so : $(SKELETON_SOURCE)
	cc --shared $(SKELETON_CFLAGS) $(SKELETON_SOURCES) -o libskeleton.so
