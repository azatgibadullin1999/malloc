CC = clang++
CFLAGS = -Wall -Wextra -Werror

SOURCE_EXT =	.c
OBJECT_EXT =	.o
INCLUDE_EXT =	.h
OUT_EXT =	.so

SOURCE_FILES_NAMES =	

INCLUDE_FILES_NAMES =	

SOURCE_FILES =	$(foreach P, $(SOURCE_FILES_NAMES), $(shell find . -name "$(P)"))
INCLUDE_FILES =	$(addprefix -I,$(foreach P, $(INCLUDE_FILES_NAMES), $(shell find . -name "$(P)")))
OBJECT_FILES =	$(SOURCE_FILES:$(SOURCE_EXT)=$(OBJECT_EXT))


ifeq( $(HOSTTYPE),) 
	HOSTTYPE := $(shell uname -m)_$(shell uname -s)
endif
OUT_FILE = libft_malloc_$(HOSTTYPE)$(OUT_EXT)

all: $(OUT_FILE)

$(OUT_FILE): $(OBJECT_FILES)
	$(CC) -o $(OUT_FILE) $^

%$(OBJECT_EXT): %$(SOURCE_EXT)
	$(CC) $(CFLAGS) $(INCLUDE_FILES) -c -o $@ $<

clean:
	rm -rf $(OBJECT_FILES)

fclean: clean
	rm -rf $(OUT_FILE)

re: fclean all

.PHONY: all clean fclean re
