//
// Created by Hoeun Lee on 25. 3. 8.
//

#include "MappingTableManager.h"

void MappingTableManager::addRelationMapping(const std::string& alias, const std::string& realTable) {
    relationMapping[alias] = realTable;
}

std::string MappingTableManager::getRealTable(const std::string& alias) const {
    auto it = relationMapping.find(alias);
    return (it != relationMapping.end()) ? it->second : "";
}

void MappingTableManager::addAttributeMapping(const std::string& originalExpr, const std::string& alias) {
    attributeMapping[originalExpr] = alias;
}

std::string MappingTableManager::getAttributeAlias(const std::string& originalExpr) const {
    auto it = attributeMapping.find(originalExpr);
    return (it != attributeMapping.end()) ? it->second : "";
}

void MappingTableManager::clear() {
    relationMapping.clear();
    attributeMapping.clear();
}

void MappingTableManager::printMappings() const {
    std::cout << "[Relation Mapping]" << std::endl;
    for (const auto& p : relationMapping) {
        std::cout << "  alias: " << p.first << " -> table: " << p.second << std::endl;
    }
    std::cout << "[Attribute Mapping]" << std::endl;
    for (const auto& p : attributeMapping) {
        std::cout << "  " << p.first << " -> " << p.second << std::endl;
    }
}
