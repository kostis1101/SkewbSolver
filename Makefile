

all: main.c stb_image_write.h
	gcc -O3 main.c -o gskewb -Wall -Wextra -pedantic -Werror
