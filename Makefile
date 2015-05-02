CC        = g++
CFLAGS    = -Wall -g -std=c++0x
MYSQLCON_DIR = mysql-connector-c++-1.1.3
MYSQL_LIB = $(MYSQLCON_DIR)/driver/libmysqlcppconn.so.7

EXECUTABLE = thermPoller

INCLUDES  =  
INCLUDES += -I $(MYSQLCON_DIR)

LIBDIRS   = 
LIBDIRS  += -L $(MYSQLCON_DIR)/driver

LIBS      =
LIBS     += -lrt -lpthread
LIBS     += -lboost_program_options
LIBS     += -lmysqlcppconn-static -lmysqlclient

OBJS      = main.o
OBJS     += SQLConnector.o
OBJS     += SQLConnectorB.o
OBJS     += DataManager.o
OBJS     += util.o
OBJS     += Config.o
OBJS     += MessageFactory.o
OBJS     += IOSample.o
OBJS     += ATCommand.o
OBJS     += ATCommandResponse.o
OBJS     += RemoteATCommand.o
OBJS     += RemoteCommandResponse.o
OBJS     += ModemStatus.o
OBJS     += PortService.o
OBJS     += XBeeCommManager.o
OBJS     += XBeeUnitManager.o
OBJS     += RemoteXBeeManager.o
OBJS     += CoordinatorXBeeManager.o

DOBJS     = $(patsubst %.o, obj/%.o, $(OBJS))

.PHONY: clean supercleain

all: $(MYSQL_LIB) $(EXECUTABLE)

$(DOBJS): | obj

obj:
	@mkdir -p $@

thermPoller: $(DOBJS) 
	$(CC) $(LIBDIRS) -o $@ $(DOBJS) $(LIBS)

obj/%.o: %.cpp
	$(CC) -c -o $@ $< $(CFLAGS) $(INCLUDES)

$(MYSQL_LIB):
	tar zxvf mysql-connector-c++-1.1.3.tar.gz;
	cd mysql-connector-c++-1.1.3; cmake .; 
	cd mysql-connector-c++-1.1.3/driver; make

clean:
	@rm -f $(EXECUTABLE)
	@rm -rf obj

superclean: clean
	rm -r mysql-connector-c++-1.1.3
