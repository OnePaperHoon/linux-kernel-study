/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ft_set_redirect_fd.c                               :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: janhan <janhan@student.42seoul.kr>         +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/03/19 14:21:34 by janhan            #+#    #+#             */
/*   Updated: 2024/04/17 18:58:20 by janhan           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../includes/minishell.h"

static void	ft_redirect_out1(t_exec_info *exec_info, t_redirect *redirect)
{
	if (exec_info->outfile_fd != NONE)
		if (close(exec_info->outfile_fd) == FAILURE)
			exit(ft_perror(errno));
	exec_info->outfile_fd = open
		(redirect->value, O_WRONLY | O_CREAT | O_TRUNC, 0644);
	if (redirect->value[0] == '\0')
	{
		ft_printf_err("minishell: %s: ambiguous redirect\n", redirect->original);
		exit(1);
	}
	if (exec_info->outfile_fd == FAILURE)
	{
		ft_printf_err("minishell: %s: Permission denied\n", redirect->value);
		exit(1);
	}
}

static void	ft_redirect_out2(t_exec_info *exec_info, t_redirect *redirect)
{
	if (exec_info->outfile_fd != NONE)
		if (close(exec_info->outfile_fd) == FAILURE)
			exit(ft_perror(errno));
	exec_info->outfile_fd = open
		(redirect->value, O_WRONLY | O_CREAT | O_APPEND, 0644);
	if (exec_info->outfile_fd == FAILURE)
	{
		ft_printf_err("minishell: %s: Permission denied\n", redirect->value);
		exit(1);
	}
}

static void	ft_redirect_in1(t_exec_info *exec_info, t_redirect *redirect)
{
	if (exec_info->infile_fd != NONE)
		if (close(exec_info->infile_fd) == FAILURE)
			exit(ft_perror(errno));
	exec_info->infile_fd = open(redirect->value, O_RDONLY);
	if (exec_info->infile_fd == FAILURE)
	{
		ft_printf_err("minishell: %s: No such file or directory\n",
			redirect->value);
		exit(1);
	}
}

static void	ft_redirect_here_doc(t_exec_info *exec_info)
{
	if (exec_info->infile_fd != NONE)
		if (close(exec_info->infile_fd) == FAILURE)
			exit(ft_perror(errno));
	exec_info->infile_fd = open("/tmp/temp_fd", O_RDONLY);
	if (exec_info->infile_fd == FAILURE)
		exit(ft_perror(1));
}

void	ft_set_redirect_fd(t_exec_info *exec_info)
{
	t_redirect	*redirect;

	exec_info->redirect_index = 0;
	while (exec_info->redirect[exec_info->redirect_index].value != 0)
	{
		redirect = &exec_info->redirect[exec_info->redirect_index];
		if (redirect->type == OUT1)
			ft_redirect_out1(exec_info, redirect);
		if (redirect->type == OUT2)
			ft_redirect_out2(exec_info, redirect);
		if (redirect->type == IN1)
			ft_redirect_in1(exec_info, redirect);
		if (redirect->type == HERE_DOC)
			ft_redirect_here_doc(exec_info);
		exec_info->redirect_index++;
	}
}
