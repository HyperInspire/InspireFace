#ifndef INSPIRE_EMBEDDING_DB_H
#define INSPIRE_EMBEDDING_DB_H

#include <sqlite3.h>
#include <vector>
#include <string>
#include <memory>
#include <stdexcept>
#include <mutex>

#define EMBEDDING_DB inspire::EmbeddingDB

namespace inspire
{

    // Search for most similar vectors
    struct FaceSearchResult
    {
        int64_t id;
        double similarity;
        std::vector<float> feature;
    };

    // Vector data structure
    struct VectorData
    {
        int64_t id; // This field is ignored in auto-increment mode
        std::vector<float> vector;
    };


    // ID mode enumeration
    enum class IdMode
    {
        AUTO_INCREMENT = 0, // Auto-incrementing ID
        MANUAL,        // Manually specify ID
    };
    
    class EmbeddingDB
    {
    public:
        ~EmbeddingDB();


        static EmbeddingDB &GetInstance();
        static void Init(const std::string &dbPath = ":memory:",
                         size_t vectorDim = 512,
                         IdMode idMode = IdMode::AUTO_INCREMENT);

        // Delete copy and move operations
        EmbeddingDB(const EmbeddingDB &) = delete;
        EmbeddingDB &operator=(const EmbeddingDB &) = delete;
        EmbeddingDB(EmbeddingDB &&) = delete;
        EmbeddingDB &operator=(EmbeddingDB &&) = delete;

        // Insert a single vector
        int64_t InsertVector(int64_t id, const std::vector<float> &vector);
        int64_t InsertVector(const std::vector<float> &vector); // For auto-increment mode

        // Batch insert vectors
        std::vector<int64_t> BatchInsertVectors(const std::vector<VectorData> &vectors);
        std::vector<int64_t> BatchInsertVectors(const std::vector<std::vector<float>> &vectors); // For auto-increment mode

        // Update vector
        void UpdateVector(int64_t id, const std::vector<float> &newVector);

        // Delete vector
        void DeleteVector(int64_t id);


        std::vector<FaceSearchResult> SearchSimilarVectors(
            const std::vector<float> &queryVector,
            size_t top_k = 3,
            float keep_similar_threshold = 0.5f,
            bool return_feature = false);

        // Get vector count
        int64_t GetVectorCount() const;

        // Get current ID mode
        IdMode GetIdMode() const { return idMode_; }

        bool IsInitialized() const { return initialized_; }

        // De-initialize database
        static void Deinit()
        {
            std::lock_guard<std::mutex> lock(instanceMutex_);
            if (instance_)
            {
                instance_.reset();
            }
        }
        

        std::vector<float> GetVector(int64_t id) const;



    private:
        // Constructor: add ID mode parameter
        explicit EmbeddingDB(const std::string &dbPath = ":memory:",
                             size_t vectorDim = 4,
                             const std::string &distanceMetric = "cosine",
                             IdMode idMode = IdMode::AUTO_INCREMENT);

    private:
        sqlite3 *db_;
        size_t vectorDim_;
        std::string tableName_;
        IdMode idMode_;
        bool initialized_ = false;

        // Helper functions
        void CheckVectorDimension(const std::vector<float> &vector) const;
        void ExecuteSQL(const std::string &sql);
        static void CheckSQLiteError(int rc, sqlite3 *db);
        int64_t GetLastInsertRowId() const;

    private:
        // Singleton related
        static std::unique_ptr<EmbeddingDB> instance_;
        static std::mutex instanceMutex_;

        // Database operation mutex
        mutable std::mutex dbMutex_;
    };

} // namespace inspire

#endif // INSPIRE_EMBEDDING_DB_H