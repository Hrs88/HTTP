all:message
	@mv $^ ../../web/exe/
message:message.cc
	@g++ -o $@ $^ -std=c++11 -lmysqlclient
clean:
	@rm ../../web/exe/message
	@rm ../../logs/message.txt -f