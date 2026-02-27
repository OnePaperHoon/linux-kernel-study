SHM_BASIC		=	shm_basic
SHM_WRITER		=	shm_writer
SHM_READER		=	shm_reader

NAMES			=	$(SHM_BASIC) $(SHM_WRITER) $(SHM_READER)

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

$(SHM_BASIC): shm_basic.c
	@echo $(CURSIVE)$(GRAY)"      - Compiling $< -"$(NONE)
	@$(CC) $(CFLAGS) $< -o $@
	@echo $(GREEN)"      - $@ Complete! -"$(NONE)

$(SHM_WRITER): shm_writer.c
	@echo $(CURSIVE)$(GRAY)"      - Compiling $< -"$(NONE)
	@$(CC) $(CFLAGS) $< -o $@
	@echo $(GREEN)"      - $@ Complete! -"$(NONE)

$(SHM_READER): shm_reader.c
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

# Testing targets (shm_basic: fork 기반 단일 바이너리 데모)
test: $(SHM_BASIC)
	@echo $(BLUE)"      - Running $(SHM_BASIC) -"$(NONE)
	@./$(SHM_BASIC)

run: test

valgrind: $(SHM_BASIC)
	@echo $(BLUE)"      - Running Valgrind -"$(NONE)
	@valgrind --leak-check=full --show-leak-kinds=all ./$(SHM_BASIC)

leak: valgrind

memcheck: valgrind

# Help
help:
	@echo "$(YELLOW)Available targets:$(NONE)"
	@echo "  $(GREEN)make$(NONE)           - Build all (shm_basic, shm_writer, shm_reader)"
	@echo "  $(GREEN)make re$(NONE)        - Rebuild all"
	@echo "  $(GREEN)make clean$(NONE)     - Remove object files"
	@echo "  $(GREEN)make fclean$(NONE)    - Remove all build files"
	@echo ""
	@echo "$(YELLOW)Testing:$(NONE)"
	@echo "  $(BLUE)make test$(NONE)       - Run shm_basic (fork 기반 데모)"
	@echo "  $(BLUE)make valgrind$(NONE)   - Memory leak check"
	@echo ""
	@echo "$(YELLOW)IPC Demo (터미널 2개):$(NONE)"
	@echo "  Terminal 1: $(BLUE)./shm_reader$(NONE)  ← 먼저 실행"
	@echo "  Terminal 2: $(BLUE)./shm_writer$(NONE)"
	@echo ""
	@echo "$(YELLOW)Debug:$(NONE)"
	@echo "  $(PURPLE)make debug$(NONE)     - Build with debug symbols"
	@echo "  $(RED)make sanitize$(NONE)  - Build with address sanitizer"

.PHONY: all clean fclean re debug sanitize test run valgrind leak memcheck help
