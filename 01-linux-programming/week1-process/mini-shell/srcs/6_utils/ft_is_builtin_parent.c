/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ft_is_builtin_parent.c                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: janhan <janhan@student.42seoul.kr>         +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/03/19 11:16:21 by janhan            #+#    #+#             */
/*   Updated: 2024/03/25 21:49:45 by sangshin         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../includes/minishell.h"

int	ft_is_builtin_parent(t_exec *exec, t_exec_info *exec_info)
{
	if (exec->exec_arr_index != 0 || exec_info->use_pipe == TRUE)
		return (FALSE);
	if (exec_info->cmd_path == NULL)
		return (FALSE);
	if (ft_strncmp(exec_info->cmd[0], "cd", ft_strlen("cd") + 1) == 0
		|| (ft_strncmp(exec_info->cmd[0], "export",
				ft_strlen("export") + 1) == 0 && exec_info->cmd[1])
		|| ft_strncmp(exec_info->cmd[0], "unset", ft_strlen("unset") + 1) == 0
		|| ft_strncmp(exec_info->cmd[0], "exit", ft_strlen("exit") + 1) == 0)
		return (TRUE);
	return (FALSE);
}
