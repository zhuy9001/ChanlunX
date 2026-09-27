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

    std::vector<int> selected;
    std::vector<int> deferredTail;
    int deferredCandidate = -1;
    int initialOpposite = -1;
    int pendingExtreme = -1;
    for (size_t index = 0; index < fractals.size();)
    {
        size_t extreme = index;
        while (index + 1 < fractals.size() && fractals[index + 1].type == fractals[extreme].type)
        {
            ++index;
            if (isMoreExtreme(fractals[index], fractals[extreme]))
            {
                extreme = index;
            }
        }
        ++index;
        const Fractal &candidate = fractals[extreme];
        if (deferredCandidate >= 0 && !deferredTail.empty())
        {
            const Fractal &deferredLast = fractals[static_cast<size_t>(deferredTail.back())];
            if (candidate.type == fractals[static_cast<size_t>(deferredCandidate)].type &&
                isMoreExtreme(candidate, fractals[static_cast<size_t>(deferredCandidate)]) &&
                hasIndependentBarBetween(deferredLast, candidate) && coversRange(deferredLast, candidate) &&
                selected.size() >= 1 && selected.back() == deferredTail.front())
            {
                selected.insert(selected.end(), deferredTail.begin() + 1, deferredTail.end());
                pendingExtreme = -1;
                deferredTail.clear();
                deferredCandidate = -1;
            }
            else if (candidate.type == deferredLast.type && isMoreExtreme(candidate, deferredLast))
            {
                deferredTail.clear();
                deferredCandidate = -1;
            }
        }
        if (pendingExtreme >= 0 && selected.size() >= 3)
        {
            const Fractal &pending = fractals[static_cast<size_t>(pendingExtreme)];
            if (candidate.type == pending.type && isMoreExtreme(candidate, pending))
            {
                pendingExtreme = static_cast<int>(extreme);
            }
            const Fractal &updated = fractals[static_cast<size_t>(pendingExtreme)];
            const Fractal &last = fractals[static_cast<size_t>(selected.back())];
            if (hasIndependentBarBetween(last, updated) && coversRange(last, updated))
            {
                selected.push_back(pendingExtreme);
                pendingExtreme = -1;
            }
            else if (candidate.type == last.type && isMoreExtreme(candidate, last) &&
                     hasIndependentBarBetween(updated, candidate) && coversRange(updated, candidate))
            {
                const Fractal &anchor = fractals[static_cast<size_t>(selected[selected.size() - 3])];
                if (hasIndependentBarBetween(anchor, updated) && coversRange(anchor, updated))
                {
                    selected.resize(selected.size() - 2);
                    selected.push_back(pendingExtreme);
                    pendingExtreme = -1;
                }
            }
        }
        while (!selected.empty())
        {
            const Fractal &last = fractals[static_cast<size_t>(selected.back())];
            if (candidate.type == last.type)
            {
                if (selected.size() == 1 && initialOpposite >= 0 &&
                    isMoreExtreme(candidate, last) &&
                    hasIndependentBarBetween(fractals[static_cast<size_t>(initialOpposite)], candidate) &&
                    coversRange(fractals[static_cast<size_t>(initialOpposite)], candidate))
                {
                    selected.back() = initialOpposite;
                    selected.push_back(static_cast<int>(extreme));
                    initialOpposite = -1;
                    break;
                }
                if (isMoreExtreme(candidate, last) &&
                    (selected.size() == 1 || coversRange(fractals[static_cast<size_t>(selected[selected.size() - 2])], candidate)))
                {
                    selected.pop_back();
                    continue;
                }
                break;
            }
            if (hasIndependentBarBetween(last, candidate) && coversRange(last, candidate))
            {
                selected.push_back(static_cast<int>(extreme));
                initialOpposite = -1;
                break;
            }
            if (selected.size() == 1 &&
                (initialOpposite < 0 || isMoreExtreme(candidate, fractals[static_cast<size_t>(initialOpposite)])))
            {
                initialOpposite = static_cast<int>(extreme);
            }
            if (selected.size() >= 3 &&
                isMoreExtreme(candidate, fractals[static_cast<size_t>(selected[selected.size() - 2])]))
            {
                if (selected.size() == 3 ||
                    !isMoreExtreme(fractals[static_cast<size_t>(selected.back())],
                                   fractals[static_cast<size_t>(selected[selected.size() - 3])]))
                {
                    if (pendingExtreme < 0 ||
                        isMoreExtreme(candidate, fractals[static_cast<size_t>(pendingExtreme)]))
                    {
                        pendingExtreme = static_cast<int>(extreme);
                    }
                    break;
                }
                const int extension = selected.back();
                if (candidate.barIndex - last.barIndex > 1 && selected.size() >= 3)
                {
                    deferredTail.assign(selected.end() - 3, selected.end());
                    deferredCandidate = static_cast<int>(extreme);
                }
                selected.pop_back();
                selected.pop_back();
                if (!selected.empty() &&
                    isMoreExtreme(fractals[static_cast<size_t>(extension)],
                                  fractals[static_cast<size_t>(selected.back())]) &&
                    (selected.size() == 1 ||
                     coversRange(fractals[static_cast<size_t>(selected[selected.size() - 2])],
                                 fractals[static_cast<size_t>(extension)])))
                {
                    selected.back() = extension;
                }
                continue;
            }
            break;
        }
        if (selected.empty())
        {
            selected.push_back(static_cast<int>(extreme));
        }
        if (pendingExtreme >= 0 && pendingExtreme <= selected.back())
        {
            pendingExtreme = -1;
        }
    }
    if (pendingExtreme >= 0 || selected.size() <= 2 ||
        (!selected.empty() && selected.back() + 1 < static_cast<int>(fractals.size())))
    {
        const int lastSelected = selected.back();
        std::vector<int> best = selected;
        size_t bestRetained = selected.size();
        const auto tryAnchor = [&](int anchor, size_t retained) {
            std::vector<std::vector<int>> paths(fractals.size());
            paths[static_cast<size_t>(anchor)] = {anchor};
            for (size_t end = static_cast<size_t>(anchor + 1); end < fractals.size(); ++end)
            {
                for (size_t start = static_cast<size_t>(anchor); start < end; ++start)
                {
                    if (paths[start].empty() || fractals[start].type == fractals[end].type ||
                        !hasIndependentBarBetween(fractals[start], fractals[end]) ||
                        !coversRange(fractals[start], fractals[end]))
                    {
                        continue;
                    }
                    if (paths[end].size() < paths[start].size() + 1)
                    {
                        paths[end] = paths[start];
                        paths[end].push_back(static_cast<int>(end));
                    }
                }
                const size_t pathSize = retained + paths[end].size() - 1;
                if (static_cast<int>(end) > lastSelected && paths[end].size() > 1 &&
                    (pathSize > best.size() ||
                     (pathSize == best.size() &&
                      (static_cast<int>(end) > best.back() ||
                       (static_cast<int>(end) == best.back() && retained > bestRetained)))))
                {
                    best.assign(selected.begin(), selected.begin() + static_cast<std::ptrdiff_t>(retained));
                    best.insert(best.end(), paths[end].begin() + 1, paths[end].end());
                    bestRetained = retained;
                }
            }
        };
        for (size_t retained = selected.size(); retained > (selected.size() > 3 ? selected.size() - 2 : 0); --retained)
        {
            tryAnchor(selected[retained - 1], retained);
        }
        if (best.back() == lastSelected)
        {
            for (size_t anchor = static_cast<size_t>(selected.front() + 1); anchor < fractals.size(); ++anchor)
            {
                std::vector<std::vector<int>> paths(fractals.size());
                paths[anchor] = {static_cast<int>(anchor)};
                for (size_t end = anchor + 1; end < fractals.size(); ++end)
                {
                    for (size_t start = anchor; start < end; ++start)
                    {
                        if (paths[start].empty() || fractals[start].type == fractals[end].type ||
                            !hasIndependentBarBetween(fractals[start], fractals[end]) ||
                            !coversRange(fractals[start], fractals[end]))
                        {
                            continue;
                        }
                        if (paths[end].size() < paths[start].size() + 1)
                        {
                            paths[end] = paths[start];
                            paths[end].push_back(static_cast<int>(end));
                        }
                    }
                    if (static_cast<int>(end) >= lastSelected &&
                        (paths[end].size() > best.size() ||
                         (paths[end].size() == best.size() && static_cast<int>(end) > best.back())))
                    {
                        best = paths[end];
                    }
                }
            }
        }
        selected = std::move(best);
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

        strokes.push_back({start.originalIndex, end.originalIndex, direction, strokeHigh, strokeLow, StructureStatus::Confirmed});
    }

    return strokes;
}

std::vector<Segment> buildSegments(const std::vector<Stroke> &strokes)
{
    std::vector<Segment> segments;
    if (strokes.size() < 3)
    {
        return segments;
    }

    size_t i = 0;
    while (i + 2 < strokes.size())
    {
        const Stroke &a = strokes[i];
        const Stroke &b = strokes[i + 1];
        const Stroke &c = strokes[i + 2];
        if (a.endIndex != b.startIndex || b.endIndex != c.startIndex)
        {
            ++i;
            continue;
        }
        if (!rangesOverlap(a.high, a.low, b.high, b.low, c.high, c.low))
        {
            ++i;
            continue;
        }

        Segment segment;
        segment.startIndex = a.startIndex;
        segment.endIndex = c.endIndex;
        segment.direction = a.direction;
        segment.high = std::max(std::max(a.high, b.high), c.high);
        segment.low = std::min(std::min(a.low, b.low), c.low);
        segment.status = StructureStatus::Confirmed;

        size_t j = i + 3;
        while (j < strokes.size() && segment.endIndex == strokes[j].startIndex &&
               rangesOverlap(segment.high, segment.low, strokes[j].high, strokes[j].low))
        {
            segment.endIndex = strokes[j].endIndex;
            segment.high = std::max(segment.high, strokes[j].high);
            segment.low = std::min(segment.low, strokes[j].low);
            ++j;
        }

        segments.push_back(segment);
        i = std::max(j, i + 3);
    }

    return segments;
}

std::vector<Pivot> buildPivots(const std::vector<Segment> &segments, int level)
{
    std::vector<Pivot> pivots;
    if (segments.size() < 3)
    {
        return pivots;
    }

    for (size_t i = 0; i + 2 < segments.size(); ++i)
    {
        const Segment &a = segments[i];
        const Segment &b = segments[i + 1];
        const Segment &c = segments[i + 2];
        if (!rangesOverlap(a.high, a.low, b.high, b.low, c.high, c.low))
        {
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
        pivots.push_back(pivot);
    }

    return pivots;
}

std::vector<Pivot> buildStrokePivots(const std::vector<Stroke> &strokes)
{
    std::vector<Pivot> pivots;
    for (size_t index = 0; index + 2 < strokes.size();)
    {
        const Stroke &first = strokes[index];
        const Stroke &second = strokes[index + 1];
        const Stroke &third = strokes[index + 2];
        if (first.endIndex != second.startIndex || second.endIndex != third.startIndex)
        {
            ++index;
            continue;
        }
        if (!rangesOverlap(first.high, first.low, second.high, second.low, third.high, third.low))
        {
            ++index;
            continue;
        }

        Pivot pivot;
        pivot.startIndex = first.startIndex;
        pivot.endIndex = third.endIndex;
        pivot.level = 0;
        pivot.direction = first.direction;
        pivot.zg = std::min(std::min(first.high, second.high), third.high);
        pivot.zd = std::max(std::max(first.low, second.low), third.low);
        pivot.gg = std::max(std::max(first.high, second.high), third.high);
        pivot.dd = std::min(std::min(first.low, second.low), third.low);
        pivot.status = StructureStatus::Confirmed;
        size_t next = index + 3;
        while (next < strokes.size() && pivot.endIndex == strokes[next].startIndex &&
               rangesOverlap(pivot.zg, pivot.zd, strokes[next].high, strokes[next].low))
        {
            pivot.endIndex = strokes[next].endIndex;
            pivot.gg = std::max(pivot.gg, strokes[next].high);
            pivot.dd = std::min(pivot.dd, strokes[next].low);
            ++next;
        }
        pivots.push_back(pivot);
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
    ChanlunAnalysis analysis;
    analysis.mergedBars = mergeBars(high, low);
    analysis.fractals = findFractals(analysis.mergedBars);
    analysis.strokes = buildStrokes(analysis.mergedBars, analysis.fractals);
    analysis.segments = buildSegments(analysis.strokes);
    analysis.pivots = buildPivots(analysis.segments, options.level);
    analysis.trend = buildTrendSnapshot(analysis.pivots);
    return analysis;
}
}
