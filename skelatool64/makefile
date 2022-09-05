
GCC_FLAGS = -Wall -Werror -g -rdynamic -I./yaml-cpp/include 

LINKER_FLAGS = -L./yaml-cpp -lassimp -lyaml-cpp -lpng -ltiff -llua -ldl

SRC_FILES = main.cpp $(shell find src/ -type f -name '*.cpp')

OBJ_FILES =	$(patsubst %.cpp, build/%.o, $(SRC_FILES))

LUA_SRC_FILES = $(shell find lua/ -type f -name '*.lua')

LUA_OBJ_FILES =	$(patsubst %.lua, build/%.o, $(LUA_SRC_FILES))

DEPS = $(patsubst %.cpp, build/%.d, $(SRC_FILES))

.PHONY: default
default: skeletool64

-include $(DEPS)

build/lua/%.o: lua/%.lua
	@mkdir -p $(@D)
	luac -o $(@:%.o=%.out) $<
	ld -r -b binary -o $@ $(@:%.o=%.out)

build/%.o: %.cpp
	@mkdir -p $(@D)
	g++ $(GCC_FLAGS) -c $< -o $@
	$(CC) $(GCC_FLAGS) -MM $^ -MF "$(@:.o=.d)" -MT"$@"

skeletool64: $(OBJ_FILES) $(LUA_OBJ_FILES)
	g++ -g -o skeletool64 $(OBJ_FILES) $(LUA_OBJ_FILES) $(LINKER_FLAGS)

clean:
	rm -r build/

init:
	

install: skeletool64
	cp skeletool64 ~/.local/bin

build/skeletool.deb: skeletool64 control
	mkdir build/skeletool/usr/local/bin -p
	cp skeletool64 build/skeletool/usr/local/bin
	mkdir build/skeletool/DEBIAN -p
	cp control build/skeletool/DEBIAN
	dpkg-deb --build build/skeletool