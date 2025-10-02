CXXFLAGS := -O2 -Wall -Wextra
STRIP ?= strip

BIN  = elf-icon-thumbnailer
CONF = elf-icon.thumbnailer

INSTALL_PREFIX = ~/.local


all: $(BIN)

clean:
	-rm -f $(BIN)

$(BIN): $(BIN).cpp
	$(CXX) $(CXXFLAGS) $(BIN).cpp -o $(BIN) $(LDFLAGS)

install: $(BIN)
	mkdir -p $(INSTALL_PREFIX)/bin $(INSTALL_PREFIX)/share/thumbnailers
	cp -f $(CONF) $(INSTALL_PREFIX)/share/thumbnailers/
	cp -f $(BIN) $(INSTALL_PREFIX)/bin/
	$(STRIP) $(INSTALL_PREFIX)/bin/$(BIN)

uninstall:
	-rm -f $(INSTALL_PREFIX)/bin/$(BIN)
	-rm -f $(INSTALL_PREFIX)/share/thumbnailers/$(CONF)

$(BIN).c: ../elfpng.h
../elfpng.h: ../elfpng.i

