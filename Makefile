biceps : biceps.c gescom.c creme.c
	cc -o biceps biceps.c gescom.c creme.c -Wall -Werror -lreadline -DTRACE

biceps-memory-leak : biceps.c gescom.c creme.c
	cc -g -O0 -o biceps-memory-leak biceps.c gescom.c creme.c -Wall -Werror -lreadline -DTRACE

memory-leak: biceps-memory-leak
	valgrind --leak-check=full --track-origins=yes ./biceps-memory-leak

clean :
	rm -f biceps biceps-memory-leak
