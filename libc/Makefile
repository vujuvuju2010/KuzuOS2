# libc/Makefile - Build KuzuOS5 C Standard Library

CC = gcc
AR = ar
CFLAGS = -m32 -fno-pie -nostdlib -nostdinc -fno-builtin -fno-stack-protector -Wall -Wextra
ARFLAGS = rcs

LIBDIR = lib
OBJDIR = obj
SRCDIR = src
INCDIR = include

# Header search path
CFLAGS += -I$(INCDIR)

# Source files
SOURCES = \
	$(SRCDIR)/string/strlen.c \
	$(SRCDIR)/string/strcpy.c \
	$(SRCDIR)/string/strcmp.c \
	$(SRCDIR)/string/memops.c \
	$(SRCDIR)/stdlib/malloc.c \
	$(SRCDIR)/stdlib/strtol.c \
	$(SRCDIR)/stdlib/exit.c \
	$(SRCDIR)/unistd/unistd.c \
	$(SRCDIR)/stdio/stdio.c

# Object files
OBJECTS = $(SOURCES:$(SRCDIR)/%.c=$(OBJDIR)/%.o)

# Target
TARGET = $(LIBDIR)/libc.a

# Build rules
all: $(TARGET)

$(TARGET): $(OBJECTS)
	@mkdir -p $(LIBDIR)
	$(AR) $(ARFLAGS) $@ $^
	@echo "Built $@"

$(OBJDIR)/%.o: $(SRCDIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c -o $@ $<
	@echo "Compiled $<"

clean:
	rm -rf $(OBJDIR) $(LIBDIR)

.PHONY: all clean
