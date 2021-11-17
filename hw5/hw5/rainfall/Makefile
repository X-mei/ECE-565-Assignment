rainfall_seq: rainfall_seq.o
	g++ rainfall_seq.o -o rainfall_seq

rainfall_seq.o: rainfall_seq.cpp
	g++ -c rainfall_seq.cpp -std=c++1z -lpthread -ggdb3

clean:
	rm rainfall_seq rainfall_seq.o