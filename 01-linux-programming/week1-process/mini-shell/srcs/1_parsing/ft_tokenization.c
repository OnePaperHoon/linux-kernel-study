/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ft_tokenization.c                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: janhan <janhan@student.42seoul.kr>         +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/02/27 23:25:46 by janhan            #+#    #+#             */
/*   Updated: 2024/03/30 23:03:03 by janhan           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../includes/minishell.h"

static int	ft_operator_process(t_parse *parse)
{
	char	redirect_c;

	if (parse->line[parse->line_index] == '|')
	{
		if (ft_make_token(parse, PIPE) == FAILURE)
			return (FAILURE);
	}
	else if (ft_is_redirect(parse->line[parse->line_index]) == TRUE)
	{
		redirect_c = parse->line[parse->line_index];
		if (parse->line[parse->line_index + 1] == redirect_c)
			parse->line_index++;
		if (ft_make_token(parse, REDIRECT) == FAILURE)
			return (FAILURE);
	}
	return (SUCCESS);
}

static void	ft_quote_process(t_parse *parse)
{
	char	end_quote;

	end_quote = parse->line[parse->line_index];
	parse->line_index++;
	while (parse->line[parse->line_index]
		&& parse->line[parse->line_index] != end_quote)
		parse->line_index++;
}

static int	ft_character_process(t_parse *parse)
{
	while (TRUE)
	{
		if (ft_is_quote(parse->line[parse->line_index]) == TRUE)
			ft_quote_process(parse);
		if (parse->line[parse->line_index + 1] == '\0'
			|| ft_is_space(parse->line[parse->line_index + 1]) == TRUE
			|| ft_is_operator(parse->line[parse->line_index + 1]) == TRUE)
		{
			if (ft_make_token(parse, WORD) == FAILURE)
				return (FAILURE);
			return (SUCCESS);
		}
		parse->line_index++;
	}
	return (SUCCESS);
}

int	ft_tokenization(t_parse *parse)
{
	while (parse->line[parse->line_index])
	{
		if (ft_is_space(parse->line[parse->line_index]) == TRUE)
			;
		else if (ft_is_operator(parse->line[parse->line_index]) == TRUE)
		{
			if (ft_operator_process(parse) == FAILURE)
				return (FAILURE);
		}
		else
		{
			if (ft_character_process(parse) == FAILURE)
				return (FAILURE);
		}
		parse->line_index++;
		parse->start_index = parse->line_index;
	}
	return (SUCCESS);
}
