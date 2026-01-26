/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ft_itoa.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: janhan <janhan@student.42seoul.kr>         +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2023/10/07 23:51:20 by janhan            #+#    #+#             */
/*   Updated: 2024/03/01 19:52:46 by janhan           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../includes/libft.h"

static int	ft_int_len(int n)
{
	int	len;

	len = 0;
	if (n <= 0)
		len++;
	while (n)
	{
		len++;
		n /= 10;
	}
	return (len);
}

static int	ft_abs_num(int n)
{
	if (n < 0)
		return (n * -1);
	return (n);
}

char	*ft_itoa(int n)
{
	int		len;
	int		sign;
	char	*result;

	sign = 1;
	if (n < 0)
		sign *= -1;
	len = ft_int_len(n);
	result = (char *)malloc(len + 1);
	if (!result)
		return (0);
	result[len--] = 0;
	while (len >= 0)
	{
		result[len] = ft_abs_num(n % 10) + '0';
		n /= 10;
		len--;
	}
	if (sign == -1)
		result[0] = '-';
	return (result);
}
