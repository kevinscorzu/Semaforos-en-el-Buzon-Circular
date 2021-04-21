all: initializer clean

initializer: src/initializer.o
	gcc src/initializer.o -lpthread -lrt -o out/initializer

initializer.o: src/initializer.c
	gcc -c src/initializer.c

clean:
	rm src/*.o