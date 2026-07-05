include ../colors.mk

NAME	= ../client

all: $(NAME)
	@printf "$(GREEN)  ok$(RESET)  $(CYAN)cli$(RESET) → client\n"

$(NAME): main.py network.py strategy.py args.py vision.py client.sh
	@cp client.sh $(NAME)
	@chmod +x $(NAME)

clean:
	@printf "$(YELLOW)  clean$(RESET) $(CYAN)cli$(RESET): (nothing)\n"

fclean:
	@printf "$(RED)  fclean$(RESET) $(CYAN)cli$(RESET): $(BLUE)client$(RESET)\n"
	@rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re
