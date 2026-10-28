CXX = g++ -m64 -march=native
CXXFLAGS = -I./common -I$(OBJDIR)/ -O3 -Wall

APP_NAME = kmeans
BUILDDIR = build
OBJDIR = $(BUILDDIR)/objs
DATADIR = data
LOGDIR = log
PNGDIR = raw
COMMONDIR = ./common

OBJS = $(OBJDIR)/main.o $(OBJDIR)/kmeansThread.o $(OBJDIR)/utils.o \
       $(OBJDIR)/ppm.o $(OBJDIR)/tasksys.o

default: $(BUILDDIR)/$(APP_NAME)

.PHONY: default dirs run plot all clean

dirs:
	mkdir -p $(OBJDIR) $(DATADIR) $(LOGDIR) $(PNGDIR)

$(BUILDDIR)/$(APP_NAME): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJS) -lm -lpthread

$(OBJDIR)/%.o: %.cpp | dirs
	$(CXX) $< $(CXXFLAGS) -c -o $@

$(OBJDIR)/%.o: $(COMMONDIR)/%.cpp | dirs
	$(CXX) $< $(CXXFLAGS) -c -o $@

$(OBJDIR)/main.o $(OBJDIR)/kmeansThread.o: $(COMMONDIR)/CycleTimer.h

run: default
	./$(BUILDDIR)/$(APP_NAME)

plot:
	python3 plot.py

all: run plot

clean:
	rm -rf $(BUILDDIR) $(DATADIR) $(LOGDIR) $(PNGDIR) *.ppm *~