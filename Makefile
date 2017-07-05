all:
	g++ -g -c utilitario.cc -o u.o
	g++ -g -c servent.cc -o s.o
	g++ -g -c client.cc -o c.o
	g++ -g u.o s.o -o servent
	g++ -g u.o c.o -o cliente
	rm -v c.o s.o u.o
