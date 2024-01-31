//
// Created by tunm on 2023/5/5.
//

#pragma once
#ifndef HYPERAI_PARAMETER_H
#define HYPERAI_PARAMETER_H
#include "nlohmann/json.hpp"
#include <iostream>

using nlohmann::json;

#ifndef HYPER_API
#define HYPER_API
#endif

namespace inspire {

/**
 * @class Parameter
 * @brief Class for managing parameters as JSON data.
 *
 * This class provides methods to set, get, and load parameters as JSON data.
 */
class HYPER_API Parameter {
public:
    Parameter() = default;;

    Parameter(const Parameter& p) : m_params(p.m_params) {}

    virtual ~Parameter() {}

    Parameter& operator=(const Parameter& other);

    /**
     * @brief Get a list of parameter names.
     * @return std::vector<std::string> A list of parameter names.
     */
    std::vector<std::string> getNameList() const;

    /**
     * @brief Convert parameters to a formatted JSON string.
     * @param indent The indentation level for formatting.
     * @return std::string The formatted JSON string representing the parameters.
     */
    std::string toString(int indent = 4) const;

    /**
     * @brief Check if a parameter exists by name.
     * @param name The name of the parameter to check.
     * @return bool True if the parameter exists, false otherwise.
     */
    bool have(const std::string& name) const noexcept {
        return m_params.contains(name);
    }

    /**
     * @brief Set a parameter with a specific name and value.
     * @param name The name of the parameter.
     * @param value The value to set for the parameter.
     */
    template <typename ValueType>
    void set(const std::string& name, const ValueType& value) {
        m_params[name] = value;
    }

    /**
     * @brief Set a parameter with a specific name and value.
     * @param name The name of the parameter.
     * @param value The value to set for the parameter.
     */
    template <typename ValueType>
    ValueType get(const std::string& name) const {
        if (!have(name)) {
            throw std::out_of_range("out_of_range in Parameter::get : " + name);
        }
        return m_params.at(name).get<ValueType>();
    }

    /**
     * @brief Load parameters from a JSON object.
     * @param j The JSON object containing parameters.
     */
    void load(const nlohmann::json& j) {
        for (const auto& item : j.items()) {
            const auto& key = item.key();
            const auto& value = item.value();

            if (value.is_boolean()) {
                set<bool>(key, value.get<bool>());
            } else if (value.is_number_integer()) {
                set<int>(key, value.get<int>());
            } else if (value.is_number_float()) {
                set<float>(key, value.get<float>());
            } else if (value.is_string()) {
                set<std::string>(key, value.get<std::string>());
            } else if (value.is_array()) {
                if (!value.empty()) {
                    if (value[0].is_number_integer()) {
                        set<std::vector<int>>(key, value.get<std::vector<int>>());
                    } else if (value[0].is_number_float()) {
                        set<std::vector<float>>(key, value.get<std::vector<float>>());
                    } // add more types as needed
                    // ...
                }
            }
            // Add handling for other types as needed
        }
    }


private:
    json m_params;      ///< JSON object to store parameters.
};

#define PARAMETERIZATION_SUPPORT                                                                                 \
protected:                                                                                                \
    inspire::Parameter m_params;                                                                            \
                                                                                                          \
public:                                                                                                   \
    const inspire::Parameter& getParameter() const {                                                        \
        return m_params;                                                                                  \
    }                                                                                                     \
                                                                                                          \
    void setParameter(const inspire::Parameter& param) {                                                    \
        m_params = param;                                                                                 \
    }                                                                                                     \
                                                                                                          \
    bool haveParam(const std::string& name) const noexcept {                                                   \
        return m_params.have(name);                                                                       \
    }                                                                                                     \
                                                                                                          \
    template <typename ValueType>                                                                         \
    void setParam(const std::string& name, const ValueType& value) {                                           \
        m_params.set<ValueType>(name, value);                                                             \
    }                                                                                                     \
                                                                                                          \
    template <typename ValueType>                                                                         \
    ValueType getParam(const std::string& name) const {                                                        \
        return m_params.get<ValueType>(name);                                                             \
    }                                                                                                     \
                                                                                                          \
    template <typename ValueType>                                                                         \
    void _initSingleParam(const inspire::Parameter& param, const std::string& name,                              \
                          const ValueType& default_value) {                                               \
        if (param.have(name)) {                                                                           \
            setParam<ValueType>(name, param.get<ValueType>(name));                                        \
        } else {                                                                                          \
            setParam<ValueType>(name, default_value);                                                     \
        }                                                                                                 \
    }                                                                                                     \
    void loadParam(const nlohmann::json& j) {                                                                  \
        m_params.load(j);                                                                                 \
    }


} // namespace hyper
#endif //HYPERAI_PARAMETER_H
