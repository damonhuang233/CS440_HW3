all: main.cpp
	g++ main.cpp -o main.out

log:
	make debug
	./debug.out -C > log.txt

debug: debug.cpp
	g++ debug.cpp -o debug.out

clean:
	rm -rf *.out EmployeeIndex BucketArray log.txt
