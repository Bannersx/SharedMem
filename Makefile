libraries:
	sudo apt-get install git build-essential
	sudo apt-get install libgsl-dev
	
initializer:
	gcc -o init Initializer.c buffer.c -lrt -lpthread
	./init -n buffer -s 5
producer:
	gcc -o prod Producer.c buffer.c -lrt -lpthread -lm -lgsl
	./prod -n buffer -m a -t 4
consumer:
	gcc -o cons Consumer.c buffer.c -lrt -lpthread -lm -lgsl
	./cons -n buffer -m a -t 5
finalizer:
	gcc -o fin Finalizer.c buffer.c -lrt -lpthread
	./fin -n buffer
