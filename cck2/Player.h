#pragma once
#include <memory>
#include <string>
#include "Subject.h"
#include "Coordinate.h"
#include "Combatent.h"
#include "Dialogue.h"

//add enter combat function

class Observer;
struct PlayerImpl;

class Player final : public Subject, public Combatent {
	std::unique_ptr <PlayerImpl> playerImpl;

public:
	Player(Observer * const map, const Coordinate playerStart, const Dialogue & userInterface);
	~Player();

	Coordinate & moveLeft(void) override;
	Coordinate & moveRight(void) override;
	Coordinate & moveUp(void) override;
	Coordinate & moveDown(void) override;
	Coordinate & position(void);

	void giveDialogue(const std::string & text, bool slowType = true) override;
	void takeTurn(void) override;
};

