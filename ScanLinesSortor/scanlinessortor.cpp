#include "scanlinessortor.h"
#include "uspfiledef.h"
#include "bpccommon.h"
#include "meshinfo.h"

#include <memory>
#include <QVector>

#define BIDIRECTIONAL
//#define CALCLENGTH
// #define CALCU_JMLENGTH

struct MPoint {
    int _x = 0;
    int _y = 0;

    MPoint(const int &x = 0, const int &y = 0) {
        _x = x;
        _y = y;
    }
};

// 比较函数
inline bool operator==(const MPoint &pt1, const MPoint &pt2) {
    return (pt1._x == pt2._x) && (pt1._y == pt2._y);
}

// 平台宽高
struct ScanLinesSortor::Priv {
    int platformWidth = 300;
    int platformHeight = 300;

    /**
     * @brief 从原始扫描线数据中提取有效的扫描线段信息
     * 
     * 该函数遍历原始扫描线数据,识别跳转点(JUMP类型)来确定每条有效扫描线的起点和终点,
     * 并将提取的扫描线段信息保存到结果向量中。
     * 
     * 工作流程:
     * 1. 遇到JUMP类型点时:
     *    - 如果是新扫描线开始,记录起点信息
     *    - 如果是已开始扫描线的结束,记录终点信息并保存该线段
     * 2. 处理最后一条未完成的扫描线(如果存在)
     *
     * @param srcVec 输入参数,原始扫描线数据向量
     * @param sortLineVec 输出参数,提取的有效扫描线段信息向量
     */
    void calcSortLineVec(const QVector<SCANLINE> &srcVec, QVector<SortLineInfo> &sortLineVec) 
    {
        int index = 0;
        bool lineStarted = false;
        SortLineInfo sortLine;
    
        // 遍历所有扫描线数据
        while(index < srcVec.size()) {
            const SCANLINE &scanLine = srcVec.at(index);
    
            // 检测到跳转点
            if(SECTION_SCANTYPE_JUMP == scanLine.nLineType) {
                // 新扫描线的起点
                if(false == lineStarted) {
                    lineStarted = true;
                    sortLine.nStartPos = index;
                    sortLine.x1 = scanLine.nX;
                    sortLine.y1 = scanLine.nY;
                }
                // 当前扫描线的终点
                else {
                    if(index < 1) {
                        qDebug() << "SCANLINE info Error";
                        return;
                    }
                    lineStarted = false;
                    sortLine.nEndPos = index - 1;
                    const SCANLINE &scanLine = srcVec.at(sortLine.nEndPos);
                    sortLine.x2 = scanLine.nX;
                    sortLine.y2 = scanLine.nY;
                    // 保存完整的扫描线段
                    sortLineVec << sortLine;
                    continue;
                }
            }
            ++ index;
        }
    
        // 处理最后一条未完成的扫描线
        if(lineStarted) 
        {
            auto endIndex = srcVec.size() - 1;
            // 检查是否是有效的扫描线段
            if(sortLine.nStartPos == endIndex) {
                return;
            }
            // 使用最后一个点作为终点
            sortLine.nEndPos = endIndex;
            sortLine.x2 = srcVec.at(endIndex).nX;
            sortLine.y2 = srcVec.at(endIndex).nY;
            sortLineVec << sortLine;
        }
    }
    

    /**
     * @brief 从路径集合中提取扫描线段信息
     * 
     * 该函数遍历路径集合,将每条有效路径(至少包含2个点)的起点和终点信息
     * 提取为扫描线段。每条路径生成一个扫描线段,包含起点终点坐标和在路径
     * 集合中的索引位置。
     *
     * @param paths 输入参数,原始路径集合
     * @param sortLineVec 输出参数,提取的扫描线段信息向量
     */
    void calcSortLineVec(const Paths &paths, QVector<SortLineInfo> &sortLineVec) 
    {
        SortLineInfo sortLine;

        // 遍历所有路径
        for(int i = 0; i < paths.size(); ++ i) {
            auto &path = paths[i];
            // 跳过少于2个点的无效路径
            if(path.size() < 2) continue;

            // 提取路径的起点和终点坐标
            sortLine.x1 = path.front().X;
            sortLine.y1 = path.front().Y;
            sortLine.x2 = path.back().X; 
            sortLine.y2 = path.back().Y;

            // 记录在路径集合中的索引位置
            sortLine.nStartPos = i;
            sortLine.nEndPos = i;

            // 保存扫描线段信息
            sortLineVec << sortLine;
        }
    }


    /**
     * @brief 计算扫描线的最优连接顺序
     * 
     * 该函数通过网格划分和最近邻搜索算法,计算扫描线的最优连接顺序。
     * 主要步骤:
     * 1. 计算所有扫描线的边界框
     * 2. 根据边界框创建网格划分
     * 3. 将扫描线端点映射到网格
     * 4. 通过网格搜索计算最优连接顺序
     * 5. 支持双向搜索以获得全局最优解
     *
     * @param srcVec 输入参数,原始扫描线信息向量,包含每条扫描线的起点终点坐标等信息
     * @param indexVec 输出参数,计算得到的最优连接顺序索引向量
     * 
     * @note 函数使用网格划分来优化搜索效率,通过BIDIRECTIONAL宏控制是否启用双向搜索
     */
    static void calcIndexVec(QVector<SortLineInfo> &srcVec, QVector<int> &indexVec) 
    {
        SortLineInfo *lpLine = nullptr;
    
        // 计算所有扫描线的边界框
        int minX = 0x7FFFFFFF;
        int maxX = -1;
        int minY = 0x7FFFFFFF;
        int maxY = -1;
        for (const auto &lineInfo : qAsConst(srcVec)) 
        {
            // 更新X坐标的最大最小值
            if (lineInfo.x1 < minX) minX = lineInfo.x1;
            if (lineInfo.x2 < minX) minX = lineInfo.x2;
            if (lineInfo.x1 > maxX) maxX = lineInfo.x1;
            if (lineInfo.x2 > maxX) maxX = lineInfo.x2;
    
            // 更新Y坐标的最大最小值
            if (lineInfo.y1 < minY) minY = lineInfo.y1;
            if (lineInfo.y2 < minY) minY = lineInfo.y2;
            if (lineInfo.y1 > maxY) maxY = lineInfo.y1;
            if (lineInfo.y2 > maxY) maxY = lineInfo.y2;
        }
    
        // 计算网格尺寸和步长
        int meshWid = std::ceil((maxX - minX) * 0.02) + 1;
        int meshHei = std::ceil((maxY - minY) * 0.02) + 1;
        double fStepX = (maxX - minX) * 0.02 / double(maxX - minX + 1);
        double fStepY = (maxY - minY) * 0.02 / double(maxY - minY + 1);
        
        // 创建网格数组
        auto totalMeshCnt = meshWid * meshHei;
        QVector<int> *mesh = new QVector<int>[2 * totalMeshCnt];

        // int minX = 0;
        // int minY = 0;
        // int nMeshSz = 300;
        // int meshWid = nMeshSz;
        // int meshHei = nMeshSz;
        // double fStepX = nMeshSz / double(platformWidth + 1) * UNITFACTOR;
        // double fStepY = nMeshSz / double(platformHeight + 1) * UNITFACTOR;
        // auto totalMeshCnt = meshWid * meshHei;
        // QVector<int> *mesh = new QVector<int>[2 * totalMeshCnt];

        // 将扫描线端点映射到网格中
        int nX = 0;
        int nY = 0;
        int nLineCnt = srcVec.size();
        for(int iIndex = 1; iIndex < nLineCnt; ++ iIndex) {
            lpLine = &srcVec[iIndex];
            // 映射起点
            {
                nX = int((lpLine->x1 - minX) * fStepX + 1E-8);
                nY = int((lpLine->y1 - minY) * fStepY + 1E-8);
                // 确保坐标在网格范围内
                if(nX < 0) nX = 0;
                if(nX > (meshWid - 1)) nX = meshWid - 1;
                if(nY < 0) nY = 0;
                if(nY > (meshHei - 1)) nY = meshHei - 1;
                mesh[nX + nY * meshWid] << iIndex;
            }
            // 映射终点
            {
                nX = int((lpLine->x2 - minX) * fStepX + 1E-8);
                nY = int((lpLine->y2 - minY) * fStepY + 1E-8);
                if(nX < 0) nX = 0;
                if(nX > (meshWid - 1)) nX = meshWid - 1;
                if(nY < 0) nY = 0;
                if(nY > (meshHei - 1)) nY = meshHei - 1;
                mesh[nX + nY * meshWid + totalMeshCnt] << iIndex;
            }
        }

        // 计算网格中最大索引数量
        auto maxCnt = 0;
        for (int i = 0; i < totalMeshCnt; ++ i) 
        {
            if (mesh[i].size() > maxCnt) maxCnt = mesh[i].size();
            if (mesh[i + totalMeshCnt].size() > maxCnt) maxCnt = mesh[i].size();
        }

        // 定义网格坐标变量
        int nMeshCoorX = 0;
        int nMeshCoorY = 0;
        int nTempMeshCoorX = 0;
        int nTempMeshCoorY = 0;
        MatchedLineInfo matchedInfo;

        // 搜索最优连接顺序
        auto funcGetCoorInDistance = [](const int &distance,
                                        const std::function<void(const int &, const int &)> &callFunc) {
            int radius = distance;
            int x = radius;
            int y = 0;
            int decision = 1 - x;

            QSet<quint64> coorSet;
            auto funcInsetPt = [&coorSet, &callFunc](const uint &x, const uint &y){
                quint64 pt = (quint64(x) << 32) | y;
                if (false == coorSet.contains(pt)) {
                    coorSet << pt;
                    callFunc(x, y);
                }
            };

            while (x >= y) {
                funcInsetPt( x,  y);
                funcInsetPt(-x,  y);
                funcInsetPt( x, -y);
                funcInsetPt(-x, -y);
                funcInsetPt( y,  x);
                funcInsetPt(-y,  x);
                funcInsetPt( y, -x);
                funcInsetPt(-y, -x);

                y++;
                if (decision <= 0) {
                    decision += 2 * y + 1;
                }
                else {
                    x--;
                    decision += 2 * (y - x) + 1;
                }
            }
        };

        auto funcReset = [&](const bool &reverse = false) {
            nLineCnt = srcVec.size();
            indexVec.clear();
            indexVec << 0;
            lpLine = &srcVec[0];
            lpLine->reversed = reverse;
            -- nLineCnt;
            matchedInfo = MatchedLineInfo();
            matchedInfo.resetValue(lpLine);
        };
        auto funcUpdateMeshCoor = [=, &nMeshCoorX, &nMeshCoorY](SortLineInfo *lpLine) {
            if(lpLine->reversed) {
                nMeshCoorX = int((lpLine->x1 - minX) * fStepX + 1E-8);
                nMeshCoorY = int((lpLine->y1 - minY) * fStepY + 1E-8);
            }
            else {
                nMeshCoorX = int((lpLine->x2 - minX) * fStepX + 1E-8);
                nMeshCoorY = int((lpLine->y2 - minY) * fStepY + 1E-8);
            }
            if(nMeshCoorX < 0) nMeshCoorX = 0;
            if(nMeshCoorX > (meshWid - 1)) nMeshCoorX = meshWid - 1;
            if(nMeshCoorY < 0) nMeshCoorY = 0;
            if(nMeshCoorY > (meshHei - 1)) nMeshCoorY = meshHei - 1;
        };
        auto funcLoop = [&](const bool &loop = true) {
            qint64 index = 0;
        LABLE_LOOP:
            index = nMeshCoorX + nMeshCoorY * meshWid;
            calcMinDisPos(srcVec, indexVec, mesh[index], false, matchedInfo);
#ifdef BIDIRECTIONAL
            calcMinDisPos(srcVec, indexVec, mesh[totalMeshCnt + index], true, matchedInfo);
#endif
            qint64 sub_index = 0;
            auto maxMeshSz = (meshWid > meshHei) ? meshWid : meshHei;
            for(int iStep = 1; iStep < maxMeshSz; ++ iStep) 
//                funcGetCoorInDistance(iStep, [=, &matchedInfo](const int &x, const int &y) {
//                     // qDebug() << "funcGetCoorInDistance" << iStep << x << y;
//                     auto nTempMeshCoorX = nMeshCoorX + x;
//                     auto nTempMeshCoorY = nMeshCoorY + y;
//                     qint64 sub_index = 0;
//                     if(nTempMeshCoorX > -1 && nTempMeshCoorX < meshWid
//                         && nTempMeshCoorY > -1 && nTempMeshCoorY < meshHei) {
//                         sub_index = nTempMeshCoorX + nTempMeshCoorY * meshWid;
//                         calcMinDisPos(srcVec, indexVec, mesh[sub_index], false, matchedInfo);
// #ifdef BIDIRECTIONAL
//                         calcMinDisPos(srcVec, indexVec, mesh[sub_index + totalMeshCnt], true, matchedInfo);
// #endif
//                     }
//                 });

{

                for (int cln = - iStep; cln <= iStep; ++ cln) {
                    nTempMeshCoorX = nMeshCoorX + cln;
                    nTempMeshCoorY = nMeshCoorY + iStep;
                    if(nTempMeshCoorX > -1 && nTempMeshCoorX < meshWid
                        && nTempMeshCoorY > -1 && nTempMeshCoorY < meshHei) {
                        sub_index = nTempMeshCoorX + nTempMeshCoorY * meshWid;
                        calcMinDisPos(srcVec, indexVec, mesh[sub_index], false, matchedInfo);
#ifdef BIDIRECTIONAL
                        calcMinDisPos(srcVec, indexVec, mesh[sub_index + totalMeshCnt], true, matchedInfo);
#endif
                    }
                    nTempMeshCoorX = nMeshCoorX + cln;
                    nTempMeshCoorY = nMeshCoorY - iStep;
                    if(nTempMeshCoorX > -1 && nTempMeshCoorX < meshWid
                        && nTempMeshCoorY > -1 && nTempMeshCoorY < meshHei) {
                        sub_index = nTempMeshCoorX + nTempMeshCoorY * meshWid;
                        calcMinDisPos(srcVec, indexVec, mesh[sub_index], false, matchedInfo);
#ifdef BIDIRECTIONAL
                        calcMinDisPos(srcVec, indexVec, mesh[sub_index + totalMeshCnt], true, matchedInfo);
#endif
                    }
                }
                for (int row = - iStep + 1; row < iStep; ++ row) {
                    nTempMeshCoorX = nMeshCoorX + iStep;
                    nTempMeshCoorY = nMeshCoorY + row;
                    if(nTempMeshCoorX > -1 && nTempMeshCoorX < meshWid
                        && nTempMeshCoorY > -1 && nTempMeshCoorY < meshHei) {
                        sub_index = nTempMeshCoorX + nTempMeshCoorY * meshWid;
                        calcMinDisPos(srcVec, indexVec, mesh[sub_index], false, matchedInfo);
#ifdef BIDIRECTIONAL
                        calcMinDisPos(srcVec, indexVec, mesh[sub_index + totalMeshCnt], true, matchedInfo);
#endif
                    }

                    nTempMeshCoorX = nMeshCoorX - iStep;
                    nTempMeshCoorY = nMeshCoorY + row;
                    if(nTempMeshCoorX > -1 && nTempMeshCoorX < meshWid
                        && nTempMeshCoorY > -1 && nTempMeshCoorY < meshHei) {
                        sub_index = nTempMeshCoorX + nTempMeshCoorY * meshWid;
                        calcMinDisPos(srcVec, indexVec, mesh[sub_index], false, matchedInfo);
#ifdef BIDIRECTIONAL
                        calcMinDisPos(srcVec, indexVec, mesh[sub_index + totalMeshCnt], true, matchedInfo);
#endif
                    }
                }

                if(false == loop) return;
                if(-1 != matchedInfo.lineIndex) {
                    indexVec << matchedInfo.lineIndex;  // 将找到的最近线段索引加入结果向量
                    lpLine = &srcVec[matchedInfo.lineIndex];
                    lpLine->reversed = matchedInfo.needReverse; // 记录该线段是否需要反向

                    matchedInfo.resetValue(lpLine);
                    funcUpdateMeshCoor(lpLine);
                    if(nLineCnt > 0) goto LABLE_LOOP;
                }
            }
        };

        funcReset(false);
        funcUpdateMeshCoor(lpLine);
        funcLoop(false);
        auto delta1 = matchedInfo.delta;

        funcReset(true);
        funcUpdateMeshCoor(lpLine);
        funcLoop(false);
        auto delta2 = matchedInfo.delta;

        funcReset(delta1 > delta2);
        funcUpdateMeshCoor(lpLine);
        funcLoop(true);

        delete []mesh;
    }

    /**
     * @brief 根据优化后的顺序重新计算扫描线序列
     * 
     * 该函数根据优化后的索引顺序重新组织扫描线序列,支持扫描线的正向和反向处理。
     * 对于需要反向的扫描线,会交换其起点和终点的扫描类型。
     *
     * @param srcVec 输入输出参数,原始扫描线向量,最终会被优化后的序列替换
     * @param sortLineVec 输入参数,扫描线段信息向量
     * @param indexVec 输入参数,优化后的扫描线索引顺序
     */
    void recalcScanLine(QVector<SCANLINE> &srcVec, QVector<SortLineInfo> sortLineVec, const QVector<int> &indexVec) 
    {
        // 创建目标向量存储重排序的扫描线
        QVector<SCANLINE> desVec;
        SortLineInfo *lpSortLine = nullptr;
    
        // 按优化后的顺序重新组织扫描线
        for(const auto &index : indexVec) {
            lpSortLine = &sortLineVec[index];
            
            // 正向处理扫描线
            if(false == lpSortLine->reversed) {
                for(int iPos = lpSortLine->nStartPos; iPos <= lpSortLine->nEndPos; ++ iPos) {
                    desVec << srcVec[iPos];
                }
            }
            // 反向处理扫描线,需要交换扫描类型
            else {
                int nScanType = SECTION_SCANTYPE_JUMP;
                int tempScanType = SECTION_SCANTYPE_JUMP;
                for(int iPos = lpSortLine->nEndPos; iPos >= lpSortLine->nStartPos; -- iPos) {
                    tempScanType = srcVec[iPos].nLineType;
                    srcVec[iPos].nLineType = nScanType;
                    desVec << srcVec[iPos];
                    nScanType = tempScanType;
                }
            }
        }
    
        // 在定义了CALCLENGTH时计算优化前后的跳转和标记长度
    #ifdef CALCLENGTH
        qint64 nJumpLen_1 = 0, nMarkLen_1 = 0;
        qint64 jumpCount_1 = 0, markCount_1 = 0;
        calcJumpAndMarkLenth(srcVec, nJumpLen_1, nMarkLen_1, jumpCount_1, markCount_1);
    
        qint64 nJumpLen_2 = 0, nMarkLen_2 = 0;
        qint64 jumpCount_2 = 0, markCount_2 = 0;
        calcJumpAndMarkLenth(desVec, nJumpLen_2, nMarkLen_2, jumpCount_2, markCount_2);
    
        qDebug() << "calcJumpAndMarkLenth" << nMarkLen_1 << nMarkLen_2 << nJumpLen_1 << nJumpLen_2 << (nJumpLen_1 - nJumpLen_2);
    #endif
    
        // 用优化后的序列替换原序列
        srcVec = std::move(desVec);
    }

    /**
     * @brief 根据优化后的顺序重新计算路径序列
     * 
     * 该函数根据优化后的索引顺序重新组织路径序列,支持路径的正向和反向处理。
     * 对于需要反向的路径,会调用ReversePath进行反转。
     *
     * @param srcVec 输入输出参数,原始路径向量,最终会被优化后的序列替换
     * @param sortLineVec 输入参数,扫描线段信息向量
     * @param indexVec 输入参数,优化后的路径索引顺序
     */
    void recalcScanLine(Paths &srcVec, QVector<SortLineInfo> sortLineVec, const QVector<int> &indexVec) {
        // 创建目标向量存储重排序的路径
        Paths desVec;
        SortLineInfo *lpSortLine = nullptr;

        // 按优化后的顺序重新组织路径
        for(const auto &index : indexVec) {
            lpSortLine = &sortLineVec[index];
            
            // 正向处理路径
            if(false == lpSortLine->reversed) {
                desVec << std::move(srcVec.at(lpSortLine->nStartPos));
            }
            // 反向处理路径
            else {
                auto path = srcVec.at(lpSortLine->nStartPos);
                ReversePath(path);
                desVec << std::move(path);
            }
        }

        // 在定义了CALCLENGTH时计算优化前后的跳转和标记长度
    #ifdef CALCLENGTH
        qint64 nJumpLen_1 = 0, nMarkLen_1 = 0;
        qint64 jumpCount_1 = 0, markCount_1 = 0;
        calcJumpAndMarkLenth(srcVec, nJumpLen_1, nMarkLen_1, jumpCount_1, markCount_1);

        qint64 nJumpLen_2 = 0, nMarkLen_2 = 0;
        qint64 jumpCount_2 = 0, markCount_2 = 0;
        calcJumpAndMarkLenth(desVec, nJumpLen_2, nMarkLen_2, jumpCount_2, markCount_2);

        qDebug() << "calcJumpAndMarkLenth" << nMarkLen_1 << nMarkLen_2 << nJumpLen_1 << nJumpLen_2 << (nJumpLen_1 - nJumpLen_2);
    #endif

        // 用优化后的序列替换原序列
        srcVec = std::move(desVec);
    }


    /**
     * @brief 计算给定网格中距离最近的扫描线位置
     * 
     * 该函数计算当前端点到网格中所有未使用扫描线端点的距离,
     * 找出距离最短的扫描线及其方向信息。
     *
     * @param srcVec 输入参数,所有扫描线信息向量
     * @param indexVec 输入参数,已使用的扫描线索引向量
     * @param listIndex 输入参数,当前网格中的扫描线索引列表
     * @param reversed 输入参数,是否检查反向连接
     * @param matchedInfo 输出参数,存储找到的最近扫描线信息
     */
    static void calcMinDisPos(const QVector<SortLineInfo> &srcVec, 
                            const QVector<int> &indexVec, 
                            const QVector<int> &listIndex,
                            const bool &reversed, 
                            MatchedLineInfo &matchedInfo) 
    {
        // 网格为空直接返回
        if(listIndex.size() < 1) return;

        double tempV = 0;
        
        // 正向连接检查
        if(false == reversed) {
            for(const auto &index : listIndex) {
                // 跳过已使用的扫描线
                if(indexVec.contains(index)) continue;
                
                // 计算到扫描线起点的欧氏距离平方
                tempV = pow(matchedInfo.endPtX - srcVec.at(index).x1, 2) +
                        pow(matchedInfo.endPtY - srcVec.at(index).y1, 2);

                // 更新最短距离信息
                if(tempV < matchedInfo.delta) {
                    matchedInfo.delta = tempV;
                    matchedInfo.lineIndex = index;
                    matchedInfo.needReverse = reversed;
                }
            }
        }
        // 反向连接检查
        else {
            for(const auto &index : listIndex) {
                // 跳过已使用的扫描线
                if(indexVec.contains(index)) continue;
                
                // 计算到扫描线终点的欧氏距离平方
                tempV = pow(matchedInfo.endPtX - srcVec.at(index).x2, 2) +
                        pow(matchedInfo.endPtY - srcVec.at(index).y2, 2);

                // 更新最短距离信息
                if(tempV < matchedInfo.delta) {
                    matchedInfo.delta = tempV;
                    matchedInfo.lineIndex = index;
                    matchedInfo.needReverse = reversed;
                }
            }
        }
    }

    /**
     * @brief 计算扫描线序列的跳转和标记长度
     * 
     * 该函数计算扫描线序列中跳转(JUMP)和标记(MARK)的总长度及次数。
     * 长度通过欧氏距离计算,并乘以0.001的比例系数转换单位。
     *
     * @param lineList 输入参数,扫描线序列
     * @param jump 输出参数,跳转总长度
     * @param mark 输出参数,标记总长度
     * @param jumpCount 输出参数,跳转次数
     * @param markCount 输出参数,标记次数
     */
    void calcJumpAndMarkLenth(const QVector<SCANLINE> &lineList, 
                            qint64 &jump, 
                            qint64 &mark, 
                            qint64 &jumpCount, 
                            qint64 &markCount) 
    {
        // 初始化计数器
        jumpCount = 0;
        markCount = 0;

        // 记录上一个点的坐标
        int nLastX = 0;
        int nLastY = 0;
        qint64 nDeltaX = 0;
        qint64 nDeltaY = 0;

        // 遍历所有扫描线计算长度
        for(const auto &line : lineList) {
            switch(line.nLineType) {
            case SECTION_SCANTYPE_JUMP:
                // 计算跳转距离
                nDeltaX = line.nX - nLastX;
                nDeltaY = line.nY - nLastY;
                jump += sqrt(nDeltaX * nDeltaX + nDeltaY * nDeltaY) * 0.001;
                nLastX = line.nX;
                nLastY = line.nY;
                ++ jumpCount;
                break;
            case SECTION_SCANTYPE_MARK:
                // 计算标记距离
                nDeltaX = line.nX - nLastX;
                nDeltaY = line.nY - nLastY;
                mark += sqrt(nDeltaX * nDeltaX + nDeltaY * nDeltaY) * 0.001;
                nLastX = line.nX;
                nLastY = line.nY;
                ++ markCount;
                break;
            }
        }
    }

    /**
     * @brief 计算路径集合的跳转和标记长度
     * 
     * 该函数计算路径集合中的跳转和标记总长度及次数。
     * 每条路径的第一个点到上一条路径终点的距离计为跳转长度,
     * 路径内部点之间的距离计为标记长度。
     *
     * @param paths 输入参数,路径集合
     * @param jump 输出参数,跳转总长度
     * @param mark 输出参数,标记总长度
     * @param jumpCount 输出参数,跳转次数
     * @param markCount 输出参数,标记次数
     */
    void calcJumpAndMarkLenth(const Paths &paths, 
                            qint64 &jump, 
                            qint64 &mark, 
                            qint64 &jumpCount, 
                            qint64 &markCount) 
    {
        // 空路径直接返回
        if (paths.size() < 1) return;

        // 初始化计数器
        jumpCount = 0;
        markCount = 0;

        // 记录点坐标
        int nLastX = 0;
        int nLastY = 0;
        qint64 nDeltaX = 0;
        qint64 nDeltaY = 0;

        // 查找第一个有效路径的起点
        bool foundPt = false;
        for (const auto &path : paths) {
            if (path.size() < 1) continue;
            nLastX = path.at(0).X;
            nLastY = path.at(0).Y;
            foundPt = true;
            break;
        }
        if (false == foundPt) return;

        // 遍历所有路径计算长度
        for (const auto &path : paths) {
            if (path.size() < 1) continue;

            // 计算到路径起点的跳转距离
            nDeltaX = path.at(0).X - nLastX;
            nDeltaY = path.at(0).Y - nLastY;
            nLastX = path.at(0).X;
            nLastY = path.at(0).Y;
            jump += sqrt(nDeltaX * nDeltaX + nDeltaY * nDeltaY) * 0.001;
            ++ jumpCount;

            // 计算路径内部的标记距离
            int index = 0;
            while ((++ index) < path.size()) {
                nDeltaX = path.at(index).X - nLastX;
                nDeltaY = path.at(index).Y - nLastY;
                nLastX = path.at(index).X;
                nLastY = path.at(index).Y;
                mark += sqrt(nDeltaX * nDeltaX + nDeltaY * nDeltaY) * 0.001;
                ++ markCount;
            }
        }
    }
};

ScanLinesSortor::ScanLinesSortor(const int &width, const int &height) :
    d(new Priv)
{
    d->platformWidth = width;
    d->platformHeight = height;
}

void ScanLinesSortor::sortScanLines(QVector<SCANLINE> &srcVec)
{
#ifdef CALCU_JMLENGTH
    qint64 nJumpLen_1 = 0, nMarkLen_1 = 0, jumpCount_1 = 0, markCount_1;
    d->calcJumpAndMarkLenth(srcVec, nJumpLen_1, nMarkLen_1, jumpCount_1, markCount_1);
    qDebug() << "sortScanLines" << nJumpLen_1 << nMarkLen_1 << jumpCount_1 << markCount_1;
    return;
#endif

    QVector<SortLineInfo> sortLineVec;
    d->calcSortLineVec(srcVec, sortLineVec);
    QVector<int> indexVec;
    d->calcIndexVec(sortLineVec, indexVec);
    d->recalcScanLine(srcVec, sortLineVec, indexVec);
}

#include "quadtreesortor.h"
#include <QElapsedTimer>

void ScanLinesSortor::sortPaths(Paths &paths)
{
#ifdef CALCU_JMLENGTH
    qint64 nJumpLen_1 = 0, nMarkLen_1 = 0;
    qint64 nJumpLen_2 = 0, nMarkLen_2 = 0;
    qint64 nJumpLen_3 = 0, nMarkLen_3 = 0;
    qint64 jumpCount_1 = 0, markCount_1 = 0;    
    qint64 jumpCount_2 = 0, markCount_2 = 0;
    qint64 jumpCount_3 = 0, markCount_3 = 0;

    d->calcJumpAndMarkLenth(paths, nJumpLen_1, nMarkLen_1, jumpCount_1, markCount_1);
#endif

    {
        QuadtreeSortor sortor;
        sortor.sortPaths(paths);
#ifdef CALCU_JMLENGTH
        d->calcJumpAndMarkLenth(paths, nJumpLen_2, nMarkLen_2, jumpCount_2, markCount_2);
        qDebug() << nJumpLen_1 << nJumpLen_2 << nMarkLen_1 << nMarkLen_2
                 << jumpCount_1 << jumpCount_2 << markCount_1 << markCount_2;
#endif
    }
    return;
    QElapsedTimer timer;
    timer.restart();
    {
        QVector<SortLineInfo> sortLineVec;
        d->calcSortLineVec(paths, sortLineVec);
        qDebug() << "SortLineVec - 1: " << timer.elapsed();
        QVector<int> indexVec;
        d->calcIndexVec(sortLineVec, indexVec);
        qDebug() << "SortLineVec - 2: " << timer.elapsed();
        d->recalcScanLine(paths, sortLineVec, indexVec);
        qDebug() << "SortLineVec - 3: " << timer.elapsed();
#ifdef CALCU_JMLENGTH
        d->calcJumpAndMarkLenth(paths, nJumpLen_3, nMarkLen_3, jumpCount_3, markCount_3);
#endif
    }
#ifdef CALCU_JMLENGTH
    qDebug() << "CalcLength: "
             << nJumpLen_1 << nJumpLen_2 << nJumpLen_3
             << nMarkLen_1 << nMarkLen_2 << nMarkLen_3;
#endif
}
