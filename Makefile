all: initializer finalizer producer consumer clean

initializer: src/initializer.o
	gcc src/initializer.o -lpthread -lrt -o out/initializer

initializer.o: src/initializer.c
	gcc -c src/initializer.c

finalizer: src/finalizer.o
	gcc src/finalizer.o -lpthread -lrt -o out/finalizer

finalizer.o: src/finalizer.c
	gcc -c src/finalizer.c

producer: src/producer.o
	gcc src/producer.o -lpthread -lrt -lm -o out/producer

producer.o: src/producer.c
	gcc -c src/producer.c

consumer: src/consumer.o
	gcc src/consumer.o -lpthread -lrt -lm -o out/consumer

consumer.o: src/consumer.c
	gcc -c src/consumer.c

clean:
	rm src/*.o