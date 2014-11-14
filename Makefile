CFLAGS = -Wall
.PHONY : all rebuild clean mrproper
all: yctget
rebuild: mrproper all

yctget: main.o serial.o
	@echo "*** Linking all main objects files..."
	@g++ serial.o main.o -o yctget

main.o: main.cpp serial.h
	@echo "*** Compiling main.o"
	@g++ $(CFLAGS) -c main.cpp -o main.o

serialtest: serialtest.o serial.o
	@echo "*** Linking serialtest executable..."
	@g++ serial.o serialtest.o -o serialtest

serialtest.o: serialtest.cpp serial.h
	@echo "*** Compiling serialtest.o"
	@g++ $(CFLAGS) -c serialtest.cpp -o serialtest.o

serial.o: serial.cpp serial.h
	@echo "*** Compiling serial.o"
	@g++ $(CFLAGS) -c serial.cpp -o serial.o

clean:
	@echo "*** Erasing objects files..."
	@rm -f *.o

mrproper: clean
	@echo "*** Erasing executables..."
	@rm -f yctget serialtest
