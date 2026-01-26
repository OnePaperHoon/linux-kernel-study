/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   get_next_line_utils.c                              :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: janhan <janhan@student.42seoul.kr>         +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/01/30 18:22:50 by janhan            #+#    #+#             */
/*   Updated: 2024/04/03 15:42:59 by janhan           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../includes/get_next_line.h"

char	*ft_strjoin_gnl(char *contents, char *buffer)
{
	char	*result;
	size_t	i;
	size_t	result_i;
	size_t	contents_len;
	size_t	buffer_len;

	contents_len = ft_strlen(contents);
	buffer_len = ft_strlen(buffer);
	result = (char *)malloc(((contents_len + buffer_len) * sizeof(char)) + 1);
	if (result == 0)
	{
		free(contents);
		return (0);
	}
	i = 0;
	result_i = 0;
	while (contents[i] != '\0')
		result[result_i++] = contents[i++];
	i = 0;
	while (buffer[i] != '\0')
		result[result_i++] = buffer[i++];
	result[result_i] = '\0';
	free(contents);
	free(buffer);
	return (result);
}

char	*ft_strchr(const char *s, int c)
{
	int		i;

	if ((char)c == '\0')
		return ((char *)&s[ft_strlen(s)]);
	i = 0;
	while (s[i] != '\0')
	{
		if (s[i] == (char)c)
			return ((char *)&s[i]);
		i++;
	}
	return (0);
}

void	ft_make_line(char *line, char *contents, size_t line_len)
{
	size_t	i;

	i = 0;
	while (i <= line_len)
	{
		line[i] = contents[i];
		i++;
	}
	line[i] = '\0';
}

char	*ft_new_contents(char *new_contents, char *contents, size_t start)
{
	size_t	i;

	i = 0;
	while (contents[start] != '\0')
		new_contents[i++] = contents[start++];
	new_contents[i] = '\0';
	return (new_contents);
}
