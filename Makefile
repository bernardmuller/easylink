CC = gcc
CFLAGS = -Wall -Wextra -std=c99
INCLUDES = -I./includes
SRCDIR = src
BUILDDIR = build
SOURCES = $(wildcard $(SRCDIR)/*.c)
MAIN_SOURCE = $(wildcard $(SRCDIR)/main.c)
OBJECTS = $(SOURCES:$(SRCDIR)/%.c=$(BUILDDIR)/%.o)
MAIN_OBJECT = $(MAIN_SOURCE:$(SRCDIR)/%.c=$(BUILDDIR)/%.o)
TARGET = myprogram

.PHONY: all clean

all: $(BUILDDIR)/$(TARGET)

$(BUILDDIR)/$(TARGET): $(OBJECTS) | $(BUILDDIR)
	$(CC) $(OBJECTS) -o $@

$(BUILDDIR)/%.o: $(SRCDIR)/%.c | $(BUILDDIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

main: $(BUILDDIR)/$(TARGET)

$(BUILDDIR)/$(TARGET): $(MAIN_OBJECT) | $(BUILDDIR)
	$(CC) $(MAIN_OBJECT) -o $@

$(BUILDDIR)/%.o: $(SRCDIR)/%.c | $(BUILDDIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILDDIR):
	mkdir -p $(BUILDDIR)

clean:
	rm -rf $(BUILDDIR)

run: $(BUILDDIR)/$(TARGET)
	$(BUILDDIR)/$(TARGET)	
