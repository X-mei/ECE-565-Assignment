rainfall_seq: rainfall_seq.o
	g++ rainfall_seq.o -o rainfall_seq

rainfall_seq.o: rainfall_seq.cpp
	g++ -c rainfall_seq.cpp -std=c++1z -lpthread -ggdb3

rainfall_pt: rainfall_pt.o
	g++ rainfall_pt.o -o rainfall_pt

rainfall_pt.o: rainfall_pt.cpp
	g++ -c rainfall_pt.cpp -std=c++1z -lpthread -ggdb3

clean:
	rm rainfall_seq rainfall_seq.o rainfall_pt rainfall_pt.o