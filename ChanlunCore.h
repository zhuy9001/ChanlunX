#ifndef __CHANLUNCORE_H__
#define __CHANLUNCORE_H__

#include <vector>

namespace chanlun
{
enum class Direction
{
    Down = -1,
    None = 0,
    Up = 1
};

enum class FractalType
{
    Bottom = -1,
    None = 0,
    Top = 1
};

enum class StructureStatus
{
    Invalid = -1,
    None = 0,
    Candidate = 1,
    Confirmed = 2,
    Extending = 3,
    Terminated = 4
};

enum class TrendKind
{
    Down = -1,
    None = 0,
    Up = 1,
    Balance = 2
};

enum class TrendCompletion
{
    NotFormed = 1,
    Forming = 2,
    CanComplete = 3,
    Completed = 4,
    Extending = 5
};

struct MergedBar
{
    float high;
    float low;
    Direction direction;
    int startIndex;
    int endIndex;
    int representativeIndex;
    int highIndex;
    int lowIndex;
};

struct Fractal
{
    FractalType type;
    int barIndex;
    int originalIndex;
    float price;
};

struct Stroke
{
    int startIndex;
    int endIndex;
    Direction direction;
    float high;
    float low;
    StructureStatus status;
};

struct Segment
{
    int startIndex;
    int endIndex;
    Direction direction;
    float high;
    float low;
    StructureStatus status;
};

struct Pivot
{
    int startIndex;
    int endIndex;
    int level;
    Direction direction;
    float zg;
    float zd;
    float gg;
    float dd;
    StructureStatus status;
    std::vector<int> strokeIndices;
    int confirmationIndex = -1;
    int terminationIndex = -1;
    int relatedPivotIndex = -1;
};

struct TrendSnapshot
{
    TrendKind kind;
    TrendCompletion completion;
    int startIndex;
    int endIndex;
};

struct AnalysisOptions
{
    int level = 0;
};

struct ChanlunAnalysis
{
    std::vector<MergedBar> mergedBars;
    std::vector<Fractal> fractals;
    std::vector<Stroke> strokes;
    std::vector<Segment> segments;
    std::vector<Pivot> pivots;
    TrendSnapshot trend;
};

int toInt(Direction direction);
int toInt(FractalType type);
int toInt(StructureStatus status);
int toInt(TrendKind kind);
int toInt(TrendCompletion completion);

std::vector<MergedBar> mergeBars(const std::vector<float> &high, const std::vector<float> &low);
std::vector<Fractal> findFractals(const std::vector<MergedBar> &bars);
std::vector<Stroke> buildStrokes(const std::vector<MergedBar> &bars, const std::vector<Fractal> &fractals);
std::vector<Segment> buildSegments(const std::vector<Stroke> &strokes);
std::vector<Pivot> buildPivots(const std::vector<Segment> &segments, int level = 0);
std::vector<Pivot> buildStrokePivots(const std::vector<Stroke> &strokes);
TrendSnapshot buildTrendSnapshot(const std::vector<Pivot> &pivots);
ChanlunAnalysis analyze(const std::vector<float> &high,
                        const std::vector<float> &low,
                        const std::vector<float> &close,
                        const AnalysisOptions &options = AnalysisOptions{});
}

#endif
