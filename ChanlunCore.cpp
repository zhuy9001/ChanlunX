#include "ChanlunCore.h"

#include <algorithm>
#include <cmath>
#include <iostream>

namespace chanlun
{
namespace
{
bool isContained(const MergedBar &left, const MergedBar &right)
{
    const bool rightInsideLeft = right.high <= left.high && right.low >= left.low;
    const bool leftInsideRight = right.high >= left.high && right.low <= left.low;
    return rightInsideLeft || leftInsideRight;
}

Direction inferDirection(const std::vector<MergedBar> &bars)
{
    if (!bars.empty() && bars.back().direction != Direction::None)
    {
        return bars.back().direction;
    }
    return Direction::None;
}

Direction compareDirection(const MergedBar &previous, const MergedBar &current)
{
    if (current.high > previous.high && current.low > previous.low)
    {
        return Direction::Up;
    }
    if (current.high < previous.high && current.low < previous.low)
    {
        return Direction::Down;
    }
    return Direction::None;
}

bool isMoreExtreme(const Fractal &candidate, const Fractal &current)
{
    if (candidate.type == FractalType::Top)
    {
        return candidate.price >= current.price;
    }
    if (candidate.type == FractalType::Bottom)
    {
        return candidate.price <= current.price;
    }
    return false;
}

bool hasIndependentBarBetween(const Fractal &left, const Fractal &right)
{
    return std::abs(right.barIndex - left.barIndex) >= 4;
}

bool rangesOverlap(float highA, float lowA, float highB, float lowB)
{
    return std::max(lowA, lowB) <= std::min(highA, highB);
}

bool rangesOverlap(float highA, float lowA, float highB, float lowB, float highC, float lowC)
{
    const float low = std::max(std::max(lowA, lowB), lowC);
    const float high = std::min(std::min(highA, highB), highC);
    return low <= high;
}

}

int toInt(Direction direction)
{
    return static_cast<int>(direction);
}

int toInt(FractalType type)
{
    return static_cast<int>(type);
}

int toInt(StructureStatus status)
{
    return static_cast<int>(status);
}

int toInt(TrendKind kind)
{
    return static_cast<int>(kind);
}

int toInt(TrendCompletion completion)
{
    return static_cast<int>(completion);
}

std::vector<MergedBar> mergeBars(const std::vector<float> &high, const std::vector<float> &low)
{
    const size_t count = std::min(high.size(), low.size());
    std::vector<MergedBar> result;
    result.reserve(count);

    for (size_t i = 0; i < count; ++i)
    {
        MergedBar current{high[i], low[i], Direction::None, static_cast<int>(i), static_cast<int>(i), static_cast<int>(i), static_cast<int>(i), static_cast<int>(i)};
        if (result.empty())
        {
            result.push_back(current);
            continue;
        }

        MergedBar &last = result.back();
        if (isContained(last, current))
        {
            const Direction direction = inferDirection(result);
            if (direction == Direction::None)
            {
                if (current.high > last.high)
                {
                    last.highIndex = current.highIndex;
                }
                if (current.low > last.low)
                {
                    last.lowIndex = current.lowIndex;
                }
                last.high = std::max(last.high, current.high);
                last.low = std::max(last.low, current.low);
                last.direction = Direction::Up;
                last.endIndex = current.endIndex;
                last.representativeIndex = last.highIndex;
                continue;
            }
            if (direction == Direction::Down)
            {
                if (current.high < last.high)
                {
                    last.highIndex = current.highIndex;
                }
                if (current.low < last.low)
                {
                    last.lowIndex = current.lowIndex;
                }
                last.high = std::min(last.high, current.high);
                last.low = std::min(last.low, current.low);
            }
            else
            {
                if (current.high > last.high)
                {
                    last.highIndex = current.highIndex;
                }
                if (current.low > last.low)
                {
                    last.lowIndex = current.lowIndex;
                }
                last.high = std::max(last.high, current.high);
                last.low = std::max(last.low, current.low);
            }
            last.direction = direction;
            last.endIndex = current.endIndex;
            last.representativeIndex = direction == Direction::Up ? last.highIndex : last.lowIndex;
            continue;
        }

        current.direction = compareDirection(last, current);
        if (last.direction == Direction::None && last.endIndex > last.startIndex &&
            current.direction != Direction::None)
        {
            last.high = high[static_cast<size_t>(last.startIndex)];
            last.low = low[static_cast<size_t>(last.startIndex)];
            last.highIndex = last.startIndex;
            last.lowIndex = last.startIndex;
            for (int index = last.startIndex + 1; index <= last.endIndex; ++index)
            {
                if (current.direction == Direction::Up)
                {
                    if (high[static_cast<size_t>(index)] > last.high)
                    {
                        last.highIndex = index;
                    }
                    if (low[static_cast<size_t>(index)] > last.low)
                    {
                        last.lowIndex = index;
                    }
                    last.high = std::max(last.high, high[static_cast<size_t>(index)]);
                    last.low = std::max(last.low, low[static_cast<size_t>(index)]);
                }
                else
                {
                    if (high[static_cast<size_t>(index)] < last.high)
                    {
                        last.highIndex = index;
                    }
                    if (low[static_cast<size_t>(index)] < last.low)
                    {
                        last.lowIndex = index;
                    }
                    last.high = std::min(last.high, high[static_cast<size_t>(index)]);
                    last.low = std::min(last.low, low[static_cast<size_t>(index)]);
                }
            }
            last.direction = current.direction;
            last.representativeIndex = current.direction == Direction::Up ? last.highIndex : last.lowIndex;
            if (isContained(last, current))
            {
                --i;
                continue;
            }
        }
        result.push_back(current);
    }

    return result;
}

std::vector<Fractal> findFractals(const std::vector<MergedBar> &bars)
{
    std::vector<Fractal> fractals;
    if (bars.size() < 3)
    {
        return fractals;
    }

    for (size_t i = 1; i + 1 < bars.size(); ++i)
    {
        const MergedBar &left = bars[i - 1];
        const MergedBar &mid = bars[i];
        const MergedBar &right = bars[i + 1];

        const bool isTop = mid.high > left.high && mid.high > right.high &&
                           mid.low > left.low && mid.low > right.low;
        const bool isBottom = mid.low < left.low && mid.low < right.low &&
                              mid.high < left.high && mid.high < right.high;

        if (isTop)
        {
            const int highIndex = mid.highIndex >= mid.startIndex && mid.highIndex <= mid.endIndex
                                      ? mid.highIndex
                                      : mid.representativeIndex;
            fractals.push_back({FractalType::Top, static_cast<int>(i), highIndex, mid.high});
        }
        else if (isBottom)
        {
            const int lowIndex = mid.lowIndex >= mid.startIndex && mid.lowIndex <= mid.endIndex
                                     ? mid.lowIndex
                                     : mid.representativeIndex;
            fractals.push_back({FractalType::Bottom, static_cast<int>(i), lowIndex, mid.low});
        }
    }

    return fractals;
}

std::vector<Stroke> buildStrokes(const std::vector<MergedBar> &bars, const std::vector<Fractal> &fractals)
{
    const auto coversRange = [&bars](const Fractal &start, const Fractal &end) {
        const int left = std::min(start.barIndex, end.barIndex);
        const int right = std::max(start.barIndex, end.barIndex);
        if (left < 0 || right >= static_cast<int>(bars.size()))
        {
            return false;
        }
        const float top = start.type == FractalType::Top ? start.price : end.price;
        const float bottom = start.type == FractalType::Bottom ? start.price : end.price;
        for (int index = left; index <= right; ++index)
        {
            if (bars[static_cast<size_t>(index)].high > top || bars[static_cast<size_t>(index)].low < bottom)
            {
                return false;
            }
        }
        return true;
    };
    std::vector<Stroke> strokes;
    if (fractals.size() < 2)
    {
        return strokes;
    }

    const auto canConnect = [&](int start, int end) {
        const Fractal &left = fractals[static_cast<size_t>(start)];
        const Fractal &right = fractals[static_cast<size_t>(end)];
        return left.type != right.type && right.barIndex > left.barIndex &&
               hasIndependentBarBetween(left, right) && coversRange(left, right) &&
               (left.type == FractalType::Top ? left.price > right.price : right.price > left.price);
    };
    std::vector<int> selected;
    std::vector<std::vector<int>> deferred;
    int initialOpposite = -1;
    for (size_t index = 0; index < fractals.size(); ++index)
    {
        const int candidateIndex = static_cast<int>(index);
        const Fractal &candidate = fractals[index];
        if (selected.empty())
        {
            selected.push_back(candidateIndex);
            continue;
        }
        bool restored = false;
        for (auto saved = deferred.rbegin(); saved != deferred.rend(); ++saved)
        {
            if (canConnect(saved->back(), candidateIndex))
            {
                bool valid = true;
                for (size_t endpoint = 1; endpoint < saved->size(); ++endpoint)
                {
                    if (!canConnect((*saved)[endpoint - 1], (*saved)[endpoint]))
                    {
                        valid = false;
                        break;
                    }
                }
                if (!valid)
                {
                    continue;
                }
                size_t common = 0;
                while (common < selected.size() && common < saved->size() &&
                       selected[common] == (*saved)[common])
                {
                    ++common;
                }
                if (common > 0 && common + 1 >= selected.size())
                {
                    selected = *saved;
                    selected.push_back(candidateIndex);
                    restored = true;
                    break;
                }
            }
        }
        if (restored)
        {
            deferred.clear();
            initialOpposite = -1;
            continue;
        }
        const Fractal &last = fractals[static_cast<size_t>(selected.back())];
        if (canConnect(selected.back(), candidateIndex))
        {
            selected.push_back(candidateIndex);
            deferred.clear();
            initialOpposite = -1;
            continue;
        }
        if (candidate.type == last.type)
        {
            if (!isMoreExtreme(candidate, last))
            {
                continue;
            }
            if (selected.size() == 1 && initialOpposite >= 0 &&
                canConnect(initialOpposite, candidateIndex))
            {
                selected = {initialOpposite, candidateIndex};
                initialOpposite = -1;
                continue;
            }
            deferred.push_back(selected);
            selected.back() = candidateIndex;
        }
        else
        {
            if (selected.size() == 1)
            {
                if (initialOpposite < 0 ||
                    isMoreExtreme(candidate, fractals[static_cast<size_t>(initialOpposite)]))
                {
                    initialOpposite = candidateIndex;
                }
                continue;
            }
            const Fractal &previous = fractals[static_cast<size_t>(selected[selected.size() - 2])];
            if (!isMoreExtreme(candidate, previous))
            {
                continue;
            }
            deferred.push_back(selected);
            if (selected.size() == 3)
            {
                continue;
            }
            selected.pop_back();
            selected.back() = candidateIndex;
        }
        while (selected.size() >= 2 &&
               !canConnect(selected[selected.size() - 2], selected.back()))
        {
            const int endpoint = selected.back();
            if (selected.size() == 2)
            {
                initialOpposite = selected.front();
                selected = {endpoint};
                break;
            }
            selected.pop_back();
            selected.pop_back();
            const Fractal &earlier = fractals[static_cast<size_t>(selected.back())];
            if (isMoreExtreme(candidate, earlier))
            {
                selected.back() = endpoint;
            }
        }
    }
    for (size_t i = 1; i < selected.size(); ++i)
    {
        const Fractal &start = fractals[static_cast<size_t>(selected[i - 1])];
        const Fractal &end = fractals[static_cast<size_t>(selected[i])];
        const Direction direction = start.type == FractalType::Bottom ? Direction::Up : Direction::Down;
        const int left = std::min(start.barIndex, end.barIndex);
        const int right = std::max(start.barIndex, end.barIndex);
        float strokeHigh = bars[left].high;
        float strokeLow = bars[left].low;
        for (int j = left; j <= right; ++j)
        {
            strokeHigh = std::max(strokeHigh, bars[static_cast<size_t>(j)].high);
            strokeLow = std::min(strokeLow, bars[static_cast<size_t>(j)].low);
        }

        const StructureStatus status = i + 1 < selected.size()
            ? StructureStatus::Confirmed : StructureStatus::Candidate;
        strokes.push_back({start.originalIndex, end.originalIndex, direction, strokeHigh, strokeLow, status});
    }

    return strokes;
}

std::vector<Segment> buildSegments(const std::vector<Stroke> &strokes)
{
    (void)strokes;
    return {};
}

std::vector<Pivot> buildPivots(const std::vector<Segment> &segments, int level)
{
    std::vector<Pivot> pivots;
    if (segments.size() < 3)
    {
        return pivots;
    }

    for (size_t i = 0; i + 2 < segments.size();)
    {
        const Segment &a = segments[i];
        const Segment &b = segments[i + 1];
        const Segment &c = segments[i + 2];
        if (a.status != StructureStatus::Confirmed ||
            b.status != StructureStatus::Confirmed ||
            c.status != StructureStatus::Confirmed ||
            a.endIndex != b.startIndex ||
            b.endIndex != c.startIndex ||
            !rangesOverlap(a.high, a.low, b.high, b.low, c.high, c.low))
        {
            ++i;
            continue;
        }

        Pivot pivot;
        pivot.startIndex = a.startIndex;
        pivot.endIndex = c.endIndex;
        pivot.level = level;
        pivot.direction = a.direction;
        pivot.zg = std::min(std::min(a.high, b.high), c.high);
        pivot.zd = std::max(std::max(a.low, b.low), c.low);
        pivot.gg = std::max(std::max(a.high, b.high), c.high);
        pivot.dd = std::min(std::min(a.low, b.low), c.low);
        pivot.status = StructureStatus::Confirmed;

        size_t next = i + 3;
        while (next < segments.size() &&
               pivot.endIndex == segments[next].startIndex &&
               segments[next].status == StructureStatus::Confirmed &&
               rangesOverlap(pivot.zg, pivot.zd, segments[next].high, segments[next].low))
        {
            pivot.endIndex = segments[next].endIndex;
            pivot.gg = std::max(pivot.gg, segments[next].high);
            pivot.dd = std::min(pivot.dd, segments[next].low);
            pivot.status = StructureStatus::Extending;
            ++next;
        }

        if (next < segments.size() &&
            pivot.endIndex == segments[next].startIndex &&
            segments[next].status == StructureStatus::Confirmed)
        {
            pivot.status = StructureStatus::Terminated;
        }

        if (pivots.empty() ||
            !rangesOverlap(pivots.back().zg, pivots.back().zd, pivot.zg, pivot.zd))
        {
            pivots.push_back(pivot);
        }
        i = next;
    }

    return pivots;
}

std::vector<Pivot> buildStrokePivots(const std::vector<Stroke> &strokes)
{
    const auto validStroke = [](const Stroke &stroke) {
        return stroke.status == StructureStatus::Confirmed && stroke.startIndex < stroke.endIndex &&
               stroke.high > stroke.low && std::isfinite(stroke.high) && std::isfinite(stroke.low) &&
               stroke.direction != Direction::None;
    };
    const auto connected = [&](size_t left, size_t right) {
        const Stroke &previous = strokes[left];
        const Stroke &current = strokes[right];
        return validStroke(previous) && validStroke(current) &&
               previous.endIndex == current.startIndex && previous.direction != current.direction &&
               (previous.direction == Direction::Up ? previous.high == current.high
                                                    : previous.low == current.low);
    };
    const auto confirmedAt = [&](size_t index) {
        if (index + 1 < strokes.size() && strokes[index].endIndex == strokes[index + 1].startIndex &&
            strokes[index].direction != strokes[index + 1].direction)
        {
            return strokes[index + 1].endIndex;
        }
        return strokes[index].endIndex;
    };
    std::vector<Pivot> pivots;
    for (size_t index = 0; index + 2 < strokes.size();)
    {
        if (!connected(index, index + 1) || !connected(index + 1, index + 2) ||
            !rangesOverlap(strokes[index].high, strokes[index].low,
                           strokes[index + 1].high, strokes[index + 1].low,
                           strokes[index + 2].high, strokes[index + 2].low))
        {
            ++index;
            continue;
        }
        Pivot pivot{};
        pivot.startIndex = strokes[index].startIndex;
        pivot.endIndex = strokes[index + 2].endIndex;
        pivot.direction = strokes[index].direction;
        pivot.zg = std::min({strokes[index].high, strokes[index + 1].high, strokes[index + 2].high});
        pivot.zd = std::max({strokes[index].low, strokes[index + 1].low, strokes[index + 2].low});
        pivot.gg = std::max({strokes[index].high, strokes[index + 1].high, strokes[index + 2].high});
        pivot.dd = std::min({strokes[index].low, strokes[index + 1].low, strokes[index + 2].low});
        pivot.status = StructureStatus::Confirmed;
        pivot.confirmationIndex = confirmedAt(index + 2);
        for (size_t member = index; member <= index + 2; ++member)
        {
            pivot.strokeIndices.push_back(static_cast<int>(member));
        }
        size_t next = index + 3;
        while (next < strokes.size() && connected(next - 1, next))
        {
            const Stroke &stroke = strokes[next];
            if (!rangesOverlap(pivot.zg, pivot.zd, stroke.high, stroke.low))
            {
                const Stroke &departure = strokes[next - 1];
                const bool outsidePullback =
                    (departure.direction == Direction::Up && stroke.direction == Direction::Down &&
                     stroke.low > pivot.zg) ||
                    (departure.direction == Direction::Down && stroke.direction == Direction::Up &&
                     stroke.high < pivot.zd);
                if (outsidePullback)
                {
                    pivot.status = StructureStatus::Terminated;
                    pivot.terminationIndex = confirmedAt(next);
                }
                break;
            }
            pivot.endIndex = stroke.endIndex;
            pivot.gg = std::max(pivot.gg, stroke.high);
            pivot.dd = std::min(pivot.dd, stroke.low);
            pivot.strokeIndices.push_back(static_cast<int>(next));
            pivot.status = StructureStatus::Extending;
            ++next;
        }
        for (size_t previous = pivots.size(); previous > 0; --previous)
        {
            if (pivots[previous - 1].status == StructureStatus::Candidate) continue;
            if (rangesOverlap(pivots[previous - 1].zg, pivots[previous - 1].zd, pivot.zg, pivot.zd))
            {
                pivot.relatedPivotIndex = static_cast<int>(previous - 1);
                pivot.status = StructureStatus::Candidate;
            }
            break;
        }
        pivots.push_back(std::move(pivot));
        index = next;
    }
    return pivots;
}

TrendSnapshot buildTrendSnapshot(const std::vector<Pivot> &pivots)
{
    if (pivots.empty())
    {
        return {TrendKind::None, TrendCompletion::NotFormed, 0, 0};
    }

    const Pivot &first = pivots.front();
    const Pivot &last = pivots.back();
    if (pivots.size() == 1)
    {
        return {TrendKind::Balance, TrendCompletion::CanComplete, first.startIndex, last.endIndex};
    }

    const Pivot &previous = pivots[pivots.size() - 2];
    if (last.zd > previous.zg)
    {
        return {TrendKind::Up, TrendCompletion::CanComplete, first.startIndex, last.endIndex};
    }
    if (last.zg < previous.zd)
    {
        return {TrendKind::Down, TrendCompletion::CanComplete, first.startIndex, last.endIndex};
    }
    return {TrendKind::Balance, TrendCompletion::Extending, first.startIndex, last.endIndex};
}

ChanlunAnalysis analyze(const std::vector<float> &high,
                        const std::vector<float> &low,
                        const std::vector<float> &close,
                        const AnalysisOptions &options)
{
    (void)close;
    (void)options;
    ChanlunAnalysis analysis;
    analysis.mergedBars = mergeBars(high, low);
    analysis.fractals = findFractals(analysis.mergedBars);
    analysis.strokes = buildStrokes(analysis.mergedBars, analysis.fractals);
    analysis.trend = buildTrendSnapshot(analysis.pivots);
    return analysis;
}
}
