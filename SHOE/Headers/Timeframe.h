#pragma once
#include <optional>
#include <vector>
#include <memory>
#include <string>
#include <stdexcept>

/// <summary>
/// Represents a single, contiguous period of time
/// </summary>
class Interval {
public:
	float start;
	float end;

	/// <param name="start">Starting timestamp of the interval. Must be >= end.</param>
	/// <param name="end">Ending timestamp of the interval. Must be <= start.</param>
	Interval(float start, float end) {
		if (start > end)
			throw std::invalid_argument("Start cannot be less than end.");

		this->start = start;
		this->end = end;
	}

	bool operator < (const Interval& other) const
	{
		return (start < other.start);
	}

	bool operator > (const Interval& other) const
	{
		return (start > other.start);
	}
};

/// <summary>
/// Represents a set of intervals within a common timeline
/// </summary>
class Timeframe : public std::enable_shared_from_this<Timeframe>
{
private:
	//All scripts assume timeframes is sorted in increasing order
	std::vector<Interval> intervals;

	std::optional<int> GetContainingIntervalIndex(float time);
	void Condense();
public:
	Timeframe() {};
	Timeframe(std::vector<Interval> intervals);

	bool Contains(float time);
	std::optional<float> NextValidInterval(float time);
	std::optional<float> EndOfCurrentInterval(float time);
	int GetIntervalCount();
	std::shared_ptr<Timeframe> Add(Interval newInterval);
	std::shared_ptr<Timeframe> And(std::shared_ptr<Timeframe> other);
	std::shared_ptr<Timeframe> Or(std::shared_ptr<Timeframe> other);

	std::string Stringify();
};

