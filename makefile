# Compiler
CC=g++

# Compiler flags
# -g turn on debugging information
# -Wall turn on compiler warnings
CFLAGS=-g -Wall -std=c++11

# Executable Name
EXE=boilerplate

# Source files
SRC=*.cpp

# define any directories containing header files other than /usr/include
INCLUDES=-Imiddleware/stb

# define library paths
LFLAGS=-L/usr/local/lib -L/usr/lib/nvidia-367/


# define any libraries to link into executable
LIBS=-lglfw -lOpenGL

# typing 'make' will invoke the first target entry in the file
# you can name this target entry anything, but "default" or "all"
# are the most commonly used names by convention
all:
	$(CC) $(CFLAGS) $(SRC) $(INCLUDES) -o $(EXE) $(LFLAGS) $(LIBS)

clean:
	rm $(EXE)
