all:test
	@mv $^ ../web/exe/
test:test.cc
	@g++ -o $@ $^ -std=c++11
clean:
	@rm ../web/exe/*
	@rm ../logs/info.txt -f