/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ft_split.c                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: janhan <janhan@student.42seoul.kr>         +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2023/10/07 01:24:29 by janhan            #+#    #+#             */
/*   Updated: 2024/03/01 19:52:47 by janhan           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../includes/libft.h"

static size_t	ft_get_count(char const *s, char c)
{
	size_t	count;

	count = 1;
	while (*s)
	{
		if (*s != c)
			break ;
		s++;
	}
	if (*s == '\0')
		return (0);
	while (*s)
	{
		if (*s == c)
		{
			if (*(s + 1) != c && *(s + 1) != 0)
				count++;
		}
		s++;
	}
	return (count);
}

static void	*ft_free_split(char **split)
{
	size_t	i;

	i = 0;
	if (split == NULL)
		return (NULL);
	while (split[i])
	{
		free(split[i]);
		i++;
	}
	free(split);
	return (NULL);
}

static char	**ft_cut_str(char const *s, char c, char **result)
{
	char	*from;
	size_t	i;

	i = 0;
	while (*s)
	{
		if (*s != c)
		{
			from = (char *)s;
			while (*from && *from != c)
				from++;
			result[i] = (char *)malloc(from - s + 1);
			if (!result[i])
				return (ft_free_split(result));
			ft_strlcpy(result[i++], s, from - s + 1);
			s = from;
		}
		else
			s++;
	}
	result[i] = 0;
	return (result);
}

char	**ft_split(char const *s, char c)
{
	char	**result;

	result = (char **)ft_calloc(sizeof(char *) * (ft_get_count(s, c) + 1), 1);
	if (!result)
		return (NULL);
	result = ft_cut_str(s, c, result);
	return (result);
}
