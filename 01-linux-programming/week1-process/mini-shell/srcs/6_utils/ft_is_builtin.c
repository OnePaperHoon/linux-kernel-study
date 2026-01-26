/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ft_is_builtin.c                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: janhan <janhan@student.42seoul.kr>         +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/03/19 11:14:49 by janhan            #+#    #+#             */
/*   Updated: 2024/03/27 22:14:23 by janhan           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../includes/minishell.h"

int	ft_is_builtin(t_exec_info *exec_info)
{
	if (ft_strncmp(exec_info->cmd[0], "cd", ft_strlen("cd") + 1) == 0
		|| ft_strncmp(exec_info->cmd[0], "export", ft_strlen("export") + 1) == 0
		|| ft_strncmp(exec_info->cmd[0], "unset", ft_strlen("unset") + 1) == 0
		|| ft_strncmp(exec_info->cmd[0], "exit", ft_strlen("exit") + 1) == 0
		|| ft_strncmp(exec_info->cmd[0], "echo", ft_strlen("echo") + 1) == 0
		|| ft_strncmp(exec_info->cmd[0], "pwd", ft_strlen("pwd") + 1) == 0
		|| ft_strncmp(exec_info->cmd[0], "env", ft_strlen("env") + 1) == 0)
		return (TRUE);
	return (FALSE);
}
