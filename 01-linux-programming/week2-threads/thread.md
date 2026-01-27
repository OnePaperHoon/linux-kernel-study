# Threads

## 개념

- Process 내에서 실행되는 경량 실행 단위
- 메모리 공유로 빠른 통신
- Stack만 독립적

## Pthread API
### 생성
```c
pthread_t tid;
pthread_create(&tid, NULL, func, arg);
```

### 대기
```c
pthread_join(tid, NULL);
```

### 주의사항
- Thread 함수는 void * 반환
- 인자도 void*로 전달
- 캐스팅 필요



