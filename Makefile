EXECUTABLE := hashtable_test

CC := g++
CFLAGS := -g -O2 -static -Wall

INCS := . /usr/local/include
LIBS := /usr/lib /usr/lib64
libs := m

SRCS := $(wildcard *.cpp)
OBJS := $(patsubst %.cpp, %.o, $(SRCS))

USER_MACROS := _TEST_HT
$(EXECUTABLE) : $(OBJS)
	$(CC) $(addprefix -I, $(INCS)) $(addprefix -L, $(LIBS)) $(CFLAGS) -o $(EXECUTABLE) $(OBJS) $(addprefix -l, $(libs))

%.o : %.cpp
	$(CC) $(addprefix -I, $(INCS)) $(addprefix -D, $(USER_MACROS)) $(CFLAGS) -c $< -o $@

.PHONY : clean
clean :
	-rm -f $(OBJS) $(EXECUTABLE)


