/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   minishell.h                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: janhan <janhan@student.42seoul.kr>         +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/02/24 00:07:42 by janhan            #+#    #+#             */
/*   Updated: 2024/04/20 22:51:04 by sangshin         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef MINISHELL_H
# define MINISHELL_H

# include <stdio.h>
# include <stdlib.h>
# include <sys/_types/_pid_t.h>
# include <sys/_types/_size_t.h>
# include <sys/termios.h>
# include <sys/syslimits.h>
# include <unistd.h>
# include <fcntl.h>
# include <readline/readline.h>
# include <readline/history.h>
# include <signal.h>
# include <sys/stat.h>
# include <dirent.h>
# include <string.h>
# include <termios.h>
# include <errno.h>
# include "libft.h"
# include "get_next_line.h"

# define NOT_HOME 1
# define TRUE 1
# define FALSE 0
# define SUCCESS 0
# define FAILURE -1
# define CHILD_PID 0
# define IN 0
# define OUT 1
# define NONE -1

/* GLOBAL VARIABLE */
extern int	g_child_exit_code;

/* LIST STRUCT */
typedef struct s_node
{
	void			*content;
	int				visit;
	struct s_node	*next_node;
}	t_node;

typedef struct s_list
{
	t_node			*front_node;
}	t_list;

/* TOKEN STRUCT */
typedef enum e_token_type
{
	WORD,
	PIPE,
	REDIRECT,
}	t_token_type;

typedef struct s_token
{
	t_token_type	type;
	char			*original;
	char			*str;
	int				env_flag;
}	t_token;

/* INFO STRUCT */
typedef struct s_info
{
	int				ac;
	char			**av;
	char			**ev;
	t_list			mini_ev;
	struct termios	termios_backup;
	struct termios	termios;
}	t_info;

/* PARSE STRUCT */
typedef struct s_parse
{
	char			*line;
	size_t			line_index;
	size_t			token_count;
	t_token			*tokens;
	size_t			token_index;
	t_token			*token;
	char			*temp_str;
	size_t			temp_str_len;
	size_t			start_index;
	ssize_t			str_index;
	int				squote_flag;
	int				dquote_flag;
	char			*target_ev;
	char			*ev_val;
	size_t			ev_val_len;
	char			*old_str;
	char			*new_str;
	char			*exit_code;
}	t_parse;

/* REDIRECT STRUCT */
typedef enum s_redirect_type
{
	OUT1,
	OUT2,
	IN1,
	HERE_DOC,
}	t_redirect_type;

typedef struct s_redirect
{
	t_redirect_type		type;
	char				*value;
	char				*original;
}	t_redirect;

/* EXEC STRUCT */
typedef struct s_exec_info
{
	char			*cmd_path;
	char			**cmd;
	size_t			cmd_index;
	t_redirect		*redirect;
	size_t			redirect_index;
	pid_t			pid;
	int				use_pipe;
	int				pipe_fd[2];
	int				infile_fd;
	int				outfile_fd;
	int				builtin_parent;
}	t_exec_info;

typedef struct s_exec
{
	t_exec_info	*exec_arr;
	size_t		exec_arr_size;
	size_t		exec_arr_index;
	int			prev_pipe_fd;
	char		**path_ev;
	pid_t		current_child_pid;
}	t_exec;

/* 0_INIT UTILS */
void	ft_init(int ac, char **av, char **ev, t_info *info);
void	ft_mini_ev_init(char **ev, t_info *info);
int		ft_init_exec(t_info *info, t_parse *parse, t_exec *exec);
void	ft_sig_init(t_info *info);
void	ft_signal_2(int sig);
void	ft_sigpipe_handler(int signum);
void	ft_sig_for_here_doc_parent(int sig);
void	ft_sig_for_here_doc_child(int sig);
void	ft_print_logo(void);

/* 1_PARSING */
int		ft_parse(t_info *info, t_parse *parse);
int		ft_count_token(t_parse *parse);
int		ft_convert_child_exit_code(t_parse *parse);
int		ft_convert_env(t_info *info, t_parse *parse);
int		ft_make_token(t_parse *parse, t_token_type type);
int		ft_tokenization(t_parse *parse);
int		ft_syntax_check(t_parse *parse);
void	ft_remove_quote(t_parse *parse);

/* 2_exec_init */
int		ft_set_exec_info(t_parse *parse, t_exec_info *exec_info);
int		ft_make_exec_info(t_info *info, t_parse *parse, t_exec *exec);

/* 3_exec */
int		ft_exec(t_info *info, t_parse *parse, t_exec *exec);
int		ft_here_doc_loop_check(t_exec *exec);
int		ft_check_here_doc(t_exec *exec);
void	ft_cmd_path_error_handle(t_exec_info *exec_info, char *cmd_path);
void	ft_cmd_null_handle(t_exec_info *exec_info);
int		ft_space_handle(t_exec_info *exec_info);
char	*ft_make_path(t_exec_info *exec_info, char *path);
int		ft_exec_builtin_parent(t_info *info, t_parse *parse,
			t_exec *exec, t_exec_info *exec_info);
int		ft_exec_builtin(t_info *info, t_parse *parse,
			t_exec *exec, t_exec_info *exec_info);
void	ft_exec_cmd(t_info *info, t_parse *parse,
			t_exec *exec, t_exec_info *exec_info);
void	ft_set_pipe_fd(t_exec *exec, t_exec_info *exec_info);
void	ft_set_redirect_fd(t_exec_info *exec_info);

/* 4_builtin */
int		ft_cd_builtin(t_info *info, t_exec_info *exec_info, t_parse *parse);
t_node	*ft_find_oldpwd(t_info *info);
int		ft_echo_builtin(t_exec_info *exec_info);
int		ft_env_builtin(t_info *info);
int		ft_unset_builtin(t_info *info, t_exec_info *exec_info);
int		ft_exit_builtin(t_list *mini_ev, t_parse *parse,
			t_exec *exec, t_exec_info *exec_info);
int		ft_export_builtin(t_info *info, t_exec_info *exec_info);
int		ft_pwd_builtin(void);

/* 5_LIST UTILS */
t_list	ft_list_init(void);
int		ft_list_push_back(void	*content, t_list *list);
void	ft_list_clear(t_list *list);
int		ft_list_del_node(t_list *list, t_node *node);
void	*list_content_finder(t_node *head, void *content);

/* 6_UTILS */
void	ft_cmd_is_directory(char *cmd_path);
int		ft_error(char *msg, int error_code);
int		ft_find_error(char *cmd_path, t_exec *exec,
			t_exec_info *exec_info);
int		ft_cmd_error_sup(t_exec_info *exec_info);
void	ft_free_tokens(t_parse *parse, size_t token_size);
int		ft_is_child_exit_code(t_parse *parse);
int		ft_is_quote(char c);
int		ft_is_operator(char c);
int		ft_is_redirect(char c);
int		ft_is_space(char c);
int		ft_is_heredoc(char c1, char c2);
int		ft_is_env(t_info *info, t_parse *parse);
int		ft_is_builtin_parent(t_exec *exec, t_exec_info *exec_info);
int		ft_is_builtin(t_exec_info *exec_info);
int		ft_perror(int error_code);
void	ft_free_exec(t_exec *exec, size_t exec_arr_i);
void	ft_free_all(t_parse *parse, t_exec *exec);

/* 7_SUBSTITUTE ENV */
int		ft_substitute_env(t_info *info, t_parse *parse);
int		ft_is_in_single_quote(char *line, int i);
char	*ft_get_env_name(char *line);
char	*ft_get_env_value(t_info *info, char *env_name);
int		ft_is_special_dallor(char *line);
int		ft_count_dallor(char *line);
#endif
