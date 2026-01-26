/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ft_exec_builtin_parent.c                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: janhan <janhan@student.42seoul.kr>         +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/03/17 09:33:48 by janhan            #+#    #+#             */
/*   Updated: 2024/04/05 08:36:24 by janhan           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../includes/minishell.h"

int	ft_exec_builtin_parent(t_info *info, t_parse *parse,
	t_exec *exec, t_exec_info *exec_info)
{
	exec_info->builtin_parent = TRUE;
	if (ft_strncmp(exec_info->cmd[0], "cd", ft_strlen("cd") + 1) == 0)
		ft_cd_builtin(info, exec_info, parse);
	if (ft_strncmp(exec_info->cmd[0], "export", ft_strlen("export") + 1) == 0)
		ft_export_builtin(info, exec_info);
	if (ft_strncmp(exec_info->cmd[0], "unset", ft_strlen("unset") + 1) == 0)
		ft_unset_builtin(info, exec_info);
	if (ft_strncmp(exec_info->cmd[0], "exit", ft_strlen("exit") + 1) == 0)
		ft_exit_builtin(&info->mini_ev, parse, exec, exec_info);
	return (SUCCESS);
}
