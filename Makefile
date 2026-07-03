include colors.mk

MAKEFLAGS += --no-print-directory

NAME		= server client gfx

all: $(NAME)
	@printf "$(GREEN)done$(RESET)  $(BLUE)zappy$(RESET): $(NAME)\n"

server:
	@printf "$(CYAN)build$(RESET) $(BLUE)server$(RESET)\n"
	@$(MAKE) -C srv

client:
	@printf "$(CYAN)build$(RESET) $(BLUE)client$(RESET)\n"
	@$(MAKE) -C cli

gfx:
	@printf "$(CYAN)build$(RESET) $(BLUE)gfx$(RESET)\n"
	@$(MAKE) -C gui

clean:
	@printf "$(YELLOW)clean$(RESET)  $(BLUE)zappy$(RESET)\n"
	@$(MAKE) -C srv clean
	@$(MAKE) -C cli clean
	@$(MAKE) -C gui clean

fclean:
	@printf "$(RED)fclean$(RESET) $(BLUE)zappy$(RESET)\n"
	@$(MAKE) -C srv fclean
	@$(MAKE) -C cli fclean
	@$(MAKE) -C gui fclean
	@printf "$(RED)  rm$(RESET)  server client gfx\n"
	@rm -f server client gfx

re: fclean all

compile_commands:
	@./tools/gen_compile_commands.sh

.PHONY: all server client gfx clean fclean re compile_commands
