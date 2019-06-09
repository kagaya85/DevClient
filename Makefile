# ！bash
TARGET = ts
OBJDIR = ./obj
SRCDIR = ./src
INCLUDE = ./include
CFLAGS = -std=c++11 -Wall
CC = g++
RM = -rm -f

all : $(TARGET)

ts : $(SRCDIR)/Main.cpp $(SRCDIR)/DevClient.cpp $(SRCDIR)/Logger.cpp
	$(CC) $(CFLAGS) $^ -o $@ -I $(INCLUDE)

.PHONY : clean
clean : 
	$(RM) $(TARGET) $(OBJDIR)/*