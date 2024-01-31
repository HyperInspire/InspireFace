//
// Created by Tunm-Air13 on 2023/9/12.
//

#include "common/test_settings.h"
#include "inspireface/face_context.h"
#include "herror.h"
#include "test_helper/test_help.h"

using namespace inspire;

TEST_CASE("test_FaceFeatureManagement", "[face_feature]") {
    DRAW_SPLIT_LINE
    TEST_PRINT_OUTPUT(true);

    SECTION("FeatureCURD") {
        DRAW_SPLIT_LINE
        // 初始化
        FaceContext ctx;
        CustomPipelineParameter param;
        param.enable_recognition = true;
        auto ret = ctx.Configuration(GET_DATA("model_zip/T1"), DetectMode::DETECT_MODE_IMAGE, 1, param);
        REQUIRE(ret == HSUCCEED);

        ctx.FaceRecognitionModule()->PrintFeatureMatrixInfo();

        // 提前知道Kunkun的位置
        int32_t KunkunIndex = 795;
        // 提前准备一个人脸 并提取特征
        auto image = cv::imread(GET_DATA("images/kun.jpg"));
        CameraStream stream;
        stream.SetDataFormat(BGR);
        stream.SetRotationMode(ROTATION_0);
        stream.SetDataBuffer(image.data, image.rows, image.cols);
        ret = ctx.FaceDetectAndTrack(stream);
        REQUIRE(ret == HSUCCEED);
        // 检测人脸
        ctx.FaceDetectAndTrack(stream);
        const auto &faces = ctx.GetTrackingFaceList();
        REQUIRE(faces.size() > 0);
        // 对Kunkun进行特征抽取
        Embedded feature;
        ret = ctx.FaceRecognitionModule()->FaceExtract(stream, faces[0], feature);
        CHECK(ret == HSUCCEED);

        // 批量导入人脸特征向量
        String mat_path = GET_DATA("test_faceset/test_faces_A1.npy");
        String tags_path = GET_DATA("test_faceset/test_faces_A1.txt");
        auto result = LoadMatrixAndTags(mat_path, tags_path);
        // 获取特征矩阵和标签名称
        EmbeddedList featureMatrix = result.first;
        std::vector<std::string> tagNames = result.second;
        REQUIRE(featureMatrix.size() == 3000);
        REQUIRE(tagNames.size() == 3000);
        REQUIRE(featureMatrix[0].size() == 512);

        for (int i = 0; i < featureMatrix.size(); ++i) {
            auto &feat = featureMatrix[i];
            auto ret = ctx.FaceRecognitionModule()->RegisterFaceFeature(feat, i, tagNames[i], i);
            CHECK(ret == HSUCCEED);
        }

        std::cout << std::endl;
        REQUIRE(ctx.FaceRecognitionModule()->GetFaceFeatureCount() == 3000);
        spdlog::trace("3000个特征向量全部载入");

        // 准备一张人脸从库中进行查找
        SearchResult searchResult;
        ret = ctx.FaceRecognitionModule()->SearchFaceFeature(feature, searchResult, 0.5f);
        REQUIRE(ret == HSUCCEED);
        CHECK(searchResult.index != -1);
        CHECK(searchResult.index == KunkunIndex);
        CHECK(searchResult.tag == "Kunkun");
        CHECK(searchResult.score == Approx(0.76096).epsilon(1e-3));
        spdlog::info("找到Kunkun -> 位置ID: {}, 置信度: {}, Tag: {}", searchResult.index, searchResult.score, searchResult.tag.c_str());
        // 把Kunkun的库内特征存一下 等等改的操作需要用到
        Embedded KunkunFeature;
        ret = ctx.FaceRecognitionModule()->GetFaceFeature(KunkunIndex, KunkunFeature);
        REQUIRE(ret == HSUCCEED);

        // 把上面对应找到的Kunkun的库中特征从人脸库中进行删除
        ret = ctx.FaceRecognitionModule()->DeleteFaceFeature(searchResult.index);
        CHECK(ret == HSUCCEED);
        // 在搜索一次看看
        SearchResult secondSearchResult;
        ret = ctx.FaceRecognitionModule()->SearchFaceFeature(feature, secondSearchResult, 0.5f);
        REQUIRE(ret == HSUCCEED);
        CHECK(secondSearchResult.index == -1);
        spdlog::info("Kunkun被删除了无法找到: {}, {}", secondSearchResult.index, secondSearchResult.tag);

        // 随便找个位置 修改这个位置的特征向量 把Kunkun继续塞回去
        auto newIndex = 2888;
        // 先插个未被使用的位置试试
        ret = ctx.FaceRecognitionModule()->UpdateFaceFeature(KunkunFeature, 3001, "Chicken", 3001);
        REQUIRE(ret == HERR_CTX_REC_BLOCK_UPDATE_FAILURE);
        ret = ctx.FaceRecognitionModule()->UpdateFaceFeature(KunkunFeature, newIndex, "Chicken", 3001);
        REQUIRE(ret == HSUCCEED);
        SearchResult thirdlySearchResult;
        ret = ctx.FaceRecognitionModule()->SearchFaceFeature(feature, thirdlySearchResult, 0.5f);
        REQUIRE(ret == HSUCCEED);
        CHECK(thirdlySearchResult.index != -1);
        CHECK(thirdlySearchResult.index == newIndex);
        CHECK(thirdlySearchResult.tag == "Chicken");
        spdlog::info("再次找到Kunkun -> 新位置ID: {}, 置信度: {}, Tag: {}", thirdlySearchResult.index, thirdlySearchResult.score, thirdlySearchResult.tag.c_str());

    }

#ifdef ENABLE_BENCHMARK
    SECTION("FeatureSearchBenchmark") {
        DRAW_SPLIT_LINE

        // 初始化
        FaceContext ctx;
        CustomPipelineParameter param;
        param.enable_recognition = true;
        auto ret = ctx.Configuration(GET_DATA("model_zip/T1"), DetectMode::DETECT_MODE_IMAGE, 1, param);
        REQUIRE(ret == HSUCCEED);

        ctx.FaceRecognitionModule()->PrintFeatureMatrixInfo();

        // 批量导入人脸特征向量
        String mat_path = GET_DATA("test_faceset/test_faces_A1.npy");
        String tags_path = GET_DATA("test_faceset/test_faces_A1.txt");
        auto result = LoadMatrixAndTags(mat_path, tags_path);
        // 获取特征矩阵和标签名称
        EmbeddedList featureMatrix = result.first;
        std::vector<std::string> tagNames = result.second;
        REQUIRE(featureMatrix.size() == 3000);
        REQUIRE(tagNames.size() == 3000);
        REQUIRE(featureMatrix[0].size() == 512);

        for (int i = 0; i < featureMatrix.size(); ++i) {
            auto &feat = featureMatrix[i];
            auto ret = ctx.FaceRecognitionModule()->RegisterFaceFeature(feat, i, tagNames[i], i);
            CHECK(ret == HSUCCEED);
        }

        std::cout << std::endl;
        REQUIRE(ctx.FaceRecognitionModule()->GetFaceFeatureCount() == 3000);
        spdlog::trace("3000个特征向量全部载入");

        // 准备一张人脸
        auto image = cv::imread(GET_DATA("images/face_sample.png"));
        CameraStream stream;
        stream.SetDataFormat(BGR);
        stream.SetRotationMode(ROTATION_0);
        stream.SetDataBuffer(image.data, image.rows, image.cols);
        ret = ctx.FaceDetectAndTrack(stream);
        REQUIRE(ret == HSUCCEED);
        // 检测人脸
        ctx.FaceDetectAndTrack(stream);
        const auto &faces = ctx.GetTrackingFaceList();
        REQUIRE(faces.size() > 0);
        // 对Kunkun进行特征抽取
        Embedded feature;
        ret = ctx.FaceRecognitionModule()->FaceExtract(stream, faces[0], feature);
        CHECK(ret == HSUCCEED);

        // 将该人脸插入较为靠后的位置
        auto regIndex = 4000;
        ret = ctx.FaceRecognitionModule()->RegisterFaceFeature(feature, regIndex, "test", 4000);
        REQUIRE(ret == HSUCCEED);

        const auto loop = 1000;
        double total = 0.0f;
        spdlog::info("开始执行{}次搜索: ", loop);
        auto out = (double) cv::getTickCount();
        for (int i = 0; i < loop; ++i) {

            // 准备一张人脸从库中进行查找
            SearchResult searchResult;
            auto timeStart = (double) cv::getTickCount();
            ret = ctx.FaceRecognitionModule()->SearchFaceFeature(feature, searchResult, 0.5f);
            double cost = ((double) cv::getTickCount() - timeStart) / cv::getTickFrequency() * 1000;
            REQUIRE(ret == HSUCCEED);
            CHECK(searchResult.index == regIndex);
            total += cost;
        }
        auto end = ((double) cv::getTickCount() - out) / cv::getTickFrequency() * 1000;

        spdlog::info("{}次总耗时: {}ms, 平均耗时: {}ms", loop, end, total / loop);
    }
#endif
}