NAME			=	pstree

CC				=	cc
CFLAGS			=	-Wall -Wextra -Werror
DEBUG_FLAGS		=	-g
SANITIZE_FLAGS	=	-fsanitize=address
RM				=	rm -f

INCLUDES		=	process.h

SRCS			=	process.c	\
					pstree.c

OBJS			=	$(SRCS:.c=.o)

# Colors
NONE			=	'\033[0m'
GREEN			=	'\033[32m'
YELLOW			=	'\033[33m'
GRAY			=	'\033[2;37m'
CURSIVE			=	'\033[3m'
PURPLE			=	'\033[35m'
BLUE			=	'\033[34m'
RED				=	'\033[31m'

all: $(NAME)

$(NAME): $(OBJS)
	@echo $(CURSIVE)$(YELLOW)"      - Linking $(NAME) -"$(NONE)
	@$(CC) $(CFLAGS) $^ -o $@
	@echo $(GREEN)"      - $(NAME) Complete! -"$(NONE)

%.o: %.c $(INCLUDES)
	@echo $(CURSIVE)$(GRAY)"      - Compiling $< -"$(NONE)
	@$(CC) $(CFLAGS) -c $< -o $@

clean:
	@$(RM) $(OBJS)
	@echo $(CURSIVE)$(BLUE)"      - Cleaned object files -"$(NONE)

fclean: clean
	@$(RM) $(NAME)
	@echo $(CURSIVE)$(PURPLE)"      - Cleaned $(NAME) -"$(NONE)

re: fclean all

# Development targets
debug: CFLAGS += $(DEBUG_FLAGS)
debug: re
	@echo $(PURPLE)"      - Debug mode enabled -"$(NONE)

sanitize: CFLAGS += $(SANITIZE_FLAGS)
sanitize: re
	@echo $(RED)"      - Address sanitizer enabled -"$(NONE)

# Testing targets
test: $(NAME)
	@echo $(BLUE)"      - Running $(NAME) -"$(NONE)
	@./$(NAME)

run: test

valgrind: $(NAME)
	@echo $(BLUE)"      - Running Valgrind -"$(NONE)
	@valgrind --leak-check=full --show-leak-kinds=all ./$(NAME)

leak: valgrind

memcheck: valgrind

# Quick checks
count: $(NAME)
	@./$(NAME) 2>/dev/null | head -1

check: $(NAME)
	@echo $(YELLOW)"=== Quick Check ==="$(NONE)
	@echo $(BLUE)"Process count:"$(NONE)
	@./$(NAME) 2>/dev/null | head -1
	@echo ""
	@echo $(BLUE)"Memory check:"$(NONE)
	@valgrind --leak-check=full ./$(NAME) 2>&1 | grep -E "(in use at exit|ERROR SUMMARY)"

# Help
help:
	@echo "$(YELLOW)Available targets:$(NONE)"
	@echo "  $(GREEN)make$(NONE)          - Build $(NAME)"
	@echo "  $(GREEN)make re$(NONE)       - Rebuild $(NAME)"
	@echo "  $(GREEN)make clean$(NONE)    - Remove object files"
	@echo "  $(GREEN)make fclean$(NONE)   - Remove all build files"
	@echo ""
	@echo "$(YELLOW)Testing:$(NONE)"
	@echo "  $(BLUE)make test$(NONE)      - Run $(NAME)"
	@echo "  $(BLUE)make valgrind$(NONE)  - Check memory leaks"
	@echo "  $(BLUE)make check$(NONE)     - Quick memory check"
	@echo ""
	@echo "$(YELLOW)Debug:$(NONE)"
	@echo "  $(PURPLE)make debug$(NONE)     - Build with debug symbols"
	@echo "  $(RED)make sanitize$(NONE)  - Build with address sanitizer"

.PHONY: all clean fclean re debug sanitize test run valgrind leak \
        memcheck count check help