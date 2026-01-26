/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ft_cd_builtin.c                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: janhan <janhan@student.42seoul.kr>         +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/04/15 16:14:17 by sangshin          #+#    #+#             */
/*   Updated: 2024/04/20 13:55:17 by janhan           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../includes/minishell.h"

static void	ft_change_oldpwd(t_info *info)
{
	char	path[PATH_MAX];
	char	*oldpwd;
	char	*result;
	t_node	*current;

	current = info->mini_ev.front_node;
	oldpwd = ft_strdup("OLDPWD=");
	if (oldpwd == NULL)
		exit(ft_error("OLDPWD malloc failed\n", FAILURE));
	while (current && ft_strncmp(current->content, "OLDPWD=", 7))
		current = current->next_node;
	if (current == NULL)
		current = ft_find_oldpwd(info);
	getcwd(path, sizeof(path));
	result = ft_strjoin(oldpwd, path);
	if (result == NULL)
		exit (ft_error("ft_strjoin faild\n", FAILURE));
	free(current->content);
	current->content = ft_strdup(result);
	if (current->content == NULL)
		exit(ft_error("ft_change_oldpwd ft_stdup failed\n", FAILURE));
	free(result);
	free(oldpwd);
}

static void	ft_change_pwd(t_info *info)
{
	char	path[PATH_MAX];
	char	*pwd;
	char	*result;
	t_node	*current;

	current = info->mini_ev.front_node;
	while (current && ft_strncmp(current->content, "PWD=", 4))
		current = current->next_node;
	if (current == NULL)
		return ;
	pwd = ft_strdup("PWD=");
	if (!pwd)
		exit(ft_error("PWD malloc failed", FAILURE));
	getcwd(path, sizeof(path));
	result = ft_strjoin(pwd, path);
	if (result == NULL)
		exit(ft_error("ft_strjoin failed", FAILURE));
	free(current->content);
	current->content = ft_strdup(result);
	if (current->content == NULL)
		exit(ft_error("ft_strdup failed", FAILURE));
	free(pwd);
	free(result);
}

static char	*ft_find_path(t_info *info)
{
	t_node	*current;
	char	*path;

	current = info->mini_ev.front_node;
	while (current && ft_strncmp(current->content, "HOME=", 5))
		current = current->next_node;
	if (current != NULL)
		path = current->content + 5;
	else
		return (NULL);
	return (path);
}

static int	ft_cd_err(t_exec_info *exec_info, t_parse *parse, char *path)
{
	if (ft_strncmp(parse->token->original, "$HOME", 5) == 0
		&& exec_info->builtin_parent == TRUE)
	{
		ft_printf_err("minishell: cd: HOME not set\n");
		return (NOT_HOME);
	}
	else if (exec_info->builtin_parent == TRUE)
	{
		ft_printf_err("minishell: cd: %s: ", path);
		return (ft_perror(SUCCESS));
	}
	else
		exit(EXIT_SUCCESS);
}

int	ft_cd_builtin(t_info *info, t_exec_info *exec_info, t_parse *parse)
{
	char	*path;

	path = exec_info->cmd[1];
	if (path == NULL)
		path = ft_find_path(info);
	if (path == NULL)
	{
		ft_printf_err("minishell: cd: HOME not set\n");
		g_child_exit_code = 1;
		return (FAILURE);
	}
	ft_change_oldpwd(info);
	if (chdir(path) == FAILURE)
	{
		g_child_exit_code = 1;
		ft_cd_err(exec_info, parse, path);
	}
	ft_change_pwd(info);
	if (exec_info->builtin_parent == TRUE)
		return (SUCCESS);
	else
		exit(EXIT_SUCCESS);
}
