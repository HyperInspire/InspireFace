//
// Created by Tunm-Air13 on 2023/9/11.
//

#ifndef HYPERFACEREPO_HERROR_H
#define HYPERFACEREPO_HERROR_H

#define HSUCCEED						    (0)
#define HERR_BASIC_BASE					0X0001							// 通用错误类型
#define HERR_UNKNOWN					    MERR_BASIC_BASE					// 错误原因不明
#define HERR_INVALID_PARAM				(MERR_BASIC_BASE+1)				// 无效的参数

#define HERR_CTX_BASE				    0X7000					        // Context 错误类型
#define HERR_CTX_INVALID_RESOURCE            (HERR_CTX_BASE+1)               // 无效的静态资源
#define HERR_CTX_TRACKER_FAILURE             (HERR_CTX_BASE+2)               // Tracker模块未初始化
#define HERR_CTX_REC_EXTRACT_FAILURE         (HERR_CTX_BASE+5)               // 人脸特征提取未进行注册
#define HERR_CTX_REC_DEL_FAILURE             (HERR_CTX_BASE+6)               // 索引越界删除人脸特征失败
#define HERR_CTX_REC_UPDATE_FAILURE          (HERR_CTX_BASE+7)               // 索引越界更新人脸特征失败
#define HERR_CTX_REC_ADD_FEAT_EMPTY          (HERR_CTX_BASE+8)               // 注册特征向量不能为空
#define HERR_CTX_REC_FEAT_SIZE_ERR           (HERR_CTX_BASE+9)               // 注册特征向量的长度错误
#define HERR_CTX_REC_INVALID_INDEX           (HERR_CTX_BASE+10)              // 无效的索引号
#define HERR_CTX_REC_CONTRAST_FEAT_ERR       (HERR_CTX_BASE+13)              // 对比的特征向量长度错误
#define HERR_CTX_REC_BLOCK_FULL              (HERR_CTX_BASE+14)              // 特征向量块满了
#define HERR_CTX_REC_BLOCK_DEL_FAILURE       (HERR_CTX_BASE+15)              // 删除失败
#define HERR_CTX_REC_BLOCK_UPDATE_FAILURE    (HERR_CTX_BASE+16)              // 删除失败


#endif //HYPERFACEREPO_HERROR_H
