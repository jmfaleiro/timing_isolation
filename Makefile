mem_test_jose: driver.o measurements.o
	gcc -g -pthread -lrt driver.o measurements.o -o mem_test_jose -lnuma

driver.o: driver.c driver.h commands.h
	gcc -c -g driver.c

measurements.o: measurements.c measurements.h commands.h
	gcc -c -g  measurements.c

driver.h: commands.h

clean:
	rm -rf *.o mem_test_jose