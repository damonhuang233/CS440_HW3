all: main.cpp
	g++ main.cpp -o main.out

clean:
	rm -rf *.out EmployeeIndex EmployeeData
