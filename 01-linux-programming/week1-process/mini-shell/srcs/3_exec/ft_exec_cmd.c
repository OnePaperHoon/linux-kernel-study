/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ft_exec_cmd.c                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: janhan <janhan@student.42seoul.kr>         +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/03/19 13:01:16 by janhan            #+#    #+#             */
/*   Updated: 2024/04/20 22:25:01 by sangshin         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../includes/minishell.h"

static int	ft_access_path(t_exec *exec, t_exec_info *exec_info)
{
	size_t	i;
	char	*cmd_path;

	cmd_path = exec_info->cmd_path;
	i = 0;
	while (exec->path_ev[i])
	{
		cmd_path = ft_make_path(exec_info, exec->path_ev[i]);
		if (access(cmd_path, X_OK) == SUCCESS)
		{
			exec_info->cmd_path = cmd_path;
			return (SUCCESS);
		}
		free(cmd_path);
		i++;
	}
	return (FAILURE);
}

static int	ft_find_cmd(t_exec *exec, t_exec_info *exec_info, t_parse *parse)
{
	char	*cmd_path;

	cmd_path = exec_info->cmd_path;
	if (ft_cmd_error_sup(exec_info) == FAILURE)
		return (FAILURE);
	if (parse->dquote_flag == TRUE && cmd_path[0] == 0)
		return (FAILURE);
	if (ft_find_error(cmd_path, exec, exec_info) == SUCCESS)
		return (SUCCESS);
	if (ft_access_path(exec, exec_info) == SUCCESS)
		return (SUCCESS);
	return (FAILURE);
}

static void	ft_set_fd(t_exec *exec, t_exec_info *exec_info)
{
	ft_set_pipe_fd(exec, exec_info);
	ft_set_redirect_fd(exec_info);
	if (exec_info->infile_fd != NONE)
	{
		if (dup2(exec_info->infile_fd, STDIN_FILENO) == FAILURE)
			exit(ft_perror(errno));
		if (close(exec_info->infile_fd) == FAILURE)
			exit(ft_perror(errno));
	}
	if (exec_info->outfile_fd != NONE)
	{
		if (dup2(exec_info->outfile_fd, STDOUT_FILENO) == FAILURE)
			exit(ft_perror(errno));
		if (close(exec_info->outfile_fd) == FAILURE)
			exit(ft_perror(errno));
	}
}

static char	**ft_make_envp(t_list *mini_envp)
{
	char	**envp;
	size_t	count;
	t_node	*node;

	count = 0;
	node = mini_envp->front_node;
	if (node == NULL)
		return (NULL);
	while (node->next_node != NULL)
	{
		count++;
		node = node->next_node;
	}
	envp = (char **)malloc(sizeof(char *) * (count + 1));
	if (envp == NULL)
		exit(ft_error("envp malloc failed\n", EXIT_FAILURE));
	count = 0;
	node = mini_envp->front_node;
	while (node->next_node != NULL)
	{
		envp[count++] = (char *)node->content;
		node = node->next_node;
	}
	envp[count] = NULL;
	return (envp);
}

void	ft_exec_cmd(t_info *info, t_parse *parse,
	t_exec *exec, t_exec_info *exec_info)
{
	tcgetattr(STDIN_FILENO, &info->termios);
	info->termios.c_lflag |= ECHOCTL;
	tcsetattr(STDIN_FILENO, TCSANOW, &info->termios);
	ft_set_fd(exec, exec_info);
	if (ft_find_cmd(exec, exec_info, parse) == FAILURE
		&& ft_is_builtin(exec_info) == FALSE)
	{
		ft_printf_err("minishell: %s: command not found\n", exec_info->cmd[0]);
		ft_free_all(parse, exec);
		exit(127);
	}
	else
	{
		ft_cmd_null_handle(exec_info);
		if (exec_info->cmd_path == NULL)
			exit(EXIT_SUCCESS);
		if (ft_is_builtin(exec_info) == TRUE)
			ft_exec_builtin(info, parse, exec, exec_info);
		else
			execve(exec_info->cmd_path, exec_info->cmd,
				ft_make_envp(&info->mini_ev));
	}
}
