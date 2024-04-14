//
// Created by tunm on 2024/4/13.
//
#include <iostream>
#include "settings/test_settings.h"
#include "inspireface/c_api/inspireface.h"
#include "unit/test_helper/test_help.h"
#include <thread>

TEST_CASE("test_FeatureHubBase", "[FeatureHub][BasicFunction]") {
    DRAW_SPLIT_LINE
    TEST_PRINT_OUTPUT(true);

    SECTION("FeatureHub basic function") {
        HResult ret;
        HF_FeatureHubConfiguration configuration = {0};
        auto dbPath = GET_SAVE_DATA(".test");
        HString dbPathStr = new char[dbPath.size() + 1];
        std::strcpy(dbPathStr, dbPath.c_str());
        configuration.enablePersistence = 1;
        configuration.dbPath = dbPathStr;
        configuration.featureblockNum = 20;
        configuration.searchMode = HF_SEARCH_MODE_EXHAUSTIVE;
        configuration.searchThreshold = 0.48f;
        // Delete the previous data before testing
        if (std::remove(configuration.dbPath) != 0) {
            spdlog::trace("Error deleting file");
        }
        ret = HF_FeatureHubDataEnable(configuration);
        REQUIRE(ret == HSUCCEED);

        ret = HF_FeatureHubDataDisable();
        REQUIRE(ret == HSUCCEED);

        delete []dbPathStr;
    }


    SECTION("Repeat the enable and disable tests") {
        HResult ret;
        auto dbPath = GET_SAVE_DATA(".test");
        HString dbPathStr = new char[dbPath.size() + 1];
        HF_FeatureHubConfiguration configuration = {0};
        configuration.enablePersistence = 0;
        configuration.dbPath = dbPathStr;
        configuration.featureblockNum = 20;
        configuration.searchMode = HF_SEARCH_MODE_EXHAUSTIVE;
        configuration.searchThreshold = 0.48f;


        ret = HF_FeatureHubDataEnable(configuration);
        REQUIRE(ret == HSUCCEED);

        ret = HF_FeatureHubDataEnable(configuration);
        REQUIRE(ret == HERR_CTX_DB_ENABLE_REPETITION);

        ret = HF_FeatureHubDataDisable();
        REQUIRE(ret == HSUCCEED);

        ret = HF_FeatureHubDataDisable();
        REQUIRE(ret == HERR_CTX_DB_DISABLE_REPETITION);

        delete []dbPathStr;
    }

    SECTION("Only memory storage is used") {
        HResult ret;
        HF_FeatureHubConfiguration configuration = {0};
        configuration.enablePersistence = 0;
        ret = HF_FeatureHubDataEnable(configuration);
        REQUIRE(ret == HSUCCEED);

        ret = HF_FeatureHubDataDisable();
        REQUIRE(ret == HSUCCEED);

    }

}

TEST_CASE("test_ConcurrencyInsertion", "[FeatureHub][Concurrency]") {
    DRAW_SPLIT_LINE
    TEST_PRINT_OUTPUT(true);

    HResult ret;
    HF_FeatureHubConfiguration configuration = {0};
    auto dbPath = GET_SAVE_DATA(".test");
    HString dbPathStr = new char[dbPath.size() + 1];
    std::strcpy(dbPathStr, dbPath.c_str());
    configuration.enablePersistence = 1;
    configuration.dbPath = dbPathStr;
    configuration.featureblockNum = 20;
    configuration.searchMode = HF_SEARCH_MODE_EXHAUSTIVE;
    configuration.searchThreshold = 0.48f;
    // Delete the previous data before testing
    if (std::remove(configuration.dbPath) != 0) {
        spdlog::trace("Error deleting file");
    }
    ret = HF_FeatureHubDataEnable(configuration);
    REQUIRE(ret == HSUCCEED);

    HInt32 baseNum;
    ret = HF_FeatureHubGetFaceCount(&baseNum);
    REQUIRE(ret == HSUCCEED);

    HInt32 featureLength;
    HF_GetFeatureLength(&featureLength);

    const int numThreads = 4;
    const int insertsPerThread = 50;
    std::vector<std::thread> threads;
    auto beginGenId = 2000;

    for (int i = 0; i < numThreads; ++i) {
        threads.emplace_back([=]() {  // 使用值捕获以避免捕获引用后变量改变
            for (int j = 0; j < insertsPerThread; ++j) {
                auto feat = GenerateRandomFeature(featureLength);
                auto name = std::to_string(beginGenId + j + i * insertsPerThread);
                std::vector<char> nameBuffer(name.begin(), name.end());
                nameBuffer.push_back('\0');
                HF_FaceFeature feature = {0};
                feature.size = feat.size();
                feature.data = feat.data();
                HF_FaceFeatureIdentity featureIdentity = {0};
                featureIdentity.feature = &feature;
                featureIdentity.customId = beginGenId + j + i * insertsPerThread; // 确保 customId 唯一
                featureIdentity.tag = nameBuffer.data();
                auto ret = HF_FeatureHubInsertFeature(featureIdentity);
                REQUIRE(ret == HSUCCEED);
            }
        });
    }

    for (auto &th : threads) {
        th.join();
    }

    HInt32 count;
    ret = HF_FeatureHubGetFaceCount(&count);
    REQUIRE(ret == HSUCCEED);
    REQUIRE(count == baseNum + numThreads * insertsPerThread); // Ensure that the previous base data is added to the newly inserted data

    ret = HF_FeatureHubDataDisable();
    REQUIRE(ret == HSUCCEED);
}


TEST_CASE("test_ConcurrencyRemove", "[FeatureHub][Concurrency]") {
    DRAW_SPLIT_LINE
    TEST_PRINT_OUTPUT(true);

    HResult ret;
    HF_FeatureHubConfiguration configuration = {0};
    auto dbPath = GET_SAVE_DATA(".test");
    HString dbPathStr = new char[dbPath.size() + 1];
    std::strcpy(dbPathStr, dbPath.c_str());
    configuration.enablePersistence = 1;
    configuration.dbPath = dbPathStr;
    configuration.featureblockNum = 20;
    configuration.searchMode = HF_SEARCH_MODE_EXHAUSTIVE;
    configuration.searchThreshold = 0.48f;
    // Delete the previous data before testing
    if (std::remove(configuration.dbPath) != 0) {
        spdlog::trace("Error deleting file");
    }
    ret = HF_FeatureHubDataEnable(configuration);
    REQUIRE(ret == HSUCCEED);

    std::vector<std::vector<HFloat>> baseFeatures;
    size_t genSizeOfBase = 1000;
    HInt32 featureLength;
    HF_GetFeatureLength(&featureLength);

    REQUIRE(featureLength > 0);
    for (int i = 0; i < genSizeOfBase; ++i) {
        auto feat = GenerateRandomFeature(featureLength);
        baseFeatures.push_back(feat);
        auto name = std::to_string(i);
        // Establish a security buffer
        std::vector<char> nameBuffer(name.begin(), name.end());
        nameBuffer.push_back('\0');
        // Construct face feature
        HF_FaceFeature feature = {0};
        feature.size = feat.size();
        feature.data = feat.data();
        HF_FaceFeatureIdentity identity = {0};
        identity.feature = &feature;
        identity.customId = i;
        identity.tag = nameBuffer.data();
        ret = HF_FeatureHubInsertFeature(identity);
        REQUIRE(ret == HSUCCEED);
    }
    HInt32 totalFace;
    ret = HF_FeatureHubGetFaceCount(&totalFace);
    REQUIRE(ret == HSUCCEED);
    REQUIRE(totalFace == genSizeOfBase);

    const int numThreads = 4;
    const int removePerThread = genSizeOfBase / 5;
    std::vector<std::thread> threads;
    for (int t = 0; t < numThreads; ++t) {
        threads.emplace_back([&, t]() {
            for (int j = 0; j < removePerThread; ++j) {
                int idToRemove = t * removePerThread + j;
                auto ret = HF_FeatureHubFaceRemove(idToRemove);
                REQUIRE(ret == HSUCCEED);
            }
        });
    }
    // Wait for all threads to complete
    for (auto &th : threads) {
        th.join();
    }
    HInt32 remainingCount;
    ret = HF_FeatureHubGetFaceCount(&remainingCount);
    REQUIRE(ret == HSUCCEED);
    REQUIRE(remainingCount == genSizeOfBase - numThreads * removePerThread);
    TEST_PRINT("Remaining Count: {}", remainingCount);

    ret = HF_FeatureHubDataDisable();
    REQUIRE(ret == HSUCCEED);

}

TEST_CASE("test_ConcurrencySearch", "[FeatureHub][Concurrency]") {
    DRAW_SPLIT_LINE
    TEST_PRINT_OUTPUT(true);

    HResult ret;
    HF_FeatureHubConfiguration configuration = {0};
    auto dbPath = GET_SAVE_DATA(".test");
    HString dbPathStr = new char[dbPath.size() + 1];
    std::strcpy(dbPathStr, dbPath.c_str());
    configuration.enablePersistence = 1;
    configuration.dbPath = dbPathStr;
    configuration.featureblockNum = 20;
    configuration.searchMode = HF_SEARCH_MODE_EXHAUSTIVE;
    configuration.searchThreshold = 0.48f;
    // Delete the previous data before testing
    if (std::remove(configuration.dbPath) != 0) {
        spdlog::trace("Error deleting file");
    }
    ret = HF_FeatureHubDataEnable(configuration);
    REQUIRE(ret == HSUCCEED);

    std::vector<std::vector<HFloat>> baseFeatures;
    size_t genSizeOfBase = 1000;
    HInt32 featureLength;
    HF_GetFeatureLength(&featureLength);
    REQUIRE(featureLength > 0);
    for (int i = 0; i < genSizeOfBase; ++i) {
        auto feat = GenerateRandomFeature(featureLength);
        baseFeatures.push_back(feat);
        auto name = std::to_string(i);
        // Establish a security buffer
        std::vector<char> nameBuffer(name.begin(), name.end());
        nameBuffer.push_back('\0');
        // Construct face feature
        HF_FaceFeature feature = {0};
        feature.size = feat.size();
        feature.data = feat.data();
        HF_FaceFeatureIdentity identity = {0};
        identity.feature = &feature;
        identity.customId = i;
        identity.tag = nameBuffer.data();
        ret = HF_FeatureHubInsertFeature(identity);
        REQUIRE(ret == HSUCCEED);
    }
    HInt32 totalFace;
    ret = HF_FeatureHubGetFaceCount(&totalFace);
    REQUIRE(ret == HSUCCEED);
    REQUIRE(totalFace == genSizeOfBase);

    auto preDataSample = 200;

    // Generate some feature vectors that are similar to those of the existing database
    auto numberOfSimilar = preDataSample;
    auto targetIds = GenerateRandomNumbers(numberOfSimilar, 0, genSizeOfBase - 1);
    std::vector<std::vector<HFloat>> similarFeatures;
    for (int i = 0; i < numberOfSimilar; ++i) {
        auto index = targetIds[i];
        HF_FaceFeatureIdentity identity = {0};
        ret = HF_FeatureHubGetFaceIdentity(index, &identity);
        REQUIRE(ret == HSUCCEED);
        std::vector<HFloat> feature(identity.feature->data, identity.feature->data + identity.feature->size);
        auto simFeat = SimulateSimilarVector(feature);
        HF_FaceFeature simFeature = {0};
        simFeature.data = simFeat.data();
        simFeature.size = simFeat.size();
        HF_FaceFeature target = {0};
        target.data = identity.feature->data;
        target.size = identity.feature->size;
        HFloat cosine;
        ret = HF_FaceComparison1v1(target, simFeature, &cosine);
        REQUIRE(ret == HSUCCEED);
        REQUIRE(cosine > 0.80f);
        similarFeatures.push_back(feature);

    }
    REQUIRE(similarFeatures.size() == numberOfSimilar);

    auto numberOfNotSimilar = preDataSample;
    std::vector<std::vector<HFloat>> notSimilarFeatures;
    // Generate some feature vectors that are not similar to the existing database
    for (int i = 0; i < numberOfNotSimilar; ++i) {
        auto feat = GenerateRandomFeature(featureLength);
        HF_FaceFeature feature = {0};
        feature.size = feat.size();
        feature.data = feat.data();
        HF_FaceFeatureIdentity mostSim = {0};
        HFloat cosine;
        HF_FeatureHubFaceSearch(feature, &cosine, &mostSim);
        REQUIRE(cosine < 0.3f);
        notSimilarFeatures.push_back(feat);
    }
    REQUIRE(notSimilarFeatures.size() == numberOfNotSimilar);

    // Multithreaded search simulation
    const int numThreads = 5;
    std::vector<std::thread> threads;
    std::mutex mutex;

    // Start threads for concurrent searching
    for (int t = 0; t < numThreads; ++t) {
        threads.emplace_back([&]() {
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<> dis(0, preDataSample - 1);
            for (int j = 0; j < 50; ++j) { // Each thread performs 50 similar searches
                int idx = dis(gen);
                auto targetId = targetIds[idx];
                HF_FaceFeature feature = {0};
                feature.data = similarFeatures[idx].data();
                feature.size = similarFeatures[idx].size();
                HFloat score;
                HF_FaceFeatureIdentity identity = {0};
                HF_FeatureHubFaceSearch(feature, &score, &identity);
                CHECK(identity.customId == targetId);
            }
            for (int j = 0; j < 50; ++j) {
                int idx = dis(gen);
                HF_FaceFeature feature = {0};
                feature.data = notSimilarFeatures[idx].data();
                feature.size = notSimilarFeatures[idx].size();
                HFloat score;
                HF_FaceFeatureIdentity identity = {0};
                HF_FeatureHubFaceSearch(feature, &score, &identity);
                CHECK(identity.customId == -1);
            }
        });
    }
    for (auto &thread : threads) {
        thread.join();
    }

    ret = HF_FeatureHubDataDisable();
    REQUIRE(ret == HSUCCEED);
}


TEST_CASE("test_FeatureCache", "[FeatureHub][Concurrency]") {
    DRAW_SPLIT_LINE
    TEST_PRINT_OUTPUT(true);

    HResult ret;
    HF_FeatureHubConfiguration configuration = {0};
    auto dbPath = GET_SAVE_DATA(".test");
    HString dbPathStr = new char[dbPath.size() + 1];
    std::strcpy(dbPathStr, dbPath.c_str());
    configuration.enablePersistence = 1;
    configuration.dbPath = dbPathStr;
    configuration.featureblockNum = 20;
    configuration.searchMode = HF_SEARCH_MODE_EXHAUSTIVE;
    configuration.searchThreshold = 0.48f;
    // Delete the previous data before testing
    if (std::remove(configuration.dbPath) != 0) {
        spdlog::trace("Error deleting file");
    }
    ret = HF_FeatureHubDataEnable(configuration);
    REQUIRE(ret == HSUCCEED);

    auto randomVec = GenerateRandomFeature(512);
    HF_FaceFeature feature = {0};
    feature.data = randomVec.data();
    feature.size = randomVec.size();
    HF_FaceFeatureIdentity identity = {0};
    identity.feature = &feature;
    identity.tag = "FK";
    identity.customId = 12;

    ret = HF_FeatureHubInsertFeature(identity);
    REQUIRE(ret == HSUCCEED);

    auto simVec = SimulateSimilarVector(randomVec);
    HF_FaceFeature simFeature = {0};
    simFeature.data = simVec.data();
    simFeature.size = simVec.size();


    for (int i = 0; i < 10; ++i) {
        HF_FaceFeatureIdentity capture = {0};
        ret = HF_FeatureHubGetFaceIdentity(12, &capture);
        REQUIRE(ret == HSUCCEED);

        HF_FaceFeature target = {0};
        target.data = capture.feature->data;
        target.size = capture.feature->size;

        HFloat cosine;
        ret = HF_FaceComparison1v1(target, simFeature, &cosine);
        REQUIRE(cosine > 0.8f);
        REQUIRE(ret == HSUCCEED);

    }

    ret = HF_FeatureHubDataDisable();
    REQUIRE(ret == HSUCCEED);

}