//
// Created by Hoeun Lee on 25. 3. 8.
//

#ifndef SIMPLE_SQL_PARSER_MAPPINGTABLEMANAGER_H
#define SIMPLE_SQL_PARSER_MAPPINGTABLEMANAGER_H

#include <unordered_map>
#include <string>
#include <iostream>

class MappingTableManager {
public:
    // Relation alias 매핑: alias -> 실제 테이블명
    // 예: "s" -> "student", "c" -> "classes"
    std::unordered_map<std::string, std::string> relationMapping;

    // Attribute alias 매핑: 원본 attribute 표현식 -> alias
    // 예: "avg(student.age)" -> "avg_age"
    std::unordered_map<std::string, std::string> attributeMapping;

    void addRelationMapping(const std::string& alias, const std::string& realTable);
    std::string getRealTable(const std::string& alias) const;

    void addAttributeMapping(const std::string& originalExpr, const std::string& alias);
    std::string getAttributeAlias(const std::string& originalExpr) const;

    void clear();
    void printMappings() const;
};

#endif //SIMPLE_SQL_PARSER_MAPPINGTABLEMANAGER_H
