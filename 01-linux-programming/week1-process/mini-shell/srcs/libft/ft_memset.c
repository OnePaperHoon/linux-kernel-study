/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ft_memset.c                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: janhan <janhan@student.42seoul.kr>         +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2023/10/04 23:40:58 by janhan            #+#    #+#             */
/*   Updated: 2024/03/01 19:52:46 by janhan           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../includes/libft.h"

void	*ft_memset(void *b, int c, size_t len)
{
	size_t	i;
	char	*temp_b;

	i = 0;
	temp_b = (char *)b;
	while (i < len)
	{
		temp_b[i] = (unsigned char)c;
		i++;
	}
	return ((void *)temp_b);
}
