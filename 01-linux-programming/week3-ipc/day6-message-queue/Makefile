MQ_BASIC		=	mq_basic
MQ_SENDER		=	mq_sender
MQ_RECEIVER		=	mq_receiver

NAMES			=	$(MQ_BASIC) $(MQ_SENDER) $(MQ_RECEIVER)

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

$(MQ_BASIC): mq_basic.c
	@echo $(CURSIVE)$(GRAY)"      - Compiling $< -"$(NONE)
	@$(CC) $(CFLAGS) $< -o $@
	@echo $(GREEN)"      - $@ Complete! -"$(NONE)

$(MQ_SENDER): mq_sender.c
	@echo $(CURSIVE)$(GRAY)"      - Compiling $< -"$(NONE)
	@$(CC) $(CFLAGS) $< -o $@
	@echo $(GREEN)"      - $@ Complete! -"$(NONE)

$(MQ_RECEIVER): mq_receiver.c
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

# Testing targets (mq_basic: fork 기반 단일 바이너리 데모)
test: $(MQ_BASIC)
	@echo $(BLUE)"      - Running $(MQ_BASIC) -"$(NONE)
	@./$(MQ_BASIC)

run: test

valgrind: $(MQ_BASIC)
	@echo $(BLUE)"      - Running Valgrind -"$(NONE)
	@valgrind --leak-check=full --show-leak-kinds=all ./$(MQ_BASIC)

leak: valgrind

memcheck: valgrind

# Help
help:
	@echo "$(YELLOW)Available targets:$(NONE)"
	@echo "  $(GREEN)make$(NONE)           - Build all (mq_basic, mq_sender, mq_receiver)"
	@echo "  $(GREEN)make re$(NONE)        - Rebuild all"
	@echo "  $(GREEN)make clean$(NONE)     - Remove object files"
	@echo "  $(GREEN)make fclean$(NONE)    - Remove all build files"
	@echo ""
	@echo "$(YELLOW)Testing:$(NONE)"
	@echo "  $(BLUE)make test$(NONE)       - Run mq_basic (fork 기반 데모)"
	@echo "  $(BLUE)make valgrind$(NONE)   - Memory leak check"
	@echo ""
	@echo "$(YELLOW)IPC Demo (터미널 2개):$(NONE)"
	@echo "  Terminal 1: $(BLUE)./mq_receiver$(NONE)  ← 먼저 실행"
	@echo "  Terminal 2: $(BLUE)./mq_sender$(NONE)"
	@echo ""
	@echo "$(YELLOW)Debug:$(NONE)"
	@echo "  $(PURPLE)make debug$(NONE)     - Build with debug symbols"
	@echo "  $(RED)make sanitize$(NONE)  - Build with address sanitizer"

.PHONY: all clean fclean re debug sanitize test run valgrind leak memcheck help
