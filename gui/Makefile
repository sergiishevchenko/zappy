include ../colors.mk

CXX		= c++
RAYLIB_CFLAGS	:= $(shell pkg-config --cflags raylib 2>/dev/null)
RAYLIB_LIBS	:= $(shell pkg-config --libs raylib 2>/dev/null)
ifeq ($(RAYLIB_LIBS),)
RAYLIB_CFLAGS	= -I/usr/local/include
RAYLIB_LIBS	= -L/usr/local/lib -lraylib -lGL -lm -lpthread -ldl -lrt -lX11
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
