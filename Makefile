CC=gcc
CFLAGS=-Wall --std=c99 -g -pedantic -I/usr/local/include
OUT_DIR=build

all: pixed

pixed: libpixed.o libglutil.o shaders.h pixed.c
	$(CC) pixed.c libpixed.o libglutil.o `pkg-config --cflags --libs glew glfw3` $(CFLAGS) -o pixed -framework OpenGL

shaders.h: shader_compiler
	./shader_compiler > shaders.h

shader_compiler: shader_compiler.c
	$(CC) shader_compiler.c -o ./shader_compiler -g

libglutil.o: libglutil.c
	$(CC) -c $(CFLAGS) libglutil.c `pkg-config --cflags glew`

libpixed.o: libpixed.c
	$(CC) -c $(CFLAGS) libpixed.c

clean:
	rm shader_compiler
	rm shaders.h
	rm *.o pixed
