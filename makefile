all: proxy.o main.o cache.o session.o parser.o
	g++ proxy.o main.o session.o cache.o parser.o -o s


main.o: main.cpp
	g++ -Wall -c main.cpp

proxy.o: proxy.cpp
	g++ -Wall -c proxy.cpp

session.o: session.cpp
	g++ -Wall -c session.cpp

cache.o: cache.cpp
	g++ -Wall -c cache.cpp

parser.o: parser.cpp
	g++ -Wall -c parser.cpp

clean:
	rm proxy.o main.o cache.o session.o parser.o s