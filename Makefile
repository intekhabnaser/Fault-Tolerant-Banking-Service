compile: bserver.o fserver.o client.o
bserver.o: BackendServer.cpp
	g++ -o back.o BackendServer.cpp -lpthread
fserver.o: FrontendServer.cpp
	g++ -o front.o FrontendServer.cpp -lpthread
client.o: Client.cpp
	g++ -o client.o Client.cpp
clean:
	rm -rf *.o