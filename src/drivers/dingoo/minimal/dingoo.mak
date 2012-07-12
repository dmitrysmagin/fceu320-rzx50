DRVOBJS += $(OBJ)/zlib/adler32.o $(OBJ)/zlib/compress.o $(OBJ)/zlib/crc32.o \
	$(OBJ)/zlib/gzio.o $(OBJ)/zlib/uncompr.o $(OBJ)/zlib/deflate.o \
	$(OBJ)/zlib/trees.o $(OBJ)/zlib/zutil.o $(OBJ)/zlib/inflate.o \
	$(OBJ)/zlib/infback.o $(OBJ)/zlib/inftrees.o $(OBJ)/zlib/inffast.o

OSOBJS = $(OBJ)/$(MAMEOS)/fastmem.o $(OBJ)/$(MAMEOS)/minimal.o \
	$(OBJ)/$(MAMEOS)/dingoo.o $(OBJ)/$(MAMEOS)/video.o \
	$(OBJ)/$(MAMEOS)/clut8to16.o \
	$(OBJ)/$(MAMEOS)/blit.o $(OBJ)/$(MAMEOS)/sound.o \
	$(OBJ)/$(MAMEOS)/input.o $(OBJ)/$(MAMEOS)/fileio.o \
	$(OBJ)/$(MAMEOS)/config.o $(OBJ)/$(MAMEOS)/fronthlp.o

ifeq ($(DEBUG_MALLOC),)
DEBUG_MALLOC_OBJ = 
else
DEBUG_MALLOC_OBJ = $(OBJ)/$(MAMEOS)/dingoo_malloc_debug.o
endif

FRONTOBJS = $(OBJ)/$(MAMEOS)/minimal.o $(OBJ)/$(MAMEOS)/dingoo_frontend.o

FRONT = mame.dge

$(FRONT): maketree $(FRONTOBJS)
	@echo Linking $@...
	$(LD) $(LDFLAGS) $(FRONTOBJS) $(LIBS) -o $@
