/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ft_strnstr.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: janhan <janhan@student.42seoul.kr>         +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2023/10/05 09:52:42 by janhan            #+#    #+#             */
/*   Updated: 2024/03/01 19:52:47 by janhan           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../includes/libft.h"

char	*ft_strnstr(const char *haystack, const char *needle, size_t len)
{
	size_t	haystack_i;
	size_t	needle_i;

	haystack_i = 0;
	if (*needle == '\0')
		return ((char *)haystack);
	while (haystack_i < len && haystack[haystack_i] != '\0')
	{
		needle_i = 0;
		while (haystack[haystack_i + needle_i] == needle[needle_i]
			&& needle[needle_i] != '\0' && haystack_i + needle_i < len)
			needle_i++;
		if (needle[needle_i] == '\0')
			return ((char *)haystack + haystack_i);
		haystack_i++;
	}
	return (NULL);
}
