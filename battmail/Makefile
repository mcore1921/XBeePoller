CC        = g++
CFLAGS    = -Wall -g -std=c++0x
MYSQLCON_DIR_ROOT = ..
MYSQLCON_DIR = $(MYSQLCON_DIR_ROOT)/mysql-connector-c++-1.1.3
MYSQL_LIB = $(MYSQLCON_DIR)/driver/libmysqlcppconn-static.a

EXECUTABLE = battmail

INCLUDES  = 
INCLUDES += -I $(MYSQLCON_DIR)
INCLUDES += -I ..

LIBDIRS   =  
LIBDIRS  += -L $(MYSQLCON_DIR)/driver
#LIBDIRS  += $(QTDIR)/lib

LIBS      =
LIBS     += -lboost_program_options
#LIBS     += -lmysqlcppconn
LIBS     += -lmysqlcppconn-static -lmysqlclient
LIBS     += -lcurl

OBJS      = main.o
OBJS     += SendMail.o
OBJS     += Payload.o
OBJS     += Config.o

DOBJS     = $(patsubst %.o, obj/%.o, $(OBJS))

LINK      =
LINK     += -Xlinker -R $(CURDIR)/$(MYSQLCON_DIR)/driver


all: $(MYSQL_LIB) $(EXECUTABLE)

$(DOBJS): | obj

obj:
	@mkdir -p $@

$(EXECUTABLE): $(DOBJS) 
	$(CC) $^ $(LINK) $(LIBDIRS) $(LIBS) -o $@ 

obj/%.o: %.cpp 
	$(CC) -c -o $@ $< $(CFLAGS) $(INCLUDES)

$(MYSQL_LIB):
	cd $(MYSQLCON_DIR_ROOT); tar zxvf mysql-connector-c++-1.1.3.tar.gz;
	cd $(MYSQLCON_DIR); cmake .; 
	cd $(MYSQLCON_DIR)/driver; make

clean:
	@rm -f $(EXECUTABLE)
	@rm -rf obj

superclean: clean
	rm -r $(MYSQLCON_DIR)


.PHONY: clean supercleain

