/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ft_atoi.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: janhan <janhan@student.42seoul.kr>         +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2023/10/05 17:48:21 by janhan            #+#    #+#             */
/*   Updated: 2024/03/01 19:52:46 by janhan           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../includes/libft.h"

int	ft_atoi(const char *str)
{
	size_t	i;
	int		flag;
	int		sign;
	int		result;

	i = 0;
	flag = 0;
	sign = 1;
	result = 0;
	while (str[i] == ' ' || (9 <= str[i] && str[i] <= 13))
		i++;
	while (str[i] == '-' || str[i] == '+')
	{
		if (str[i] == '-')
			sign *= -1;
		i++;
		flag++;
	}
	while ('0' <= str[i] && str[i] <= '9')
		result = (result * 10) + (str[i++] - '0');
	if (flag > 1)
		return (0);
	return (result * sign);
}
