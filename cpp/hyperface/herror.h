//
// Created by Tunm-Air13 on 2023/9/11.
//

#ifndef HYPERFACEREPO_HERROR_H
#define HYPERFACEREPO_HERROR_H

#define HSUCCEED						        (0)
#define HERR_BASIC_BASE					    0X0001							// 通用错误类型
#define HERR_UNKNOWN					        HERR_BASIC_BASE					// 错误原因不明
#define HERR_INVALID_PARAM				    (HERR_BASIC_BASE+1)				// 无效的参数
#define HERR_INVALID_IMAGE_STREAM_HANDLE	    (HERR_BASIC_BASE+24)			// 无效的图像句柄
#define HERR_INVALID_CONTEXT_HANDLE	        (HERR_BASIC_BASE+25)			// 无效的对象句柄
#define HERR_INVALID_FACE_TOKEN     	        (HERR_BASIC_BASE+30)			// 无效的人脸token
#define HERR_INVALID_FACE_FEATURE    	    (HERR_BASIC_BASE+31)			// 无效的人脸特征

#define HERR_CTX_BASE				        0X500					         // Context 错误类型
#define HERR_CTX_INVALID_RESOURCE            (HERR_CTX_BASE+1)               // 无效的静态资源
#define HERR_CTX_FUNCTION_UNUSABLE           (HERR_CTX_BASE+2)               // 功能不可用的
#define HERR_CTX_TRACKER_FAILURE             (HERR_CTX_BASE+3)               // Tracker模块未初始化

#define HERR_CTX_PIPELINE_FAILURE            (HERR_CTX_BASE+8)               // Pipeline模块未初始化

#define HERR_CTX_REC_EXTRACT_FAILURE         (HERR_CTX_BASE+15)               // 人脸特征提取未进行注册
#define HERR_CTX_REC_DEL_FAILURE             (HERR_CTX_BASE+16)               // 索引越界删除人脸特征失败
#define HERR_CTX_REC_UPDATE_FAILURE          (HERR_CTX_BASE+17)               // 索引越界更新人脸特征失败
#define HERR_CTX_REC_ADD_FEAT_EMPTY          (HERR_CTX_BASE+18)               // 注册特征向量不能为空
#define HERR_CTX_REC_FEAT_SIZE_ERR           (HERR_CTX_BASE+19)               // 注册特征向量的长度错误
#define HERR_CTX_REC_INVALID_INDEX           (HERR_CTX_BASE+20)              // 无效的索引号
#define HERR_CTX_REC_CONTRAST_FEAT_ERR       (HERR_CTX_BASE+23)              // 对比的特征向量长度错误
#define HERR_CTX_REC_BLOCK_FULL              (HERR_CTX_BASE+24)              // 特征向量块满了
#define HERR_CTX_REC_BLOCK_DEL_FAILURE       (HERR_CTX_BASE+25)              // 删除失败
#define HERR_CTX_REC_BLOCK_UPDATE_FAILURE    (HERR_CTX_BASE+26)              // 删除失败

#define HERR_CTX_FACE_DATA_ERROR             (HERR_CTX_BASE+30)               // 人脸数据解析错误


#endif //HYPERFACEREPO_HERROR_H
