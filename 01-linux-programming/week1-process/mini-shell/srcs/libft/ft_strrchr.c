/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ft_strrchr.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: janhan <janhan@student.42seoul.kr>         +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2023/10/06 02:08:35 by janhan            #+#    #+#             */
/*   Updated: 2024/03/01 19:52:47 by janhan           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../includes/libft.h"

char	*ft_strrchr(const char *s, int c)
{
	unsigned char	temp_c;
	size_t			i;
	size_t			j;

	i = ft_strlen((char *)s);
	j = 0;
	temp_c = (char)c;
	while (j <= i)
	{
		if (s[i - j] == temp_c)
			return ((char *)&s[i - j]);
		j++;
	}
	return (0);
}
