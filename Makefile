all: main.cpp
	g++ main.cpp -o main.out

log:
	make debug
	./debug.out -C > log.txt

debug: main_debug.cpp
	g++ main_debug.cpp -o debug.out

clean:
	rm -rf *.out EmployeeIndex BucketArray log.txt
