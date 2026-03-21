SELECT_SERVER	=	select_echo_server
POLL_SERVER		=	poll_echo_server
EPOLL_SERVER	=	epoll_echo_server
BENCHMARK		=	benchmark

CC				=	cc
CFLAGS			=	-Wall -Wextra -Werror
DEBUG_FLAGS		=	-g
SANITIZE_FLAGS	=	-fsanitize=address
RM				=	rm -f

ALL_BINS		=	$(SELECT_SERVER) $(POLL_SERVER) $(EPOLL_SERVER) $(BENCHMARK)

# Colors
NONE			=	'\033[0m'
GREEN			=	'\033[32m'
YELLOW			=	'\033[33m'
GRAY			=	'\033[2;37m'
CURSIVE			=	'\033[3m'
PURPLE			=	'\033[35m'
BLUE			=	'\033[34m'
RED				=	'\033[31m'

all: $(ALL_BINS)

# epoll은 Linux 전용 — macOS에서는 링크만 되고 실행 시 에러남
$(SELECT_SERVER): select_echo_server.c
	@echo $(CURSIVE)$(GRAY)"      - Compiling $< -"$(NONE)
	@$(CC) $(CFLAGS) $< -o $@
	@echo $(GREEN)"      - $@ Complete! -"$(NONE)

$(POLL_SERVER): poll_echo_server.c
	@echo $(CURSIVE)$(GRAY)"      - Compiling $< -"$(NONE)
	@$(CC) $(CFLAGS) $< -o $@
	@echo $(GREEN)"      - $@ Complete! -"$(NONE)

$(EPOLL_SERVER): epoll_echo_server.c
	@echo $(CURSIVE)$(GRAY)"      - Compiling $< -"$(NONE)
	@$(CC) $(CFLAGS) $< -o $@
	@echo $(GREEN)"      - $@ Complete! -"$(NONE)

$(BENCHMARK): benchmark.c
	@echo $(CURSIVE)$(GRAY)"      - Compiling $< -"$(NONE)
	@$(CC) $(CFLAGS) $< -o $@ -pthread
	@echo $(GREEN)"      - $@ Complete! -"$(NONE)

clean:
	@$(RM) *.o
	@echo $(CURSIVE)$(BLUE)"      - Cleaned object files -"$(NONE)

fclean: clean
	@$(RM) $(ALL_BINS)
	@echo $(CURSIVE)$(PURPLE)"      - Cleaned all binaries -"$(NONE)

re: fclean all

# Development targets
debug: CFLAGS += $(DEBUG_FLAGS)
debug: re
	@echo $(PURPLE)"      - Debug mode enabled -"$(NONE)

sanitize: CFLAGS += $(SANITIZE_FLAGS)
sanitize: re
	@echo $(RED)"      - Address sanitizer enabled -"$(NONE)

# Testing targets
test: all
	@echo $(BLUE)"      - Servers built. Run each in a separate terminal: -"$(NONE)
	@echo $(YELLOW)"      Usage:"$(NONE)
	@echo "        Terminal 1: ./$(SELECT_SERVER)   (select 기반)"
	@echo "        Terminal 1: ./$(POLL_SERVER)     (poll 기반)"
	@echo "        Terminal 1: ./$(EPOLL_SERVER)    (epoll 기반, Linux only)"
	@echo "        Terminal 2: ./$(BENCHMARK)       (성능 비교 벤치마크)"

valgrind: $(SELECT_SERVER)
	@echo $(BLUE)"      - Running Valgrind on $(SELECT_SERVER) -"$(NONE)
	@valgrind --leak-check=full --show-leak-kinds=all ./$(SELECT_SERVER)

leak: valgrind
memcheck: valgrind

# Help
help:
	@echo "$(YELLOW)Available targets:$(NONE)"
	@echo "  $(GREEN)make$(NONE)              - Build all servers + benchmark"
	@echo "  $(GREEN)make re$(NONE)           - Rebuild all"
	@echo "  $(GREEN)make clean$(NONE)        - Remove object files"
	@echo "  $(GREEN)make fclean$(NONE)       - Remove all binaries"
	@echo ""
	@echo "$(YELLOW)Binaries:$(NONE)"
	@echo "  $(BLUE)./$(SELECT_SERVER)$(NONE)   - select() 기반 에코 서버"
	@echo "  $(BLUE)./$(POLL_SERVER)$(NONE)     - poll() 기반 에코 서버"
	@echo "  $(BLUE)./$(EPOLL_SERVER)$(NONE)    - epoll() 기반 에코 서버 (Linux only)"
	@echo "  $(BLUE)./$(BENCHMARK)$(NONE)       - select/poll/epoll 성능 비교"
	@echo ""
	@echo "$(YELLOW)Debug:$(NONE)"
	@echo "  $(PURPLE)make debug$(NONE)         - Build with debug symbols"
	@echo "  $(RED)make sanitize$(NONE)      - Build with address sanitizer"

.PHONY: all clean fclean re debug sanitize test valgrind leak memcheck help
