/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ft_error_handle.c                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: janhan <janhan@student.42seoul.kr>         +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/03/29 02:25:00 by janhan            #+#    #+#             */
/*   Updated: 2024/04/20 22:53:02 by sangshin         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../includes/minishell.h"

void	ft_cmd_path_error_handle(t_exec_info *exec_info, char *cmd_path)
{
	if (access(cmd_path, F_OK) == FAILURE)
	{
		ft_printf_err("minishell: %s: No such file or directory\n",
			exec_info->cmd[0]);
		exit(127);
	}
	if (access(cmd_path, X_OK) == FAILURE)
	{
		ft_printf_err("minishell: %s: Permission denied\n",
			exec_info->cmd[0]);
		exit(126);
	}
}

void	ft_cmd_null_handle(t_exec_info *exec_info)
{
	if (exec_info->use_pipe == 1)
		if (exec_info->cmd_index > 1
			&& exec_info->cmd[exec_info->cmd_index - 1] != NULL
			&& exec_info->cmd[exec_info->cmd_index - 1][0] == '\0')
			exec_info->cmd[exec_info->cmd_index - 1] = NULL;
	if (exec_info->cmd[exec_info->cmd_index] != NULL
		&& exec_info->cmd[exec_info->cmd_index][0] == '\0')
		exec_info->cmd[exec_info->cmd_index] = NULL;
}

char	*ft_make_path(t_exec_info *exec_info, char *path)
{
	char	*result;
	char	*temp;

	temp = ft_strjoin(path, "/");
	if (temp == NULL)
		exit(EXIT_FAILURE);
	result = ft_strjoin(temp, exec_info->cmd_path);
	if (result == NULL)
	{
		free(temp);
		exit(EXIT_FAILURE);
	}
	free(temp);
	return (result);
}

int	ft_find_error(char *cmd_path, t_exec *exec, t_exec_info *exec_info)
{
	if (cmd_path == NULL)
		return (SUCCESS);
	if (ft_strchr(cmd_path, '/') != 0)
	{
		ft_cmd_is_directory(cmd_path);
		ft_cmd_path_error_handle(exec_info, cmd_path);
		if (access(cmd_path, X_OK) == SUCCESS)
			return (SUCCESS);
	}
	if ((exec->path_ev[0] == NULL && !ft_is_builtin(exec_info)))
	{
		if (access(cmd_path, X_OK) == SUCCESS)
			return (SUCCESS);
		ft_printf_err("minishell: %s: No such file or directory\n",
			exec_info->cmd[0]);
		exit(127);
	}
	return (FAILURE);
}

int	ft_cmd_error_sup(t_exec_info *exec_info)
{
	if (exec_info->cmd_path[0] == '.'
		&& exec_info->cmd_path[1] == '.'
		&& exec_info->cmd_path[2] == '\0')
	{
		return (FAILURE);
	}
	else if (exec_info->cmd_path[0] == '.'
		&& exec_info->cmd_path[1] == '\0')
	{
		ft_printf_err("minishell: .: filename argument required\n\
.: usage: . filename [arguments]\n");
		exit(2);
	}
	return (SUCCESS);
}
