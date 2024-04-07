//
// Created by Tunm-Air13 on 2024/4/7.
//
#pragma once
#ifndef INSPIREFACE_ENVIRO_H
#define INSPIREFACE_ENVIRO_H

#include <string>

class Enviro {
public:
    static Enviro& getInstance() {
        static Enviro instance;
        return instance;
    }

    Enviro(Enviro const&) = delete;
    void operator=(Enviro const&) = delete;

    std::string getPackName() const { return packName; }
    void setPackName(const std::string& name) { packName = name; }

private:
    Enviro() {}

    std::string packName = "Pikachu";
};

#endif //INSPIREFACE_ENVIRO_H
