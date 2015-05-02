CC        = g++
CFLAGS    = -Wall -g -std=c++0x

EXECUTABLE = main

INCLUDES  =  

LIBDIRS   = 

LIBS	  = -lpthread -lrt

OBJS      = main.o
OBJS     += IOSample.o


all: $(EXECUTABLE)

main: $(OBJS) 
	$(CC) $(LIBDIRS) -o $@ $(OBJS) $(LIBS)

%.o: %.cpp
	$(CC) -c -o $@ $< $(CFLAGS) $(INCLUDES)

clean:
	rm $(OBJS) $(EXECUTABLE)
