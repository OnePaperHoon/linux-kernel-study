SIG_BASIC		=	signal_basic
SIG_PC			=	signal_parent_child

NAMES			=	$(SIG_BASIC) $(SIG_PC)

CC				=	cc
CFLAGS			=	-Wall -Wextra -Werror
DEBUG_FLAGS		=	-g
SANITIZE_FLAGS	=	-fsanitize=address
RM				=	rm -f

# Colors
NONE			=	'\033[0m'
GREEN			=	'\033[32m'
YELLOW			=	'\033[33m'
GRAY			=	'\033[2;37m'
CURSIVE			=	'\033[3m'
PURPLE			=	'\033[35m'
BLUE			=	'\033[34m'
RED				=	'\033[31m'

all: $(NAMES)

$(SIG_BASIC): signal_basic.c
	@echo $(CURSIVE)$(GRAY)"      - Compiling $< -"$(NONE)
	@$(CC) $(CFLAGS) $< -o $@
	@echo $(GREEN)"      - $@ Complete! -"$(NONE)

$(SIG_PC): signal_parent_child.c
	@echo $(CURSIVE)$(GRAY)"      - Compiling $< -"$(NONE)
	@$(CC) $(CFLAGS) $< -o $@
	@echo $(GREEN)"      - $@ Complete! -"$(NONE)

clean:
	@$(RM) *.o
	@echo $(CURSIVE)$(BLUE)"      - Cleaned object files -"$(NONE)

fclean: clean
	@$(RM) $(NAMES)
	@echo $(CURSIVE)$(PURPLE)"      - Cleaned binaries -"$(NONE)

re: fclean all

# Development targets
debug: CFLAGS += $(DEBUG_FLAGS)
debug: re
	@echo $(PURPLE)"      - Debug mode enabled -"$(NONE)

sanitize: CFLAGS += $(SANITIZE_FLAGS)
sanitize: re
	@echo $(RED)"      - Address sanitizer enabled -"$(NONE)

# Testing targets
test: $(SIG_BASIC)
	@echo $(BLUE)"      - Running $(SIG_BASIC) (Ctrl+C 3번으로 종료) -"$(NONE)
	@./$(SIG_BASIC)

test-pc: $(SIG_PC)
	@echo $(BLUE)"      - Running $(SIG_PC) -"$(NONE)
	@./$(SIG_PC)

run: test

valgrind: $(SIG_PC)
	@echo $(BLUE)"      - Running Valgrind (signal_parent_child) -"$(NONE)
	@valgrind --leak-check=full --show-leak-kinds=all ./$(SIG_PC)

leak: valgrind

memcheck: valgrind

# Help
help:
	@echo "$(YELLOW)Available targets:$(NONE)"
	@echo "  $(GREEN)make$(NONE)            - Build all (signal_basic, signal_parent_child)"
	@echo "  $(GREEN)make re$(NONE)         - Rebuild all"
	@echo "  $(GREEN)make clean$(NONE)      - Remove object files"
	@echo "  $(GREEN)make fclean$(NONE)     - Remove all build files"
	@echo ""
	@echo "$(YELLOW)Testing:$(NONE)"
	@echo "  $(BLUE)make test$(NONE)        - signal_basic 실행 (Ctrl+C 3번으로 종료)"
	@echo "  $(BLUE)make test-pc$(NONE)     - signal_parent_child 실행"
	@echo "  $(BLUE)make valgrind$(NONE)    - Memory leak check"
	@echo ""
	@echo "$(YELLOW)Debug:$(NONE)"
	@echo "  $(PURPLE)make debug$(NONE)      - Build with debug symbols"
	@echo "  $(RED)make sanitize$(NONE)   - Build with address sanitizer"

.PHONY: all clean fclean re debug sanitize test test-pc run valgrind leak memcheck help
