// =============================================================================
// http_parser.h: HTTP 요청 파싱 모듈 헤더
//
// 목적: HTTP/1.1 요청 메시지를 구조체로 변환하는 파서의 인터페이스를 정의한다.
//
// HTTP/1.1 요청 메시지 구조:
//   <메서드> <경로> <버전>\r\n        ← 요청 라인 (이 파서가 처리하는 부분)
//   <헤더이름>: <헤더값>\r\n          ← 헤더 (이 예제에서는 무시)
//   ...
//   \r\n                              ← 헤더 끝 구분자
//   [바디]                            ← GET에서는 없음
//
// 예시:
//   GET /index.html HTTP/1.1\r\n
//   Host: localhost:8080\r\n
//   Connection: keep-alive\r\n
//   \r\n
// =============================================================================

#ifndef HTTP_PARSER_H
# define HTTP_PARSER_H

# define METHOD_LEN     8       // "OPTIONS" + null 여유
# define PATH_LEN       256     // 요청 경로 최대 길이
# define VERSION_LEN    16      // "HTTP/1.1" + null 여유

// HTTP 메서드 열거형
// 이 서버는 GET만 처리하며, 나머지는 405 Method Not Allowed 반환
typedef enum e_http_method
{
    HTTP_GET,
    HTTP_POST,
    HTTP_UNKNOWN
}   t_http_method;

// HTTP 요청 구조체
// parse_http_request()의 출력 결과를 담는 POD(Plain Old Data) 구조체
// 헤더와 바디는 이 예제에서 파싱하지 않는다 (경로와 메서드만 사용)
typedef struct s_http_request
{
    t_http_method   method;             // 요청 메서드: HTTP_GET / HTTP_POST / HTTP_UNKNOWN
    char            path[PATH_LEN];     // 요청 경로 ex) "/", "/index.html", "/style.css"
    char            version[VERSION_LEN]; // HTTP 버전 ex) "HTTP/1.1"
    int             valid;              // 1: 정상 파싱 완료, 0: 파싱 실패
}   t_http_request;

// parse_http_request: 원시 HTTP 요청 버퍼 → t_http_request 구조체로 변환
// raw : recv()로 수신한 원시 요청 문자열 (null-terminated)
// req : 파싱 결과를 저장할 구조체 포인터
void    parse_http_request(const char *raw, t_http_request *req);

#endif
