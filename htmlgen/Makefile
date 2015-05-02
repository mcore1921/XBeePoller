CC        = g++
CFLAGS    = -g -Wall
MYSQLCON_DIR_ROOT = ..
MYSQLCON_DIR = $(MYSQLCON_DIR_ROOT)/mysql-connector-c++-1.1.3
MYSQL_LIB = $(MYSQLCON_DIR)/driver/libmysqlcppconn-static.a
EXE       = thermHtmlGen

SOURCES     = main.cpp
SOURCES    += GenDataForHtml.cpp
SOURCES    += FormatForHtml.cpp

OBJS      = $(SOURCES:.cpp=.o)


INCLUDES  = 
INCLUDES += -I $(MYSQLCON_DIR)

LIBDIRS   = 
LIBDIRS  += -L /usr/lib/x86_64-linux-gnu
LIBDIRS  += -L $(MYSQLCON_DIR)/driver

LIBS      =
LIBS     += -lboost_program_options
LIBS     += -lmysqlcppconn-static -lmysqlclient

LINK      =

SUBDIRS   = 

all: $(MYSQL_LIB) $(SUBDIRS) $(EXE)

clean:
	rm -f main gtest *.o $(GENERATED) $(EXE)

superclean: clean
	rm -r mysql-connector-c++-1.1.3

$(SUBDIRS):
	$(MAKE) -C $@

.PHONY: $(SUBDIRS) clean supercleain

thermHtmlGen: $(OBJS) main.o 
	$(CC) $^ $(LINK) $(LIBDIRS) $(LIBS) -o $@  

$(MYSQL_LIB):
	cd $(MYSQLCON_DIR_ROOT); tar zxvf mysql-connector-c++-1.1.3.tar.gz;
	cd $(MYSQLCON_DIR); cmake .; 
	cd $(MYSQLCON_DIR)/driver; make

%_mo.cpp : %.h
	$(MOC) $< -o $@

%.o: %.cpp 
	$(CC) -c -o $@ $< $(CFLAGS) $(INCLUDES)
