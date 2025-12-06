CC = cc
CFLAGS = -Wall
SOURCES = main.c miniaudio.c
EXECUTABLE = main

all: $(EXECUTABLE)

$(EXECUTABLE): $(SOURCES)
	${CC} ${CFLAGS} ${SOURCES} -o ${EXECUTABLE} ${LDFLAGS}

clean:
	rm -rf ${EXECUTABLE}
