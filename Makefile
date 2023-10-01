make: proj2.c
	gcc -std=gnu99 -Wall -Wextra -Werror -pedantic -pthread -lrt proj2.c -o proj2

clean:
	rm *.o