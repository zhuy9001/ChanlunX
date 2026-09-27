#ifndef __CHANLUNSIGNALS_H__
#define __CHANLUNSIGNALS_H__

#include "ChanlunCore.h"

#include <vector>

namespace chanlun
{
enum class TradeSignalType
{
    ThirdSell = -3,
    SecondSell = -2,
    FirstSell = -1,
    None = 0,
    FirstBuy = 1,
    SecondBuy = 2,
    ThirdBuy = 3
};

struct MacdOptions
{
    int shortPeriod = 12;
    int longPeriod = 26;
    int signalPeriod = 9;
};

struct MacdPoint
{
    float dif;
    float dea;
    float histogram;
};

struct TradeSignal
{
    int index;
    TradeSignalType type;
    StructureStatus status;
    int pivotIndex;
};

int toInt(TradeSignalType type);
std::vector<MacdPoint> calculateMacd(const std::vector<float> &close, const MacdOptions &options = MacdOptions{});
std::vector<TradeSignal> buildThirdBuySellSignals(const std::vector<Pivot> &pivots, const std::vector<Stroke> &strokes);
std::vector<float> buildOperationStateSeries(int count, const std::vector<TradeSignal> &signals);
}

#endif
