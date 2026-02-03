CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -g $(shell pkg-config --cflags gtk+-3.0)
LDFLAGS = -lportaudio -lsndfile -lsamplerate $(shell pkg-config --libs gtk+-3.0)
#-lSDL2 -lSDL2_image

ASAN_FLAGS = -fsanitize=address -fsanitize=undefined -fno-omit-frame-pointer
ASAN_ENV = ASAN_OPTIONS=detect_leaks=1:symbolize=1:log_path=./asan_log


BINDIR := bin
OBJDIR := obj
SRCDIR := src

TARGET := $(BINDIR)/soundboard
SRCS := $(wildcard $(SRCDIR)/*.c)
OBJS := $(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(SRCS))

all: $(TARGET)

#debug: CFLAGS += $(ASAN_FLAGS)
#debug: LDFLAGS += $(ASAN_FLAGS)
debug: $(TARGET)

release: CFLAGS += -O2
release: $(TARGET)

$(TARGET): $(OBJS) | $(BINDIR)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(OBJDIR)/%.o: $(SRCDIR)/%.c | $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR):
	mkdir -p $(OBJDIR)

$(BINDIR):
	mkdir -p $(BINDIR)

run: debug
	$(ASAN_ENV) ./$(TARGET)

gdb: debug
	gdb ./$(TARGET)

clean:
	rm -rf $(OBJDIR) $(BINDIR)

.PHONY: all debug release run gdb clean
