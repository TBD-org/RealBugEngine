#include "Character.h"

void Character::GetHit(float damage_) {
	//We assume that the character is always alive when this method gets called, so no need to check if character was alive before taking lives
	lifePoints -= damage_;
	isAlive = lifePoints > 0.0f;
	if (!isAlive) {
		OnDeath();
	}
}

void Character::OnDeath() {

}

void Character::Recover(int recoveryLife_) {
	int difference = totalLifePoints - lifePoints;
	if (recoveryLife_ > difference) {
		lifePoints = totalLifePoints;
	} else {
		lifePoints += recoveryLife_;
	}
}

void Character::SetTotalLifePoints(int totalLifePoints_) {
	totalLifePoints = totalLifePoints_;
}

void Character::SetDamageHit(float damageHit_) {
	damageHit = damageHit_;
}

float Character::GetTotalLifePoints() const {
	return totalLifePoints;
}

bool Character::IsFullHealth() const {
	return lifePoints == totalLifePoints;
}
