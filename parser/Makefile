CC = gcc

SRC= main.c types.c data.c read_csv.c mba_csv.c 
INCLUDE = -I/usr/include/ 
LIBDIR  = 
LIBS= -lm 
CFLAGS=-Wall -g $(INCLUDE) 
PROJ=parser


$(PROJ): Makefile $(SRC)
	$(CC) $(FRAMEWORK) $(CFLAGS) $(DEFINES) -o $(PROJ) $(SRC) $(LIBDIR) $(LIBS) 

clean: 
	rm -f $(PROJ)
	
