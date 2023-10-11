//
// Created by Tunm-Air13 on 2023/10/11.
//
#pragma once
#ifndef HYPERFACEREPO_SQLITEFACEMANAGE_H
#define HYPERFACEREPO_SQLITEFACEMANAGE_H

#include "data_type.h"
#include "log.h"
#include "sqlite/sqlite3.h"  // Include the SQLite3 header
#include <vector>
#include <string>

namespace hyper {

typedef struct {
    int32_t customId;
    std::string tag;
    std::vector<float> feature;
} FaceFeatureInfo;

class HYPER_API SQLiteFaceManage {
public:
    SQLiteFaceManage();
    ~SQLiteFaceManage();

    int32_t OpenDatabase(const std::string& dbPath);
    int32_t CloseDatabase();

    int32_t CreateTable();
    int32_t InsertFeature(const FaceFeatureInfo& info);
    int32_t GetFeature(int32_t customId, FaceFeatureInfo& outInfo);
    int32_t DeleteFeature(int32_t customId);
    int32_t UpdateFeature(const FaceFeatureInfo& info);

    int32_t ViewTotal();

private:
    std::shared_ptr<sqlite3> m_db_;  // Pointer to the SQLite database

};

}   // namespace hyper

#endif //HYPERFACEREPO_SQLITEFACEMANAGE_H
