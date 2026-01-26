/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ft_mini_ev_init.c                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: janhan <janhan@student.42seoul.kr>         +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/02/24 00:46:59 by janhan            #+#    #+#             */
/*   Updated: 2024/03/30 23:39:21 by janhan           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../includes/minishell.h"

void	ft_mini_ev_init(char **ev, t_info *info)
{
	size_t	i;

	i = 0;
	info->mini_ev = ft_list_init();
	while (ev[i])
	{
		ft_list_push_back(ft_strdup(ev[i]), &(info->mini_ev));
		i++;
	}
}
