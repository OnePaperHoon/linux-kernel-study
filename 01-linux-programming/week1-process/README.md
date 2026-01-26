# Week 1: Process & Memory Management

## 학습 목표
- fork, exec, wait 시스템 콜 마스터
- /proc 파일시스템 이해
- 프로세스 생명주기 학습

## 프로젝트

### 1. Mini Shell (from 42Seoul)
기존 minishell을 단순화하여 핵심 개념 정리
- [코드 보기](./mini-shell)
- 주요 학습: fork/exec/wait 조합

### 2. Process Tree Visualizer (New)
/proc를 활용한 프로세스 트리 출력 도구
- [코드 보기](./process-tree)
- 주요 학습: /proc 파일시스템, 프로세스 관계

## 배운 점

### fork() 동작 원리
- COW (Copy-on-Write)
- 자식 프로세스는 부모의 복사본
- return 값으로 부모/자식 구분

### exec 계열 함수
- execve: 직접 경로 지정
- execvp: PATH 자동 검색
- 성공하면 리턴 안함 (프로세스 교체)

### wait 계열 함수
- waitpid: 특정 자식 대기
- WEXITSTATUS: 종료 코드 확인
- 좀비 프로세스 방지

## 면접 예상 질문 & 답변

Q: fork 후 자식 프로세스는 부모의 메모리를 어떻게 가져오나요?
A: COW(Copy-on-Write) 방식으로 처음엔 공유하다가 수정 시 복사합니다.
   [process-tree 프로젝트에서 확인 가능]

Q: exec 계열 함수들의 차이점은?
A: execve는 환경변수와 인자를 직접 전달하고,
   execvp는 PATH에서 실행파일을 자동으로 찾습니다.
   [mini-shell에서 execvp 사용]

## 다음 주 계획
- Thread 기반 프로그래밍
- pthread API
- 동기화 (mutex, semaphore)