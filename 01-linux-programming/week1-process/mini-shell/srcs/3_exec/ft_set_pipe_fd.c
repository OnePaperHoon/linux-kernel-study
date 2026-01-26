/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ft_set_pipe_fd.c                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: janhan <janhan@student.42seoul.kr>         +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/03/19 14:03:06 by janhan            #+#    #+#             */
/*   Updated: 2024/03/19 14:16:48 by janhan           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../includes/minishell.h"

void	ft_set_pipe_fd(t_exec *exec, t_exec_info *exec_info)
{
	if (exec->prev_pipe_fd != NONE)
	{
		if (dup2(exec->prev_pipe_fd, STDIN_FILENO) == FAILURE)
			exit(ft_perror(errno));
		if (close(exec->prev_pipe_fd) == FAILURE)
			exit(ft_perror(EXIT_FAILURE));
	}
	if (exec_info->use_pipe == TRUE)
	{
		if (dup2(exec_info->pipe_fd[OUT], STDOUT_FILENO) == FAILURE)
			exit(ft_perror(errno));
		if (close(exec_info->pipe_fd[OUT]) == FAILURE)
			exit(ft_perror(errno));
		if (close(exec_info->pipe_fd[IN]) == FAILURE)
			exit(ft_perror(errno));
	}
}
