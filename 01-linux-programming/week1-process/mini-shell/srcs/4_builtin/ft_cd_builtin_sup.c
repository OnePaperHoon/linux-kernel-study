/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ft_cd_builtin_sup.c                                :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: janhan <janhan@student.42seoul.kr>         +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/04/17 15:34:07 by janhan            #+#    #+#             */
/*   Updated: 2024/04/20 13:46:57 by janhan           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../includes/minishell.h"

t_node	*ft_find_oldpwd(t_info *info)
{
	t_node	*current;
	char	*content;

	content = ft_strdup("OLDPWD=");
	if (content == NULL)
		exit(ft_error("ft_find_oldpwd malloc failed\n", FAILURE));
	if (ft_list_push_back(content, &info->mini_ev) == FAILURE)
		exit(ft_error("OLDPWD list push back failed\n", FAILURE));
	current = info->mini_ev.front_node;
	while (current && ft_strncmp(current->content, "OLDPWD=", 7))
		current = current->next_node;
	if (current == NULL)
		exit(ft_error("OLDPWD not found\n", FAILURE));
	return (current);
}
