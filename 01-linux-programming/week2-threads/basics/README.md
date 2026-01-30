# Day 3: Thread Basics

Thread 생성과 Race Condition 기초

## 프로젝트

### 1. [hello-thread](./hello-thread)
- pthread_create/join 기초
- Thread 함수 작성
- 인자 전달/반환

### 2. [multi-threads](./multi-threads)
- 여러 thread 관리
- Thread 배열
- 실행 순서 관찰

### 3. [shared-counter](./shared-counter)
- Race condition 데모
- 동기화 필요성 확인
- 내일 Mutex로 해결

## 실행
```bash
cd hello-thread && make test
cd multi-threads && make test
cd shared-counter && make test
```

## 배운 것
- pthread API 사용법
- Thread는 메모리 공유
- Race condition 발생 조건
- Critical section 개념

## 다음
Day 4: Mutex와 Condition Variable