all: initializer finalizer clean

initializer: src/initializer.o
	gcc src/initializer.o -lpthread -lrt -o out/initializer

initializer.o: src/initializer.c
	gcc -c src/initializer.c

finalizer: src/finalizer.o
	gcc src/finalizer.o -lpthread -lrt -o out/finalizer

finalizer.o: src/finalizer.c
	gcc -c src/finalizer.c

clean:
	rm src/*.o