/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ft_cmd_is_directory.c                              :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: janhan <janhan@student.42seoul.kr>         +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/03/19 14:37:19 by janhan            #+#    #+#             */
/*   Updated: 2024/03/27 20:17:49 by janhan           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../includes/minishell.h"

void	ft_cmd_is_directory(char *cmd_path)
{
	DIR	*dir;

	dir = opendir(cmd_path);
	if (dir != NULL)
	{
		ft_printf_err("%s: is a directory\n", cmd_path);
		exit(126);
	}
}
