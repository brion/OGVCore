# Header file for OGVCore test thingy

.FAKE : all clean


all : ogvcoretest

clean :
	rm -f libskeleton.so
	rm -f ogvcoretest


# ogvcoretest

CFLAGS=-std=c++11 `pkg-config --cflags ogg vorbis theora` -Ilibskeleton/include -Iinclude
LDFLAGS=`pkg-config --libs ogg vorbis theora`

SOURCES=src/testmain.cpp \
        src/OGVCore/Decoder.cpp \
        src/OGVCore/Player.cpp

PRIVATE_HEADERS=src/OGVCore/Bisector.h

PUBLIC_HEADERS=include/OGVCore.h

ogvcoretest : $(SOURCES) $(PRIVATE_HEADERS) $(PUBLIC_HEADERS) libskeleton.so
	c++ $(CFLAGS) $(SOURCES) libskeleton.so -o ogvcoretest $(LDFLAGS)



# libskeleton

SKELETON_CFLAGS=-Ilibskeleton/include
SKELETON_SOURCES=libskeleton/src/skeleton.c \
                 libskeleton/src/skeleton_query.c \
                 libskeleton/src/skeleton_vector.c

libskeleton.so : $(SKELETON_SOURCE)
	cc --shared $(SKELETON_CFLAGS) $(SKELETON_SOURCES) -o libskeleton.so
