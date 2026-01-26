/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ft_env_builtin.c                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: janhan <janhan@student.42seoul.kr>         +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/03/19 08:27:28 by janhan            #+#    #+#             */
/*   Updated: 2024/04/17 18:59:09 by janhan           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../includes/minishell.h"

int	ft_env_builtin_has_value(char *content)
{
	size_t	i;

	i = 0;
	while (content[i])
	{
		if (content[i] == '=')
			return (TRUE);
		i++;
	}
	return (FALSE);
}

int	ft_env_builtin(t_info *info)
{
	t_node	*cur;

	cur = info->mini_ev.front_node;
	while (cur)
	{
		if (ft_env_builtin_has_value(cur->content) == TRUE)
			printf("%s\n", (char *)(cur->content));
		cur = cur->next_node;
	}
	exit(EXIT_SUCCESS);
}
