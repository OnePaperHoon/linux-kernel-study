/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ft_remove_quote.c                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: janhan <janhan@student.42seoul.kr>         +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/02/28 00:32:36 by janhan            #+#    #+#             */
/*   Updated: 2024/04/03 15:44:42 by janhan           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../includes/minishell.h"

static void	ft_find_quote(t_parse *parse)
{
	char	c;
	char	quote;

	c = parse->token->str[parse->str_index];
	if (ft_is_quote(c) == TRUE)
	{
		quote = c;
		parse->start_index = parse->str_index;
		parse->str_index++;
		while (parse->token->str[parse->str_index]
			&& parse->token->str[parse->str_index] != quote)
			parse->str_index++;
		ft_strlcpy(&parse->token->str[parse->start_index],
			&parse->token->str[parse->start_index] + 1,
			ft_strlen(&parse->token->str[parse->start_index]));
		ft_strlcpy(&parse->token->str[parse->str_index - 1],
			&parse->token->str[parse->str_index],
			ft_strlen(&parse->token->str[parse->str_index - 1]));
		parse->str_index -= 2;
	}
}

void	ft_remove_quote(t_parse *parse)
{
	parse->token_index = 0;
	while (parse->token_index < parse->token_count)
	{
		parse->token = &parse->tokens[parse->token_index];
		parse->str_index = 0;
		while (parse->token->str[parse->str_index])
		{
			ft_find_quote(parse);
			parse->str_index++;
		}
		parse->token_index++;
	}
}
