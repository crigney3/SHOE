#include "../Headers/Timeframe.h"
#include <algorithm>
#include <functional>

Timeframe::Timeframe(std::vector<Interval> intervals)
{
	this->intervals.insert(this->intervals.end(), intervals.begin(), intervals.end());
	Condense();
}

/// <summary>
/// Gets the index of the interval that contains the given timestamp
/// </summary>
/// <param name="time">Time since start of playback</param>
/// <returns>Index of the interval that contains the given timestamp should one exist, null if not</returns>
std::optional<int> Timeframe::GetContainingIntervalIndex(float time)
{
	if (intervals.size() == 0 || intervals[0].start > time || intervals[intervals.size() - 1].end < time) return std::nullopt;

	int index = 0;
	for (int i = 1; i < intervals.size(); i++) {
		if (intervals[i].start < time)
			index++;
	}

	if (intervals[index].end > time)
		return index;

	return std::nullopt;
}

/// <summary>
/// Merges any overlapping stored intervals
/// </summary>
void Timeframe::Condense()
{
	if (intervals.size() < 2) return;

	std::sort(intervals.begin(), intervals.end(), std::less<Interval>());

	for (int i = 0; i < intervals.size() - 1; i++) {
		//If end of current interval is after start of the next, or the starts are the same, merge
		if (intervals[i].end - intervals[i + 1].start > -2 * FLT_EPSILON || std::abs(intervals[i].start - intervals[i + 1].start) < 2 * FLT_EPSILON) {
			intervals[i].end = std::max(intervals[i].end, intervals[i + 1].end);
			intervals.erase(intervals.begin() + i + 1);
			i--;
		}
	}
}

/// <summary>
/// Returns whether the given time is within the bounds of a contained interval
/// </summary>
/// <param name="time">Time since start of playback</param>
/// <returns>True if there is an interval the given time is within</returns>
bool Timeframe::Contains(float time)
{
	return GetContainingIntervalIndex(time).has_value();
}

/// <summary>
/// Returns the start of the next upcoming valid interval relative to the given time. 
/// Ignores the interval the time is within, shoudl one exist.
/// </summary>
/// <param name="time">Time since start of playback</param>
/// <returns>The starting timestamp of the next interval if one exists, null if not</returns>
std::optional<float> Timeframe::NextValidInterval(float time)
{
	int index = intervals.size() - 1;

	//If there are no stored intervals or the given time is after the end of the last interval, skip execution
	if (index == -1 || intervals[index].start < time) return std::nullopt;

	for (int i = index - 1; i >= 0; i--) {
		if (intervals[i].end > time)
			index--;
	}

	return intervals[index].start;
}

/// <summary>
/// Finds the end of the valid interval the given time is during, should one exist
/// </summary>
/// <param name="time">Time since start of playback</param>
/// <returns>The end of the interval if time is within one, null if not</returns>
std::optional<float> Timeframe::EndOfCurrentInterval(float time)
{
	std::optional<int> index = GetContainingIntervalIndex(time);

	if (index.has_value())
		return intervals[index.value()].end;

	return std::nullopt;
}

/// <summary>
/// Returns the number of contained disparate time intervals
/// </summary>
int Timeframe::GetIntervalCount()
{
	return intervals.size();
}

/// <summary>
/// Adds a new interval to the contained set
/// </summary>
/// <param name="newInterval">Interval to add</param>
/// <returns>A pointer to this object for method chaining</returns>
std::shared_ptr<Timeframe> Timeframe::Add(Interval newInterval)
{
	intervals.push_back(Interval(newInterval.start, newInterval.end));

	//std::sort(intervals.begin(), intervals.end(), std::greater<Interval>());
	Condense();

	return shared_from_this();
}

/// <summary>
/// Finds all intervals that are overlapped by both timeframes.
/// Based on examples at https://www.geeksforgeeks.org/find-intersection-of-intervals-given-by-two-lists/
/// </summary>
/// <param name="other">The timeframe to compare with</param>
/// <returns>A timeframe with the new set of intervals</returns>
std::shared_ptr<Timeframe> Timeframe::And(std::shared_ptr<Timeframe> other)
{
	//If either timeframe has no intervals, there cannot be any overlap
	if (intervals.size() == 0 || other->GetIntervalCount() == 0) return std::make_shared<Timeframe>();

	// Indices used to keep track of traversal
	int i = 0, j = 0;

	std::vector<Interval> combinedSet = std::vector<Interval>();

	// Loop through all intervals unless
	// one of the interval gets exhausted
	while (i < intervals.size() && j < other->GetIntervalCount())
	{
		// Left bound for intersecting segment
		float l = std::max(intervals[i].start, other->intervals[j].start);

		// Right bound for intersecting segment
		float r = std::min(intervals[i].end, other->intervals[j].end);

		// If segment is valid add it
		if (l <= r)
			combinedSet.push_back(Interval(l, r));

		// If i-th interval's right bound is
		// smaller increment i else increment j
		if (intervals[i].end < other->intervals[j].end)
			i++;
		else
			j++;
	}

	return std::make_shared<Timeframe>(combinedSet);
}

/// <summary>
/// Combines the two sets of contained intervals into one
/// </summary>
/// <param name="other">The timeframe to compare with</param>
/// <returns>A new timeframe with the combined set of intervals</returns>
std::shared_ptr<Timeframe> Timeframe::Or(std::shared_ptr<Timeframe> other)
{
	//Some optimized edge cases 
	if (intervals.size() == 0 && other->GetIntervalCount() == 0) return std::make_shared<Timeframe>();
	if (intervals.size() == 0) return other;
	if (other->GetIntervalCount() == 0) return shared_from_this();

	std::vector<Interval> combinedSet = std::vector<Interval>();

	combinedSet.insert(combinedSet.end(), intervals.begin(), intervals.end());
	combinedSet.insert(combinedSet.end(), other->intervals.begin(), other->intervals.end());

	return std::make_shared<Timeframe>(combinedSet);
}

/// <summary>
/// Converts the contained intervals into a formatted string
/// </summary>
std::string Timeframe::Stringify()
{
	std::string asString;
	for (int i = 0; i < intervals.size(); i++) {
		if (i > 0) asString += ", ";
		asString += "[" + std::to_string(intervals[i].start) + ", " + std::to_string(intervals[i].end) + "]";
	}
	return asString;
}
