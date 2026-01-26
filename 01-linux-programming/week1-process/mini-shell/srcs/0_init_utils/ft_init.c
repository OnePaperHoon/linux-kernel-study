/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ft_init.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: janhan <janhan@student.42seoul.kr>         +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/02/24 00:39:11 by janhan            #+#    #+#             */
/*   Updated: 2024/04/22 19:00:36 by sangshin         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../includes/minishell.h"

int	g_child_exit_code;

void	ft_init(int ac, char **av, char **ev, t_info *info)
{
	g_child_exit_code = 0;
	info->ac = ac;
	info->av = av;
	info->ev = ev;
	tcgetattr(STDIN_FILENO, &info->termios_backup);
	ft_mini_ev_init(ev, info);
	ft_print_logo();
}
