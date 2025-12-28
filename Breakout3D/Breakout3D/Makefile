SHELL := /bin/sh

# --------- dirs ----------
SRC_DIR := src
INC_DIR := include
OBJ_DIR ?= obj

EXE ?= breakout3d

# --------- compiler ----------
CXX := g++
CXXFLAGS := -std=c++17 -Wall -I$(INC_DIR) -I. -MMD -MP

# --------- OS libs ----------
SYSTEM_UNAME := $(shell uname -s)

ifeq ($(SYSTEM_UNAME),Linux)
	LDLIBS := -lm -lGLU -lGL -lGLEW -lglfw
endif
ifeq ($(SYSTEM_UNAME),Darwin)
	LDLIBS := -lm -framework OpenGL -L/opt/local/lib/ -lGLEW -lglfw
endif

# --------- sources (recursive) ----------
SRC = $(shell find $(SRC_DIR) -name "*.cpp")
OBJ = $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(SRC))
DEP = $(OBJ:.o=.d)

.PHONY: all clean run debug

all: $(EXE)

# link
$(EXE): $(OBJ)
	$(CXX) $(OBJ) $(LDLIBS) -o $@

# compile (keeps folder structure inside obj/)
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf obj obj_debug breakout3d breakout3d_debug

run: $(EXE)
	./$(EXE)

debug:
	$(MAKE) OBJ_DIR=obj_debug EXE=breakout3d_debug CXXFLAGS="$(CXXFLAGS) -O0 -g -DBREAKOUT3D_DEBUG" all

-include $(DEP)
