#####################################################################
##                           ROM NAME                              ##

NAME = dma_restart
EXT	 = gbc

##                                                                 ##
#####################################################################
 
#####################################################################
##                        SOURCE FOLDERS                           ##

SOURCE = .

##                                                                 ##
#####################################################################

BIN	= $(NAME).$(EXT)

ASMFILES = $(foreach dir,$(SOURCE),$(wildcard $(dir)/*.asm))

RGBASM = ../rgbasm
RGBLINK  = ../rgblink
RGBFIX = ../rgbfix

OBJ = $(ASMFILES:.asm=.obj)

all: $(BIN)

build:
	@make clean
	@make
	@rm -f $(OBJ)

clean:
	@rm -f $(OBJ) $(BIN) $(NAME).sym $(NAME).map

%.obj : %.asm
	@$(RGBASM) $(INCLUDE) -o$@ $<

$(BIN): $(OBJ)
	@$(RGBLINK) -o $(BIN) -p 0xFF -m $(NAME).map -n $(NAME).sym $(OBJ)
	@$(RGBFIX) -p 0xFF -v $(BIN)

#####################################################################
