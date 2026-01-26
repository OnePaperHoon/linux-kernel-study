/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ft_is_child_exit_code.c                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: janhan <janhan@student.42seoul.kr>         +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/02/28 00:26:16 by janhan            #+#    #+#             */
/*   Updated: 2024/02/28 00:26:53 by janhan           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../includes/minishell.h"

int	ft_is_child_exit_code(t_parse *parse)
{
	if (parse->token->str[parse->str_index + 1] == '?')
		return (TRUE);
	return (FALSE);
}
