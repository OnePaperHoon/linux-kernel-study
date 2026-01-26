/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ft_convert_child.c                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: janhan <janhan@student.42seoul.kr>         +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/02/28 00:21:42 by janhan            #+#    #+#             */
/*   Updated: 2024/04/17 18:57:01 by janhan           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../includes/minishell.h"

int	ft_convert_child_exit_code(t_parse *parse)
{
	parse->exit_code = ft_itoa(g_child_exit_code);
	if (parse->exit_code == NULL)
		return (FAILURE);
	parse->token->str[parse->str_index] = '\0';
	parse->old_str = ft_strjoin(parse->token->str, parse->exit_code);
	if (parse->old_str == NULL)
	{
		free(parse->exit_code);
		return (FAILURE);
	}
	parse->new_str
		= ft_strjoin(parse->old_str, &parse->token->str[parse->str_index + 2]);
	if (parse->new_str == NULL)
	{
		free(parse->exit_code);
		free(parse->old_str);
		return (FAILURE);
	}
	free(parse->token->str);
	parse->token->str = parse->new_str;
	parse->str_index += ft_strlen(parse->exit_code) - 1;
	free(parse->exit_code);
	free(parse->old_str);
	return (SUCCESS);
}
