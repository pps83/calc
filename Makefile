CC=g++
OBJS := $(patsubst %.cpp,%.o,$(wildcard *.cpp))
main: $(OBJS)

clean:
	$(RM) $(OBJS) $(subst .o,.d,$(OBJS)) main

test: main
	./main test

override CPPFLAGS += -MMD -std=c++11 -Wall -O2 -Wl,--stack,0x8000000
override LDLIBS += -lreadline
-include $(subst .o,.d,$(OBJS))
