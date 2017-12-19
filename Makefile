BIN := simple-elf-parser hello
# CXXOPT := -Wl,--verbose

all: $(BIN)
	@### Do Nothing 

%: %.c
	gcc -Wl,-Ttext=0x400000 -o $@ $^

%: %.cpp
	$(CXX) $(CXXOPT) -o $@ $^

clean:
	rm -f $(BIN) .[0-9]*

test: $(BIN)
	./simple-elf-parser hello