#include "ChanlunTdx.h"

#include "ChanlunCore.h"
#include "ChanlunSignals.h"

#include <algorithm>

namespace chanlun
{
namespace
{
std::vector<float> zeroSeries(size_t count)
{
    return std::vector<float>(count, 0.0F);
}

std::vector<float> closeOrMid(const std::vector<float> &high, const std::vector<float> &low, const std::vector<float> &close)
{
    const size_t count = std::min(high.size(), low.size());
    if (close.size() >= count)
    {
        return std::vector<float>(close.begin(), close.begin() + static_cast<long long>(count));
    }

    std::vector<float> result(count, 0.0F);
    for (size_t i = 0; i < count; ++i)
    {
        result[i] = (high[i] + low[i]) * 0.5F;
    }
    return result;
}

void setIfInRange(std::vector<float> &series, int index, float value)
{
    if (index >= 0 && static_cast<size_t>(index) < series.size())
    {
        series[static_cast<size_t>(index)] = value;
    }
}

void fillRange(std::vector<float> &series, int start, int end, float value)
{
    if (series.empty())
    {
        return;
    }
    const int left = std::max(0, start);
    const int right = std::min(static_cast<int>(series.size()) - 1, end);
    for (int i = left; i <= right; ++i)
    {
        series[static_cast<size_t>(i)] = value;
    }
}

float endpointValueForStart(const Stroke &stroke)
{
    return stroke.direction == Direction::Up ? -1.0F : 1.0F;
}

float endpointValueForEnd(const Stroke &stroke)
{
    return stroke.direction == Direction::Up ? 1.0F : -1.0F;
}

std::vector<float> projectMergedDirection(const ChanlunAnalysis &analysis, size_t count)
{
    std::vector<float> out = zeroSeries(count);
    for (const MergedBar &bar : analysis.mergedBars)
    {
        setIfInRange(out, bar.endIndex, static_cast<float>(toInt(bar.direction)));
    }
    return out;
}

std::vector<float> projectContainmentHigh(const ChanlunAnalysis &analysis,
                                          size_t count)
{
    std::vector<float> out = zeroSeries(count);
    for (const MergedBar &bar : analysis.mergedBars)
    {
        if (bar.endIndex > bar.startIndex && bar.direction != Direction::None)
        {
            fillRange(out, bar.startIndex, bar.endIndex, bar.high);
        }
    }
    return out;
}

std::vector<float> projectContainmentLow(const ChanlunAnalysis &analysis,
                                         size_t count)
{
    std::vector<float> out = zeroSeries(count);
    for (const MergedBar &bar : analysis.mergedBars)
    {
        if (bar.endIndex > bar.startIndex && bar.direction != Direction::None)
        {
            fillRange(out, bar.startIndex, bar.endIndex, bar.low);
        }
    }
    return out;
}

std::vector<float> projectContainmentStart(const ChanlunAnalysis &analysis, size_t count)
{
    std::vector<float> out = zeroSeries(count);
    for (const MergedBar &bar : analysis.mergedBars)
    {
        if (bar.endIndex > bar.startIndex && bar.direction != Direction::None)
        {
            setIfInRange(out, bar.startIndex, 1.0F);
        }
    }
    return out;
}

std::vector<float> projectContainmentEnd(const ChanlunAnalysis &analysis, size_t count)
{
    std::vector<float> out = zeroSeries(count);
    for (const MergedBar &bar : analysis.mergedBars)
    {
        if (bar.endIndex > bar.startIndex && bar.direction != Direction::None)
        {
            setIfInRange(out, bar.endIndex, 1.0F);
        }
    }
    return out;
}

std::vector<float> projectFractals(const ChanlunAnalysis &analysis, size_t count)
{
    std::vector<float> out = zeroSeries(count);
    for (const Fractal &fractal : analysis.fractals)
    {
        setIfInRange(out, fractal.originalIndex, static_cast<float>(toInt(fractal.type)));
    }
    return out;
}

std::vector<float> projectStrokes(const ChanlunAnalysis &analysis, size_t count)
{
    std::vector<float> out = zeroSeries(count);
    for (const Stroke &stroke : analysis.strokes)
    {
        setIfInRange(out, stroke.startIndex, endpointValueForStart(stroke));
        setIfInRange(out, stroke.endIndex, endpointValueForEnd(stroke));
    }
    return out;
}

std::vector<float> projectSegments(const ChanlunAnalysis &analysis, size_t count)
{
    std::vector<float> out = zeroSeries(count);
    for (const Segment &segment : analysis.segments)
    {
        if (segment.status != StructureStatus::Confirmed)
        {
            continue;
        }
        setIfInRange(out, segment.startIndex, segment.direction == Direction::Up ? -1.0F : 1.0F);
        setIfInRange(out, segment.endIndex, segment.direction == Direction::Up ? 1.0F : -1.0F);
    }
    return out;
}

std::vector<float> projectPivotValue(const ChanlunAnalysis &analysis, size_t count, bool highValue)
{
    std::vector<float> out = zeroSeries(count);
    for (const Pivot &pivot : analysis.pivots)
    {
        fillRange(out, pivot.startIndex, pivot.endIndex, highValue ? pivot.zg : pivot.zd);
    }
    return out;
}

std::vector<float> projectPivotStartEnd(const ChanlunAnalysis &analysis, size_t count)
{
    std::vector<float> out = zeroSeries(count);
    for (const Pivot &pivot : analysis.pivots)
    {
        setIfInRange(out, pivot.startIndex, 1.0F);
        setIfInRange(out, pivot.endIndex, 2.0F);
    }
    return out;
}

std::vector<float> projectStrokePivotValue(const ChanlunAnalysis &analysis, size_t count, bool highValue)
{
    std::vector<float> out = zeroSeries(count);
    for (const Pivot &pivot : buildStrokePivots(analysis.strokes))
    {
        if (pivot.status == StructureStatus::Candidate || pivot.status == StructureStatus::Invalid) continue;
        fillRange(out, pivot.startIndex, pivot.endIndex, highValue ? pivot.zg : pivot.zd);
    }
    return out;
}

std::vector<float> projectStrokePivotStartEnd(const ChanlunAnalysis &analysis, size_t count)
{
    std::vector<float> out = zeroSeries(count);
    for (const Pivot &pivot : buildStrokePivots(analysis.strokes))
    {
        if (pivot.status == StructureStatus::Candidate || pivot.status == StructureStatus::Invalid) continue;
        setIfInRange(out, pivot.startIndex, 1.0F);
        setIfInRange(out, pivot.endIndex, 2.0F);
    }
    return out;
}

std::vector<float> projectPivotDirection(const ChanlunAnalysis &analysis, size_t count)
{
    std::vector<float> out = zeroSeries(count);
    for (const Pivot &pivot : analysis.pivots)
    {
        fillRange(out, pivot.startIndex, pivot.endIndex, static_cast<float>(toInt(pivot.direction)));
    }
    return out;
}

std::vector<float> projectTrendKind(const ChanlunAnalysis &analysis, size_t count)
{
    std::vector<float> out = zeroSeries(count);
    fillRange(out, analysis.trend.startIndex, analysis.trend.endIndex, static_cast<float>(toInt(analysis.trend.kind)));
    return out;
}

std::vector<float> projectTrendCompletion(const ChanlunAnalysis &analysis, size_t count)
{
    std::vector<float> out = zeroSeries(count);
    fillRange(out, analysis.trend.startIndex, analysis.trend.endIndex, static_cast<float>(toInt(analysis.trend.completion)));
    return out;
}

std::vector<float> projectThirdBuySell(const ChanlunAnalysis &analysis, size_t count)
{
    std::vector<float> out = zeroSeries(count);
    const auto signals = buildThirdBuySellSignals(analysis.pivots, analysis.strokes);
    for (const TradeSignal &signal : signals)
    {
        setIfInRange(out, signal.index, static_cast<float>(toInt(signal.type)));
    }
    return out;
}

std::vector<float> projectMacdHistogram(const std::vector<float> &close)
{
    std::vector<float> out = zeroSeries(close.size());
    const auto macd = calculateMacd(close);
    for (size_t i = 0; i < macd.size(); ++i)
    {
        out[i] = macd[i].histogram;
    }
    return out;
}

std::vector<float> projectOperationState(const ChanlunAnalysis &analysis, size_t count)
{
    return buildOperationStateSeries(static_cast<int>(count), buildThirdBuySellSignals(analysis.pivots, analysis.strokes));
}

std::vector<float> projectMergedIds(const ChanlunAnalysis &analysis, size_t count)
{
    std::vector<float> out = zeroSeries(count);
    for (size_t i = 0; i < analysis.mergedBars.size(); ++i)
    {
        const MergedBar &bar = analysis.mergedBars[i];
        fillRange(out, bar.startIndex, bar.endIndex, static_cast<float>(i + 1));
    }
    return out;
}
}

std::vector<float> projectStrokePivotBoundary(const std::vector<Pivot> &pivots,
                                              size_t count, bool leftBoundary, bool highValue)
{
    std::vector<float> out = zeroSeries(count);
    for (const Pivot &pivot : pivots)
    {
        if (pivot.status == StructureStatus::Candidate || pivot.status == StructureStatus::Invalid) continue;
        setIfInRange(out, leftBoundary ? pivot.startIndex : pivot.endIndex,
                     highValue ? pivot.zg : pivot.zd);
    }
    return out;
}

std::vector<float> evaluateTdxFunction(int functionId,
                                       const std::vector<float> &high,
                                       const std::vector<float> &low,
                                       const std::vector<float> &close)
{
    const size_t count = std::min(high.size(), low.size());
    const std::vector<float> normalizedClose = closeOrMid(high, low, close);
    const ChanlunAnalysis analysis = analyze(high, low, normalizedClose);

    switch (functionId)
    {
    case 100:
        return projectMergedDirection(analysis, count);
    case 101:
        return projectFractals(analysis, count);
    case 102:
        return projectContainmentHigh(analysis, count);
    case 103:
        return projectContainmentLow(analysis, count);
    case 104:
        return projectContainmentStart(analysis, count);
    case 105:
        return projectContainmentEnd(analysis, count);
    case 110:
        return projectStrokes(analysis, count);
    case 120:
        return projectSegments(analysis, count);
    case 200:
        return projectPivotValue(analysis, count, true);
    case 201:
        return projectPivotValue(analysis, count, false);
    case 202:
        return projectPivotStartEnd(analysis, count);
    case 203:
        return projectPivotDirection(analysis, count);
    case 204:
        return projectTrendKind(analysis, count);
    case 205:
        return zeroSeries(count);
    case 210:
        return projectStrokePivotValue(analysis, count, true);
    case 211:
        return projectStrokePivotValue(analysis, count, false);
    case 212:
        return projectStrokePivotStartEnd(analysis, count);
    case 213:
        return projectStrokePivotBoundary(buildStrokePivots(analysis.strokes), count, true, true);
    case 214:
        return projectStrokePivotBoundary(buildStrokePivots(analysis.strokes), count, true, false);
    case 215:
        return projectStrokePivotBoundary(buildStrokePivots(analysis.strokes), count, false, true);
    case 216:
        return projectStrokePivotBoundary(buildStrokePivots(analysis.strokes), count, false, false);
    case 300:
        return projectThirdBuySell(analysis, count);
    case 320:
        return projectMacdHistogram(normalizedClose);
    case 400:
        return projectOperationState(analysis, count);
    case 900:
        return projectMergedIds(analysis, count);
    default:
        return zeroSeries(count);
    }
}

void fillTdxFunction(int functionId, int nCount, float *pOut, float *pHigh, float *pLow, float *pClose)
{
    if (nCount <= 0 || pOut == nullptr || pHigh == nullptr || pLow == nullptr)
    {
        return;
    }

    const std::vector<float> high(pHigh, pHigh + nCount);
    const std::vector<float> low(pLow, pLow + nCount);
    const std::vector<float> close = pClose == nullptr ? std::vector<float>() : std::vector<float>(pClose, pClose + nCount);
    const std::vector<float> series = evaluateTdxFunction(functionId, high, low, close);

    for (int i = 0; i < nCount; ++i)
    {
        pOut[i] = static_cast<size_t>(i) < series.size() ? series[static_cast<size_t>(i)] : 0.0F;
    }
}
}

void Func100(int nCount, float *pOut, float *pHigh, float *pLow, float *pClose)
{
    chanlun::fillTdxFunction(100, nCount, pOut, pHigh, pLow, pClose);
}

void Func101(int nCount, float *pOut, float *pHigh, float *pLow, float *pClose)
{
    chanlun::fillTdxFunction(101, nCount, pOut, pHigh, pLow, pClose);
}

void Func102(int nCount, float *pOut, float *pHigh, float *pLow, float *pClose)
{
    chanlun::fillTdxFunction(102, nCount, pOut, pHigh, pLow, pClose);
}

void Func103(int nCount, float *pOut, float *pHigh, float *pLow, float *pClose)
{
    chanlun::fillTdxFunction(103, nCount, pOut, pHigh, pLow, pClose);
}

void Func104(int nCount, float *pOut, float *pHigh, float *pLow, float *pClose)
{
    chanlun::fillTdxFunction(104, nCount, pOut, pHigh, pLow, pClose);
}

void Func105(int nCount, float *pOut, float *pHigh, float *pLow, float *pClose)
{
    chanlun::fillTdxFunction(105, nCount, pOut, pHigh, pLow, pClose);
}

void Func110(int nCount, float *pOut, float *pHigh, float *pLow, float *pClose)
{
    chanlun::fillTdxFunction(110, nCount, pOut, pHigh, pLow, pClose);
}

void Func120(int nCount, float *pOut, float *pHigh, float *pLow, float *pClose)
{
    chanlun::fillTdxFunction(120, nCount, pOut, pHigh, pLow, pClose);
}

void Func200(int nCount, float *pOut, float *pHigh, float *pLow, float *pClose)
{
    chanlun::fillTdxFunction(200, nCount, pOut, pHigh, pLow, pClose);
}

void Func201(int nCount, float *pOut, float *pHigh, float *pLow, float *pClose)
{
    chanlun::fillTdxFunction(201, nCount, pOut, pHigh, pLow, pClose);
}

void Func202(int nCount, float *pOut, float *pHigh, float *pLow, float *pClose)
{
    chanlun::fillTdxFunction(202, nCount, pOut, pHigh, pLow, pClose);
}

void Func203(int nCount, float *pOut, float *pHigh, float *pLow, float *pClose)
{
    chanlun::fillTdxFunction(203, nCount, pOut, pHigh, pLow, pClose);
}

void Func204(int nCount, float *pOut, float *pHigh, float *pLow, float *pClose)
{
    chanlun::fillTdxFunction(204, nCount, pOut, pHigh, pLow, pClose);
}

void Func205(int nCount, float *pOut, float *pHigh, float *pLow, float *pClose)
{
    chanlun::fillTdxFunction(205, nCount, pOut, pHigh, pLow, pClose);
}

void Func210(int nCount, float *pOut, float *pHigh, float *pLow, float *pClose)
{
    chanlun::fillTdxFunction(210, nCount, pOut, pHigh, pLow, pClose);
}

void Func211(int nCount, float *pOut, float *pHigh, float *pLow, float *pClose)
{
    chanlun::fillTdxFunction(211, nCount, pOut, pHigh, pLow, pClose);
}

void Func212(int nCount, float *pOut, float *pHigh, float *pLow, float *pClose)
{
    chanlun::fillTdxFunction(212, nCount, pOut, pHigh, pLow, pClose);
}

void Func213(int nCount, float *pOut, float *pHigh, float *pLow, float *pClose)
{
    chanlun::fillTdxFunction(213, nCount, pOut, pHigh, pLow, pClose);
}

void Func214(int nCount, float *pOut, float *pHigh, float *pLow, float *pClose)
{
    chanlun::fillTdxFunction(214, nCount, pOut, pHigh, pLow, pClose);
}

void Func215(int nCount, float *pOut, float *pHigh, float *pLow, float *pClose)
{
    chanlun::fillTdxFunction(215, nCount, pOut, pHigh, pLow, pClose);
}

void Func216(int nCount, float *pOut, float *pHigh, float *pLow, float *pClose)
{
    chanlun::fillTdxFunction(216, nCount, pOut, pHigh, pLow, pClose);
}

void Func300(int nCount, float *pOut, float *pHigh, float *pLow, float *pClose)
{
    chanlun::fillTdxFunction(300, nCount, pOut, pHigh, pLow, pClose);
}

void Func320(int nCount, float *pOut, float *pHigh, float *pLow, float *pClose)
{
    chanlun::fillTdxFunction(320, nCount, pOut, pHigh, pLow, pClose);
}

void Func400(int nCount, float *pOut, float *pHigh, float *pLow, float *pClose)
{
    chanlun::fillTdxFunction(400, nCount, pOut, pHigh, pLow, pClose);
}

void Func900(int nCount, float *pOut, float *pHigh, float *pLow, float *pClose)
{
    chanlun::fillTdxFunction(900, nCount, pOut, pHigh, pLow, pClose);
}
