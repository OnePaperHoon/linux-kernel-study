TARGETS		=	udp_server udp_client udp_broadcast

CC			=	cc
CFLAGS		=	-Wall -Wextra -Werror
DEBUG_FLAGS	=	-g
SANITIZE_FLAGS	=	-fsanitize=address
RM			=	rm -f

# Colors
NONE		=	'\033[0m'
GREEN		=	'\033[32m'
YELLOW		=	'\033[33m'
GRAY		=	'\033[2;37m'
CURSIVE		=	'\033[3m'
PURPLE		=	'\033[35m'
BLUE		=	'\033[34m'
RED			=	'\033[31m'

all: $(TARGETS)

udp_server: udp_server.c
	@echo $(CURSIVE)$(GRAY)"      - Compiling $< -"$(NONE)
	@$(CC) $(CFLAGS) $< -o $@
	@echo $(GREEN)"      - $@ Complete! -"$(NONE)

udp_client: udp_client.c
	@echo $(CURSIVE)$(GRAY)"      - Compiling $< -"$(NONE)
	@$(CC) $(CFLAGS) $< -o $@
	@echo $(GREEN)"      - $@ Complete! -"$(NONE)

udp_broadcast: udp_broadcast.c
	@echo $(CURSIVE)$(GRAY)"      - Compiling $< -"$(NONE)
	@$(CC) $(CFLAGS) $< -o $@
	@echo $(GREEN)"      - $@ Complete! -"$(NONE)

clean:
	@$(RM) *.o
	@echo $(CURSIVE)$(BLUE)"      - Cleaned object files -"$(NONE)

fclean: clean
	@$(RM) $(TARGETS)
	@echo $(CURSIVE)$(PURPLE)"      - Cleaned all binaries -"$(NONE)

re: fclean all

debug: CFLAGS += $(DEBUG_FLAGS)
debug: re
	@echo $(PURPLE)"      - Debug mode enabled -"$(NONE)

sanitize: CFLAGS += $(SANITIZE_FLAGS)
sanitize: re
	@echo $(RED)"      - Address sanitizer enabled -"$(NONE)

help:
	@echo "$(YELLOW)Targets:$(NONE)"
	@echo "  $(GREEN)udp_server$(NONE)    - UDP echo server (recvfrom/sendto)"
	@echo "  $(GREEN)udp_client$(NONE)    - UDP client with RTT measurement"
	@echo "  $(GREEN)udp_broadcast$(NONE) - UDP broadcast sender/receiver"
	@echo ""
	@echo "$(YELLOW)Usage:$(NONE)"
	@echo "  Terminal 1: $(BLUE)./udp_server$(NONE)"
	@echo "  Terminal 2: $(BLUE)./udp_client$(NONE)"
	@echo "  Terminal 1: $(BLUE)./udp_broadcast receiver$(NONE)"
	@echo "  Terminal 2: $(BLUE)./udp_broadcast sender$(NONE)"

.PHONY: all clean fclean re debug sanitize help
