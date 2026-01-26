# Minishell

42Seoul 과제로 진행한 Unix 쉘 축소 버전 구현 프로젝트입니다.

## 프로젝트 개요

Minishell은 Bash 쉘의 핵심 기능들을 C언어로 구현한 프로젝트입니다. 파이프, 리다이렉션, 환경변수 처리, 빌트인 명령어 등 쉘의 주요 기능을 지원합니다.

## 주요 기능

### 지원하는 기능
- **파이프 (`|`)**: 여러 명령어 연결
- **리다이렉션**: `>` (출력), `>>` (출력 추가), `<` (입력)
- **Heredoc (`<<`)**: 여러 줄 입력
- **환경변수 확장**: `$VAR`, `$?` (이전 명령어 종료 코드)
- **따옴표 처리**: 작은따옴표(`'`), 큰따옴표(`"`)
- **시그널 처리**: `Ctrl+C`, `Ctrl+\`

### 빌트인 명령어

| 명령어 | 설명 |
|--------|------|
| `echo` | 문자열 출력 (`-n` 옵션 지원) |
| `cd` | 디렉토리 변경 |
| `pwd` | 현재 디렉토리 출력 |
| `export` | 환경변수 추가/수정 |
| `unset` | 환경변수 삭제 |
| `env` | 환경변수 목록 출력 |
| `exit` | 쉘 종료 |

## 디렉토리 구조

```
minishell/
├── Makefile
├── README.md
├── includes/
│   ├── minishell.h          # 메인 헤더
│   ├── libft.h
│   └── get_next_line.h
└── srcs/
    ├── main.c
    ├── 0_init_utils/        # 초기화
    ├── 1_parsing/           # 파싱
    ├── 2_exec_init/         # 실행 초기화
    ├── 3_exec/              # 실행 엔진
    ├── 4_builtin/           # 빌트인 명령어
    ├── 5_list_utils/        # 리스트 유틸리티
    ├── 6_utils/             # 유틸리티 함수
    ├── 7_subs_env/          # 환경변수 치환
    ├── gnl/                 # get_next_line
    └── libft/               # libft 라이브러리
```

## 실행 흐름

```
main()
  │
  ├── ft_init()              # 환경변수, 시그널 핸들러 초기화
  │
  ├── readline()             # 사용자 입력
  │
  ├── ft_substitute_env()    # 환경변수 치환
  │
  ├── ft_parse()             # 파싱
  │   ├── ft_count_token()
  │   ├── ft_tokenization()
  │   ├── ft_convert_env()
  │   ├── ft_remove_quote()
  │   └── ft_syntax_check()
  │
  ├── ft_make_exec_info()    # 실행 정보 생성
  │
  ├── ft_exec()              # 명령어 실행
  │   ├── ft_check_here_doc()
  │   ├── ft_make_child()
  │   ├── ft_set_pipe_fd()
  │   ├── ft_set_redirect_fd()
  │   └── ft_wait_child()
  │
  └── ft_free_all()          # 메모리 해제
```

## 주요 구조체

### t_token
토큰 정보를 저장합니다.
```c
typedef struct s_token
{
    int     type;      // WORD, PIPE, REDIRECT
    char    *original; // 원본 문자열
    char    *str;      // 처리된 문자열
    int     env_flag;  // 환경변수 포함 여부
}   t_token;
```

### t_exec_info
각 명령어의 실행 정보를 저장합니다.
```c
typedef struct s_exec_info
{
    char        *cmd_path;      // 명령어 경로
    char        **cmd;          // 명령어 및 인자 배열
    t_redirect  *redirect;      // 리다이렉션 배열
    pid_t       pid;            // 프로세스 ID
    int         use_pipe;       // 파이프 사용 여부
    int         pipe_fd[2];     // 파이프 파일 디스크립터
    int         infile_fd;      // 입력 파일 디스크립터
    int         outfile_fd;     // 출력 파일 디스크립터
    int         builtin_parent; // 부모에서 실행할 빌트인 여부
}   t_exec_info;
```

## 빌드 및 실행

```bash
# 빌드
make

# 실행
./minishell

# 정리
make clean   # .o 파일 삭제
make fclean  # 실행파일 + .o 파일 삭제
make re      # 재빌드
```

## 사용 예시

```bash
minishell$ echo "Hello World"
Hello World

minishell$ ls -la | grep minishell
-rwxr-xr-x  1 user  staff  12345 Jan 26 10:00 minishell

minishell$ cat < input.txt > output.txt

minishell$ export MY_VAR="test" && echo $MY_VAR
test

minishell$ cat << EOF
> Hello
> World
> EOF
Hello
World
```

---

## 개발 노트

### 해결한 이슈

#### env 전체 unset 후 /bin/ls 무한루프
`ft_make_envp`에서 무한 루프 발생 → 수정 완료

#### PWD 세그폴트
```c
// ft_cd_builtin.c
ft_change_pwd(){
    current = info->mini_ev.front_node;
    while (current && ft_strncmp(current->content, "PWD=", 4))
        current = current->next_node;
    if (current == NULL)
        return ; // PWD가 없을때 처리
}
```

#### `.` 또는 `..` 단일 명령어 에러
```c
// ft_cmd_error_sup 함수 추가
int ft_cmd_error_sup(t_exec_info *exec_info)
{
    if (exec_info->cmd_path[0] == '.'
        && exec_info->cmd_path[1] == '.'
        && exec_info->cmd_path[2] == '\0')
        return (FAILURE);
    else if (exec_info->cmd_path[0] == '.'
        && exec_info->cmd_path[1] == '\0')
    {
        ft_printf_err("minishell: .: filename argument required\n\
.: usage: . filename [arguments]\n");
        exit(2);
    }
    return (SUCCESS);
}
```

#### `/`, `./`, `../` 경로 처리
```c
// 기존: if (ft_strchr(cmd_path, '/') != 0)
// /가 하나라도 있으면 경로로 처리하도록 변경
if ((cmd_path[0] == '.' && cmd_path[1] == '/') || cmd_path[0] == '/')
{
    ft_cmd_is_directory(cmd_path);
    ft_cmd_path_error_handle(exec_info, cmd_path);
    if (access(cmd_path, X_OK) == SUCCESS)
        return (SUCCESS);
}
```

### 알려진 제한사항
- `||` 연산자 미지원
- `cat` 입력 중 `Ctrl+C` + `Ctrl+\` 조합 시 출력 이상

### 테스트 명령어
```bash
# 환경변수 전체 unset 테스트
env | cut -d '=' -f 1 | tr '\n' ' ' | pbcopy
unset [붙여넣기]
```

---

## 라이선스

42Seoul 교육 프로젝트
