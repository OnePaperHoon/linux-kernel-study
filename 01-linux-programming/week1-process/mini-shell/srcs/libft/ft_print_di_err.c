/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ft_print_di_err.c                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: janhan <janhan@student.42seoul.kr>         +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/03/19 16:12:13 by janhan            #+#    #+#             */
/*   Updated: 2024/03/19 16:12:14 by janhan           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../includes/libft.h"

static void	ft_run_alt(long long n, int fd, int *res)
{
	if (n > 9)
		ft_run_alt(n / 10, fd, res);
	ft_putchar_fd((n % 10) + '0', fd);
	(*res)++;
}

static void	ft_putnbr_fd_alt(int n, int fd, int *res)
{
	long long	n_save;

	if (n == 0)
	{
		ft_putchar_fd('0', fd);
		(*res)++;
		return ;
	}
	else if (n < 0)
	{
		ft_putchar_fd('-', fd);
		(*res)++;
		n_save = (unsigned int) n * -1;
	}
	else
		n_save = n;
	ft_run_alt(n_save, fd, res);
}

void	ft_print_di(int nb, int *res)
{
	ft_putnbr_fd_alt(nb, 2, res);
}
