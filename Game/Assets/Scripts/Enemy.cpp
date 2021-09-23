#include "Enemy.h"

void Enemy::SetFallingSpeed(int fallingSpeed_) {
	fallingSpeed = fallingSpeed_;
}

void Enemy::SetSearchRadius(float searchRadius_) {
	searchRadius = searchRadius_;
}

void Enemy::SetAttackRange(float attackRange_) {
	attackRange = attackRange_;
}

void Enemy::SetTimeToDie(float timeToDie_) {
	timeToDie = timeToDie_;
}
