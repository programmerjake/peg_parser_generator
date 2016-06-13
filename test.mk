.PHONY: all clean
ifdef windir
# windows
EXEEXT := .exe
else
EXEEXT :=
endif

all: build-test/test$(EXEEXT)

build-test/test$(EXEEXT): build-test/test_main.o build-test/test.o
	g++ build-test/test_main.o build-test/test.o -o build-test/test$(EXEEXT)

build-test/test_main.o: test.h test_main.cpp
	mkdir -p build-test && g++ -c -std=c++11 -Wall -g -o build-test/test_main.o test_main.cpp

build-test/test.o: test.h test.cpp
	mkdir -p build-test && g++ -c -std=c++11 -Wall -g -o build-test/test.o test.cpp

clean: rm -rf build-test