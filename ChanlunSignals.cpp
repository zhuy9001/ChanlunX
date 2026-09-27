#include "ChanlunSignals.h"

#include <algorithm>

namespace chanlun
{
namespace
{
float emaNext(float previous, float value, int period)
{
    const float alpha = 2.0F / (static_cast<float>(period) + 1.0F);
    return alpha * value + (1.0F - alpha) * previous;
}
}

int toInt(TradeSignalType type)
{
    return static_cast<int>(type);
}

std::vector<MacdPoint> calculateMacd(const std::vector<float> &close, const MacdOptions &options)
{
    std::vector<MacdPoint> result;
    result.reserve(close.size());
    if (close.empty())
    {
        return result;
    }

    float shortEma = close.front();
    float longEma = close.front();
    float dea = 0.0F;

    for (float value : close)
    {
        shortEma = emaNext(shortEma, value, std::max(1, options.shortPeriod));
        longEma = emaNext(longEma, value, std::max(1, options.longPeriod));
        const float dif = shortEma - longEma;
        dea = emaNext(dea, dif, std::max(1, options.signalPeriod));
        result.push_back({dif, dea, (dif - dea) * 2.0F});
    }

    return result;
}

std::vector<TradeSignal> buildThirdBuySellSignals(const std::vector<Pivot> &pivots, const std::vector<Stroke> &strokes)
{
    std::vector<TradeSignal> signals;
    for (size_t pivotIndex = 0; pivotIndex < pivots.size(); ++pivotIndex)
    {
        const Pivot &pivot = pivots[pivotIndex];
        bool sawUpLeave = false;
        bool sawDownLeave = false;

        for (const Stroke &stroke : strokes)
        {
            if (stroke.startIndex <= pivot.endIndex)
            {
                continue;
            }

            if (!sawUpLeave && stroke.direction == Direction::Up && stroke.low > pivot.zd && stroke.high > pivot.zg)
            {
                sawUpLeave = true;
                continue;
            }
            if (sawUpLeave && stroke.direction == Direction::Down)
            {
                if (stroke.low >= pivot.zg)
                {
                    signals.push_back({stroke.endIndex, TradeSignalType::ThirdBuy, StructureStatus::Confirmed, static_cast<int>(pivotIndex)});
                }
                break;
            }

            if (!sawDownLeave && stroke.direction == Direction::Down && stroke.high < pivot.zg && stroke.low < pivot.zd)
            {
                sawDownLeave = true;
                continue;
            }
            if (sawDownLeave && stroke.direction == Direction::Up)
            {
                if (stroke.high <= pivot.zd)
                {
                    signals.push_back({stroke.endIndex, TradeSignalType::ThirdSell, StructureStatus::Confirmed, static_cast<int>(pivotIndex)});
                }
                break;
            }
        }
    }
    return signals;
}

std::vector<float> buildOperationStateSeries(int count, const std::vector<TradeSignal> &signals)
{
    std::vector<float> result(static_cast<size_t>(std::max(0, count)), 0.0F);
    float state = 0.0F;
    size_t signalIndex = 0;
    for (int i = 0; i < count; ++i)
    {
        while (signalIndex < signals.size() && signals[signalIndex].index == i)
        {
            const int signalValue = toInt(signals[signalIndex].type);
            if (signalValue > 0)
            {
                state = 1.0F;
            }
            else if (signalValue < 0)
            {
                state = -1.0F;
            }
            ++signalIndex;
        }
        result[static_cast<size_t>(i)] = state;
    }
    return result;
}
}
