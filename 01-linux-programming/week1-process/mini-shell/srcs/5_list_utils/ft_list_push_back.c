/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ft_list_push_back.c                                :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: janhan <janhan@student.42seoul.kr>         +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/02/24 00:49:35 by janhan            #+#    #+#             */
/*   Updated: 2024/04/17 19:04:37 by janhan           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../includes/minishell.h"

int	ft_list_push_back(void *content, t_list *list)
{
	t_node	*new_node;
	t_node	*finder;

	if (list == NULL || content == NULL)
		return (ft_error("argument is wrong", FAILURE));
	new_node = malloc(sizeof(t_node));
	if (new_node == NULL)
		return (ft_error("new_node malloc return NULL", FAILURE));
	new_node->content = content;
	new_node->visit = FALSE;
	new_node->next_node = NULL;
	finder = list->front_node;
	while (finder && finder->next_node)
		finder = finder->next_node;
	if (finder == NULL)
		list->front_node = new_node;
	else
		finder->next_node = new_node;
	return (SUCCESS);
}
