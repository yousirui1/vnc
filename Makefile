VERSION = 0.1

CROSS_COMPILE = #arm-

DEBUG = -g #-O2

AS = $(CROSS_COMPILE)as
CC = $(CROSS_COMPILE)gcc
CXX = $(CROSS_COMPILE)g++
LD = $(CROSS_COMPILE)ld
AR = $(CROSS_COMPILE)ar
STRIP = $(CROSS_COMPILE)strip
OBJCOPY = $(CROSS_COMPILE)objcopy

CONFIG_COMPILER=gnu

outdir = ./bin

exeobj = vnc

mainobj =  main.o inirw.o display.o

cppobj = 

CFLAGS = -I. -I./include/ -I./SDL/include -I./ffmpeg/include \
		 -L./ffmpeg/lib/arm -lavcodec -lavformat -lswscale -lavutil -lavdevice \
		 -L./SDL/lib -lSDL2 -lpthread 

CXXFLAGS = $(CFLAGS)

VPATH = .:./include:./lib:./src:./tools:.


$(exeobj):$(mainobj) $(cppobj)
	$(CXX) $(DEBUG)  -o $(outdir)/$(exeobj) $(mainobj) $(cppobj) $(CXXFLAGS) 
	rm -f *.o
	@echo "Version $(VERSION)"

$(mainobj):%.o:%.c
	$(CC) -Wall $(DEBUG) $(CFLAGS) -c $< -o $@
	

$(cppobj):%.o:%.cpp
	$(CXX) -Wall $(DEBUG) $(CXXFLAGS) -c $< -o $@


clean:
	rm -f *.o $(outdir)/$(exeobj)


