#----- Modify CC and CFLAGS as appropriate in your environment
CC = gcc
CFLAGS = -O3 -fopenmp

LIB = -lm -lc

UNITFILES = 

#------------ EXECUTABLE FILES ---------------
all: histo histo_locks histo_atomic histo_creative

% : %.o $(UNITFILES)
	$(CC) $(CFLAGS) -o % %.o $(UNITFILES) $(LIB)

%.o : %.c
	$(CC) $(CFLAGS) -c %.c $(INCLUDE)

clean:
	rm -f histo *.o


