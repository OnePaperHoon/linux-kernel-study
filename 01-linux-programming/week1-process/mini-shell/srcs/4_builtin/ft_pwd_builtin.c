/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ft_pwd_builtin.c                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: janhan <janhan@student.42seoul.kr>         +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/03/19 10:57:46 by janhan            #+#    #+#             */
/*   Updated: 2024/04/17 19:36:04 by janhan           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../includes/minishell.h"

int	ft_pwd_builtin(void)
{
	char	path[PATH_MAX];

	if (getcwd(path, sizeof(path)))
		printf("%s\n", path);
	else
		exit(EXIT_FAILURE);
	exit(EXIT_SUCCESS);
}
