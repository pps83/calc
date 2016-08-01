CC=g++
OBJS := $(patsubst %.cpp,%.o,$(wildcard *.cpp))
main: $(OBJS)

clean:
	$(RM) $(OBJS) $(subst .o,.d,$(OBJS)) main

test: main
	./main test

override CPPFLAGS += -MMD -std=c++11 -Wall
-include $(subst .o,.d,$(OBJS))
