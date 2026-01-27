NAME			=	read_proc

CC				=	cc
CFLAGS			=	-Wall -Wextra -Werror #-fsanitize=address
RM				=	rm -f

INCLUDES		=	

SRCS			=	main.c

OBJS_MAND		=	$(SRCS:.c=.o)

NONE='\033[0m'
GREEN='\033[32m'
YELLOW='\033[33m'
GRAY='\033[2;37m'
CURSIVE='\033[3m'
PURPLE='\033[35m'
BLUE='\033[34m'
DELETELINE='\033[K;

all : $(NAME)

$(NAME) : $(OBJS_MAND)
	@echo $(CURSIVE)$(YELLOW) "      - Making $(NAME) -" $(NONE)
	@$(CC) $(CFLAGS) $^ -lm -o $@ 
	@echo $(CURSIVE)$(YELLOW) "      - Compiling $(NAME) -" $(NONE)
	@echo $(GREEN) "      - read_proc Complete -"$(NONE)

%.o : %.c $(INCLUDES)
	@echo $(CURSIVE)$(YELLOW) "      - Making object files -" $(NONE)
	@$(CC) $(CFLAGS) -c $< -o $@

clean :
	@rm -fr $(OBJS_MAND)
	@echo $(CURSIVE)$(BLUE) "      - clean OBJ files -" $(NONE)

fclean : clean
	@rm -fr $(NAME)
	@echo $(CURSIVE)$(PURPLE)"       - clean $(NAME) file -"$(NONE)

re	:
	@make fclean
	@make all

.PHONY: all make clean fclean bonus re