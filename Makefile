CXX = g++ # Compiler

WINCXX = x86_64-w64-mingw32-g++ # MinGW cross-compiler for Windows

EMCC = em++

CFLAGS = -std=c++20 -Wall -Wextra -O3 -lraylib
WEBCFLAGS = -Llib/web -lraylib -DPLATFORM_WEB -Os
INCLUDE = -Iinclude/ # Flags

WEBLFLAGS = -s USE_GLFW=3 \
						-s ASYNCIFY \
						-s WASM=1 \
						-s ALLOW_MEMORY_GROWTH=1 \
						--shell-file src/web/shell.html
LFLAGS = -Llib/linux/ -lraylib # Libraries 
WINLFLAGS = -Llib/windows -lraylib -lmingw32 -lglu32 -lopengl32 -lgdi32 -lwinmm
# -static-libgcc -static-libstdc++


SRCS = $(wildcard src/*.cpp) # Recursively linking the source files

OBJS = $(SRCS:.cpp=.o) # Turning Source files into object files

EXEC = ProjectName # Path to the binary
WASM_EXEC = ProjectName.html
WIN_EXEC = ProjectName.exe

PLATFORM ?= LINUX

ifeq (${PLATFORM}, LINUX)
all: $(EXEC) # Creating a default target


# Rule to make source files object files
%.o : %.cpp
	$(CXX) $(CFLAGS) $(INCLUDE) -c -o $@ $<

# $< is a shorthand for the first prerequisite
# Rule to create executable
$(EXEC) : $(OBJS)
	$(CXX) -o $@ $^ $(LFLAGS)

# $@ is a shorthand for the target $(EXEC)
# $^ is a shorthand for the prerequisites $(OBJS)

# Cleaning target
clean:
	rm -f $(EXEC) $(OBJS)

move:
	mv $(EXEC) release/linux/
	cp -r res/* release/linux/res/
	cp ProjectNameLogo.png release/linux/

.PHONY:
	all clean
endif

ifeq ($(PLATFORM), WEB)
all: $(WASM_EXEC)

$(WASM_EXEC) : $(SRCS)
	$(EMCC) $(INCLUDE) $^ -o $@ $(WEBCFLAGS) $(WEBLFLAGS)

clean:
	rm *.wasm *.js *.html

move:
	mv *.wasm *.js *.html release/web/
	cp -r res/* release/web/res/
	cp ProjectNameLogo.png release/web/

.PHONY:
	all clean
endif
ifeq ($(PLATFORM), WINDOWS)
all: $(WIN_EXEC)

$(WIN_EXEC): $(OBJS)
	$(WINCXX) -o $@ $^ $(WINLFLAGS)
	

%.o : %.cpp
	$(WINCXX) $(CFLAGS) $(INCLUDE) -c -o $@ $<

clean:
	rm -f $(WIN_EXEC) $(OBJS)

move:
	@mkdir -p release/windows/
	@mkdir -p release/windows/res/
	mv $(WIN_EXEC) release/windows/
	cp -r res/* release/windows/res/
	cp ProjectNameLogo.png release/windows/

.PHONY:
	all clean
endif
