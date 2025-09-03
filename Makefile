CC = gcc
CFLAGS = -std=c99 -Wall -Wextra -DGL_DEBUG
CFLAGS += -Ithird_party

OUT = tsunade
SRC = \
    $(wildcard src/*.c) \
    $(wildcard src/api/*.c) \
    $(wildcard third_party/lua/*.c)
OBJ = $(SRC:.c=.o)

ifeq ($(OS),Windows_NT)
    LDFLAGS = -lgdi32 -lopengl32
else
    LDFLAGS = -lGL -lGLU -lX11 -lXi -lXcursor -lm
    CFLAGS += -D_POSIX_C_SOURCE=199309L
    CFLAGS += -D_GNU_SOURCE
    CFLAGS += -DSOKOL_GLCORE
endif

.PHONY: build, clean

build: $(OBJ)
	$(CC) -o $(OUT) $^ $(LDFLAGS)

%.o: %.c
	$(CC) -o $@ -c $< $(CFLAGS)

clean:
	rm -f $(OBJ) $(OUT)