/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ft_get_env.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sangshin <zxcv1867@naver.com>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/04/09 20:40:21 by sangshin          #+#    #+#             */
/*   Updated: 2024/04/15 17:46:51 by sangshin         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../includes/minishell.h"

char	*ft_get_env_name(char *line)
{
	int		i;
	int		start;
	char	*env_name;

	i = 0;
	while (line[i] != '$' || ft_is_in_single_quote(line, i) == TRUE
		|| ft_is_special_dallor(&line[i]) == TRUE)
		i++;
	i++;
	start = i;
	while (ft_isalnum(line[i]) || line[i] == '_')
		i++;
	env_name = ft_substr(line, start, i - start);
	if (env_name == NULL)
		exit(ft_error("env_name malloc failed", FAILURE));
	return (env_name);
}

char	*ft_get_env_value(t_info *info, char *env_name)
{
	t_node	*node;
	char	*env_name_with_eq;

	env_name_with_eq = ft_strjoin(env_name, "=");
	if (env_name_with_eq == NULL)
		exit(ft_error("env_name_with_eq malloc failed", FAILURE));
	node = info->mini_ev.front_node;
	while (node)
	{
		if (ft_strncmp(node->content, env_name_with_eq,
				ft_strlen(env_name_with_eq)) == 0)
		{
			free(env_name_with_eq);
			return (ft_strchr(node->content, '=') + 1);
		}
		node = node->next_node;
	}
	free(env_name_with_eq);
	return ("");
}
