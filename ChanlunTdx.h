#ifndef __CHANLUNTDX_H__
#define __CHANLUNTDX_H__

#include <vector>
#include "ChanlunCore.h"

namespace chanlun
{
std::vector<float> projectStrokePivotBoundary(const std::vector<Pivot> &pivots,
                                              size_t count, bool leftBoundary, bool highValue);
std::vector<float> evaluateTdxFunction(int functionId,
                                       const std::vector<float> &high,
                                       const std::vector<float> &low,
                                       const std::vector<float> &close);
void fillTdxFunction(int functionId, int nCount, float *pOut, float *pHigh, float *pLow, float *pClose);
}

void Func100(int nCount, float *pOut, float *pHigh, float *pLow, float *pClose);
void Func101(int nCount, float *pOut, float *pHigh, float *pLow, float *pClose);
void Func102(int nCount, float *pOut, float *pHigh, float *pLow, float *pClose);
void Func103(int nCount, float *pOut, float *pHigh, float *pLow, float *pClose);
void Func104(int nCount, float *pOut, float *pHigh, float *pLow, float *pClose);
void Func105(int nCount, float *pOut, float *pHigh, float *pLow, float *pClose);
void Func110(int nCount, float *pOut, float *pHigh, float *pLow, float *pClose);
void Func120(int nCount, float *pOut, float *pHigh, float *pLow, float *pClose);
void Func200(int nCount, float *pOut, float *pHigh, float *pLow, float *pClose);
void Func201(int nCount, float *pOut, float *pHigh, float *pLow, float *pClose);
void Func202(int nCount, float *pOut, float *pHigh, float *pLow, float *pClose);
void Func203(int nCount, float *pOut, float *pHigh, float *pLow, float *pClose);
void Func204(int nCount, float *pOut, float *pHigh, float *pLow, float *pClose);
void Func205(int nCount, float *pOut, float *pHigh, float *pLow, float *pClose);
void Func210(int nCount, float *pOut, float *pHigh, float *pLow, float *pClose);
void Func211(int nCount, float *pOut, float *pHigh, float *pLow, float *pClose);
void Func212(int nCount, float *pOut, float *pHigh, float *pLow, float *pClose);
void Func213(int nCount, float *pOut, float *pHigh, float *pLow, float *pClose);
void Func214(int nCount, float *pOut, float *pHigh, float *pLow, float *pClose);
void Func215(int nCount, float *pOut, float *pHigh, float *pLow, float *pClose);
void Func216(int nCount, float *pOut, float *pHigh, float *pLow, float *pClose);
void Func300(int nCount, float *pOut, float *pHigh, float *pLow, float *pClose);
void Func320(int nCount, float *pOut, float *pHigh, float *pLow, float *pClose);
void Func400(int nCount, float *pOut, float *pHigh, float *pLow, float *pClose);
void Func900(int nCount, float *pOut, float *pHigh, float *pLow, float *pClose);

#endif
