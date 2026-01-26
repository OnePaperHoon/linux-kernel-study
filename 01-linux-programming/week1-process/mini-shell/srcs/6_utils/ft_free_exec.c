/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ft_free_exec.c                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: janhan <janhan@student.42seoul.kr>         +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/03/01 19:22:26 by janhan            #+#    #+#             */
/*   Updated: 2024/04/17 19:04:48 by janhan           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../includes/minishell.h"

void	ft_free_exec(t_exec *exec, size_t exec_arr_i)
{
	size_t	i;

	i = 0;
	while (i < exec_arr_i)
	{
		free(exec->exec_arr[i].cmd);
		free(exec->exec_arr[i].redirect);
		i++;
	}
	free(exec->exec_arr);
	i = 0;
	while (exec->path_ev[i])
		free(exec->path_ev[i++]);
	free(exec->path_ev);
}
