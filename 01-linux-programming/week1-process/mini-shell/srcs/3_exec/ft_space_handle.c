/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ft_space_handle.c                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: janhan <janhan@student.42seoul.kr>         +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/04/08 15:25:43 by janhan            #+#    #+#             */
/*   Updated: 2024/04/08 15:34:09 by janhan           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../includes/minishell.h"

int	ft_space_handle(t_exec_info *exec_info)
{
	free(exec_info->cmd);
	exec_info->cmd = ft_split(exec_info->cmd_path, ' ');
	if (exec_info->cmd == NULL)
		return (ft_error("Failed malloc exec_info->cmd\n", FAILURE));
	free(exec_info->cmd_path);
	exec_info->cmd_path = ft_strdup(exec_info->cmd[0]);
	if (exec_info->cmd_path == NULL)
		return (ft_error("Failed malloc exec_info->cmd_path\n", FAILURE));
	return (SUCCESS);
}
