
run: Cliente.o Servidor.o Pacote.o Valentao.o Origem.o 
	g++ Cliente.o Servidor.o Pacote.o Valentao.o Origem.o -o run -lpthread


Pacote.o: Pacote.cpp
	g++ -c Pacote.cpp

Cliente.o: Cliente.cpp
	g++ -c Cliente.cpp

Servidor.o: Servidor.cpp
	g++ -c Servidor.cpp -lpthread


Valentao.o: Valentao.cpp
	g++ -c Valentao.cpp

Origem.o:	Origem.cpp 
	g++ -c Origem.cpp -lpthread



clean: 
	rm *.o run
