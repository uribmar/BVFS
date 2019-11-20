CXX=g++ -std=c++17 -g -w -fmax-errors=1 -m32

bvfs_tester: tester.cpp bvfs.h
	${CXX} tester.cpp -o tester.out

run: bvfs_tester
	./tester.out


clean:
	@echo "Cleaning..."
	rm -f tester.out
