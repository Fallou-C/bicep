biceps : biceps.c gescom.c creme.c
	cc -o biceps biceps.c gescom.c creme.c -Wall -Werror -lreadline -DTRACE

val: biceps
	valgrind --leak-check=full ./biceps

clean :
	rm -f biceps
