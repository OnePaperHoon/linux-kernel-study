/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ft_substitute_env.c                                :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: janhan <janhan@student.42seoul.kr>         +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/04/09 20:36:16 by sangshin          #+#    #+#             */
/*   Updated: 2024/04/17 19:32:36 by janhan           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../includes/minishell.h"

int	ft_is_in_single_quote(char *line, int i)
{
	int	count;
	int	double_quote_flag;

	count = 0;
	double_quote_flag = FALSE;
	while (i >= 0)
	{
		if (line[i] == '"')
			double_quote_flag = !double_quote_flag;
		if (line[i] == '\'' && double_quote_flag == FALSE)
			count++;
		i--;
	}
	if (count % 2 == 0 || double_quote_flag == TRUE)
		return (FALSE);
	else
		return (TRUE);
}

int	ft_is_there_squote(char *env_value)
{
	int	i;

	i = 0;
	while (env_value[i])
	{
		if (env_value[i] == '\'')
			return (TRUE);
		i++;
	}
	return (FALSE);
}

static void	ft_replace_str_sup(char **line, char **arr,
int squote_flag, char *env_value)
{
	int	i;

	i = 0;
	while (*line != arr[0])
		arr[2][i++] = *(*line)++;
	*line -= i;
	if (squote_flag)
		arr[2][i++] = '"';
	while (*env_value)
		arr[2][i++] = *env_value++;
	if (squote_flag)
		arr[2][i++] = '"';
	while (*arr[1])
		arr[2][i++] = *arr[1]++;
}

static void	ft_replace_str(char **line, char *env_name, char *env_value)
{
	char	*arr[3];
	char	*env_name_with_dallor;
	int		squote_flag;

	squote_flag = ft_is_there_squote(env_value);
	env_name_with_dallor = ft_strjoin("$", env_name);
	arr[0] = ft_strnstr(*line, env_name_with_dallor, ft_strlen(*line));
	arr[1] = arr[0] + ft_strlen(env_name) + 1;
	arr[2] = ft_calloc(ft_strlen(*line) + ft_strlen(env_value)
			- ft_strlen(env_name) + 1 + (squote_flag * 2), sizeof(char));
	ft_replace_str_sup(line, arr, squote_flag, env_value);
	free(env_name_with_dallor);
	free(*line);
	*line = arr[2];
}

int	ft_substitute_env(t_info *info, t_parse *parse)
{
	int		dallor_count;
	char	*env_name;
	char	*env_value;

	dallor_count = ft_count_dallor(parse->line);
	while (dallor_count)
	{
		env_name = ft_get_env_name(parse->line);
		env_value = ft_get_env_value(info, env_name);
		ft_replace_str(&parse->line, env_name, env_value);
		free(env_name);
		dallor_count--;
	}
	return (SUCCESS);
}
