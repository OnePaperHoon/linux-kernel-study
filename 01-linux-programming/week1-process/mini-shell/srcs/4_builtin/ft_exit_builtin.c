/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ft_exit_builtin.c                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: janhan <janhan@student.42seoul.kr>         +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/03/19 09:03:27 by janhan            #+#    #+#             */
/*   Updated: 2024/03/30 15:49:16 by janhan           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../includes/minishell.h"

static int	ft_isnum(int c)
{
	if ('0' <= c && c <= '9')
		return (TRUE);
	else
		return (FALSE);
}

static void	ft_exit_builtin_arg_check(t_exec *exec, size_t i, int sing_flag)
{
	exec->exec_arr[exec->exec_arr_index].cmd[1]
		= ft_strtrim(exec->exec_arr[exec->exec_arr_index].cmd[1], " ");
	while (exec->exec_arr[exec->exec_arr_index].cmd[1][i])
	{
		if (ft_isnum(exec->exec_arr[exec->exec_arr_index].cmd[1][i]))
			i++;
		else
		{
			ft_printf_err("exit\n");
			ft_printf_err("minishell: exit: %s: numeric argument required\n",
				exec->exec_arr[exec->exec_arr_index].cmd[1]);
			exit(255);
		}
	}
	if (i == 1 && sing_flag == TRUE)
	{
		ft_printf_err("exit\n");
		ft_printf_err("minishell: exit: %s: numeric argument required\n",
			exec->exec_arr[exec->exec_arr_index].cmd[1]);
		exit(255);
	}
}

static void	ft_exit_builtin_isnum(t_exec *exec)
{
	size_t	i;
	int		sign_flag;

	sign_flag = FALSE;
	i = 0;
	if ((exec->exec_arr[exec->exec_arr_index].cmd[1][0] == '-')
		|| (exec->exec_arr[exec->exec_arr_index].cmd[1][0] == '+'))
	{
		sign_flag = TRUE;
		i++;
	}
	ft_exit_builtin_arg_check(exec, i, sign_flag);
	printf("exit\n");
}

static void	ft_exit_builtin_no_arg(t_list *mini_ev, t_parse *parse,
	t_exec *exec, t_exec_info *exec_info)
{
	if ((exec->exec_arr[exec->exec_arr_index].cmd[1]) == NULL)
	{
		if (exec_info->builtin_parent == TRUE)
			ft_printf_err("exit\n");
		ft_list_clear(mini_ev);
		ft_free_all(parse, exec);
		exit(EXIT_SUCCESS);
	}
}

int	ft_exit_builtin(t_list *mini_ev, t_parse *parse, t_exec *exec,
	t_exec_info *exec_info)
{
	int	exit_value;

	ft_exit_builtin_no_arg(mini_ev, parse, exec, exec_info);
	if (exec->exec_arr[exec->exec_arr_index].cmd[2])
	{
		ft_printf_err("minishell: exit: too many arguments\n");
		g_child_exit_code = 1;
		if (exec_info->builtin_parent == TRUE)
			return (SUCCESS);
		else
			exit(1);
	}
	ft_exit_builtin_isnum(exec);
	exit_value = ft_atoi(exec->exec_arr[exec->exec_arr_index].cmd[1]);
	ft_list_clear(mini_ev);
	ft_free_all(parse, exec);
	exit(exit_value);
}
