#include <iostream>
#include <fstream>
#include "json.hpp"
#include <vector>
#include <string>
#include <optional>

using json = nlohmann::json;

// Структуры для хранения данных
struct Field {
    std::string f;
    std::optional<std::string> value;
    std::optional<json> op;  // Может содержать вложенные структуры
};

struct Operation {
    std::string op;
    json branch_true;  // Может содержать сложные вложенные структуры
};

struct Row {
    int id;
    std::string variable;
    std::optional<std::string> value;
    std::optional<Field> field;
    std::optional<Operation> operation;
};

// Функция для чтения JSON из файла
json read_json_from_file(const std::string& filename) {
    std::ifstream input_file(filename);
    if (!input_file.is_open()) {
        throw std::runtime_error("Could not open file: " + filename);
    }
    return json::parse(input_file);
}

// Функция парсинга Field
Field parse_field(const json& j) {
    Field f;
    f.f = j["f"].get<std::string>();
    
    if (j.contains("value")) {
        if (j["value"].is_string()) {
            f.value = j["value"].get<std::string>();
        } else if (j["value"].is_null()) {
            f.value = std::nullopt;
        }
    }
    
    if (j.contains("op")) {
        f.op = j["op"];
    }
    
    return f;
}

// Функция парсинга Operation
Operation parse_operation(const json& j) {
    Operation op;
    op.op = j["op"].get<std::string>();
    op.branch_true = j["branch true"];
    return op;
}

int main() {
    try {
        // 1. Чтение JSON из файла
        json j = read_json_from_file("input.json");

        // 2. Парсинг данных
        std::vector<Row> rows;
        for (const auto& item : j["code"]["rows"]) {
            Row row;
            row.id = item["id"].get<int>();
            
            if (item.contains("variable")) {
                row.variable = item["variable"].get<std::string>();
            }
            
            if (item.contains("value")) {
                if (item["value"].is_string()) {
                    row.value = item["value"].get<std::string>();
                } else if (item["value"].is_null()) {
                    row.value = std::nullopt;
                } else if (item["value"].is_object()) {
                    // Для случаев, когда value - объект (как в id:86)
                    row.value = item["value"].dump();
                }
            }
            
            if (item.contains("field")) {
                row.field = parse_field(item["field"]);
            }
            
            if (item.contains("operation")) {
                row.operation = parse_operation(item["operation"]);
            }
            
            rows.push_back(row);
        }

        // 3. Вывод результатов
        std::cout << "Parsed " << rows.size() << " rows:\n";
        for (const auto& row : rows) {
            std::cout << "\nID: " << row.id << "\n";
            std::cout << "Variable: " << row.variable << "\n";
            
            if (row.value) {
                std::cout << "Value: " << *row.value << "\n";
            }
            
            if (row.field) {
                std::cout << "Field:\n";
                std::cout << "  f: " << row.field->f << "\n";
                if (row.field->value) {
                    std::cout << "  value: " << *row.field->value << "\n";
                }
                
                if (row.field->op) {
                    std::cout << "  op: " << row.field->op->dump(2) << "\n";
                }
            }
            
            if (row.operation) {
                std::cout << "Operation:\n";
                std::cout << "  op: " << row.operation->op << "\n";
                std::cout << "  branch_true: " << row.operation->branch_true.dump(2) << "\n";
            }
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}