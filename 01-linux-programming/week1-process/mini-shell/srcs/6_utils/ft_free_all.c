/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ft_free_all.c                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: janhan <janhan@student.42seoul.kr>         +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/03/01 19:34:32 by janhan            #+#    #+#             */
/*   Updated: 2024/03/01 19:35:08 by janhan           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../includes/minishell.h"

void	ft_free_all(t_parse *parse, t_exec *exec)
{
	ft_free_tokens(parse, parse->token_count);
	ft_free_exec(exec, exec->exec_arr_size);
}
