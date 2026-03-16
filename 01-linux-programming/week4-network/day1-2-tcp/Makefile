SERVER			=	tcp_echo_server
CLIENT			=	tcp_echo_client
MULTI_CLIENT	=	tcp_multi_client

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

all: $(SERVER) $(CLIENT) $(MULTI_CLIENT)

# tcp_multi_client는 pthread를 사용하므로 -pthread 링크 필요
$(MULTI_CLIENT): tcp_multi_client.c
	@echo $(CURSIVE)$(GRAY)"      - Compiling $< -"$(NONE)
	@$(CC) $(CFLAGS) $< -o $@ -pthread
	@echo $(GREEN)"      - $@ Complete! -"$(NONE)

# 나머지는 단일 소스 파일로 컴파일
$(SERVER): tcp_echo_server.c
	@echo $(CURSIVE)$(GRAY)"      - Compiling $< -"$(NONE)
	@$(CC) $(CFLAGS) $< -o $@
	@echo $(GREEN)"      - $@ Complete! -"$(NONE)

$(CLIENT): tcp_echo_client.c
	@echo $(CURSIVE)$(GRAY)"      - Compiling $< -"$(NONE)
	@$(CC) $(CFLAGS) $< -o $@
	@echo $(GREEN)"      - $@ Complete! -"$(NONE)

clean:
	@$(RM) *.o
	@echo $(CURSIVE)$(BLUE)"      - Cleaned object files -"$(NONE)

fclean: clean
	@$(RM) $(SERVER) $(CLIENT) $(MULTI_CLIENT)
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
	@echo $(BLUE)"      - Run server in background, then test clients -"$(NONE)
	@echo $(YELLOW)"      Usage:"$(NONE)
	@echo "        Terminal 1: ./$(SERVER)"
	@echo "        Terminal 2: ./$(CLIENT)"
	@echo "        Terminal 2: ./$(MULTI_CLIENT)"

valgrind: $(SERVER)
	@echo $(BLUE)"      - Running Valgrind on $(SERVER) -"$(NONE)
	@valgrind --leak-check=full --show-leak-kinds=all ./$(SERVER)

leak: valgrind
memcheck: valgrind

# Help
help:
	@echo "$(YELLOW)Available targets:$(NONE)"
	@echo "  $(GREEN)make$(NONE)              - Build all (server, client, multi_client)"
	@echo "  $(GREEN)make re$(NONE)           - Rebuild all"
	@echo "  $(GREEN)make clean$(NONE)        - Remove object files"
	@echo "  $(GREEN)make fclean$(NONE)       - Remove all binaries"
	@echo ""
	@echo "$(YELLOW)Testing:$(NONE)"
	@echo "  $(BLUE)make test$(NONE)          - Show usage instructions"
	@echo "  $(BLUE)make valgrind$(NONE)      - Check memory leaks (server)"
	@echo ""
	@echo "$(YELLOW)Debug:$(NONE)"
	@echo "  $(PURPLE)make debug$(NONE)         - Build with debug symbols"
	@echo "  $(RED)make sanitize$(NONE)      - Build with address sanitizer"

.PHONY: all clean fclean re debug sanitize test valgrind leak memcheck help
