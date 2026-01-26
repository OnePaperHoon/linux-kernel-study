/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ft_print_u_err.c                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: janhan <janhan@student.42seoul.kr>         +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/03/19 16:12:27 by janhan            #+#    #+#             */
/*   Updated: 2024/03/19 16:12:27 by janhan           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../includes/libft.h"

static void	ft_run_alt(unsigned int n, int fd, int *res)
{
	if (n > 9)
		ft_run_alt(n / 10, fd, res);
	ft_putchar_fd((n % 10) + '0', fd);
	(*res)++;
}

static void	ft_putnbr_fd_alt(unsigned int n, int fd, int *res)
{
	if (n == 0)
	{
		ft_putchar_fd('0', fd);
		(*res)++;
		return ;
	}
	ft_run_alt(n, fd, res);
}

void	ft_print_u(unsigned int nb, int *res)
{
	ft_putnbr_fd_alt(nb, 2, res);
}
