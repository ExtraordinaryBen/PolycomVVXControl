# to install prerequisites on Debian Linux:
#   apt-get install libcsv-dev libexpat1-dev libcurl4-gnutls-dev
# to compile run:
#   make PREFIX=/usr/local

PREFIX = /tmp
#BINEXT = .exe
BINEXT = 
BINDIR = $(PREFIX)/bin/
CPP  = g++
CC   = gcc
AR   = ar
OBJ  = polycomvvx.o polycomvvxcontrol.o
INCS =  -I.
BIN  = $(BINDIR)PolycomVVXControl$(BINEXT)
CFLAGS = $(INCS) -fexpensive-optimizations -Os
CPPFLAGS = $(INCS) -fexpensive-optimizations -Os
LDFLAGS = -lcsv -lexpat -lcurl
ifeq ($(OS),Windows_NT)
CFLAGS += -DSTATIC
#LDFLAGS += -static -lws2_32 -lwldap32 -lz
LDFLAGS += -lz -Wl,--as-needed -static $(shell curl-config --static-libs) -lshishi -lgnutls -lnettle -lhogweed -ltasn1 -lgmp -lintl -lgcrypt -lgpg-error -lsicuin -lsicuuc -lsicudt -lws2_32 -lwldap32 -lwinmm -lcrypt32
endif
RM = rm -f
CP = cp -f

#.PHONY: all all-after clean clean-custom

all: $(BIN)

check: $(BIN)
	$(BIN) -?

clean:
	${RM} $(OBJ) $(LIBS) $(BIN)
	rmdir $(BINDIR) || true

$(BINDIR):
	mkdir $(BINDIR)

#.o: $^.cpp
#	$(CPP) -c $^.cpp -o $@.o $(CFLAGS)
	
$(BIN): $(BINDIR) $(OBJ)
	$(CPP) $(OBJ) -s -o $(BIN) $(LDFLAGS)
