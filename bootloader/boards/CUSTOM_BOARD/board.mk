# List of all the board related files.
BOARDSRC = $(BOARDSDIR)/CUSTOM_BOARD/board.c

# Required include directories
BOARDINC = $(BOARDSDIR)/CUSTOM_BOARD/

# Shared variables
ALLCSRC += $(BOARDSRC)
ALLINC  += $(BOARDINC)
