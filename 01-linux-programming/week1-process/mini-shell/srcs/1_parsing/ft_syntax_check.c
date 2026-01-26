/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ft_syntax_check.c                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: janhan <janhan@student.42seoul.kr>         +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/03/01 18:41:14 by janhan            #+#    #+#             */
/*   Updated: 2024/04/17 18:57:31 by janhan           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../includes/minishell.h"

static int	ft_check_pipe_syntax(t_parse *parse)
{
	if (parse->token_index == 0 || parse->token_index == parse->token_count - 1)
	{
		g_child_exit_code = 258;
		return (ft_error("Syntax Error: PIPE became first or last", FAILURE));
	}
	else
	{
		if (parse->tokens[parse->token_index - 1].type == PIPE
			|| parse->tokens[parse->token_index + 1].type == PIPE)
		{
			g_child_exit_code = 258;
			return (ft_error("Syntax Error: No word between PIPE", FAILURE));
		}
	}
	return (SUCCESS);
}

static int	ft_check_redirect_syntax(t_parse *parse)
{
	if (parse->token_index == parse->token_count - 1)
	{
		g_child_exit_code = 258;
		return (ft_error("Syntax Error: No word after REDIRECT", FAILURE));
	}
	else
	{
		if (parse->tokens[parse->token_index + 1].type != WORD)
		{
			g_child_exit_code = 258;
			return (ft_error("Syntax Error: No word after REDIRECT", FAILURE));
		}
	}
	return (SUCCESS);
}

int	ft_syntax_check(t_parse *parse)
{
	parse->token_index = 0;
	while (parse->token_index < parse->token_count)
	{
		if (parse->tokens[parse->token_index].type == PIPE)
		{
			if (ft_check_pipe_syntax(parse) == FAILURE)
				return (FAILURE);
		}
		else if (parse->tokens[parse->token_index].type == REDIRECT)
		{
			if (ft_check_redirect_syntax(parse) == FAILURE)
				return (FAILURE);
		}
		parse->token_index++;
	}
	return (SUCCESS);
}
