//
// Created by tunm on 2023/5/5.
//

#include "parameter.h"

namespace hyper {


std::string Parameter::toString(int indent) const {
    if (indent != 0)
        return m_params.dump(indent);
    else
        return m_params.dump();

}

std::vector<std::string> Parameter::getNameList() const {
    std::vector<std::string> keys;
    for (const auto& element : m_params.items()) {
        keys.push_back(element.key());
    }
    return keys;
}

Parameter &Parameter::operator=(const Parameter &other) {
    if (this != &other) { // 检查自赋值
        m_params = other.m_params; // 使用 nlohmann::json 的赋值运算符进行深拷贝
    }
    return *this;
}


} // namespace hyper