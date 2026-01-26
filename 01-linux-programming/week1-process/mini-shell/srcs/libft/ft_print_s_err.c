/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ft_print_s_err.c                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: janhan <janhan@student.42seoul.kr>         +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/03/19 16:12:23 by janhan            #+#    #+#             */
/*   Updated: 2024/03/19 16:12:23 by janhan           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../includes/libft.h"

void	ft_print_s(char *str, int *res)
{
	size_t	len;

	if (str == NULL)
	{
		write(2, "(null)", 6);
		(*res) = (*res) + 6;
		return ;
	}
	len = ft_strlen(str);
	write(2, str, len);
	(*res) = (*res) + len;
}
