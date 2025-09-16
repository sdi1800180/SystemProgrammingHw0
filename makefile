CC = g++
CFLAGS = -std=c++11 -Wall -c

EXEC1 = jobExecutorServer
EXEC2 = jobCommander

OBJ1 = jobExecutorServer.o
OBJ2 = jobCommander.o

all: $(EXEC1) $(EXEC2)

$(EXEC1): $(OBJ1)
	$(CC) -o $(EXEC1) $(OBJ1)

$(OBJ1): jobExecutorServer.cpp
	$(CC) $(CFLAGS) jobExecutorServer.cpp -o $(OBJ1)

$(EXEC2): $(OBJ2)
	$(CC) -o $(EXEC2) $(OBJ2)

$(OBJ2): jobCommander.cpp
	$(CC) $(CFLAGS) jobCommander.cpp -o $(OBJ2)

multijob.sh:
	chmod +x multijob.sh

allJobsStop.sh:
	chmod +x allJobsStop.sh

clean:
	rm -f *.o $(EXEC1) $(EXEC2)
