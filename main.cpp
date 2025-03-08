#include <iostream>

#include "SimpleSQLParser.h"
#include "MappingTableManager.h"
#include "SimpleSQLParser.h"

int main(int argc, char* argv[]) {
    {
        std::cout << "=== Test 1: Attribute Alias ===" << std::endl;
        MappingTableManager mt;
        std::string q1 = "select avg(s.age) as avg_age from student as s;";
        auto processedList = preprocessSQLQueries(q1, mt);
        for (const auto& p : processedList)
            std::cout << "[Processed] " << p << std::endl;
        mt.printMappings();
        std::cout << std::endl;
    }

    {
        std::cout << "=== Test 2: Relation Alias ===" << std::endl;
        MappingTableManager mt;
        std::string q2 = "select s.sid from student as s where s.age > 15;";
        auto processedList = preprocessSQLQueries(q2, mt);
        for (const auto& p : processedList)
            std::cout << "[Processed] " << p << std::endl;
        mt.printMappings();
        std::cout << std::endl;
    }

    {
        std::cout << "=== Test 3: Implicit Relation Reference ===" << std::endl;
        MappingTableManager mt;
        std::string q3 = "select age from student;";
        auto processedList = preprocessSQLQueries(q3, mt);
        for (const auto& p : processedList)
            std::cout << "[Processed] " << p << std::endl;
        mt.printMappings();
        std::cout << std::endl;
    }

    {
        std::cout << "=== Test 4: Implicit JOIN ===" << std::endl;
        MappingTableManager mt;
        std::string q4 = "select s.name, c.classid from student as s, classes as c where s.sid = c.sid;";
        auto processedList = preprocessSQLQueries(q4, mt);
        for (const auto& p : processedList)
            std::cout << "[Processed] " << p << std::endl;
        mt.printMappings();
        std::cout << std::endl;
    }

    {
        std::cout << "=== Test 5: Multiple Queries ===" << std::endl;
        MappingTableManager mt;
        std::string q5 = "select a from b; select c from d;";
        auto processedList = preprocessSQLQueries(q5, mt);
        for (const auto& p : processedList)
            std::cout << "[Processed] " << p << std::endl;
        std::cout << std::endl;
    }

    return 0;
}
