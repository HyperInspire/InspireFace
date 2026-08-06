#include "embedding_db.h"
#include "sqlite-vec.h"
#include "isf_check.h"
#include <algorithm>
#include <random>
#include <sstream>
#if defined(__ANDROID__)
#include <android/log.h>
#endif

static std::string GenerateUUID() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(0, 15);
    static std::uniform_int_distribution<> dis2(8, 11);

    std::stringstream ss;
    ss << std::hex;
    for (int i = 0; i < 8; i++) ss << dis(gen);
    ss << "-";
    for (int i = 0; i < 4; i++) ss << dis(gen);
    ss << "-4";
    for (int i = 0; i < 3; i++) ss << dis(gen);
    ss << "-";
    ss << dis2(gen);
    for (int i = 0; i < 3; i++) ss << dis(gen);
    ss << "-";
    for (int i = 0; i < 12; i++) ss << dis(gen);
    return ss.str();
}

namespace inspire {

std::unique_ptr<EmbeddingDB> EmbeddingDB::instance_ = nullptr;
std::mutex EmbeddingDB::instanceMutex_;

EmbeddingDB &EmbeddingDB::GetInstance() {
    std::lock_guard<std::mutex> lock(instanceMutex_);
    INSPIREFACE_CHECK_MSG(instance_, "EmbeddingDB not initialized. Call Init() first.");
    return *instance_;
}

void EmbeddingDB::Init(const std::string &dbPath, size_t vectorDim, IdMode idMode) {
    std::lock_guard<std::mutex> lock(instanceMutex_);
    if (instance_) {
        INSPIRE_LOGW("EmbeddingDB already initialized, skipping duplicate initialization");
        return;
    }
    instance_.reset(new EmbeddingDB(dbPath, vectorDim, "cosine", idMode));
}

EmbeddingDB::EmbeddingDB(const std::string &dbPath, size_t vectorDim, const std::string &distanceMetric, IdMode idMode)
: vectorDim_(vectorDim), tableName_("vec_items"), idMode_(idMode) {
    int rc = sqlite3_auto_extension((void (*)())sqlite3_vec_init);
    CheckSQLiteError(rc, nullptr);

    // Open database
    rc = sqlite3_open(dbPath.c_str(), &db_);
    CheckSQLiteError(rc, db_);

    // // Create vector table
    // std::string createTableSQL = "CREATE VIRTUAL TABLE IF NOT EXISTS " + tableName_ + " USING vec0(embedding float[" + std::to_string(vectorDim_) +
    //                              "], tname TEXT, distance_metric=" + distanceMetric + ")";
    std::string createTableSQL = "CREATE VIRTUAL TABLE IF NOT EXISTS " + tableName_ +
        " USING vec0(embedding float[" + std::to_string(vectorDim_) + "], tname TEXT, uuid TEXT)";

    ExecuteSQL(createTableSQL);
    initialized_ = true;
}

EmbeddingDB::~EmbeddingDB() {
    if (db_) {
        sqlite3_close(db_);
    }
}

bool EmbeddingDB::InsertVector(const std::vector<float> &vector, int64_t &allocId, 
                            const std::string &tName, const std::string &tUUID) {
    std::lock_guard<std::mutex> lock(dbMutex_);
    return InsertVector(0, vector, allocId, tName, tUUID);
}

bool EmbeddingDB::InsertVector(int64_t id, const std::vector<float> &vector, int64_t &allocId, 
                            const std::string &tName, const std::string &tUUID) {
    CheckVectorDimension(vector);

    sqlite3_stmt *stmt;
    std::string sql;
    std::string uuid = tUUID;

    if (uuid == "")
        uuid = GenerateUUID();

    if (idMode_ == IdMode::AUTO_INCREMENT) {
        sql = "INSERT INTO " + tableName_ + " (tname, embedding, uuid) VALUES (?, ?, ?)";
    } else {
        sql = "INSERT INTO " + tableName_ + " (rowid, tname, embedding, uuid) VALUES (?, ?, ?, ?)";
    }

    int rc = sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        INSPIRE_LOGE("Failed to prepare statement: %s", sqlite3_errmsg(db_));
        sqlite3_finalize(stmt);
        return false;
    }

    if (idMode_ == IdMode::AUTO_INCREMENT) {
        sqlite3_bind_text(stmt, 1, tName.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_blob(stmt, 2, vector.data(), vector.size() * sizeof(float), SQLITE_STATIC);
        sqlite3_bind_text(stmt, 3, uuid.c_str(), -1, SQLITE_TRANSIENT);
    } else {
        sqlite3_bind_int64(stmt, 1, id);
        sqlite3_bind_text(stmt, 2, tName.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_blob(stmt, 3, vector.data(), vector.size() * sizeof(float), SQLITE_STATIC);
        sqlite3_bind_text(stmt, 4, uuid.c_str(), -1, SQLITE_TRANSIENT);
    }

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        INSPIRE_LOGE("Failed to insert vector: %s", sqlite3_errmsg(db_));
        return false;
    }

    allocId = idMode_ == IdMode::AUTO_INCREMENT ? GetLastInsertRowId() : id;
    return true;
}


std::vector<float> EmbeddingDB::GetVector(int64_t id) const {
    std::lock_guard<std::mutex> lock(dbMutex_);

    sqlite3_stmt *stmt;
    std::string sql = "SELECT embedding FROM " + tableName_ + " WHERE rowid = ?";

    int rc = sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr);
    CheckSQLiteError(rc, db_);

    sqlite3_bind_int64(stmt, 1, id);

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_ROW) {
        sqlite3_finalize(stmt);
        // throw std::runtime_error("Vector with id " + std::to_string(id) + " not found");
        return {};
    }

    const float *blob_data = static_cast<const float *>(sqlite3_column_blob(stmt, 0));
    size_t blob_size = sqlite3_column_bytes(stmt, 0) / sizeof(float);
    std::vector<float> result(blob_data, blob_data + blob_size);

    sqlite3_finalize(stmt);
    return result;
}

std::vector<int64_t> EmbeddingDB::BatchInsertVectors(const std::vector<VectorData> &vectors, 
                        const std::string &tName, const std::string &tUUID) {
    ExecuteSQL("BEGIN");
    std::vector<int64_t> insertedIds;
    insertedIds.reserve(vectors.size());

    for (const auto &data : vectors) {
        int64_t id = 0;
        bool ret = InsertVector(data.id, data.vector, id, tName, tUUID);
        INSPIREFACE_CHECK_MSG(ret, "Failed to insert vector");
        insertedIds.push_back(id);
    }
    ExecuteSQL("COMMIT");

    return insertedIds;
}

std::vector<int64_t> EmbeddingDB::BatchInsertVectors(const std::vector<std::vector<float>> &vectors, 
                        const std::string &tName, const std::string &tUUID) {
    ExecuteSQL("BEGIN");
    std::vector<int64_t> insertedIds;
    insertedIds.reserve(vectors.size());

    for (const auto &vector : vectors) {
        int64_t id = 0;
        bool ret = InsertVector(0, vector, id, tName, tUUID);
        INSPIREFACE_CHECK_MSG(ret, "Failed to insert vector");
        insertedIds.push_back(id);
    }
    ExecuteSQL("COMMIT");

    return insertedIds;
}

int64_t EmbeddingDB::GetLastInsertRowId() const {
    return sqlite3_last_insert_rowid(db_);
}

void EmbeddingDB::UpdateVector(int64_t id, const std::vector<float> &newVector) {
    CheckVectorDimension(newVector);

    sqlite3_stmt *stmt;
    std::string sql = "UPDATE " + tableName_ + " SET embedding = ? WHERE rowid = ?";

    int rc = sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr);
    CheckSQLiteError(rc, db_);

    sqlite3_bind_blob(stmt, 1, newVector.data(), newVector.size() * sizeof(float), SQLITE_STATIC);
    sqlite3_bind_int64(stmt, 2, id);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    INSPIREFACE_CHECK_MSG(rc == SQLITE_DONE, "Failed to update vector");
    if (sqlite3_changes(db_) == 0) {
        INSPIRE_LOGF("Vector with id %ld not found", id);
    }
}

void EmbeddingDB::DeleteVector(int64_t id) {
    sqlite3_stmt *stmt;
    std::string sql = "DELETE FROM " + tableName_ + " WHERE rowid = ?";

    int rc = sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr);
    CheckSQLiteError(rc, db_);

    sqlite3_bind_int64(stmt, 1, id);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    CheckSQLiteError(rc == SQLITE_DONE ? SQLITE_OK : rc, db_);
}

std::vector<FaceSearchResult> EmbeddingDB::SearchSimilarVectors(
        const std::vector<float> &queryVector,
        size_t top_k,
        float keep_similar_threshold,
        bool return_feature) {

    std::lock_guard<std::mutex> lock(dbMutex_);
    CheckVectorDimension(queryVector);

    sqlite3_stmt *stmt;
    std::string sql;

    if (return_feature) {
        sql = "SELECT rowid, distance, embedding, tname "
              "FROM " + tableName_ + " "
              "WHERE embedding MATCH ? "
              "ORDER BY distance "
              "LIMIT ?";
    } else {
        sql = "SELECT rowid, distance, tname "
              "FROM " + tableName_ + " "
              "WHERE embedding MATCH ? "
              "ORDER BY distance "
              "LIMIT ?";
    }

    int rc = sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr);
    CheckSQLiteError(rc, db_);

    sqlite3_bind_blob(stmt, 1, queryVector.data(), queryVector.size() * sizeof(float), SQLITE_STATIC);
    sqlite3_bind_int64(stmt, 2, top_k);

    std::vector<FaceSearchResult> results;
    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        FaceSearchResult result;
        result.id = sqlite3_column_int64(stmt, 0);

        // Extract distance (col 1) and compute similarity
        double distance_val = sqlite3_column_double(stmt, 1);
        
        // sqlite-vec returns L2 (Euclidean) distance for normalized vectors
        // For normalized vectors: cosine_similarity = 1 - (L2_distance^2 / 2)
        // This is because: L2_distance^2 = 2 * (1 - cosine_similarity)
        float l2_dist = static_cast<float>(distance_val);
        float similarity = 1.0f - (l2_dist * l2_dist / 2.0f);
        
        // Clamp for safety (embeddings should yield [0,1] similarity, but allow [-1,1])
        similarity = std::max(-1.0f, std::min(1.0f, similarity));
        result.similarity = similarity;

        if (return_feature) {
            // embedding is col 2, tname is col 3
            const float *blob_data = static_cast<const float *>(sqlite3_column_blob(stmt, 2));
            int blob_bytes = sqlite3_column_bytes(stmt, 2);
            if (blob_data && blob_bytes > 0 && (blob_bytes % sizeof(float) == 0)) {
                size_t blob_size = static_cast<size_t>(blob_bytes) / sizeof(float);
                result.feature.assign(blob_data, blob_data + blob_size);
            } else {
                // Handle null/empty/invalid blob
                INSPIRE_LOGW("Invalid embedding blob for ID %ld", result.id);
                result.feature.clear();
            }
            const unsigned char *tname_text = sqlite3_column_text(stmt, 3);
            if (tname_text) {
                result.tname = reinterpret_cast<const char*>(tname_text);
            }
        } else {
            // tname is col 2 when not returning feature
            const unsigned char *tname_text = sqlite3_column_text(stmt, 2);
            if (tname_text) {
                result.tname = reinterpret_cast<const char*>(tname_text);
            }
        }

        results.push_back(result);
    }

    sqlite3_finalize(stmt);

    // Filter based on real similarity threshold
    results.erase(std::remove_if(results.begin(), results.end(),
                                 [keep_similar_threshold](const FaceSearchResult &r) {
                                     return r.similarity < keep_similar_threshold;
                                 }),
                  results.end());

    return results;
}


int64_t EmbeddingDB::GetVectorCount() const {
    std::lock_guard<std::mutex> lock(dbMutex_);
    sqlite3_stmt *stmt;
    std::string sql = "SELECT COUNT(*) FROM " + tableName_;

    int rc = sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr);
    CheckSQLiteError(rc, db_);

    rc = sqlite3_step(stmt);
    CheckSQLiteError(rc == SQLITE_ROW ? SQLITE_OK : rc, db_);

    int64_t count = sqlite3_column_int64(stmt, 0);
    sqlite3_finalize(stmt);

    return count;
}

void EmbeddingDB::CheckVectorDimension(const std::vector<float> &vector) const {
    INSPIREFACE_CHECK_MSG(vector.size() == vectorDim_,
                          ("Vector dimension mismatch. Expected: " + std::to_string(vectorDim_) + ", Got: " + std::to_string(vector.size())).c_str());
}

void EmbeddingDB::ExecuteSQL(const std::string &sql) {
    std::lock_guard<std::mutex> lock(dbMutex_);
    char *errMsg = nullptr;
    int rc = sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, &errMsg);

    if (errMsg) {
        std::string error = errMsg;
        sqlite3_free(errMsg);
        INSPIREFACE_CHECK_MSG(false, ("SQL error: " + error).c_str());
    }

    CheckSQLiteError(rc, db_);
}

void EmbeddingDB::CheckSQLiteError(int rc, sqlite3 *db) {
    std::string error = db ? sqlite3_errmsg(db) : "SQLite error";
    INSPIREFACE_CHECK_MSG(rc == SQLITE_OK, error.c_str());
}

void EmbeddingDB::ShowTable() {
    if (!initialized_) {
        INSPIRE_LOGE("EmbeddingDB is not initialized");
        return;
    }

    std::lock_guard<std::mutex> lock(dbMutex_);
    sqlite3_stmt *stmt;

    std::string sql = "SELECT rowid, tname, uuid, embedding FROM " + tableName_;

    int rc = sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr);
    CheckSQLiteError(rc, db_);

    // Print header
#ifdef __ANDROID__
    __android_log_print(ANDROID_LOG_INFO, "EmbeddingDB", "=== Table Content ===");
    __android_log_print(ANDROID_LOG_INFO, "EmbeddingDB", "ID | tname | uuid | embedding_size");
    __android_log_print(ANDROID_LOG_INFO, "EmbeddingDB", "-----------------------------------------------------------");
#else
    printf("=== Table Content ===\n");
    printf("ID | tname | uuid | embedding_size\n");
    printf("-----------------------------------------------------------\n");
#endif

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int64_t id = sqlite3_column_int64(stmt, 0);
        const unsigned char *tname_text = sqlite3_column_text(stmt, 1);
        const unsigned char *uuid_text  = sqlite3_column_text(stmt, 2);
        const void *vector_blob = sqlite3_column_blob(stmt, 3);
        int blob_bytes = sqlite3_column_bytes(stmt, 3);

        std::string tname_str = tname_text ? reinterpret_cast<const char*>(tname_text) : "";
        std::string uuid_str  = uuid_text  ? reinterpret_cast<const char*>(uuid_text)  : "";

        size_t vector_size = blob_bytes / sizeof(float);

#ifdef __ANDROID__
        __android_log_print(ANDROID_LOG_INFO, "EmbeddingDB", "%" PRId64 " | %s | %s | %zu",
                            id, tname_str.c_str(), uuid_str.c_str(), vector_size);
#else
        printf("%" PRId64 " | %s | %s | %zu\n",
               id, tname_str.c_str(), uuid_str.c_str(), vector_size);
#endif
    }

    sqlite3_finalize(stmt);
}

std::vector<int64_t> EmbeddingDB::GetAllIds() {
    if (!initialized_) {
        INSPIRE_LOGE("EmbeddingDB is not initialized");
        return {};
    }
    std::lock_guard<std::mutex> lock(dbMutex_);
    std::vector<int64_t> ids;

    sqlite3_stmt *stmt;
    std::string sql = "SELECT rowid FROM " + tableName_;

    int rc = sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr);
    CheckSQLiteError(rc, db_);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        ids.push_back(sqlite3_column_int64(stmt, 0));
    }

    sqlite3_finalize(stmt);
    return ids;
}

std::vector<std::pair<std::string, std::string>> EmbeddingDB::GetAllTNames() {
    if (!initialized_) {
        INSPIRE_LOGE("EmbeddingDB is not initialized");
        return {};
    }

    std::lock_guard<std::mutex> lock(dbMutex_);
    std::vector<std::pair<std::string, std::string>> results;

    sqlite3_stmt *stmt;
    std::string sql = "SELECT DISTINCT tname, uuid FROM " + tableName_;

    int rc = sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr);
    CheckSQLiteError(rc, db_);

    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        const unsigned char *tname_text = sqlite3_column_text(stmt, 0);
        const unsigned char *uuid_text  = sqlite3_column_text(stmt, 1);

        std::string tname = tname_text ? reinterpret_cast<const char*>(tname_text) : "";
        std::string uuid  = uuid_text  ? reinterpret_cast<const char*>(uuid_text)  : "";

        results.emplace_back(tname, uuid);
    }

    sqlite3_finalize(stmt);
    return results;
}

}  // namespace inspire
