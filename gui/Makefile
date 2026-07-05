include ../colors.mk

CXX		= c++
UNAME_S		:= $(shell uname -s)
UNAME_M		:= $(shell uname -m)
ifeq ($(UNAME_S),Darwin)
ifeq ($(UNAME_M),arm64)
export PKG_CONFIG_PATH := /opt/homebrew/lib/pkgconfig:$(PKG_CONFIG_PATH)
RAYLIB_PREFIX	= /opt/homebrew
else
RAYLIB_PREFIX	= /usr/local
endif
RAYLIB_FRAMEWORKS = -framework OpenGL -framework Cocoa -framework IOKit \
		    -framework CoreAudio -framework CoreVideo
else
RAYLIB_PREFIX	= /usr/local
endif
RAYLIB_CFLAGS	:= $(shell pkg-config --cflags raylib 2>/dev/null)
RAYLIB_LIBS	:= $(shell pkg-config --libs raylib 2>/dev/null)
ifeq ($(RAYLIB_LIBS),)
RAYLIB_CFLAGS	= -I$(RAYLIB_PREFIX)/include
RAYLIB_LIBS	= -L$(RAYLIB_PREFIX)/lib -lraylib
ifeq ($(UNAME_S),Darwin)
RAYLIB_LIBS	+= $(RAYLIB_FRAMEWORKS)
else
RAYLIB_LIBS	+= -lGL -lm -lpthread -ldl -lrt -lX11
endif
endif
CXXFLAGS	= -Wall -Wextra -Werror -std=c++17 -I include $(RAYLIB_CFLAGS)
LDFLAGS		= $(RAYLIB_LIBS)
SRC		= src/main.cpp src/network.cpp src/render.cpp
OBJ		= $(SRC:src/%.cpp=obj/%.o)
NAME		= ../gfx

all: $(NAME)
	@printf "$(GREEN)  ok$(RESET)  $(CYAN)gui$(RESET) → gfx\n"

$(NAME): $(OBJ)
	@$(CXX) $(CXXFLAGS) -o $(NAME) $(OBJ) $(LDFLAGS)

obj/%.o: src/%.cpp | obj
	@printf "$(MAGENTA)  c++$(RESET) $<\n"
	@$(CXX) $(CXXFLAGS) -c $< -o $@

obj:
	@mkdir -p obj

clean:
	@printf "$(YELLOW)  clean$(RESET) $(CYAN)gui$(RESET): obj/\n"
	@rm -rf obj

fclean: clean
	@printf "$(RED)  fclean$(RESET) $(CYAN)gui$(RESET): gfx\n"
	@rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re
