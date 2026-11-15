CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Wpedantic

all: musicdb_report

musicdb_report: main.c musicdb.c musicdb.h
	$(CC) $(CFLAGS) main.c musicdb.c -o musicdb_report

check:
	$(CC) $(CFLAGS) -fsanitize=address,undefined main.c musicdb.c -o musicdb_report
	./musicdb_report data 2485 > report.txt

clean:
	rm -f musicdb_report report.txt memcheck.txt
