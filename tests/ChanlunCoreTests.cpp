#include "ChanlunCore.h"
#include "ChanlunSignals.h"
#include "ChanlunTdx.h"

#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>
#include <vector>

namespace
{
void require(bool condition, const std::string &message)
{
    if (!condition)
    {
        std::cerr << "FAILED: " << message << "\n";
        std::exit(1);
    }
}

void requireNear(float actual, float expected, const std::string &message)
{
    if (std::fabs(actual - expected) > 0.0001F)
    {
        std::cerr << "FAILED: " << message << " expected " << expected << " actual " << actual << "\n";
        std::exit(1);
    }
}

chanlun::Stroke makeStroke(int start, int end, chanlun::Direction direction, float high, float low)
{
    chanlun::Stroke stroke;
    stroke.startIndex = start;
    stroke.endIndex = end;
    stroke.direction = direction;
    stroke.high = high;
    stroke.low = low;
    stroke.status = chanlun::StructureStatus::Confirmed;
    return stroke;
}

chanlun::Segment makeSegment(int start, int end, chanlun::Direction direction, float high, float low)
{
    chanlun::Segment segment;
    segment.startIndex = start;
    segment.endIndex = end;
    segment.direction = direction;
    segment.high = high;
    segment.low = low;
    segment.status = chanlun::StructureStatus::Confirmed;
    return segment;
}

void testSequentialContainmentUsesTrendDirection()
{
    const std::vector<float> high{10.0F, 11.0F, 10.5F, 12.0F};
    const std::vector<float> low{8.0F, 9.0F, 9.5F, 10.0F};

    const auto merged = chanlun::mergeBars(high, low);

    require(merged.size() == 3, "contained bar is merged into the prior up bar");
    require(merged[1].startIndex == 1, "merged bar preserves start index");
    require(merged[1].endIndex == 2, "merged bar preserves end index");
    requireNear(merged[1].high, 11.0F, "up containment keeps max high");
    requireNear(merged[1].low, 9.5F, "up containment keeps max low");
}

void testFractalsOnlyUseMergedBars()
{
    const std::vector<chanlun::MergedBar> bars{
        {10.0F, 8.0F, chanlun::Direction::None, 0, 0, 0},
        {12.0F, 10.0F, chanlun::Direction::Up, 1, 1, 1},
        {11.0F, 7.0F, chanlun::Direction::Down, 2, 2, 2},
        {9.0F, 5.0F, chanlun::Direction::Down, 3, 3, 3},
        {10.0F, 6.0F, chanlun::Direction::Up, 4, 4, 4},
    };

    const auto fractals = chanlun::findFractals(bars);

    require(fractals.size() == 2, "finds one top and one bottom fractal");
    require(fractals[0].type == chanlun::FractalType::Top, "first fractal is a top");
    require(fractals[0].barIndex == 1, "top fractal maps to merged bar index");
    require(fractals[0].originalIndex == 1, "top fractal maps to original bar index");
    require(fractals[1].type == chanlun::FractalType::Bottom, "second fractal is a bottom");
    require(fractals[1].barIndex == 3, "bottom fractal maps to merged bar index");
}

void testStrokesRequireAlternatingIndependentFractals()
{
    const std::vector<chanlun::MergedBar> bars{
        {10.0F, 8.0F, chanlun::Direction::None, 0, 0, 0},
        {12.0F, 10.0F, chanlun::Direction::Up, 1, 1, 1},
        {11.0F, 9.0F, chanlun::Direction::Down, 2, 2, 2},
        {10.0F, 8.0F, chanlun::Direction::Down, 3, 3, 3},
        {9.0F, 7.0F, chanlun::Direction::Down, 4, 4, 4},
        {8.0F, 6.0F, chanlun::Direction::Down, 5, 5, 5},
        {9.0F, 7.0F, chanlun::Direction::Up, 6, 6, 6},
        {10.0F, 8.0F, chanlun::Direction::Up, 7, 7, 7},
        {11.0F, 9.0F, chanlun::Direction::Up, 8, 8, 8},
        {13.0F, 11.0F, chanlun::Direction::Up, 9, 9, 9},
        {12.0F, 10.0F, chanlun::Direction::Down, 10, 10, 10},
    };
    const auto fractals = chanlun::findFractals(bars);

    const auto strokes = chanlun::buildStrokes(bars, fractals);

    require(strokes.size() == 2, "builds two alternating strokes");
    require(strokes[0].direction == chanlun::Direction::Down, "top to bottom is a down stroke");
    require(strokes[0].startIndex == 1 && strokes[0].endIndex == 5, "down stroke endpoints are preserved");
    require(strokes[1].direction == chanlun::Direction::Up, "bottom to top is an up stroke");
    require(strokes[1].startIndex == 5 && strokes[1].endIndex == 9, "up stroke endpoints are preserved");
}

void testSegmentsRequireThreeOverlappingStrokes()
{
    const std::vector<chanlun::Stroke> strokes{
        makeStroke(1, 3, chanlun::Direction::Down, 12.0F, 8.0F),
        makeStroke(3, 5, chanlun::Direction::Up, 11.0F, 9.0F),
        makeStroke(5, 7, chanlun::Direction::Down, 13.0F, 10.0F),
    };

    const auto segments = chanlun::buildSegments(strokes);

    require(segments.size() == 1, "three overlapping strokes form a segment");
    require(segments[0].startIndex == 1 && segments[0].endIndex == 7, "segment spans the three strokes");
    require(segments[0].direction == chanlun::Direction::Down, "segment keeps first stroke direction");
}

void testPivotsUseThreeOverlappingSegments()
{
    const std::vector<chanlun::Segment> segments{
        makeSegment(1, 3, chanlun::Direction::Up, 12.0F, 8.0F),
        makeSegment(3, 5, chanlun::Direction::Down, 11.0F, 9.0F),
        makeSegment(5, 7, chanlun::Direction::Up, 13.0F, 10.0F),
    };

    const auto pivots = chanlun::buildPivots(segments);

    require(pivots.size() == 1, "three overlapping segments form a pivot");
    requireNear(pivots[0].zd, 10.0F, "pivot ZD is max low");
    requireNear(pivots[0].zg, 11.0F, "pivot ZG is min high");
    requireNear(pivots[0].dd, 8.0F, "pivot DD is min low");
    requireNear(pivots[0].gg, 13.0F, "pivot GG is max high");
}

void testMacdSupportsDefaultSignalLayer()
{
    const std::vector<float> close{1.0F, 2.0F, 3.0F, 4.0F, 5.0F, 6.0F};

    const auto macd = chanlun::calculateMacd(close, chanlun::MacdOptions{});

    require(macd.size() == close.size(), "MACD output length matches input");
    require(macd.back().dif > 0.0F, "rising prices produce positive DIF");
}

void testTdxAdapterProjectsFractalSeries()
{
    const std::vector<float> high{10.0F, 12.0F, 11.0F, 9.0F, 10.0F};
    const std::vector<float> low{8.0F, 10.0F, 7.0F, 5.0F, 6.0F};
    const std::vector<float> close{9.0F, 11.0F, 8.0F, 6.0F, 7.0F};

    const auto series = chanlun::evaluateTdxFunction(101, high, low, close);

    require(series.size() == high.size(), "TDX function output length matches input");
    requireNear(series[1], 1.0F, "function 101 marks top fractal");
    requireNear(series[3], -1.0F, "function 101 marks bottom fractal");
}

void testPendingContainmentWaitsForDirection()
{
    const std::vector<float> high{10.0F, 9.0F};
    const std::vector<float> low{5.0F, 6.0F};
    const auto pending = chanlun::mergeBars(high, low);
    require(pending.size() == 1 && pending[0].direction == chanlun::Direction::Up,
            "initial containment uses a stable upward direction");
    requireNear(chanlun::evaluateTdxFunction(102, high, low, {} )[0], 10.0F,
                "initial containment draws its merged high");

    const auto rising = chanlun::mergeBars({10.0F, 9.0F, 11.0F}, {5.0F, 6.0F, 7.0F});
    requireNear(rising[0].high, 10.0F, "up breakout retains higher group high");
    requireNear(rising[0].low, 6.0F, "up breakout raises group low");
    const auto falling = chanlun::mergeBars({10.0F, 9.0F, 8.0F}, {5.0F, 6.0F, 4.0F});
    requireNear(falling[0].high, 10.0F, "later down breakout does not rewrite initial group high");
    requireNear(falling[0].low, 6.0F, "later down breakout does not rewrite initial group low");

    const auto chained = chanlun::mergeBars({10.0F, 9.0F, 8.0F, 11.0F},
                                            {5.0F, 6.0F, 7.0F, 8.0F});
    require(chained.size() == 2 && chained[0].endIndex == 2,
            "three initially contained bars are resolved together");
    requireNear(chained[0].low, 7.0F, "initial group merges each contained bar in sequence");
}

void testInitialContainmentDoesNotUseBoundingEnvelope()
{
    const auto merged = chanlun::mergeBars({10.0F, 9.0F, 9.5F, 11.0F},
                                           {5.0F, 6.0F, 5.5F, 7.0F});
    require(merged.size() == 3, "crossing third bar stays independent of initial contained pair");
    require(merged[0].endIndex == 1 && merged[1].startIndex == 2,
            "sequential containment stops at first non-contained bar");
    requireNear(merged[0].low, 6.0F, "later direction does not rewrite the initial contained pair");
}

void testContainmentLevelsUseMergedPrices()
{
    const std::vector<float> high{12.0F, 10.0F, 9.0F, 8.0F};
    const std::vector<float> low{8.0F, 6.0F, 7.0F, 5.0F};
    const auto highLine = chanlun::evaluateTdxFunction(102, high, low, {});
    const auto lowLine = chanlun::evaluateTdxFunction(103, high, low, {});
    requireNear(highLine[1], 9.0F, "down containment marks merged lower high");
    requireNear(highLine[2], 9.0F, "merged high spans both bars");
    requireNear(lowLine[1], 6.0F, "down containment marks merged lower low");
}

void testFractalEndpointUsesExtremeOriginalCandle()
{
    const std::vector<float> high{48.0F, 54.84F, 53.0F, 52.0F, 49.0F, 46.0F, 45.0F, 46.0F, 48.0F};
    const std::vector<float> low{44.0F, 50.0F, 51.0F, 48.0F, 45.0F, 42.0F, 40.0F, 43.0F, 45.0F};
    const auto bars = chanlun::mergeBars(high, low);
    const auto fractals = chanlun::findFractals(bars);
    require(fractals.size() >= 2, "merged top and later bottom remain identifiable");
    require(fractals[0].type == chanlun::FractalType::Top, "first fractal is the high");
    require(fractals[0].originalIndex == 1, "top endpoint stays on the 54.84 candle");
    requireNear(high[fractals[0].originalIndex], 54.84F, "top marker has the actual price");
    const auto strokes = chanlun::buildStrokes(bars, fractals);
    require(!strokes.empty(), "separated top and bottom form a stroke");
    require(strokes[0].startIndex == 1, "stroke begins at actual 54.84 high");
}

void testStrokeRequiresBarOutsideBothFractals()
{
    const std::vector<chanlun::MergedBar> bars{
        {10.0F, 8.0F, chanlun::Direction::None, 0, 0, 0},
        {12.0F, 10.0F, chanlun::Direction::Up, 1, 1, 1},
        {11.0F, 7.0F, chanlun::Direction::Down, 2, 2, 2},
        {9.0F, 5.0F, chanlun::Direction::Down, 3, 3, 3},
        {10.0F, 6.0F, chanlun::Direction::Up, 4, 4, 4},
        {13.0F, 11.0F, chanlun::Direction::Up, 5, 5, 5},
        {12.0F, 8.0F, chanlun::Direction::Down, 6, 6, 6},
    };
    const auto strokes = chanlun::buildStrokes(bars, chanlun::findFractals(bars));
    require(strokes.empty(), "fractal centers two bars apart have no independent bar");
}

void testNearbyOppositeFractalDoesNotReplaceConfirmedEndpoint()
{
    const std::vector<chanlun::MergedBar> bars{
        {10.0F, 8.0F, chanlun::Direction::None, 0, 0, 0},
        {12.0F, 10.0F, chanlun::Direction::Up, 1, 1, 1},
        {11.0F, 9.0F, chanlun::Direction::Down, 2, 2, 2},
        {10.0F, 8.0F, chanlun::Direction::Down, 3, 3, 3},
        {11.0F, 9.0F, chanlun::Direction::Up, 4, 4, 4},
        {9.0F, 7.0F, chanlun::Direction::Down, 5, 5, 5},
        {8.0F, 6.0F, chanlun::Direction::Down, 6, 6, 6},
        {7.0F, 5.0F, chanlun::Direction::Down, 7, 7, 7},
        {8.0F, 6.0F, chanlun::Direction::Up, 8, 8, 8},
    };
    const auto fractals = chanlun::findFractals(bars);
    const auto strokes = chanlun::buildStrokes(bars, fractals);
    require(strokes.size() == 1, "nearby bottom must not erase the previous top");
    require(strokes[0].startIndex == 1 && strokes[0].endIndex == 7,
            "stroke joins the actual top to the confirmed bottom");
}

void testThreeOverlappingStrokesFormStrokeLevelPivot()
{
    const std::vector<chanlun::Stroke> strokes{
        makeStroke(1, 5, chanlun::Direction::Up, 12.0F, 8.0F),
        makeStroke(5, 9, chanlun::Direction::Down, 12.0F, 9.0F),
        makeStroke(9, 13, chanlun::Direction::Up, 13.0F, 9.0F),
    };
    const auto pivots = chanlun::buildStrokePivots(strokes);
    require(pivots.size() == 1, "three overlapping strokes form a stroke-level pivot");
    requireNear(pivots[0].zg, 12.0F, "stroke pivot upper bound");
    requireNear(pivots[0].zd, 9.0F, "stroke pivot lower bound");
    require(pivots[0].startIndex == 1 && pivots[0].endIndex == 13, "pivot spans three strokes");
}

void testStrokePivotProjectionUsesSeparateFunctionIds()
{
    const std::vector<float> high{10, 12, 11, 10, 9, 8, 9, 10, 11, 13, 12, 11, 10, 9, 10, 11};
    const std::vector<float> low{8, 10, 9, 8, 7, 6, 7, 8, 9, 11, 10, 9, 8, 7, 8, 9};
    const auto zg = chanlun::evaluateTdxFunction(210, high, low, {});
    const auto zd = chanlun::evaluateTdxFunction(211, high, low, {});
    const auto boundary = chanlun::evaluateTdxFunction(212, high, low, {});
    require(zg.size() == high.size() && zd.size() == high.size() && boundary.size() == high.size(),
            "stroke pivot outputs keep input length");
    const auto analysis = chanlun::analyze(high, low, {});
    const auto pivots = chanlun::buildStrokePivots(analysis.strokes);
    if (!pivots.empty())
    {
        requireNear(zg[pivots[0].startIndex], pivots[0].zg, "stroke ZG projection");
        requireNear(zd[pivots[0].endIndex], pivots[0].zd, "stroke ZD projection");
        requireNear(boundary[pivots[0].startIndex], 1.0F, "stroke pivot start projection");
    }
}

void testStrokePivotExtendsInsteadOfRepeatingSlidingWindows()
{
    const std::vector<chanlun::Stroke> strokes{
        makeStroke(1, 5, chanlun::Direction::Up, 12.0F, 8.0F),
        makeStroke(5, 9, chanlun::Direction::Down, 12.0F, 9.0F),
        makeStroke(9, 13, chanlun::Direction::Up, 13.0F, 9.0F),
        makeStroke(13, 17, chanlun::Direction::Down, 11.0F, 8.5F),
    };
    const auto pivots = chanlun::buildStrokePivots(strokes);
    require(pivots.size() == 1, "adjacent overlapping triples extend one pivot");
    require(pivots[0].endIndex == 17, "extension reaches the fourth stroke");
    requireNear(pivots[0].zg, 12.0F, "extension preserves initial ZG");
    requireNear(pivots[0].zd, 9.0F, "extension preserves initial ZD");
}

void testAdjacentPivotBoundariesKeepBothPriceRanges()
{
    const std::vector<chanlun::Pivot> pivots{
        {1, 5, 0, chanlun::Direction::Up, 12.0F, 9.0F, 14.0F, 7.0F, chanlun::StructureStatus::Confirmed},
        {5, 9, 0, chanlun::Direction::Down, 20.0F, 17.0F, 22.0F, 15.0F, chanlun::StructureStatus::Confirmed},
    };
    const auto leftHigh = chanlun::projectStrokePivotBoundary(pivots, 11, true, true);
    const auto leftLow = chanlun::projectStrokePivotBoundary(pivots, 11, true, false);
    const auto rightHigh = chanlun::projectStrokePivotBoundary(pivots, 11, false, true);
    const auto rightLow = chanlun::projectStrokePivotBoundary(pivots, 11, false, false);
    requireNear(rightHigh[5], 12.0F, "previous pivot right high survives shared index");
    requireNear(rightLow[5], 9.0F, "previous pivot right low survives shared index");
    requireNear(leftHigh[5], 20.0F, "next pivot left high survives shared index");
    requireNear(leftLow[5], 17.0F, "next pivot left low survives shared index");
    requireNear(rightHigh[4], 0.0F, "right boundary only appears at end");
    requireNear(leftHigh[6], 0.0F, "left boundary only appears at start");
}

void testStrokeCandidateRemovesUnconfirmedCounterMove()
{
    std::vector<chanlun::MergedBar> bars;
    for (int index = 0; index <= 12; ++index)
    {
        bars.push_back({90, 85, chanlun::Direction::None, index, index, index});
    }
    bars[0].high = 110;
    bars[4].low = 80;
    bars[8].high = 96;
    bars[12].low = 75;
    const std::vector<chanlun::Fractal> fractals{
        {chanlun::FractalType::Top, 0, 0, 110},
        {chanlun::FractalType::Bottom, 4, 4, 80},
        {chanlun::FractalType::Top, 8, 8, 96},
        {chanlun::FractalType::Bottom, 12, 12, 75},
    };
    const auto strokes = chanlun::buildStrokes(bars, fractals);
    require(strokes.size() == 3, "valid alternating fractals remain distinct down/up/down strokes");
    require(strokes[0].startIndex == 0 && strokes[0].endIndex == 4,
            "first down stroke remains after a later lower low");
}

void testDownStrokeExtendsPastUnconfirmedRebound()
{
    std::vector<chanlun::MergedBar> bars;
    for (int index = 0; index <= 14; ++index)
    {
        bars.push_back({16.0F, 12.0F, chanlun::Direction::None, index, index, index});
    }
    bars[0].high = 25.0F;
    bars[5].low = 10.0F;
    bars[8].high = 19.0F;
    bars[12].low = 8.0F;
    bars[14].high = 18.0F;
    const std::vector<chanlun::Fractal> fractals{
        {chanlun::FractalType::Top, 0, 0, 25.0F},
        {chanlun::FractalType::Bottom, 5, 5, 10.0F},
        {chanlun::FractalType::Top, 8, 8, 19.0F},
        {chanlun::FractalType::Bottom, 12, 12, 8.0F},
    };
    const auto strokes = chanlun::buildStrokes(bars, fractals);
    require(strokes.size() == 1 && strokes[0].startIndex == 0 && strokes[0].endIndex == 12,
            "lower bottom extends stroke when intermediate rebound cannot form an independent stroke");
}

void testInvalidReversalRollsBackEarlierStrokes()
{
    std::vector<chanlun::MergedBar> bars;
    for (int index = 0; index <= 33; ++index)
    {
        bars.push_back({16.0F, 14.0F, chanlun::Direction::None, index, index, index});
    }
    bars[0].high = 17.58F;
    bars[5].low = 13.0F;
    bars[10].high = 14.57F;
    bars[15].low = 12.68F;
    bars[17].high = 14.64F;
    bars[29].low = 10.0F;
    bars[33].high = 13.60F;
    const std::vector<chanlun::Fractal> fractals{
        {chanlun::FractalType::Top, 0, 0, 17.58F},
        {chanlun::FractalType::Bottom, 5, 5, 13.0F},
        {chanlun::FractalType::Top, 10, 10, 14.57F},
        {chanlun::FractalType::Bottom, 15, 15, 12.68F},
        {chanlun::FractalType::Top, 17, 17, 14.64F},
        {chanlun::FractalType::Bottom, 29, 29, 10.0F},
        {chanlun::FractalType::Top, 33, 33, 13.60F},
    };
    const auto strokes = chanlun::buildStrokes(bars, fractals);
    require(!strokes.empty() && strokes.front().startIndex == 0 && strokes.front().endIndex == 29,
            "invalid reversal rolls back short turns and extends the down stroke");
}

void testConfirmedPriorStrokeSurvivesLaterRollback()
{
    std::vector<chanlun::MergedBar> bars;
    for (int index = 0; index <= 35; ++index)
    {
        bars.push_back({16.0F, 14.0F, chanlun::Direction::None, index, index, index});
    }
    bars[0].high = 24.0F;
    bars[5].low = 13.0F;
    bars[10].high = 21.0F;
    bars[15].low = 12.0F;
    bars[20].high = 19.0F;
    bars[25].low = 11.0F;
    bars[27].high = 23.0F;
    bars[35].low = 10.0F;
    const std::vector<chanlun::Fractal> fractals{
        {chanlun::FractalType::Top, 0, 0, 24.0F},
        {chanlun::FractalType::Bottom, 5, 5, 13.0F},
        {chanlun::FractalType::Top, 10, 10, 21.0F},
        {chanlun::FractalType::Bottom, 15, 15, 12.0F},
        {chanlun::FractalType::Top, 20, 20, 19.0F},
        {chanlun::FractalType::Bottom, 25, 25, 11.0F},
        {chanlun::FractalType::Top, 27, 27, 23.0F},
        {chanlun::FractalType::Bottom, 35, 35, 10.0F},
    };
    const auto strokes = chanlun::buildStrokes(bars, fractals);
    require(!strokes.empty() && strokes.front().startIndex == 0 && strokes.back().endIndex == 35,
            "revalidated chain reaches the later extreme");
    for (size_t index = 1; index < strokes.size(); ++index)
    {
        require(strokes[index - 1].endIndex == strokes[index].startIndex,
                "revalidated strokes remain connected");
    }
}

void testConfirmedStrokeContinuesAfterInvalidTail()
{
    std::vector<chanlun::MergedBar> bars;
    for (int index = 0; index <= 20; ++index)
    {
        bars.push_back({18.0F, 12.0F, chanlun::Direction::None, index, index, index});
    }
    bars[0].low = 5.0F;
    bars[5].high = 25.0F;
    bars[10].low = 10.0F;
    bars[13].high = 28.0F;
    bars[14].high = 29.0F;
    bars[20].low = 8.0F;
    const std::vector<chanlun::Fractal> fractals{
        {chanlun::FractalType::Bottom, 0, 0, 5.0F},
        {chanlun::FractalType::Top, 5, 5, 25.0F},
        {chanlun::FractalType::Bottom, 10, 10, 10.0F},
        {chanlun::FractalType::Top, 13, 13, 28.0F},
        {chanlun::FractalType::Top, 14, 14, 29.0F},
        {chanlun::FractalType::Bottom, 20, 20, 8.0F},
    };
    const auto strokes = chanlun::buildStrokes(bars, fractals);
    require(strokes.size() == 4, "valid later turn continues the confirmed chain");
    require(strokes[0].startIndex == 0 && strokes[0].endIndex == 5 &&
                strokes[1].startIndex == 5 && strokes[1].endIndex == 10,
            "confirmed prefix remains intact after a short turn");
    require(strokes[2].startIndex == 10 && strokes[2].endIndex == 14 &&
                strokes[3].startIndex == 14 && strokes[3].endIndex == 20,
            "later valid extreme restores a continuous chain");
}

void testInvalidTurnRevalidatesEarlierStrokes()
{
    std::vector<chanlun::MergedBar> bars;
    for (int index = 0; index <= 57; ++index)
    {
        bars.push_back({14.0F, 13.5F, chanlun::Direction::None, index, index, index});
    }
    bars[0].high = 17.58F;
    bars[15].low = 13.0F;
    bars[26].high = 14.57F;
    bars[32].low = 12.68F;
    bars[34].high = 14.64F;
    bars[57].low = 10.6F;
    const std::vector<chanlun::Fractal> fractals{
        {chanlun::FractalType::Top, 0, 0, 17.58F},
        {chanlun::FractalType::Bottom, 15, 15, 13.0F},
        {chanlun::FractalType::Top, 26, 26, 14.57F},
        {chanlun::FractalType::Bottom, 32, 32, 12.68F},
        {chanlun::FractalType::Top, 34, 34, 14.64F},
        {chanlun::FractalType::Bottom, 57, 57, 10.6F},
    };
    const auto strokes = chanlun::buildStrokes(bars, fractals);
    require(strokes.size() == 1 && strokes[0].startIndex == 0 && strokes[0].endIndex == 57,
            "an invalid short reversal propagates endpoint revalidation to the earlier down stroke");
}

void testConsecutiveSameTypeFractalsChooseExtreme()
{
    std::vector<chanlun::MergedBar> bars;
    for (int index = 0; index <= 14; ++index)
    {
        bars.push_back({18.0F, 12.0F, chanlun::Direction::None, index, index, index});
    }
    bars[0].low = 5.0F;
    bars[4].high = 20.0F;
    bars[5].high = 22.0F;
    bars[6].high = 21.0F;
    bars[14].low = 8.0F;
    const std::vector<chanlun::Fractal> fractals{
        {chanlun::FractalType::Bottom, 0, 0, 5.0F},
        {chanlun::FractalType::Top, 4, 4, 20.0F},
        {chanlun::FractalType::Top, 5, 5, 22.0F},
        {chanlun::FractalType::Top, 6, 6, 21.0F},
        {chanlun::FractalType::Bottom, 14, 14, 8.0F},
    };
    const auto strokes = chanlun::buildStrokes(bars, fractals);
    require(strokes.size() == 2 && strokes[0].endIndex == 5 && strokes[1].startIndex == 5,
            "consecutive same-type fractals select their most extreme endpoint");
}

void testStrokeAggregatesDoNotCrossGap()
{
    const std::vector<chanlun::Stroke> strokes{
        makeStroke(0, 5, chanlun::Direction::Up, 25, 5),
        makeStroke(5, 10, chanlun::Direction::Down, 25, 10),
        makeStroke(13, 20, chanlun::Direction::Down, 28, 8),
    };
    require(chanlun::buildSegments(strokes).empty(), "segment cannot cross a disconnected stroke chain");
    require(chanlun::buildStrokePivots(strokes).empty(), "pivot cannot cross a disconnected stroke chain");
}

void test300652HistoricalChainRecovers()
{
    std::ifstream input(std::string(CHANLUN_TEST_SOURCE_DIR) + "/tests/data/300652-unadjusted-daily.csv");
    require(input.good(), "300652 fixture is available");
    std::vector<std::string> dates;
    std::vector<float> high;
    std::vector<float> low;
    std::string line;
    while (std::getline(input, line))
    {
        std::istringstream row(line);
        std::string field;
        std::getline(row, field, ',');
        dates.push_back(field);
        std::getline(row, field, ',');
        high.push_back(std::stof(field));
        std::getline(row, field, ',');
        low.push_back(std::stof(field));
    }
    require(dates.size() == 663, "300652 complete historical window is loaded");
    const auto analysis = chanlun::analyze(high, low, {});
    bool foundAugustContainment = false;
    for (const auto &bar : analysis.mergedBars)
    {
        if (dates[bar.startIndex] == "2024-08-29" && dates[bar.endIndex] == "2024-08-30")
        {
            requireNear(bar.high, 18.48F, "300652 August containment retains upward high");
            requireNear(bar.low, 16.77F, "300652 August containment uses merged low");
            foundAugustContainment = true;
        }
    }
    require(foundAugustContainment, "300652 August containment group is present");
    bool hasRevalidatedDownStroke = false;
    for (const auto &stroke : analysis.strokes)
    {
        require(stroke.startIndex < stroke.endIndex, "300652 stroke points forward in time");
        hasRevalidatedDownStroke |= dates[stroke.startIndex] == "2024-06-19" &&
                                    dates[stroke.endIndex] == "2024-08-28" &&
                                    stroke.direction == chanlun::Direction::Down;
    }
    require(hasRevalidatedDownStroke, "300652 invalid intermediate turns extend the June down stroke");
    for (size_t index = 1; index < analysis.strokes.size(); ++index)
    {
        require(analysis.strokes[index - 1].endIndex == analysis.strokes[index].startIndex,
                "300652 revalidated strokes share endpoints");
    }
    require(!analysis.strokes.empty() && dates[analysis.strokes.back().endIndex] >= "2026-03-04",
            "300652 does not remain stranded at the 2024 rollback boundary");
    bool hasPostFebruaryStroke = false;
    for (const auto &stroke : analysis.strokes)
    {
        if (dates[stroke.startIndex] >= "2026-02-24")
        {
            hasPostFebruaryStroke = true;
        }
    }
    require(hasPostFebruaryStroke, "300652 resumes with a valid stroke after February");
}

void test000801HistoricalTailRevalidates()
{
    std::ifstream input(std::string(CHANLUN_TEST_SOURCE_DIR) + "/tests/data/000801-unadjusted-daily.csv");
    require(input.good(), "000801 historical fixture is available");
    std::vector<std::string> dates;
    std::vector<float> high;
    std::vector<float> low;
    std::string line;
    while (std::getline(input, line))
    {
        std::istringstream row(line);
        std::string field;
        std::getline(row, field, ',');
        dates.push_back(field);
        std::getline(row, field, ',');
        high.push_back(std::stof(field));
        std::getline(row, field, ',');
        low.push_back(std::stof(field));
    }
    require(dates.size() == 660, "000801 full historical fixture is loaded");
    const auto strokes = chanlun::analyze(high, low, {}).strokes;
    bool extended = false;
    for (const auto &stroke : strokes)
    {
        extended |= dates[stroke.startIndex] == "2026-02-25" &&
                    dates[stroke.endIndex] == "2026-06-26" &&
                    stroke.direction == chanlun::Direction::Down;
    }
    require(extended, "000801 invalid tail turns extend the February down stroke to June");
}

void test300383PreservesValidPrefix()
{
    std::ifstream input(std::string(CHANLUN_TEST_SOURCE_DIR) + "/tests/data/300383-daily.csv");
    require(input.good(), "300383 fixture is available");
    std::vector<std::string> dates;
    std::vector<float> high;
    std::vector<float> low;
    std::string line;
    while (std::getline(input, line))
    {
        std::istringstream row(line);
        std::string field;
        std::getline(row, field, ',');
        dates.push_back(field);
        std::getline(row, field, ',');
        high.push_back(std::stof(field));
        std::getline(row, field, ',');
        low.push_back(std::stof(field));
    }
    const auto strokes = chanlun::analyze(high, low, {}).strokes;
    bool hasFirst = false;
    bool hasSecond = false;
    bool hasThird = false;
    for (const auto &stroke : strokes)
    {
        hasFirst |= dates[stroke.startIndex] == "2025-12-17" && dates[stroke.endIndex] == "2026-01-14";
        hasSecond |= dates[stroke.startIndex] == "2026-01-14" && dates[stroke.endIndex] == "2026-01-21";
        hasThird |= dates[stroke.startIndex] == "2026-01-21" && dates[stroke.endIndex] == "2026-02-03";
    }
    require(hasFirst && hasSecond && hasThird,
            "300383 preserves the valid prefix while revalidating the later tail");
}

void testHistoricalStrokeChainsContinue()
{
    const struct Case
    {
        const char *code;
        const char *lastStalledDate;
    } cases[]{
        {"603360", "2026-01-01"},
        {"000025", "2025-04-07"},
        {"000028", "2026-06-18"},
        {"000034", "2026-02-27"},
        {"300383", "2026-06-25"},
    };
    for (const auto &testCase : cases)
    {
        std::ifstream input(std::string(CHANLUN_TEST_SOURCE_DIR) + "/tests/data/" +
                            testCase.code + "-full-daily.csv");
        require(input.good(), std::string(testCase.code) + " fixture is available");
        std::vector<std::string> dates;
        std::vector<float> high;
        std::vector<float> low;
        std::string line;
        while (std::getline(input, line))
        {
            std::istringstream row(line);
            std::string field;
            std::getline(row, field, ',');
            dates.push_back(field);
            std::getline(row, field, ',');
            high.push_back(std::stof(field));
            std::getline(row, field, ',');
            low.push_back(std::stof(field));
        }
        const auto analysis = chanlun::analyze(high, low, {});
        require(!analysis.strokes.empty() && dates[analysis.strokes.back().endIndex] > testCase.lastStalledDate,
                std::string(testCase.code) + " continues beyond its previous stalled endpoint");
        for (const auto &stroke : analysis.strokes)
        {
            int startBar = -1;
            int endBar = -1;
            for (const auto &fractal : analysis.fractals)
            {
                if (fractal.originalIndex == stroke.startIndex) startBar = fractal.barIndex;
                if (fractal.originalIndex == stroke.endIndex) endBar = fractal.barIndex;
            }
            require(startBar >= 0 && endBar - startBar >= 4,
                    std::string(testCase.code) + " stroke has an independent merged bar");
            float highest = -std::numeric_limits<float>::infinity();
            float lowest = std::numeric_limits<float>::infinity();
            for (int bar = startBar; bar <= endBar; ++bar)
            {
                highest = std::max(highest, analysis.mergedBars[static_cast<size_t>(bar)].high);
                lowest = std::min(lowest, analysis.mergedBars[static_cast<size_t>(bar)].low);
            }
            const float top = stroke.direction == chanlun::Direction::Up ?
                                  analysis.mergedBars[static_cast<size_t>(endBar)].high :
                                  analysis.mergedBars[static_cast<size_t>(startBar)].high;
            const float bottom = stroke.direction == chanlun::Direction::Up ?
                                     analysis.mergedBars[static_cast<size_t>(startBar)].low :
                                     analysis.mergedBars[static_cast<size_t>(endBar)].low;
            requireNear(top, highest, std::string(testCase.code) + " stroke covers merged highs");
            requireNear(bottom, lowest, std::string(testCase.code) + " stroke covers merged lows");
        }
        for (size_t index = 1; index < analysis.strokes.size(); ++index)
        {
            require(analysis.strokes[index - 1].endIndex == analysis.strokes[index].startIndex,
                    std::string(testCase.code) + " adjacent strokes share endpoints");
            require(analysis.strokes[index - 1].direction != analysis.strokes[index].direction,
                    std::string(testCase.code) + " strokes alternate direction");
        }
    }
}

void testRecentHistoryDoesNotEraseValidReversals()
{
    const struct Case
    {
        const char *code;
        const char *cutoff;
    } cases[]{
        {"000883", "2025-06-01"},
        {"000885", "2025-06-01"},
        {"000975", "2025-03-01"},
        {"001220", "2026-03-01"},
        {"001316", "2025-10-01"},
    };
    for (const auto &testCase : cases)
    {
        std::ifstream input(std::string(CHANLUN_TEST_SOURCE_DIR) + "/tests/data/" +
                            testCase.code + "-full-daily.csv");
        require(input.good(), std::string(testCase.code) + " fixture is available");
        std::vector<std::string> dates;
        std::vector<float> high;
        std::vector<float> low;
        std::string line;
        while (std::getline(input, line))
        {
            std::istringstream row(line);
            std::string field;
            std::getline(row, field, ',');
            dates.push_back(field);
            std::getline(row, field, ',');
            high.push_back(std::stof(field));
            std::getline(row, field, ',');
            low.push_back(std::stof(field));
        }
        const auto strokes = chanlun::analyze(high, low, {}).strokes;
        size_t recent = 0;
        bool first = false;
        bool second = false;
        bool third = false;
        bool novemberRise = false;
        bool novemberFall = false;
        for (const auto &stroke : strokes)
        {
            recent += dates[stroke.startIndex] >= testCase.cutoff;
            if (std::string(testCase.code) == "001316")
            {
                novemberRise |= dates[stroke.startIndex] == "2025-10-20" &&
                                dates[stroke.endIndex] == "2025-11-13";
                novemberFall |= dates[stroke.startIndex] == "2025-11-13" &&
                                dates[stroke.endIndex] == "2025-11-21";
                first |= dates[stroke.endIndex] == "2025-12-01";
                second |= dates[stroke.startIndex] == "2025-12-01" &&
                          dates[stroke.endIndex] == "2025-12-17";
                third |= dates[stroke.startIndex] == "2025-12-17" &&
                         dates[stroke.endIndex] == "2026-02-05";
            }
        }
        require(recent >= 3, std::string(testCase.code) + " keeps multiple recent strokes");
        if (std::string(testCase.code) == "001316")
        {
            require(novemberRise && novemberFall && first && second && third,
                    "001316 keeps valid November and December reversals");
        }
        if (std::string(testCase.code) == "000975")
        {
            bool springTop = false;
            bool summerBottom = false;
            bool autumnTop = false;
            bool winterBottom = false;
            for (const auto &stroke : strokes)
            {
                springTop |= dates[stroke.endIndex] == "2025-04-17";
                summerBottom |= dates[stroke.endIndex] == "2025-07-31";
                autumnTop |= dates[stroke.endIndex] == "2025-10-14";
                winterBottom |= dates[stroke.endIndex] == "2025-11-18";
            }
            require(springTop && summerBottom && autumnTop && winterBottom,
                    "000975 keeps independently valid 2025 reversals");
        }
    }
}

void testHistoryWindowRetainsLaterStrokes()
{
    for (const char *code : {"000883", "000885", "000975", "001316"})
    {
        std::ifstream input(std::string(CHANLUN_TEST_SOURCE_DIR) + "/tests/data/" +
                            code + "-full-daily.csv");
        require(input.good(), std::string(code) + " full fixture is available");
        std::vector<std::string> dates;
        std::vector<float> high;
        std::vector<float> low;
        std::string line;
        while (std::getline(input, line))
        {
            std::istringstream row(line);
            std::string field;
            std::getline(row, field, ',');
            dates.push_back(field);
            std::getline(row, field, ',');
            high.push_back(std::stof(field));
            std::getline(row, field, ',');
            low.push_back(std::stof(field));
        }
        const auto first = std::lower_bound(dates.begin(), dates.end(), std::string("2025-01-01"));
        const size_t offset = static_cast<size_t>(first - dates.begin());
        const auto full = chanlun::analyze(high, low, {}).strokes;
        const auto shortWindow = chanlun::analyze(
            {high.begin() + static_cast<std::ptrdiff_t>(offset), high.end()},
            {low.begin() + static_cast<std::ptrdiff_t>(offset), low.end()}, {}).strokes;
        size_t common = 0;
        for (const auto &stroke : shortWindow)
        {
            const std::string &start = dates[offset + static_cast<size_t>(stroke.startIndex)];
            const std::string &end = dates[offset + static_cast<size_t>(stroke.endIndex)];
            if (start < "2026-01-01") continue;
            for (const auto &historical : full)
            {
                common += dates[static_cast<size_t>(historical.startIndex)] == start &&
                          dates[static_cast<size_t>(historical.endIndex)] == end;
            }
        }
        require(common >= 2, std::string(code) + " keeps later strokes across load windows");
    }
}

void testConfirmedDecemberPrefixSurvivesLaterData()
{
    std::ifstream input(std::string(CHANLUN_TEST_SOURCE_DIR) + "/tests/data/001316-full-daily.csv");
    require(input.good(), "001316 prefix fixture is available");
    std::vector<std::string> dates;
    std::vector<float> high;
    std::vector<float> low;
    std::string line;
    while (std::getline(input, line))
    {
        std::istringstream row(line);
        std::string field;
        std::getline(row, field, ',');
        dates.push_back(field);
        std::getline(row, field, ',');
        high.push_back(std::stof(field));
        std::getline(row, field, ',');
        low.push_back(std::stof(field));
    }
    const auto cutoff = std::lower_bound(dates.begin(), dates.end(), std::string("2026-02-20"));
    const size_t count = static_cast<size_t>(cutoff - dates.begin());
    const auto prefix = chanlun::analyze({high.begin(), high.begin() + static_cast<std::ptrdiff_t>(count)},
                                         {low.begin(), low.begin() + static_cast<std::ptrdiff_t>(count)}, {}).strokes;
    const auto full = chanlun::analyze(high, low, {}).strokes;
    for (const auto &stroke : prefix)
    {
        if (dates[stroke.endIndex] < "2025-11-01" || dates[stroke.endIndex] > "2025-12-17")
        {
            continue;
        }
        bool retained = false;
        for (const auto &later : full)
        {
            retained |= later.startIndex == stroke.startIndex && later.endIndex == stroke.endIndex;
        }
        require(retained, "001316 confirmed November and December prefix remains stable");
    }
}


void testUpStrokeCandidateExtendsToNewHigh()
{
    std::vector<chanlun::MergedBar> bars;
    for (int index = 0; index <= 12; ++index)
    {
        bars.push_back({80, 78, chanlun::Direction::None, index, index, index});
    }
    bars[0].low = 72;
    bars[4].high = 89;
    bars[8].low = 74;
    bars[12].high = 93;
    const std::vector<chanlun::Fractal> fractals{
        {chanlun::FractalType::Bottom, 0, 0, 72},
        {chanlun::FractalType::Top, 4, 4, 89},
        {chanlun::FractalType::Bottom, 8, 8, 74},
        {chanlun::FractalType::Top, 12, 12, 93},
    };
    const auto strokes = chanlun::buildStrokes(bars, fractals);
    require(strokes.size() == 3, "valid alternating fractals remain distinct up/down/up strokes");
    require(strokes[0].startIndex == 0 && strokes[0].endIndex == 4,
            "first up stroke remains after a later higher high");
}

void testStrokeEndpointMustCoverEveryMergedBar()
{
    std::vector<chanlun::MergedBar> bars;
    for (int index = 0; index <= 10; ++index)
    {
        bars.push_back({15.0F, 8.0F, chanlun::Direction::None, index, index, index});
    }
    bars[0].high = 20.0F;
    bars[4].low = 5.0F;
    bars[7].low = 4.0F;
    bars[10].low = 6.0F;
    const std::vector<chanlun::Fractal> downFractals{
        {chanlun::FractalType::Top, 0, 0, 20.0F},
        {chanlun::FractalType::Bottom, 4, 4, 5.0F},
        {chanlun::FractalType::Bottom, 10, 10, 6.0F},
    };
    const auto downStrokes = chanlun::buildStrokes(bars, downFractals);
    for (const auto &stroke : downStrokes)
    {
        requireNear(stroke.low, bars[stroke.endIndex].low,
                    "no down stroke may end above a lower bar inside its range");
    }

    for (auto &bar : bars)
    {
        bar.low = 8.0F;
    }
    bars[0].high = 15.0F;
    bars[0].low = 2.0F;
    bars[4].high = 18.0F;
    bars[7].high = 22.0F;
    bars[10].high = 19.0F;
    const std::vector<chanlun::Fractal> upFractals{
        {chanlun::FractalType::Bottom, 0, 0, 2.0F},
        {chanlun::FractalType::Top, 4, 4, 18.0F},
        {chanlun::FractalType::Top, 10, 10, 19.0F},
    };
    const auto upStrokes = chanlun::buildStrokes(bars, upFractals);
    for (const auto &stroke : upStrokes)
    {
        requireNear(stroke.high, bars[stroke.endIndex].high,
                    "no up stroke may end below a higher bar inside its range");
    }
}

void testInvalidCandidateDoesNotBreakStrokeChain()
{
    std::vector<chanlun::MergedBar> bars;
    for (int index = 0; index <= 15; ++index)
    {
        bars.push_back({15.0F, 8.0F, chanlun::Direction::None, index, index, index});
    }
    bars[0].low = 5.0F;
    bars[4].high = 20.0F;
    bars[7].high = 22.0F;
    bars[8].low = 6.0F;
    bars[11].high = 23.0F;
    bars[15].low = 4.0F;
    const std::vector<chanlun::Fractal> fractals{
        {chanlun::FractalType::Bottom, 0, 0, 5.0F},
        {chanlun::FractalType::Top, 4, 4, 20.0F},
        {chanlun::FractalType::Bottom, 8, 8, 6.0F},
        {chanlun::FractalType::Top, 11, 11, 23.0F},
        {chanlun::FractalType::Bottom, 15, 15, 4.0F},
    };
    const auto strokes = chanlun::buildStrokes(bars, fractals);
    require(strokes.size() >= 2, "invalid intermediate endpoint does not break all subsequent strokes");
    require(strokes.front().startIndex == 0 && strokes.front().endIndex == 11,
            "invalid top is replaced by later valid extreme");
    require(strokes.back().startIndex == 11 && strokes.back().endIndex == 15,
            "later down stroke connects to the replacement top");
    for (size_t index = 1; index < strokes.size(); ++index)
    {
        require(strokes[index - 1].endIndex == strokes[index].startIndex,
                "consecutive strokes share a confirmed endpoint");
    }
}

void testContainmentMarkersProjectMergedGroupBoundaries()
{
    const std::vector<float> high{70.0F, 80.0F, 110.0F, 130.0F, 120.0F};
    const std::vector<float> low{50.0F, 60.0F, 90.0F, 80.0F, 100.0F};
    const std::vector<float> close{60.0F, 70.0F, 100.0F, 105.0F, 110.0F};

    const auto highLine = chanlun::evaluateTdxFunction(102, high, low, close);
    const auto lowLine = chanlun::evaluateTdxFunction(103, high, low, close);
    const auto startMarker = chanlun::evaluateTdxFunction(104, high, low, close);
    const auto endMarker = chanlun::evaluateTdxFunction(105, high, low, close);

    requireNear(highLine[2], 130.0F, "containment high line covers group start");
    requireNear(highLine[3], 130.0F, "containment high line covers group middle");
    requireNear(highLine[4], 130.0F, "containment high line covers group end");
    requireNear(lowLine[2], 100.0F, "containment low line covers group start");
    requireNear(lowLine[3], 100.0F, "containment low line covers group middle");
    requireNear(lowLine[4], 100.0F, "containment low line covers group end");
    requireNear(startMarker[2], 1.0F, "containment group start marker");
    requireNear(endMarker[4], 1.0F, "containment group end marker");
    for (int index = 2; index < 5; ++index)
    {
        require(highLine[index] > 0.0F && lowLine[index] > 0.0F,
                "each original candle in a containment group carries both horizontal levels");
    }
}

void testTdxAdapterReturnsZeroSeriesForUnknownFunction()
{
    const std::vector<float> high{10.0F, 11.0F, 12.0F};
    const std::vector<float> low{8.0F, 9.0F, 10.0F};
    const std::vector<float> close{9.0F, 10.0F, 11.0F};

    const auto series = chanlun::evaluateTdxFunction(9999, high, low, close);

    require(series.size() == high.size(), "unknown function keeps output length");
    requireNear(series[0], 0.0F, "unknown function emits zero");
    requireNear(series[1], 0.0F, "unknown function emits zero");
    requireNear(series[2], 0.0F, "unknown function emits zero");
}
}

int main(int argc, char **argv)
{
    if (argc == 2)
    {
        std::ifstream input(argv[1]);
        std::vector<std::string> dates;
        std::vector<float> high;
        std::vector<float> low;
        std::vector<float> close;
        std::string line;
        while (std::getline(input, line))
        {
            std::istringstream row(line);
            std::string field;
            std::getline(row, field, ',');
            dates.push_back(field);
            std::getline(row, field, ',');
            high.push_back(std::stof(field));
            std::getline(row, field, ',');
            low.push_back(std::stof(field));
            std::getline(row, field, ',');
            close.push_back(std::stof(field));
        }
        const auto analysis = chanlun::analyze(high, low, close);
        for (size_t index = 0; index < analysis.strokes.size(); ++index)
        {
            const auto &stroke = analysis.strokes[index];
            const int first = std::min(stroke.startIndex, stroke.endIndex);
            const int last = std::max(stroke.startIndex, stroke.endIndex);
            int startBar = -1;
            int endBar = -1;
            for (const auto &fractal : analysis.fractals)
            {
                if (fractal.originalIndex == stroke.startIndex) startBar = fractal.barIndex;
                if (fractal.originalIndex == stroke.endIndex) endBar = fractal.barIndex;
            }
            require(startBar >= 0 && endBar >= startBar, "stroke endpoints map to merged fractals");
            float highest = -std::numeric_limits<float>::infinity();
            float lowest = std::numeric_limits<float>::infinity();
            for (int barIndex = startBar; barIndex <= endBar; ++barIndex)
            {
                highest = std::max(highest, analysis.mergedBars[barIndex].high);
                lowest = std::min(lowest, analysis.mergedBars[barIndex].low);
            }
            const float top = stroke.direction == chanlun::Direction::Up ? analysis.mergedBars[endBar].high : analysis.mergedBars[startBar].high;
            const float bottom = stroke.direction == chanlun::Direction::Up ? analysis.mergedBars[startBar].low : analysis.mergedBars[endBar].low;
            requireNear(top, highest, "stroke top must cover merged highs " + dates[first] + " -> " + dates[last]);
            requireNear(bottom, lowest, "stroke bottom must cover merged lows " + dates[first] + " -> " + dates[last]);
            if (index > 0)
            {
                require(analysis.strokes[index - 1].endIndex == stroke.startIndex,
                        "adjacent strokes must share an endpoint");
            }
        }
        std::cout << "bars " << high.size() << " merged " << analysis.mergedBars.size()
                  << " fractals " << analysis.fractals.size() << " strokes " << analysis.strokes.size()
                  << " segments " << analysis.segments.size() << " pivots " << analysis.pivots.size() << "\n";
        for (const auto &fractal : analysis.fractals)
        {
            std::cout << "F " << dates[fractal.originalIndex] << ' '
                      << (fractal.type == chanlun::FractalType::Top ? 'T' : 'B') << ' '
                      << fractal.price << " merged#" << fractal.barIndex << "\n";
        }
        for (const auto &stroke : analysis.strokes)
        {
            std::cout << dates[stroke.startIndex] << ' ' << (stroke.direction == chanlun::Direction::Up ? low[stroke.startIndex] : high[stroke.startIndex])
                      << " -> " << dates[stroke.endIndex] << ' ' << (stroke.direction == chanlun::Direction::Up ? high[stroke.endIndex] : low[stroke.endIndex]) << "\n";
        }
        const auto strokePivots = chanlun::buildStrokePivots(analysis.strokes);
        std::cout << "stroke pivots " << strokePivots.size() << "\n";
        for (const auto &pivot : strokePivots)
        {
            std::cout << "P " << dates[pivot.startIndex] << " -> " << dates[pivot.endIndex]
                      << " ZD=" << pivot.zd << " ZG=" << pivot.zg << "\n";
        }
        return 0;
    }
    testSequentialContainmentUsesTrendDirection();
    testPendingContainmentWaitsForDirection();
    testInitialContainmentDoesNotUseBoundingEnvelope();
    testContainmentLevelsUseMergedPrices();
    testFractalsOnlyUseMergedBars();
    testStrokesRequireAlternatingIndependentFractals();
    testFractalEndpointUsesExtremeOriginalCandle();
    testStrokeRequiresBarOutsideBothFractals();
    testNearbyOppositeFractalDoesNotReplaceConfirmedEndpoint();
    testThreeOverlappingStrokesFormStrokeLevelPivot();
    testStrokePivotProjectionUsesSeparateFunctionIds();
    testStrokePivotExtendsInsteadOfRepeatingSlidingWindows();
    testAdjacentPivotBoundariesKeepBothPriceRanges();
    testStrokeCandidateRemovesUnconfirmedCounterMove();
    testDownStrokeExtendsPastUnconfirmedRebound();
    testInvalidReversalRollsBackEarlierStrokes();
    testConfirmedPriorStrokeSurvivesLaterRollback();
    testConfirmedStrokeContinuesAfterInvalidTail();
    testInvalidTurnRevalidatesEarlierStrokes();
    testConsecutiveSameTypeFractalsChooseExtreme();
    testStrokeAggregatesDoNotCrossGap();
    testUpStrokeCandidateExtendsToNewHigh();
    testStrokeEndpointMustCoverEveryMergedBar();
    testInvalidCandidateDoesNotBreakStrokeChain();
    testSegmentsRequireThreeOverlappingStrokes();
    testPivotsUseThreeOverlappingSegments();
    testMacdSupportsDefaultSignalLayer();
    testTdxAdapterProjectsFractalSeries();
    testContainmentMarkersProjectMergedGroupBoundaries();
    testTdxAdapterReturnsZeroSeriesForUnknownFunction();
    test300652HistoricalChainRecovers();
    test000801HistoricalTailRevalidates();
    test300383PreservesValidPrefix();
    testHistoricalStrokeChainsContinue();
    testRecentHistoryDoesNotEraseValidReversals();
    testHistoryWindowRetainsLaterStrokes();
    testConfirmedDecemberPrefixSurvivesLaterData();
    std::cout << "ChanlunCoreTests passed\n";
    return 0;
}
