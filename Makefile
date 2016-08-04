CC=g++
OBJS := $(patsubst %.cpp,%.o,$(wildcard *.cpp))
calc: $(OBJS)

clean:
	$(RM) $(OBJS) $(subst .o,.d,$(OBJS)) calc

test: calc
	./calc test

override CPPFLAGS += -MMD -std=c++11 -Wall -O2 -Wl,--stack,0x8000000
override LDLIBS += -lreadline
-include $(subst .o,.d,$(OBJS))
