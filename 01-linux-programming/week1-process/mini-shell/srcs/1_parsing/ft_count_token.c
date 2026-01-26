/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ft_count_token.c                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: janhan <janhan@student.42seoul.kr>         +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/02/27 23:05:11 by janhan            #+#    #+#             */
/*   Updated: 2024/04/17 18:57:22 by janhan           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../includes/minishell.h"

static int	ft_pipe_process(t_parse *parse)
{
	if (parse->line[parse->line_index + 1] == '|')
	{
		g_child_exit_code = 258;
		return (ft_error("Syntax Error: Not support '||' symbol in minishell",
				FAILURE));
	}
	else
		parse->token_count++;
	return (SUCCESS);
}

static int	ft_redirect_process(t_parse *parse)
{
	char	redirect_c;
	int		repeat_count;

	repeat_count = 1;
	redirect_c = parse->line[parse->line_index];
	while (parse->line[parse->line_index + 1] == redirect_c)
	{
		repeat_count++;
		if (repeat_count == 3)
		{
			g_child_exit_code = 258;
			return (ft_error("Syntax Error: Used \
more than 3 redirection character (Ex: '<<<...')", FAILURE));
		}
		parse->line_index++;
	}
	parse->token_count++;
	return (SUCCESS);
}

static int	ft_operator_process(t_parse *parse)
{
	if (parse->line[parse->line_index] == '|')
	{
		if (ft_pipe_process(parse) == FAILURE)
			return (FAILURE);
	}
	else if (ft_is_redirect(parse->line[parse->line_index]) == TRUE)
		if (ft_redirect_process(parse) == FAILURE)
			return (FAILURE);
	return (SUCCESS);
}

static int	ft_quote_process(t_parse *parse)
{
	char	end_quote;

	end_quote = parse->line[parse->line_index];
	parse->line_index++;
	while (parse->line[parse->line_index]
		&& parse->line[parse->line_index] != end_quote)
		parse->line_index++;
	if (parse->line[parse->line_index] == '\0')
	{
		g_child_exit_code = 258;
		ft_printf_err("Syntax Error: Quote %c is not closed\n", end_quote);
		return (FAILURE);
	}
	return (SUCCESS);
}

int	ft_count_token(t_parse *parse)
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
			if (ft_is_quote(parse->line[parse->line_index]) == TRUE)
				if (ft_quote_process(parse) == FAILURE)
					return (FAILURE);
			if (parse->line[parse->line_index + 1] == '\0'
				|| ft_is_space(parse->line[parse->line_index + 1]) == TRUE
				|| ft_is_operator(parse->line[parse->line_index + 1]) == TRUE)
				parse->token_count++;
		}
		parse->line_index++;
	}
	return (SUCCESS);
}
