//
// Created by tunm on 2024/4/8.
//
#include "log.h"

// Static Logger initialization
LogManager* LogManager::instance = nullptr;
std::mutex LogManager::mutex;