#include "FloorIsLava.h"

#include "GameObject.h"
#include "GameplaySystems.h"

#include <math.h>

EXPOSE_MEMBERS(FloorIsLava) {
    MEMBER(MemberType::FLOAT, timeTilesActive),
	MEMBER(MemberType::FLOAT, timeWarning),
	MEMBER(MemberType::STRING, container),
	MEMBER(MemberType::BOOL, sequential)

};

GENERATE_BODY_IMPL(FloorIsLava);

void FloorIsLava::Start() {
	
	tiles = GetOwner().GetChildren();		
	
	if (container == "corridor") {
		sequentialPatterns = corridorPatterns;
	}
	else if (container == "arena") {
		pattern1 = arenaPattern1.pattern;
		pattern2 = arenaPattern2.pattern;
		pattern3 = arenaPattern3.pattern;
	}
	else if (container == "boss") {
		pattern1 = bossPattern1.pattern;
		pattern2 = bossPattern2.pattern;
		pattern3 = bossPattern3.pattern;
	}
	
	timeRemainingWarning = timeWarning;	
}

void FloorIsLava::Update() {
	
	//select a random corridor and arena pattern
	if (patternFinished) {
		if (sequential) {
			currentTilesPattern = sequentialPatterns[sequentialCount].pattern;
			++sequentialCount;
			if (sequentialCount >= CORRIDOR_PATTERNS) {
				sequentialCount = 0;
			}		



			nextTilesPattern = sequentialPatterns[sequentialCount].pattern;



			
		}
		else {
			currentPattern = nextPattern;
			while (currentPattern == nextPattern) {
				nextPattern = 1 + (rand() % 3);
			}
			SetPattern(currentPattern, currentTilesPattern);
			SetPattern(nextPattern, nextTilesPattern);
		}
		patternFinished = false;
	}
	
	if (warningActive) {
		if (firstTimeWarning) {
			UpdateWarningTiles(true);
			firstTimeWarning = false;
		}

		if (timeRemainingWarning > 0.f) {
			timeRemainingWarning -= Time::GetDeltaTime();
		}
		else {
			UpdateWarningTiles(false);
			warningActive = false;
			fireActive = true;
			timeRemainingTilesActive = timeTilesActive;
			firstTimeFireActive = true;
		}
	}

	if (fireActive) {
		if (firstTimeFireActive) {
			UpdateFireActiveTiles(true);
			firstTimeFireActive = false;
		}

		if (timeRemainingTilesActive > 0.f) {
			timeRemainingTilesActive -= Time::GetDeltaTime();
		}
		else {
			UpdateFireActiveTiles(false);
			if (timerTilesClosingRemaining <= timerTilesClosing) {
				timerTilesClosingRemaining += Time::GetDeltaTime();
			}
			else {
				warningActive = true;
				fireActive = false;
				patternFinished = true;
				timeRemainingWarning = timeWarning;
				firstTimeWarning = true;
				timerTilesClosingRemaining = 0.f;
			}
		}

	}

}

void FloorIsLava::SetPattern(int pattern, bool*& boolPattern)
{
	switch (pattern)
	{
	case 1:
		boolPattern = pattern1;
		break;
	case 2:
		boolPattern = pattern2;
		break;
	case 3:
		boolPattern = pattern3;
		break;
	default:
		break;
	}
}

void FloorIsLava::UpdateWarningTiles(bool activate)
{
	for (int i = 0; i < tiles.size(); ++i) {
		if (currentTilesPattern[i]) {
			if (tiles[i]) {
				if (activate) {
					ComponentAnimation* animation = tiles[i]->GetComponent<ComponentAnimation>();
					if (animation) {
						animation->SendTrigger("ClosedOpening");
					}
					//tiles[i].activate warning particles
				}
				else {
					//tiles[i].deactivate warning particles
				}
			}			
		}
	}
}

void FloorIsLava::UpdateFireActiveTiles(bool activate)
{
	for (int i = 0; i < tiles.size(); ++i) {
		if (currentTilesPattern[i]) {
			if (tiles[i]) {
				ComponentBoxCollider* boxCollider = tiles[i]->GetComponent<ComponentBoxCollider>();
				GameObject* childFireParticlesObject = tiles[i]->GetChild("FireVerticalParticleSystem");
				ComponentAnimation* animation = tiles[i]->GetComponent<ComponentAnimation>();
				ComponentParticleSystem* fireParticles = nullptr;
				if (childFireParticlesObject) {
					fireParticles = childFireParticlesObject->GetComponent<ComponentParticleSystem>();					
				}
				if (boxCollider && fireParticles && animation) {
					if (activate) {
						boxCollider->Enable();
						fireParticles->PlayChildParticles();
					}
					else {
						if (!nextTilesPattern[i]) {
							boxCollider->Disable();
							fireParticles->StopChildParticles();
							animation->SendTrigger("OpenedClosing");
						}
					}
				}
			}
		}		
	}
}


