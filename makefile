all: proxy.o main.o cache.o session.o parser.o
	g++ -g proxy.o main.o session.o cache.o parser.o -o s


main.o: main.cpp
	g++ -g -Wall -c  main.cpp

proxy.o: proxy.cpp
	g++ -g -Wall -c proxy.cpp

session.o: session.cpp
	g++ -g -Wall -c session.cpp

cache.o: cache.cpp
	g++ -g -Wall -c cache.cpp

parser.o: parser.cpp
	g++ -g -Wall -c parser.cpp

clean:
	rm proxy.o main.o cache.o session.o parser.o s