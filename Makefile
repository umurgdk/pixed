CC=gcc
CFLAGS=-Wall --std=c99 -g
OUT_DIR=build

all: pixed

pixed: libpixed.o libcol.o shaders.h libglutil.o
	$(CC) pixed.c libpixed.o libcol.o libglutil.o `pkg-config --cflags --libs glew glfw3` $(CFLAGS) -o pixed -framework OpenGL

shaders.h: shader_compiler
	./shader_compiler > shaders.h

shader_compiler: shader_compiler.c
	$(CC) shader_compiler.c -o ./shader_compiler -g

libglutil.o: libglutil.c
	$(CC) -c $(CFLAGS) libglutil.c

libpixed.o: libpixed.c
	$(CC) -c $(CFLAGS) libpixed.c

libcol.o: libcol.c
	$(CC) -c $(CFLAGS) libcol.c

clean:
	rm shader_compiler
	rm shaders.h
	rm *.o pixed
