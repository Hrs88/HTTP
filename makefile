server:main.cc
	@g++ -o $@ $^ -std=c++11 -lpthread
clean:
	@rm server
	@rm logs/*