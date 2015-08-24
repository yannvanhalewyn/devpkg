# Compiler
CC=cc
CFLAGS=-Wall -g -I/usr/include/apr-1 -Ibstrlib-master/
LDFLAGS=-lapr-1 -laprutil-1

# DIRS
SRCDIR=src
OBJDIR=obj
BINDIR=bin

# Files
SOURCES := $(shell find $(SRCDIR)/*.c)
HEADERS := $(shell find $(SRCDIR)/*.h)
OBJECTS := $(SOURCES:$(SRCDIR)/%.c=$(OBJDIR)/%.o)
BUILD=devpkg

SRC:=$(SRCDIR)/$(SRC)
BUILD:=$(BINDIR)/$(BUILD)

BSTRSRC=bstrlib-master/bstrlib.c
BSTROBJ=$(OBJDIR)/bstrlib.o

$(BUILD): $(OBJECTS) $(BSTROBJ)
	@[ -d $(BINDIR) ] || (mkdir $(BINDIR) && echo "\n--- made $(BINDIR) dir\n")
	$(CC) $(LDFLAGS) $^ -o $@

$(OBJECTS): $(OBJDIR)/%.o: $(SRCDIR)/%.c $(HEADERS)
	@[ -d $(OBJDIR) ] || (mkdir $(OBJDIR) && echo "\n--- made $(OBJDIR) dir\n")
	$(CC) $(CFLAGS) -c $< -o $@

$(BSTROBJ): $(BSTRSRC)
	cc -c $< -o $@

# =====
# PHONY
# =====
run: $(BUILD)
	./$(BUILD)

clean:
	rm -rf $(BINDIR) $(OBJDIR)

print:
	@echo SOURCES = $(SOURCES)
	@echo HEADERS = $(HEADERS)
	@echo OBJECTS = $(OBJECTS)
	@echo BUILD = $(BUILD)

PHONY: run clean print
