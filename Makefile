# Compiler and Flags
CXX = g++
CXXFLAGS = -std=c++17 -I. -IExternal -IExternal/include -Wall -Wextra

# Libraries and Frameworks (Mac specific)
LDFLAGS = -L/usr/local/lib -L/opt/homebrew/lib
LIBS = -lglfw -framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo

# Source Files
LOGIC_SRCS = Playfield.cpp Player.cpp Enemy.cpp Projectile.cpp Collectibles.cpp \
             Economy.cpp Waves.cpp Menus.cpp Cutscene.cpp Game.cpp
RENDER_SRCS = Render/Assets.cpp Render/Canvas.cpp Render/CharacterSprites.cpp \
              Render/Environment.cpp Render/Renderer.cpp Render/SceneRenderer.cpp \
              Render/SpriteAtlasData.cpp
GAME_SRCS = main.cpp External/src/glad.c $(LOGIC_SRCS) $(RENDER_SRCS)
TEST_SRCS = Tests/logic_tests.cpp $(LOGIC_SRCS)

# Targets
TARGET_GAME = Project1
TARGET_TESTS = logic_tests

.PHONY: all clean run run_tests atlas

all: $(TARGET_GAME) $(TARGET_TESTS)

# Rebuild the embedded character atlas (needs python3 + Pillow)
atlas:
	python3 Tools/gen_sprite_atlas.py
	python3 Tools/verify_sprite_atlas.py

# Build the Game
$(TARGET_GAME): $(GAME_SRCS)
	$(CXX) $(CXXFLAGS) $(GAME_SRCS) -o $(TARGET_GAME) $(LDFLAGS) $(LIBS)

# Build the Logic Tests
$(TARGET_TESTS): $(TEST_SRCS)
	$(CXX) $(CXXFLAGS) -ITests $(TEST_SRCS) -o $(TARGET_TESTS)

# Run the Game
run: $(TARGET_GAME)
	./$(TARGET_GAME)

# Run the Logic Tests
run_tests: $(TARGET_TESTS)
	./$(TARGET_TESTS)

# Clean build files
clean:
	rm -f $(TARGET_GAME) $(TARGET_TESTS)
