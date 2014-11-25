CXXFLAGS=-g -Wall -Wextra -DNDEBUG $(OPTFLAGS)
LIBS=$(OPTLIBS)
PREFIX?=/usr/local
CC=g++

SOURCES=$(wildcard src/*.cpp)
OBJECTS=$(patsubst %.cpp,%.o,$(SOURCES))

TEST_SRC=$(wildcard tests/*_tests.cpp)
TESTS=$(patsubst %.cpp,%,$(TEST_SRC))

TARGET=build/ccmix
SO_TARGET=$(patsubst %.a,%.so,$(TARGET))

# The Target Build
all: $(TARGET) gmd mixkey

dev: CXXFLAGS=-g -Wall -Wexta $(OPTFLAGS)
dev: all
	
gmd: $(OBJECTS) src/gmdedit/gmdedit.o
	$(CC) src/gmdedit/gmdedit.o src/mixid.o src/mix_db_gamedb.o src/mix_db_gmd.o \
	-o build/gmdedit

mixkey: src/mixkey/mixkey.o src/mixkey/mix_dexoder.o
	$(CC) src/mixkey/mixkey.o src/mixkey/mix_dexoder.o -o build/mixkey -lcryptopp

win32:
	/usr/bin/make -f Makefile.win32 CC=i586-mingw32msvc-g++ \
	CXX=i586-mingw32msvc-g++

	
$(TARGET): build $(OBJECTS)
	$(CC) $(OBJECTS) -o $(TARGET) -lcryptopp

build:
	@mkdir -p build
	@mkdir -p bin

# The Cleaner
clean:
	rm -rf $(OBJECTS) $(TESTS)
	rm -f tests/tests.log
	find . -name "*.gc*" -exec rm {} \;
	rm -rf `find . -name "*.dSYM" -print`

# The Install
install: all
	install -d $(DESTDIR)/$(PREFIX)/lib/
	install $(TARGET) $(DESTDIR)/$(PREFIX)/lib/

# The Checker
BADFUNCS='[^_.>a-zA-Z0-9](str(n?cpy|n?cat|xfrm|n?dup|str|pbrk|tok|_)|stpn?cpy|a?sn?printf|byte_)'
check:
	@echo Files with potentially dangerous functions.
	@egrep $(BADFUNCS) $(SOURCES) || true
