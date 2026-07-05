include ../colors.mk

CC		= cc
CFLAGS	= -Wall -Wextra -Werror -I include
SRC		= src/main.c src/network.c src/world.c src/utils.c \
		  src/commands.c src/gui.c src/admin.c
OBJ		= $(SRC:src/%.c=obj/%.o)
NAME	= ../server

all: $(NAME)
	@printf "$(GREEN)  ok$(RESET)  $(CYAN)srv$(RESET) → server\n"

$(NAME): $(OBJ)
	@$(CC) $(CFLAGS) -o $(NAME) $(OBJ)

obj/%.o: src/%.c | obj
	@printf "$(MAGENTA)  cc$(RESET)  $<\n"
	@$(CC) $(CFLAGS) -c $< -o $@

obj:
	@mkdir -p obj

clean:
	@printf "$(YELLOW)  clean$(RESET) $(CYAN)srv$(RESET): obj/\n"
	@rm -rf obj

fclean: clean
	@printf "$(RED)  fclean$(RESET) $(CYAN)srv$(RESET): server\n"
	@rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re
