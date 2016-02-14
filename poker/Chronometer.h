#include <cstdint>
#include <Windows.h>

class Chronometer
{
public:
	typedef int64_t TCounter;

	Chronometer( bool start = false );

	// set start position
	void Start( const TCounter& startPosition = 0 );

	// returns the elapsed time since start (in seconds)
	double GetElapsedTime() const;
	// returns the elapsed time since start (in milliseconds)
	double GetElapsedTimeMs() const;

	// returns the elapsed time since custom position (in seconds)
	double GetElapsedTimeSince( const TCounter& startPosition ) const;
	// returns the elapsed time since custom position (in milliseconds)
	double GetElapsedTimeSinceMs( const TCounter& startPosition ) const;

protected:
	TCounter position;
	double positionToTimeFactor; // factor used to convert position into seconds
	double positionToTimeMsFactor; // factor used to convert position into milliseconds
};

inline Chronometer::Chronometer( bool start )
{
	// Get counter frequency (unchanged after system startup)
	TCounter frequency;
	QueryPerformanceFrequency((LARGE_INTEGER*)&frequency);

	// Compute factors for position to time transformations
	positionToTimeFactor = 1.0 / frequency;
	positionToTimeMsFactor = 1000.0 / frequency;

	if( start )
	{
		// Get current counter position
		QueryPerformanceCounter( (LARGE_INTEGER*)&position );
	}
	else
	{
		position = 0;
	}
}

inline void Chronometer::Start( const TCounter& startPosition )
{
	if( startPosition != 0 )
	{
		position = startPosition;
	}
	else
	{
		// Get current counter position
		QueryPerformanceCounter( (LARGE_INTEGER*)&position );
	}
}

inline double Chronometer::GetElapsedTime() const
{
	return GetElapsedTimeSince( position );
}

inline double Chronometer::GetElapsedTimeMs() const
{
	return GetElapsedTimeSinceMs( position );
}

inline double Chronometer::GetElapsedTimeSince( const TCounter& startPosition ) const
{
	TCounter now;
	QueryPerformanceCounter( (LARGE_INTEGER*)&now );

	TCounter delta = now - startPosition;
	return delta * positionToTimeFactor;
}

inline double Chronometer::GetElapsedTimeSinceMs( const TCounter& startPosition ) const
{
	TCounter now;
	QueryPerformanceCounter( (LARGE_INTEGER*)&now );

	TCounter delta = now - startPosition;
	return delta * positionToTimeMsFactor;
}
