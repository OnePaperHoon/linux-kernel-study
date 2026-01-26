# **************************************************************************** #
#                                                                              #
#                                                         :::      ::::::::    #
#    Makefile                                           :+:      :+:    :+:    #
#                                                     +:+ +:+         +:+      #
#    By: janhan <janhan@student.42seoul.kr>         +#+  +:+       +#+         #
#                                                 +#+#+#+#+#+   +#+            #
#    Created: 2024/02/24 13:41:14 by janhan            #+#    #+#              #
#    Updated: 2024/04/20 15:09:44 by janhan           ###   ########.fr        #
#                                                                              #
# **************************************************************************** #

NAME					=	minishell

CC						=	cc
CFLAGS					=	-Wall -Wextra -Werror #-fsanitize=address
RM						=	rm -f

# READLINE_LINK			=	-l readline
#	FOR M1 MAC
READLINE_LINK			=	-l readline -L/opt/homebrew/opt/readline/lib

INCLUDES				=	includes/


LIBFT_DIR				=	srcs/libft
LIBFT					=	libft.a
LIBFT_A					=	$(LIBFT_DIR)/libft.a

SRCS					=	srcs/main.c										\
							srcs/0_init_utils/ft_init_exec.c				\
							srcs/0_init_utils/ft_init.c						\
							srcs/0_init_utils/ft_mini_ev_init.c				\
							srcs/0_init_utils/ft_sig_init.c					\
							srcs/1_parsing/ft_convert_child.c				\
							srcs/1_parsing/ft_convert_env.c					\
							srcs/1_parsing/ft_count_token.c					\
							srcs/1_parsing/ft_make_token.c					\
							srcs/1_parsing/ft_parse.c						\
							srcs/1_parsing/ft_remove_quote.c				\
							srcs/1_parsing/ft_syntax_check.c				\
							srcs/1_parsing/ft_tokenization.c				\
							srcs/2_exec_init/ft_exec_init.c					\
							srcs/2_exec_init/ft_set_exec_info.c				\
							srcs/3_exec/ft_check_here_doc.c					\
							srcs/3_exec/ft_error_handle.c					\
							srcs/3_exec/ft_exec_builtin_parent.c			\
							srcs/3_exec/ft_exec_builtin.c					\
							srcs/3_exec/ft_exec_cmd.c						\
							srcs/3_exec/ft_exec.c							\
							srcs/3_exec/ft_here_doc_loop_check.c			\
							srcs/3_exec/ft_set_pipe_fd.c					\
							srcs/3_exec/ft_set_redirect_fd.c				\
							srcs/3_exec/ft_space_handle.c					\
							srcs/4_builtin/ft_cd_builtin.c					\
							srcs/4_builtin/ft_cd_builtin_sup.c				\
							srcs/4_builtin/ft_echo_builtin.c				\
							srcs/4_builtin/ft_env_builtin.c					\
							srcs/4_builtin/ft_export_builtin.c				\
							srcs/4_builtin/ft_exit_builtin.c				\
							srcs/4_builtin/ft_export_builtin.c				\
							srcs/4_builtin/ft_pwd_builtin.c					\
							srcs/4_builtin/ft_unset_builtin.c				\
							srcs/5_list_utils/ft_list_clear.c				\
							srcs/5_list_utils/ft_list_del_node.c			\
							srcs/5_list_utils/ft_list_init.c				\
							srcs/5_list_utils/ft_list_push_back.c			\
							srcs/6_utils/ft_cmd_is_directory.c				\
							srcs/6_utils/ft_error.c							\
							srcs/6_utils/ft_free_all.c						\
							srcs/6_utils/ft_free_exec.c						\
							srcs/6_utils/ft_free_tokens.c					\
							srcs/6_utils/ft_is_builtin_parent.c				\
							srcs/6_utils/ft_is_builtin.c					\
							srcs/6_utils/ft_is_child_exit_code.c			\
							srcs/6_utils/ft_is_env.c						\
							srcs/6_utils/ft_is_heredoc.c					\
							srcs/6_utils/ft_is_operator.c					\
							srcs/6_utils/ft_is_quote.c						\
							srcs/6_utils/ft_is_redirect.c					\
							srcs/6_utils/ft_is_space.c						\
							srcs/6_utils/ft_perror.c						\
							srcs/6_utils/ft_print_logo.c					\
							srcs/7_subs_env//ft_count_dallor.c				\
							srcs/7_subs_env/ft_get_env.c					\
							srcs/7_subs_env/ft_substitute_env.c				\
							srcs/gnl/get_next_line_utils.c					\
							srcs/gnl/get_next_line.c

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
	@make -C $(LIBFT_DIR) -s
	@$(CC) $(CFLAGS) $(LIBFT_A)  $^ -o $@ $(READLINE_LINK)
	@echo $(CURSIVE)$(YELLOW) "      - Compiling $(NAME) -" $(NONE)
	@echo $(GREEN) "      - Minishell Complete -"$(NONE)

%.o : %.c
	@echo $(CURSIVE)$(YELLOW) "      - Making object files -" $(NONE)
	@$(CC) $(CFLAGS) -I/opt/homebrew/opt/readline/include -c $< -o $@

clean :
	@rm -fr $(OBJS_MAND)
	@make clean -C $(LIBFT_DIR)
	@echo $(CURSIVE)$(BLUE) "      - clean OBJ files -" $(NONE)

fclean : clean
	@rm -fr $(NAME)
	@make fclean -C $(LIBFT_DIR)
	@echo $(CURSIVE)$(PURPLE)"       - clean $(NAME) file -"$(NONE)

re	:
	@make fclean
	@make all

.PHONY: all make clean fclean bonus re
