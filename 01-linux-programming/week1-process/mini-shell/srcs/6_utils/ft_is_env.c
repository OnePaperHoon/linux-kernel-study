/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ft_is_env.c                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: janhan <janhan@student.42seoul.kr>         +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/02/28 00:27:30 by janhan            #+#    #+#             */
/*   Updated: 2024/04/17 19:04:59 by janhan           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../includes/minishell.h"

int	ft_is_env(t_info *info, t_parse *parse)
{
	t_node	*env_node;

	env_node = info->mini_ev.front_node;
	while (env_node)
	{
		if (ft_strncmp(env_node->content,
				parse->target_ev, ft_strlen(parse->target_ev)) == 0)
		{
			parse->ev_val = ft_strdup(ft_strchr(env_node->content, '=') + 1);
			parse->ev_val_len = ft_strlen(parse->ev_val);
			return (TRUE);
		}
		if (env_node->next_node == NULL)
			break ;
		env_node = env_node->next_node;
	}
	return (FALSE);
}
