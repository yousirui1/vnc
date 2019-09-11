VERSION = 0.1

CROSS_COMPILE = #arm-

TARGET_ARCH = x86

DEBUG = -g #-O2

AS = $(CROSS_COMPILE)as
CC = $(CROSS_COMPILE)gcc
CXX = $(CROSS_COMPILE)g++
LD = $(CROSS_COMPILE)ld
AR = $(CROSS_COMPILE)ar
STRIP = $(CROSS_COMPILE)strip
OBJCOPY = $(CROSS_COMPILE)objcopy
CP = cp

CONFIG_COMPILER=gnu

outdir = ./bin

exeobj = RemoteMonitor

dllobj =  RemoteMonitor.dll

libobj = libRemoteMonitor.dll.a

mainobj = main.o inirw.o queue.o  display.o socket.o server.o client.o log.o ffmpeg.o  external.o

cppobj = 
mainobj = main.o inirw.o queue.o  display.o socket.o server.o client.o log.o ffmpeg.o control.o 


all: $(exeobj)


DEFINES := #-D DLL

ifeq ($(TARGET_ARCH), arm)
CFLAGS = -I. -I./include/ -I./SDL/include -I./ffmpeg/include \
		 -L./SDL/lib -lSDL2 -lpthread   -lXtst  -lXext -lXinerama -lXrandr -lXfixes -lXdamage -lX11 \
		 -L./ffmpeg/lib/arm -lavcodec -lavformat -lswscale -lavutil -lavdevice \
		 -L./SDL/lib -lSDL2 -lpthread 
else ifeq ($(TARGET_ARCH), x86)
CFLAGS = -I. -I./include/ -I./SDL/include/win -I./ffmpeg/include \
         -L./ffmpeg/lib/win -lavcodec -lavformat -lswscale -lavutil -lavdevice \
         -L./SDL/lib/win -lSDL2 -lmingw32 -lm -lws2_32  -lpthreadGC2 -lgdi32 \
		 #-mwindows
else
endif





CXXFLAGS = $(CFLAGS)

VPATH = .:./include:./lib:./src:./tools:.


$(exeobj):$(mainobj) $(cppobj) 
	$(CXX) $(DEBUG)  -o $(outdir)/$(exeobj) $(mainobj) $(cppobj) $(CXXFLAGS) 
	rm -f *.o
	@echo "Version $(VERSION)"
ifeq ($(TARGET_ARCH), arm)

else ifeq ($(TARGET_ARCH), x86)
	$(CP) ./lib/*.dll ./bin
	$(CP) ./ffmpeg/bin/*.dll ./bin
	$(CP) ./SDL/bin/*.dll ./bin
endif
	@echo "Build $(TARGET_ARCH) program  OK"
	

$(mainobj):%.o:%.c
	$(CC) -Wall $(DEBUG) $(CFLAGS) $(DEFINES) -c $< -o $@ 
	

$(cppobj):%.o:%.cpp
	$(CXX) -Wall $(DEBUG) $(CXXFLAGS) $(DEFINES) -c $< -o $@


$(dllobj):$(mainobj) $(cppobj)
	$(CXX) $(DEBUG) -shared -o $(outdir)/$(dllobj) $(mainobj) $(cppobj) $(CXXFLAGS) $(DLL) -Wl,--kill-at,--out-implib,$(outdir)/$(libobj) $(DEFINES)
	rm -f *.o
	@echo "Version $(VERSION)"
ifeq ($(TARGET_ARCH), x86)
	$(CP) ./lib/*.dll ./bin
	$(CP) ./ffmpeg/bin/*.dll ./bin
	$(CP) ./SDL/bin/*.dll ./bin
endif


dll:$(dllobj)
	@echo "build  only dll $(dllobj)"

clean:
	rm -f *.o $(outdir)/$(exeobj) $(outdir)/*.dll $(outdir)/*.a

