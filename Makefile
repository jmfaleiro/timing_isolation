mem_test_jose: driver.o measurements.o
	gcc -g -pthread -O3 -fverbose-asm -Wall -lrt driver.o measurements.o -o mem_test_jose -lnuma

driver.o: driver.c driver.h commands.h
	gcc -c -Wall -O3 -g driver.c

measurements.o: measurements.c measurements.h commands.h
	gcc -c -Wall -O3 -g  measurements.c

driver.h: commands.h

clean:
	rm -rf *.o mem_test_jose