/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ft_count_dallor.c                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sangshin <zxcv1867@naver.com>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/04/09 20:41:23 by sangshin          #+#    #+#             */
/*   Updated: 2024/04/09 20:50:28 by sangshin         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../includes/minishell.h"

int	ft_is_special_dallor(char *line)
{
	int	i;

	i = 0;
	while (line[i] != '$')
		i++;
	i++;
	if (line[i] == '_' || line[i] == '-' || line[i] == '?'
		|| line[i] == '\0' || line[i] == ' ' || line[i] == '"'
		|| ft_isdigit(line[i]))
		return (TRUE);
	return (FALSE);
}

int	ft_count_dallor(char *line)
{
	int	i;
	int	count;

	i = 0;
	count = 0;
	while (line[i])
	{
		if (line[i] == '$' && !ft_is_special_dallor(&line[i])
			&& ft_is_in_single_quote(line, i) == FALSE)
			count++;
		i++;
	}
	return (count);
}
