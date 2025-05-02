serverd:main.cc
	@g++ -o $@ $^ -std=c++11 -lpthread
clean:
	@rm serverd -f
	@rm logs/* -f