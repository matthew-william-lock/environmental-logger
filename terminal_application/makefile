.RECIPEPREFIX +=
CC = g++
CFLAGS = -Wall -lm -lrt -lwiringPi -lpthread

PROG = bin/*
OBJS = obj/*

default:
    mkdir -p bin obj
    $(CC) $(CFLAGS) -c src/Logger.cpp -o obj/Logger
    $(CC) $(CFLAGS) -c src/CurrentTime.cpp -o obj/CurrentTime
    $(CC) $(CFLAGS) obj/Logger obj/CurrentTime -o bin/EnvironmentLogger
run:
    sudo ./bin/EnvironmentLogger
clean:
    rm $(PROG) $(OBJS)
