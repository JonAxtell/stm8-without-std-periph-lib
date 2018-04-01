# Makefile for simple blinking stm8 project with SDCC compiler
# Author:	Jon Axtell
# Copyright 2018
# LICENSE:	GNU-LGPL

# We're assuming POSIX conformance
.POSIX:

# Compiler
CC = sdcc

# Platform
PLATFORM = stm8

# Product name
PNAME = blink

# Output directory for intermediate and final compiled file(s)
ODIR = bin
# Directory for helpers
IDIR = 
SDIR = 

# In case you ever want a different name for the main source file
MAINSRC = main.c

# These are the sources that must be compiled to .rel files:
EXTRASRCS = 

# The list of .rel files can be derived from the list of their source files
RELS = $(EXTRASRCS:.c=.rel)

INCLUDES = -I$(IDIR)
CFLAGS   = -m$(PLATFORM) --disable-warning 116 --stack-auto
LIBS     = -l$(PLATFORM)

# This just provides the conventional target name "all"; it is optional
# Note: I assume you set PNAME via some means not exhibited in your original file
all: $(PNAME)

# How to build the overall program
$(PNAME): $(MAINSRC) $(RELS)
	@mkdir -p $(ODIR)
#	$(CC) $(INCLUDES) $(CFLAGS) $(LIBS) $(MAINSRC) $(wildcard $(ODIR)/*.rel) -o$(ODIR)/
	$(CC) $(INCLUDES) $(CFLAGS) $(LIBS) $(MAINSRC) -o$(ODIR)/

# How to build any .rel file from its corresponding .c file
# GNU would have you use a pattern rule for this, but that's GNU-specific
.c.rel:
	@mkdir -p $(ODIR)
	$(CC) -c $(INCLUDES) $(CFLAGS) $(LIBS) $< -o$(ODIR)/

# Suffixes appearing in suffix rules we care about.
# Necessary because .rel is not one of the standard suffixes.
.SUFFIXES: .c .rel

#phonies

.PHONY:	clean flash

clean:
	@echo "Removing $(ODIR)..."
	@rm -rf $(ODIR)
	@echo "Done."

flash:
	../stm8flash/stm8flash -cstlinkv2 -pstm8s105k4 -w$(ODIR)/main.ihx