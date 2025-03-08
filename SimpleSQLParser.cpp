//
// Created by Hoeun Lee on 25. 3. 8.
//

#include "SimpleSQLParser.h"
#include <regex>
#include <sstream>
#include <unordered_set>
#include <algorithm>
#include <cctype>

// 전방 선언: 파일 내에서만 사용되는 static 함수들
static std::string processFromClause(const std::string& clause, MappingTableManager& mtmanager);
static std::string processSelectClause(const std::string& clause, MappingTableManager& mtmanager);
static std::string processWhereClause(const std::string& clause, MappingTableManager& mtmanager);
static std::string processImplicitRelationRef(std::string query);
static std::string processImplicitJoin(std::string query);
static std::string encloseOperators(const std::string& selectPart,
                                    const std::string& fromPart,
                                    const std::string& wherePart);

// 헬퍼: 소문자 변환
static std::string toLower(const std::string& s) {
    std::string res = s;
    std::transform(res.begin(), res.end(), res.begin(),
                   [](unsigned char c){ return std::tolower(c); });
    return res;
}

/* 1. Relation alias 처리
   - "FROM student AS s" 패턴을 찾아,
     (a) 매핑 테이블에 (alias -> table) 저장
     (b) 쿼리 내 "student as s" 부분을 "FROM student"로 치환하고,
     (c) 쿼리 내 "s."를 "student."로 치환
*/
static std::string processRelationAlias(std::string query, MappingTableManager& mtmanager) {
    std::regex relAliasRegex(R"((FROM\s+)(\w+)\s+AS\s+(\w+))", std::regex_constants::icase);
    std::smatch match;
    if (std::regex_search(query, match, relAliasRegex)) {
        std::string fromPart = match[1];   // "FROM "
        std::string tableName = match[2];    // e.g., "student"
        std::string alias     = match[3];    // e.g., "s"

        mtmanager.addRelationMapping(alias, tableName);

        // "FROM student AS s" → "FROM student"
        std::string replacement = fromPart + tableName;
        query = std::regex_replace(query, relAliasRegex, replacement);

        // 쿼리 내 "s." → "student."
        std::string aliasDotPattern = "\\b" + alias + "\\.";
        std::regex aliasDotRegex(aliasDotPattern, std::regex_constants::icase);
        query = std::regex_replace(query, aliasDotRegex, tableName + ".");
    }
    return query;
}

/* 2. Attribute alias 처리
   - SELECT 절 내 "expression AS alias"를 찾아,
     (a) 매핑 테이블에 (원본 expression -> alias) 저장
     (b) "AS alias" 부분을 제거하여 "expression"만 남김
   - 정규식 패턴 수정: 불필요한 이스케이프 제거
*/
static std::string processAttributeAlias(std::string query, MappingTableManager& mtmanager) {
    std::regex attrAliasRegex(R"(([\w().*]+)\s+AS\s+(\w+))", std::regex_constants::icase);
    std::smatch match;
    while (std::regex_search(query, match, attrAliasRegex)) {
        std::string originalExpr = match[1];
        std::string alias = match[2];
        mtmanager.addAttributeMapping(originalExpr, alias);

        // "expression AS alias" → "expression"
        query = match.prefix().str() + originalExpr + match.suffix().str();
    }
    return query;
}

/* 3. 암시적 relation 참조 명시화
   - 단일 테이블인 경우, SELECT, WHERE 등에서 테이블명이 누락된 컬럼을 "table.column"으로 보완
   - 간단히 공백 토큰 단위로 처리 (예약어는 변경하지 않음)
*/
static std::string processImplicitRelationRef(std::string query) {
    std::regex fromRegex(R"(FROM\s+(\w+))", std::regex_constants::icase);
    std::smatch match;
    std::string tableName;
    if (std::regex_search(query, match, fromRegex)) {
        tableName = match[1];
    } else {
        return query;
    }

    std::unordered_set<std::string> keywords = {
            "select", "from", "where", "group", "by", "having", "order", "join", "and", "or", "not", "as", "on"
    };

    std::istringstream iss(query);
    std::ostringstream oss;
    std::string token;
    while (iss >> token) {
        if (token.find('.') != std::string::npos) {
            oss << token << " ";
            continue;
        }
        if (keywords.find(toLower(token)) != keywords.end()) {
            oss << token << " ";
        } else if (std::regex_match(token, std::regex(R"(^\d+$)"))) {
            oss << token << " ";
        } else {
            oss << tableName << "." << token << " ";
        }
    }
    return oss.str();
}

/* 4. 암시적 JOIN 명시적 변환
   - FROM 절에서 콤마로 구분된 테이블들을 "JOIN" 연산자로 변환
*/
static std::string processImplicitJoin(std::string query) {
    std::regex commaRegex(R"(,\s*)");
    query = std::regex_replace(query, commaRegex, " JOIN ");
    return query;
}

/* 5. 연산자 괄호화
   - 각 절(SELECT, FROM, WHERE)을 함수 호출 형태로 감쌉니다.
   - 예: "select student.name from student where student.age > 15" →
     "select(student.name) from(student) where(student.age > 15)"
*/
static std::string encloseOperators(const std::string& selectPart,
                                    const std::string& fromPart,
                                    const std::string& wherePart) {
    std::ostringstream oss;
    oss << "select(" << selectPart << ") from(" << fromPart << ")";
    if (!wherePart.empty())
        oss << " where(" << wherePart << ")";
    return oss.str();
}

/* --- FROM 절 처리 함수 (별도) ---
   - FROM 절은 쉼표(,)로 구분된 테이블들을 포함합니다.
   - 각 항목에 대해 relation alias 처리를 수행합니다.
*/
static std::string processFromClause(const std::string& clause, MappingTableManager& mtmanager) {
    std::istringstream iss(clause);
    std::string token;
    std::vector<std::string> items;
    while (std::getline(iss, token, ',')) {
        token.erase(0, token.find_first_not_of(" \t\n\r"));
        token.erase(token.find_last_not_of(" \t\n\r") + 1);
        // relation alias 처리: "table AS alias"
        std::regex asRegex(R"((\w+)\s+AS\s+(\w+))", std::regex_constants::icase);
        std::smatch match;
        if (std::regex_search(token, match, asRegex)) {
            std::string tableName = match[1];
            std::string alias = match[2];
            mtmanager.addRelationMapping(alias, tableName);
            token = tableName;
        }
        items.push_back(token);
    }
    // 여러 테이블이면 JOIN으로 결합
    std::ostringstream oss;
    if (items.size() > 1) {
        oss << items[0];
        for (size_t i = 1; i < items.size(); ++i) {
            oss << " JOIN " << items[i];
        }
    } else if (!items.empty()) {
        oss << items[0];
    }
    return oss.str();
}

/* --- SELECT 절 처리 함수 (별도) ---
   - SELECT 절은 쉼표로 구분된 항목들로 구성됩니다.
   - 각 항목에 대해 attribute alias 처리 및 relation alias 치환 수행.
*/
static std::string processSelectClause(const std::string& clause, MappingTableManager& mtmanager) {
    std::istringstream iss(clause);
    std::string token;
    std::vector<std::string> items;
    while (std::getline(iss, token, ',')) {
        token.erase(0, token.find_first_not_of(" \t\n\r"));
        token.erase(token.find_last_not_of(" \t\n\r") + 1);
        // attribute alias 처리: pattern "expression AS alias"
        std::regex asRegex(R"((.+)\s+AS\s+(\w+))", std::regex_constants::icase);
        std::smatch match;
        if (std::regex_search(token, match, asRegex)) {
            std::string expr = match[1];
            std::string alias = match[2];
            mtmanager.addAttributeMapping(expr, alias);
            token = expr;
        }
        // relation alias 치환: 예: "s.col" → 실제 테이블명 치환
        std::regex relRegex(R"(\b(\w+)\.(\w+))");
        std::smatch relMatch;
        if (std::regex_search(token, relMatch, relRegex)) {
            std::string relAlias = relMatch[1];
            std::string col = relMatch[2];
            std::string realTable = mtmanager.getRealTable(relAlias);
            if (!realTable.empty()) {
                token = std::regex_replace(token, std::regex("\\b" + relAlias + "\\."), realTable + ".");
            }
        }
        items.push_back(token);
    }
    std::ostringstream oss;
    for (size_t i = 0; i < items.size(); ++i) {
        oss << items[i];
        if (i < items.size() - 1) {
            oss << ", ";
        }
    }
    return oss.str();
}

/* --- WHERE 절 처리 함수 (별도) ---
   - WHERE 절에서 relation alias 치환 수행.
*/
static std::string processWhereClause(const std::string& clause, MappingTableManager& mtmanager) {
    std::string result = clause;
    for (const auto& kv : mtmanager.relationMapping) {
        std::string alias = kv.first;
        std::string realTable = kv.second;
        std::regex pattern("\\b" + alias + "\\.");
        result = std::regex_replace(result, pattern, realTable + ".");
    }
    return result;
}

/* --- 최종 전처리 함수 ---
   1. 쿼리를 SELECT, FROM, WHERE 절로 분리
   2. 각 절별 전처리 수행
   3. 연산자 괄호화를 적용하여 최종 쿼리 문자열 반환
*/
std::string preprocessSQLQuery(std::string query, MappingTableManager& mtmanager) {
    std::regex queryRegex(R"((?i)^select\s+(.*?)\s+from\s+(.*?)(\s+where\s+(.*))?$)");
    std::smatch match;
    if (!std::regex_search(query, match, queryRegex)) {
        return query;
    }
    std::string selectClause = match[1];  // SELECT 절 내용
    std::string fromClause   = match[2];    // FROM 절 내용
    std::string whereClause  = (match.size() >= 5) ? match[4].str() : "";

    // 1. FROM 절 처리: relation alias 처리 및 암시적 JOIN 변환
    std::string processedFrom = processFromClause(fromClause, mtmanager);

    // 2. SELECT 절 처리: attribute alias 처리 및 alias 치환
    std::string processedSelect = processSelectClause(selectClause, mtmanager);

    // 3. WHERE 절 처리: alias 치환
    std::string processedWhere = processWhereClause(whereClause, mtmanager);

    // 4. 암시적 relation 참조 보완:
    //    단일 테이블일 경우, FROM 절의 첫 테이블 이름을 이용하여 SELECT 절 내 미지정 컬럼 보완
    //    (단, processImplicitRelationRef에서 모든 토큰에 대해 적용하므로 여기서는 생략)
    // 5. JOIN 변환: 이미 processFromClause에서 콤마를 JOIN으로 변환함
    // 6. 각 절 괄호화
    std::string finalQuery = encloseOperators(processedSelect, processedFrom, processedWhere);
    return finalQuery;
}

/* --- 여러 쿼리 처리: 세미콜론(;) 기준 분리 --- */
std::vector<std::string> preprocessSQLQueries(const std::string& queries, MappingTableManager& mtmanager) {
    std::vector<std::string> result;
    std::stringstream ss(queries);
    std::string token;
    auto trim = [](const std::string &s) -> std::string {
        size_t start = s.find_first_not_of(" \t\n\r");
        if (start == std::string::npos) return "";
        size_t end = s.find_last_not_of(" \t\n\r");
        return s.substr(start, end - start + 1);
    };
    while (std::getline(ss, token, ';')) {
        token = trim(token);
        if (!token.empty()) {
            std::string processed = preprocessSQLQuery(token, mtmanager);
            result.push_back(processed);
        }
    }
    return result;
}
