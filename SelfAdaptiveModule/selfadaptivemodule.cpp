#include "selfadaptivemodule.h"

SelfAdaptiveModule::SelfAdaptiveModule()
{
}

///
/// @brief 获取指定层厚对应的速度因子
/// @param fThickness 目标层厚值
/// @return 对应的层厚速度因子
/// @details 实现步骤:
///   1. 检查层厚列表是否为空
///   2. 遍历所有层厚值
///   3. 计算与目标层厚的误差
///   4. 找出误差最小的层厚对应的速度因子
///
VarioLayerThicknessSpeedFactor SelfAdaptiveModule::getMarkSpeedRatio(const double &fThickness)
{
    // 空列表返回默认值
    if(listLayersThickness.size() < 1) return VarioLayerThicknessSpeedFactor();

    // 初始化返回值
    VarioLayerThicknessSpeedFactor varioLayerAreaFactor;

    // 遍历查找最接近的层厚值
    int nTotalLayer = listLayersThickness.size();
    double fError = 1E20;
    double fErrorTemp = 0.0;
    for(int iLayer = 0; iLayer < nTotalLayer; ++ iLayer)
    {
        // 计算当前层厚与目标值的误差
        fErrorTemp = abs(fThickness - double(listLayersThickness.at(iLayer)));
        
        // 更新最小误差对应的速度因子
        if(fErrorTemp < fError)
        {
            varioLayerAreaFactor = _selfAdaptiveParas.mapLayerThicknessSpeedFactor.value(
                listLayersThickness.at(iLayer), VarioLayerThicknessSpeedFactor());
            fError = fErrorTemp;
        }
    }

    return varioLayerAreaFactor;
}


///
/// @brief 创建层厚列表
/// @details 实现步骤:
///   1. 从层厚速度因子映射中获取所有层厚值
///   2. 检查列表是否为空
///   3. 对层厚值进行升序排序
///
void SelfAdaptiveModule::createThicknessList()
{
    // 获取所有层厚值
    listLayersThickness = _selfAdaptiveParas.mapLayerThicknessSpeedFactor.keys();

    // 空列表直接返回
    if(listLayersThickness.size() < 1) return;

    // 对层厚值进行升序排序
    std::sort(listLayersThickness.begin(), listLayersThickness.end(), [](const float &v1, const float &v2) {
        return v1 < v2;
    });
}

