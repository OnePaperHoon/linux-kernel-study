# /proc 파일 시스템

## 주요 파일들

### /proc/[pid]/stat
- 한 줄로 된 프로세스 정보
- 공백으로 구분
- 필요한 필드:
  * 1번: PID
  * 2번: 명령어 (괄호로 감싸짐)
  * 3번: State (R, S, D, Z, T)
  * 4번: PPID

예시:
```
1234 (myprogram) R 1233 1234 1234 0 -1 ...
```

### /proc/[pid]/cmdline
- 전체 명령어 라인
- null 문자로 구분
- argv[0], argv[1], ... 형태

### /proc/[pid]/status
- 사람이 읽기 쉬운 형태
- 키: 값 형식