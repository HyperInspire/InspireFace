//
// Created by Tunm-Air13 on 2021/9/17.
//

#include "model_loader.h"
#include <cassert>
#include "log.h"

namespace hyper {

    unsigned char key[256] =
            {0xAB, 0x7C, 0x3A, 0x24, 0xE9, 0x3E, 0xC4, 0x55, 0x29, 0x46, 0x96, 0x63, 0xB2, 0xCF, 0xCC, 0x88,
             0x28, 0xFD, 0xCC, 0x27, 0xB1, 0x71, 0x84, 0x02, 0x57, 0x1C, 0xF3, 0xAD, 0xAE, 0x50, 0xEB, 0x43,
             0xB6, 0x1D, 0x15, 0x57, 0x02, 0xDA, 0x35, 0x2D, 0x1A, 0x02, 0xAF, 0x2A, 0x1F, 0x42, 0x4B, 0x8A,
             0xAD, 0xCF, 0x35, 0x51, 0x05, 0xFD, 0x48, 0x84, 0x6C, 0x0E, 0x8B, 0x98, 0xCE, 0x47, 0xFC, 0x2B,
             0xA7, 0x4A, 0x8D, 0xF2, 0x24, 0x9F, 0x6D, 0xF3, 0x85, 0x94, 0x88, 0xF4, 0xC5, 0x47, 0x51, 0x33,
             0x7D, 0x01, 0xBF, 0x57, 0x06, 0xC5, 0x95, 0xA7, 0xDD, 0x29, 0xE7, 0x7A, 0x4C, 0x65, 0xDA, 0xEE,
             0x47, 0xAA, 0xAB, 0xDD, 0x95, 0xB5, 0xF3, 0x0D, 0x30, 0xA3, 0x29, 0xA7, 0xED, 0x07, 0x68, 0xEB,
             0x5F, 0x3A, 0x72, 0x21, 0xF9, 0xF2, 0xF6, 0xD3, 0x74, 0x16, 0x0E, 0x39, 0x70, 0xD1, 0x0C, 0xF5,
             0x5E, 0xE6, 0x76, 0x01, 0x9D, 0x43, 0x4F, 0xE5, 0xE3, 0xD8, 0x98, 0x2D, 0xDF, 0xA9, 0x17, 0x1A,
             0x1D, 0x22, 0x57, 0x98, 0x29, 0xAB, 0xF0, 0x70, 0xF7, 0x7D, 0x08, 0xBF, 0x82, 0xB4, 0x1A, 0xA7,
             0xB4, 0xA5, 0xF7, 0x44, 0x86, 0x71, 0x0A, 0xE1, 0x68, 0xDB, 0xDF, 0x6C, 0xE3, 0x56, 0xE7, 0x28,
             0x7D, 0x62, 0x75, 0xA2, 0xDD, 0x19, 0x0D, 0xE6, 0x2F, 0x05, 0xDE, 0xF2, 0xCA, 0x35, 0x8D, 0x61,
             0x10, 0x90, 0x34, 0x8F, 0x96, 0x69, 0xAB, 0x6B, 0x86, 0x53, 0x06, 0x4E, 0x41, 0x36, 0x5E, 0x7D,
             0x48, 0xA2, 0xD5, 0x28, 0x5C, 0x64, 0xD4, 0x9D, 0xE4, 0x57, 0x97, 0xBC, 0x90, 0x7D, 0xEC, 0xAB,
             0x3C, 0x4E, 0x37, 0xCB, 0xB1, 0x1A, 0xBA, 0xEA, 0x04, 0xE8, 0x13, 0xBA, 0x40, 0x70, 0x06, 0x81,
             0x45, 0x89, 0x7D, 0x13, 0xEF, 0xB3, 0xCD, 0xFE, 0xDE, 0x1B, 0x3B, 0x04, 0x1B, 0xB3, 0xBF, 0x00};

    void xor_encryption(char *data, int length, const unsigned char *key, int key_length) {
        for (int i = 0; i < length; i++) {
            data[i] = data[i] ^ key[i % key_length];
        }
    }

    ModelLoader::~ModelLoader() {
        //release memory
        if (status_ == 0) {
            for (int i = 0; i < number_of_models; i++) {
                delete model[i].prototxtBuffer;
                delete model[i].caffemodelBuffer;
            }
            delete model;
            delete modelSize;
        }

    }

    std::size_t ModelLoader::Count() const {
        return n2i_.size();
    }

    Model *ModelLoader::ReadModel(int idx) {
        assert(idx < number_of_models);
        return &model[idx];
    }

    Model *ModelLoader::ReadModel(const std::string &name) {
        int idx = n2i_[name];
        return &model[idx];
    }


    void ModelLoader::Reset(const std::string &model_path) {
        std::ifstream file(model_path, std::ios::binary);
        magic_number = 0;
        number_of_models = 0;
        file.read((char *) &magic_number, sizeof(magic_number));
        if (magic_number != 1128) {
            LOGE("The loaded resource file cannot be matched.");
            status_ = PACK_ERROR;
            return;
//            exit(0);
            //assert(magic_number != 1127);
        }
        file.read((char *) &number_of_models, sizeof(number_of_models));
        modelSize = new ModelSize[number_of_models];
        //std::cout<<"ModelSize:"<<sizeof(ModelSize)<<std::endl;
        file.read((char *) modelSize, sizeof(ModelSize) * number_of_models);
        model = new Model[number_of_models];
        std::cout << magic_number << std::endl;
        std::cout << number_of_models << std::endl;
        for (int i = 0; i < number_of_models; i++) {
            model[i].prototxtBuffer = new char[modelSize[i].prototxt_size];
            model[i].caffemodelBuffer = new char[modelSize[i].caffemodel_size];
            model[i].modelsize = modelSize[i];
            file.read(model[i].prototxtBuffer, modelSize[i].prototxt_size);
            xor_encryption(model[i].prototxtBuffer, modelSize[i].prototxt_size, key, 256);
            file.read(model[i].caffemodelBuffer, modelSize[i].caffemodel_size);
            xor_encryption(model[i].caffemodelBuffer, modelSize[i].caffemodel_size, key, 256);
        }

        std::ifstream ifs(model_path + ".index");
        std::string line;
        int pp = 0;
        while (std::getline(ifs, line)) {
            n2i_[line] = pp;
            pp += 1;
        }
//        assert(n2i_.size() == number_of_models);
        if (n2i_.size() != number_of_models) {
            status_ = PACK_MODELS_NOT_MATCH;
            return;
        }
        status_ = PASS;
    }

    ModelLoader::ModelLoader(std::string model_path) {
        Reset(model_path);
    }

    ModelLoader::ModelLoader() {

    }

    int ModelLoader::GetStatusCode() {
        return status_;
    }

}

