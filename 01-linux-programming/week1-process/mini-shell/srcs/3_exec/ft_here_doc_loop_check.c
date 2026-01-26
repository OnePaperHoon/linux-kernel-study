/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ft_here_doc_loop_check.c                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: janhan <janhan@student.42seoul.kr>         +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/03/29 02:17:52 by janhan            #+#    #+#             */
/*   Updated: 2024/03/29 02:17:52 by janhan           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../includes/minishell.h"

int	ft_here_doc_loop_check(t_exec *exec)
{
	int	i;
	int	j;

	i = 0;
	j = 0;
	while (TRUE)
	{
		while (TRUE)
		{
			if (exec->exec_arr[i].redirect[j].value == 0)
				return (FAILURE);
			if (exec->exec_arr[i].redirect[j].type == HERE_DOC)
				return (SUCCESS);
			j++;
		}
		i++;
	}
	return (FAILURE);
}
