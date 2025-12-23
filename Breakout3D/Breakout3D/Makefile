SHELL := /bin/sh

# --------- dirs ----------
SRC_DIR := src
INC_DIR := include
OBJ_DIR := obj

EXE := breakout3d

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
SRC := $(shell find $(SRC_DIR) -name "*.cpp")
OBJ := $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(SRC))
DEP := $(OBJ:.o=.d)

.PHONY: all clean run

all: $(EXE)

# link
$(EXE): $(OBJ)
	$(CXX) $(OBJ) $(LDLIBS) -o $@

# compile (keeps folder structure inside obj/)
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJ_DIR) $(EXE)

run: $(EXE)
	./$(EXE)

-include $(DEP)
