EXECUTABLE := httpserver

CC := g++
CFLAGS := -g -O2 -static -Wall

INCS := . /usr/local/include
LIBS := /usr/lib /usr/lib64
libs := m

SRCS := $(wildcard *.cpp)
OBJS := $(patsubst %.cpp, %.o, $(SRCS))

$(EXECUTABLE) : $(OBJS)
	$(CC) $(addprefix -I, $(INCS)) $(addprefix -L, $(LIBS)) $(CFLAGS) -o $(EXECUTABLE) $(OBJS) $(addprefix -l, $(libs))

%.o : %.cpp
	$(CC) $(addprefix -I, $(INCS)) $(CFLAGS) -c $< -o $@

.PHONY : clean
clean :
	-rm -f $(OBJS) $(EXECUTABLE)


