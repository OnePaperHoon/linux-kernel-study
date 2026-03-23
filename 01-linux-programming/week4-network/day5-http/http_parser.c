// =============================================================================
// http_parser.c: HTTP 요청 파싱 구현
//
// 목적: recv()로 수신한 원시 HTTP 요청 문자열을 파싱해 t_http_request 구조체에 담는다.
//
// HTTP/1.1 요청 라인 형식:
//   "<메서드> <경로> <버전>\r\n"
//   ex) "GET /index.html HTTP/1.1\r\n"
//
// 이 파서는 첫 줄(요청 라인)만 파싱한다.
// 헤더와 바디는 정적 파일 서버에서는 필요하지 않으므로 무시한다.
//
// 파싱 흐름:
//   parse_http_request(raw, req)
//     → sscanf로 요청 라인에서 "메서드 경로 버전" 추출
//     → parse_method()로 메서드 문자열 → 열거형 변환
//     → req->valid = 1 설정 (성공 시)
// =============================================================================

#include "http_parser.h"
#include <string.h>
#include <stdio.h>

// -----------------------------------------------------------------------------
// parse_method: 메서드 문자열 → t_http_method 열거형 변환
// -----------------------------------------------------------------------------
static t_http_method    parse_method(const char *method_str)
{
    if (strncmp(method_str, "GET", 3) == 0)
        return (HTTP_GET);
    if (strncmp(method_str, "POST", 4) == 0)
        return (HTTP_POST);
    return (HTTP_UNKNOWN);
}

// -----------------------------------------------------------------------------
// parse_http_request: 원시 HTTP 요청 문자열 → t_http_request 구조체로 변환
//
// sscanf를 이용해 요청 라인 "메서드 경로 버전"을 파싱한다.
// 파싱 실패 시 req->valid = 0 상태로 반환 (호출자가 에러 처리)
// -----------------------------------------------------------------------------
void    parse_http_request(const char *raw, t_http_request *req)
{
    char    method_str[METHOD_LEN];

    // 구조체 초기화 (valid = 0, 나머지 필드 = 0)
    memset(req, 0, sizeof(t_http_request));

    // 요청 라인 파싱: "GET /index.html HTTP/1.1"
    // %7s  : 메서드 (최대 7자 → "OPTIONS\0" 수용)
    // %255s: 경로   (최대 255자)
    // %15s : 버전   (최대 15자 → "HTTP/1.1\0" 수용)
    if (sscanf(raw, "%7s %255s %15s", method_str, req->path, req->version) != 3)
        return ;    // 파싱 실패 → valid = 0 유지

    req->method = parse_method(method_str);
    req->valid  = 1;
}
