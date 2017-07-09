all: target
no-addr: target

all: CXXFLAGS=-DUSEADDR


target:
	g++ $(CXXFLAGS) -g -c utilitario.cc -o u.o
	g++ $(CXXFLAGS) -g -c servent.cc -o s.o
	g++ $(CXXFLAGS) -g -c client.cc -o c.o
	g++ $(CXXFLAGS) -g u.o s.o -o servent
	g++ $(CXXFLAGS) -g u.o c.o -o cliente
	rm -v c.o s.o u.o
