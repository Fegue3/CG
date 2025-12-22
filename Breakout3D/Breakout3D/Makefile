CXX = g++
CXXFLAGS = -std=c++17 -Wall -Iinclude
LIBS = -lglfw -lGL -lGLEW

SRC = \
  src/main.cpp \
  src/engine/Window.cpp \
  src/engine/Time.cpp \
  src/engine/Shader.cpp \
  src/engine/Mesh.cpp \
  src/engine/Renderer.cpp \
  src/game/Game.cpp \
  src/game/GameConfig.cpp \
  src/game/GameState.cpp

OBJ = $(SRC:.cpp=.o)
BIN = breakout3d

all: $(BIN)

$(BIN): $(OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LIBS)

clean:
	rm -f $(OBJ) $(BIN)

run: $(BIN)
	./$(BIN)
