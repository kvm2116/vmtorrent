all: p2p_lev

p2p_lev: test.cpp
	g++ -o $@ $< p2p_leveldb.cpp -lleveldb -lpthread -lsnappy

clean:
	rm -f p2p_lev
