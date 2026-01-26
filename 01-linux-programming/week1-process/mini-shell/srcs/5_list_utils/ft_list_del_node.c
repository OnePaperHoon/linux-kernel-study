/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ft_list_del_node.c                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: janhan <janhan@student.42seoul.kr>         +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/02/24 14:06:02 by janhan            #+#    #+#             */
/*   Updated: 2024/04/17 19:04:26 by janhan           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../includes/minishell.h"

int	ft_list_del_node(t_list *list, t_node *node)
{
	t_node	*finder;

	finder = list->front_node;
	if (list == NULL || node == NULL)
		return (ft_error("argument has been worng", FAILURE));
	if (list->front_node == node)
	{
		list->front_node = node->next_node;
		free(node->content);
		free(node);
		return (SUCCESS);
	}
	while (finder != node && finder->next_node != node)
		finder = finder->next_node;
	finder->next_node = node->next_node;
	free(node->content);
	free(node);
	node = 0;
	return (SUCCESS);
}
