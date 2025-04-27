#ifndef POLYGONSTARTCHANGER_H
#define POLYGONSTARTCHANGER_H

#include "bpccommon.h"

#define ZERO_F 1.0E-5
#define IS_ZERO(x) (qAbs(x) <= ZERO_F)
#define IS_SAME(x, y) IS_ZERO(x-y)
#define INT_INF 10E8
#define IS_INF(x) (qAbs(x) >= INT_INF)

#define CONTOUR_OUTLINE 1  //外轮廓
#define CONTOUR_INHOLE  0  //内孔
#define POINT_IN_CONTOUR     1  //点在多边形内
#define POINT_OUT_OF_CONTOUR 0  //点在多边形外
#define POINT_ON_CONTOUR    -1  //点在多边形的边上

#define stepTryMax 1E4

namespace Utek {

//const int stepTryMax = int(1E4);  //推进寻找有效点次数上限，超过该次数不再寻找

//输入参数包
typedef struct STARTCHANGERPARA{
    int changerType;        //0不改变，1为添加开始扫描，2为添加关闭扫描
    float insertAngle;      //切入角tan值,范围大于0
    float insertLength;     //切入长度（应当已随IntPoint缩放倍率进行缩放）

    float smallPolyArea;    //忽略小轮廓的面积（应当已随IntPoint缩放倍率进行平方缩放）
    float smallPolyLength;  //忽略小轮廓的周长（应当已随IntPoint缩放倍率进行缩放）
}StartChangerPara;

//输出包
typedef struct RETURNPATHPACK{
    bool successFirst   = false;   //首点找到
    bool successLast    = false;   //末点找到
    ClipperLib::IntPoint firstPt;  //找到的首点坐标
    ClipperLib::IntPoint lastPt;   //找到的末点坐标
    ClipperLib::Path result;       //结果路径(可能已经因推进寻找而次序改变)
}ReturnPathPack;

struct Vector2d{
    float x;
    float y;

    Vector2d (float tx = 0.0f, float ty = 0.0f)
    {
        x = tx;
        y = ty;
    }
};

class PolygonStartChanger
{
public:
    PolygonStartChanger();
    ReturnPathPack getStartChanger(const ClipperLib::Path &path,
                                   const StartChangerPara &paras) const;

private:
    ReturnPathPack getStartChangerPrivate(const ClipperLib::Path &path,
                                          const StartChangerPara &paras) const;
    float getLength(const ClipperLib::Path &path) const;
    float dis(const ClipperLib::IntPoint &p1,
              const ClipperLib::IntPoint &p2) const;
    float area(const ClipperLib::Path &path) const;

    ClipperLib::IntPoint getPossibleStartPt(const ClipperLib::IntPoint &firstPt,
                                            const ClipperLib::IntPoint &beforePt,
                                            float startK, float startDis) const;

    ClipperLib::IntPoint getPossibleStartPtRev(const ClipperLib::IntPoint &firstPt,
                                               const ClipperLib::IntPoint &beforePt,
                                               float startK, float startDis) const;

    Vector2d normalVector(const ClipperLib::IntPoint &pt1,
                          const ClipperLib::IntPoint &pt2) const;
    Vector2d normalVector(float x1, float y1, float x2, float y2) const;
    Vector2d normalVector(float k) const;
    Vector2d unitVector(const Vector2d& vec) const;
    float crossProduct(const Vector2d& vec1,
                       const Vector2d& vec2) const;
    void push(ClipperLib::Path &path, int moveStep) const;

    bool intersecting(const ClipperLib::Path &, const int &, const ClipperLib::IntPoint &) const;
};
}

#endif // POLYGONSTARTCHANGER_H
