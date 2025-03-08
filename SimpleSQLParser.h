//
// Created by Hoeun Lee on 25. 3. 8.
//

#ifndef SIMPLE_SQL_PARSER_SIMPLESQLPARSER_H
#define SIMPLE_SQL_PARSER_SIMPLESQLPARSER_H

#include <string>
#include <vector>
#include "MappingTableManager.h"

/**
 * @brief 전처리 함수: 단일 SQL 쿼리를 처리하여,
 *   - relation alias, attribute alias 처리,
 *   - 암시적 relation 참조 보완,
 *   - 암시적 JOIN 변환,
 *   - 그리고 각 절(SELECT, FROM, WHERE)을 괄호로 묶어 표현
 * 를 수행합니다.
 *
 * @param query 원본 SQL 쿼리 (세미콜론 없이 단일 쿼리)
 * @param mtmanager 매핑 테이블 매니저
 * @return 전처리 후 쿼리 문자열
 */
std::string preprocessSQLQuery(std::string query, MappingTableManager& mtmanager);

/**
 * @brief 여러 SQL 쿼리를 세미콜론(;) 기준으로 분리하고,
 *        각 쿼리를 전처리한 결과를 vector<string>로 반환합니다.
 *
 * @param queries 복수 SQL 쿼리 문자열
 * @param mtmanager 매핑 테이블 매니저
 * @return 전처리된 쿼리들의 vector
 */
std::vector<std::string> preprocessSQLQueries(const std::string& queries, MappingTableManager& mtmanager);

#endif //SIMPLE_SQL_PARSER_SIMPLESQLPARSER_H
