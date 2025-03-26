#include <inspirecv/inspirecv.h>
#include <inspireface/track_module/face_track_module.h>
#include <inspireface/include/inspireface/launch.h>
#include <inspireface/include/inspireface/frame_process.h>
#include <inspireface/pipeline_module/face_pipeline_module.h>
#include <inspireface/common/face_data/face_serialize_tools.h>
#include <inspireface/include/inspireface/feature_hub_db.h>

using namespace inspire;

static std::vector<float> FT = {
  0.0706566,   0.00640248,  0.0418103,   -0.00597861, 0.0269879,   0.0187478,   0.0486305,   0.0349162,   -0.0080779,  -0.0550556,  0.0229963,
  -0.00683422, -0.0338589,  0.0533989,   -0.0371725,  0.000972469, 0.0612415,   0.0389846,   -0.00126743, -0.0128782,  0.0935529,   0.0588179,
  0.0164787,   -0.00732871, -0.0458209,  -0.0100137,  -0.0372892,  0.000871123, 0.0245121,   -0.0811471,  -0.00481095, 0.0266868,   0.0712961,
  -0.0675362,  -0.0117453,  0.0658745,   -0.0694139,  -0.00704822, -0.0237313,  0.0209365,   0.0131902,   0.00192449,  -0.0593105,  0.0191942,
  -0.00625798, 0.00748682,  0.0533557,   0.0314002,   -0.0627113,  0.0827862,   0.00336722,  -0.0191575,  -0.0180252,  0.0150318,   -0.0686462,
  0.0465634,   0.0627244,   0.0449248,   -0.037054,   -0.0486668,  0.040752,    0.0143315,   -0.0763842,  -0.0161973,  0.0319588,   0.0112792,
  -0.102007,   0.0649219,   0.0630833,   0.0421069,   0.0519043,   -0.084082,   0.0249516,   0.023046,    0.071994,    -0.0272229,  0.0167103,
  -0.00694243, 0.0366775,   0.0672882,   0.0122419,   -0.0233413,  -0.0144258,  -0.012853,   -0.0202025,  0.000983093, -0.00776073, -0.0268638,
  0.00682446,  0.0262906,   -0.0407654,  -0.0144264,  -0.0310807,  0.0596711,   0.0238081,   -0.0138019,  0.000502882, 0.0496892,   0.0126823,
  0.0511028,   -0.0310699,  -0.0322141,  0.00996936,  0.0675392,   -0.0164277,  0.0930009,   -0.037467,   0.0419618,   -0.00358901, -0.0309569,
  -0.0225608,  -0.0332198,  0.00102291,  0.108814,    -0.0831313,  0.048208,    -0.0277542,  -0.061584,   0.0721224,   -0.0795082,  0.0340047,
  0.056139,    -0.0166783,  -0.0803042,  -0.014245,   -0.0476374,  0.048495,    0.0378856,   0.0706566,   0.00640248,  0.0418103,   -0.00597861,
  0.0269879,   0.0187478,   0.0486305,   0.0349162,   -0.0080779,  -0.0550556,  0.0229963,   -0.00683422, -0.0338589,  0.0533989,   -0.0371725,
  0.000972469, 0.0612415,   0.0389846,   -0.00126743, -0.0128782,  0.0935529,   0.0588179,   0.0164787,   -0.00732871, -0.0458209,  -0.0100137,
  -0.0372892,  0.000871123, 0.0245121,   -0.0811471,  -0.00481095, 0.0266868,   0.0712961,   -0.0675362,  -0.0117453,  0.0658745,   -0.0694139,
  -0.00704822, -0.0237313,  0.0209365,   0.0131902,   0.00192449,  -0.0593105,  0.0191942,   -0.00625798, 0.00748682,  0.0533557,   0.0314002,
  -0.0627113,  0.0827862,   0.00336722,  -0.0191575,  -0.0180252,  0.0150318,   -0.0686462,  0.0465634,   0.0627244,   0.0449248,   -0.037054,
  -0.0486668,  0.040752,    0.0143315,   -0.0763842,  -0.0161973,  0.0319588,   0.0112792,   -0.102007,   0.0649219,   0.0630833,   0.0421069,
  0.0519043,   -0.084082,   0.0249516,   0.023046,    0.071994,    -0.0272229,  0.0167103,   -0.00694243, 0.0366775,   0.0672882,   0.0122419,
  -0.0233413,  -0.0144258,  -0.012853,   -0.0202025,  0.000983093, -0.00776073, -0.0268638,  0.00682446,  0.0262906,   -0.0407654,  -0.0144264,
  -0.0310807,  0.0596711,   0.0238081,   -0.0138019,  0.000502882, 0.0496892,   0.0126823,   0.0511028,   -0.0310699,  -0.0322141,  0.00996936,
  0.0675392,   -0.0164277,  0.0930009,   -0.037467,   0.0419618,   -0.00358901, -0.0309569,  -0.0225608,  -0.0332198,  0.00102291,  0.108814,
  -0.0831313,  0.048208,    -0.0277542,  -0.061584,   0.0721224,   -0.0795082,  0.0340047,   0.056139,    -0.0166783,  -0.0803042,  -0.014245,
  -0.0476374,  0.048495,    0.0378856,   0.0706566,   0.00640248,  0.0418103,   -0.00597861, 0.0269879,   0.0187478,   0.0486305,   0.0349162,
  -0.0080779,  -0.0550556,  0.0229963,   -0.00683422, -0.0338589,  0.0533989,   -0.0371725,  0.000972469, 0.0612415,   0.0389846,   -0.00126743,
  -0.0128782,  0.0935529,   0.0588179,   0.0164787,   -0.00732871, -0.0458209,  -0.0100137,  -0.0372892,  0.000871123, 0.0245121,   -0.0811471,
  -0.00481095, 0.0266868,   0.0712961,   -0.0675362,  -0.0117453,  0.0658745,   -0.0694139,  -0.00704822, -0.0237313,  0.0209365,   0.0131902,
  0.00192449,  -0.0593105,  0.0191942,   -0.00625798, 0.00748682,  0.0533557,   0.0314002,   -0.0627113,  0.0827862,   0.00336722,  -0.0191575,
  -0.0180252,  0.0150318,   -0.0686462,  0.0465634,   0.0627244,   0.0449248,   -0.037054,   -0.0486668,  0.040752,    0.0143315,   -0.0763842,
  -0.0161973,  0.0319588,   0.0112792,   -0.102007,   0.0649219,   0.0630833,   0.0421069,   0.0519043,   -0.084082,   0.0249516,   0.023046,
  0.071994,    -0.0272229,  0.0167103,   -0.00694243, 0.0366775,   0.0672882,   0.0122419,   -0.0233413,  -0.0144258,  -0.012853,   -0.0202025,
  0.000983093, -0.00776073, -0.0268638,  0.00682446,  0.0262906,   -0.0407654,  -0.0144264,  -0.0310807,  0.0596711,   0.0238081,   -0.0138019,
  0.000502882, 0.0496892,   0.0126823,   0.0511028,   -0.0310699,  -0.0322141,  0.00996936,  0.0675392,   -0.0164277,  0.0930009,   -0.037467,
  0.0419618,   -0.00358901, -0.0309569,  -0.0225608,  -0.0332198,  0.00102291,  0.108814,    -0.0831313,  0.048208,    -0.0277542,  -0.061584,
  0.0721224,   -0.0795082,  0.0340047,   0.056139,    -0.0166783,  -0.0803042,  -0.014245,   -0.0476374,  0.048495,    0.0378856,   0.0706566,
  0.00640248,  0.0418103,   -0.00597861, 0.0269879,   0.0187478,   0.0486305,   0.0349162,   -0.0080779,  -0.0550556,  0.0229963,   -0.00683422,
  -0.0338589,  0.0533989,   -0.0371725,  0.000972469, 0.0612415,   0.0389846,   -0.00126743, -0.0128782,  0.0935529,   0.0588179,   0.0164787,
  -0.00732871, -0.0458209,  -0.0100137,  -0.0372892,  0.000871123, 0.0245121,   -0.0811471,  -0.00481095, 0.0266868,   0.0712961,   -0.0675362,
  -0.0117453,  0.0658745,   -0.0694139,  -0.00704822, -0.0237313,  0.0209365,   0.0131902,   0.00192449,  -0.0593105,  0.0191942,   -0.00625798,
  0.00748682,  0.0533557,   0.0314002,   -0.0627113,  0.0827862,   0.00336722,  -0.0191575,  -0.0180252,  0.0150318,   -0.0686462,  0.0465634,
  0.0627244,   0.0449248,   -0.037054,   -0.0486668,  0.040752,    0.0143315,   -0.0763842,  -0.0161973,  0.0319588,   0.0112792,   -0.102007,
  0.0649219,   0.0630833,   0.0421069,   0.0519043,   -0.084082,   0.0249516,   0.023046,    0.071994,    -0.0272229,  0.0167103,   -0.00694243,
  0.0366775,   0.0672882,   0.0122419,   -0.0233413,  -0.0144258,  -0.012853,   -0.0202025,  0.000983093, -0.00776073, -0.0268638,  0.00682446,
  0.0262906,   -0.0407654,  -0.0144264,  -0.0310807,  0.0596711,   0.0238081,   -0.0138019,  0.000502882, 0.0496892,   0.0126823,   0.0511028,
  -0.0310699,  -0.0322141,  0.00996936,  0.0675392,   -0.0164277,  0.0930009,   -0.037467,   0.0419618,   -0.00358901, -0.0309569,  -0.0225608,
  -0.0332198,  0.00102291,  0.108814,    -0.0831313,  0.048208,    -0.0277542,  -0.061584,   0.0721224,   -0.0795082,  0.0340047,   0.056139,
  -0.0166783,  -0.0803042,  -0.014245,   -0.0476374,  0.048495,    0.0378856,
};

int main() {
    std::string expansion_path = "";
    INSPIREFACE_CONTEXT->Load("test_res/pack/Pikachu");

    DatabaseConfiguration configuration;
    configuration.primary_key_mode = PrimaryKeyMode::MANUAL_INPUT;
    configuration.enable_persistence = false;
    configuration.recognition_threshold = 0.48f;

    FEATURE_HUB_DB->EnableHub(configuration);

    // std::vector<float> feature(512, 0.0f);

    int64_t result_id = 0;
    auto ret = FEATURE_HUB_DB->FaceFeatureInsert(FT, 10086, result_id);
    if (ret != HSUCCEED) {
        INSPIRE_LOGE("Failed to insert face feature");
        INSPIRE_LOGI("result id: %lld", result_id);
    } else {
        INSPIRE_LOGI("Insert face feature success, result_id: %lld", result_id);
    }

    // std::vector<float> query_feature(512, 20.0f);
    FaceSearchResult search_result;
    ret = FEATURE_HUB_DB->SearchFaceFeature(FT, search_result, true);
    if (ret != HSUCCEED) {
        INSPIRE_LOGE("Failed to search face feature");
    } else {
        INSPIRE_LOGI("Search face feature success, result_id: %lld", search_result.id);
    }

    return 0;
}