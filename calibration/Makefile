CC        = g++
CFLAGS    = -Wall -g -std=c++0x

EXECUTABLE = calib

INCLUDES  =  

LIBDIRS   = 

LIBS      =

OBJS      = main.o

DOBJS     = $(patsubst %.o, obj/%.o, $(OBJS))

.PHONY: clean 

all: $(EXECUTABLE)

$(DOBJS): | obj

obj:
	@mkdir -p $@

$(EXECUTABLE): $(DOBJS) 
	$(CC) $(LIBDIRS) -o $@ $(DOBJS) $(LIBS)

obj/%.o: %.cpp
	$(CC) -c -o $@ $< $(CFLAGS) $(INCLUDES)

clean:
	@rm -f $(EXECUTABLE)
	@rm -rf obj

