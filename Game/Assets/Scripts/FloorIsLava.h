#pragma once

#include "Scripting/Script.h"

#include "TilesPatterns.h"

#include <vector>

class FloorIsLava : public Script
{
	GENERATE_BODY(FloorIsLava);

public:

	void Start() override;
	void Update() override;

public:
	std::string container = "";

	float timeTilesActive = 10.f;
	float timeWarning = 3.f;

	bool sequential = false;

private:
	std::vector<GameObject*> tiles;

	int nextPattern = 1;
	int currentPattern = 1;

	bool* currentTilesPattern = nullptr;
	bool* nextTilesPattern = nullptr;

	std::vector<TilesPattern> sequentialPatterns;

	bool* pattern1 = nullptr;
	bool* pattern2 = nullptr;
	bool* pattern3 = nullptr;

	float timeRemainingTilesActive = 0.f;
	float timeRemainingWarning = 0.f;

	float timerTilesClosing = 2.f;
	float timerTilesClosingRemaining = 0.f;

	bool warningActive = true;
	bool fireActive = false;
	bool patternFinished = true;

	bool firstTimeWarning = true;
	bool firstTimeFireActive = true;

	int sequentialCount = 0;

private:
	void SetPattern(int pattern, bool*& boolPattern);
	void UpdateWarningTiles(bool activate);
	void UpdateFireActiveTiles(bool activate);
};

