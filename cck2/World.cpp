#include "World.h"
#include "LevelOne.h"
#include <Windows.h>
#include "WorldImpl.h"
#include "Display.h"
#include "Enemy.h"
#include "Combatent.h"
#include "Combat.h"
#include "Coordinate.h"
#include "DisplayedMap.h"
#include "CmdInterpreter.h"
#include "DisplayCommands.h"
#include "Dialogue.h"
#include <memory>

//delete these
#include "Coordinate.h"
#include "Map.h"
#include "Iostream.h"
#include "Debug.h"

using std::make_unique;
using std::move;
using std::vector;

class Observer;

/* Main constructor */
World::World(CmdInterpreter * const cmd) : worldImpl(make_unique <WorldImpl> ()) {
	// Restricts the display's access to the cmdInterpreter's public functions 
	auto displayCmd = make_unique <DisplayCommands>(cmd); 

	// Create the world
	worldImpl->display = make_unique <Display>(move(displayCmd));

	/* Restricts the map's access to the display's public functions. Similar to 
	passing the display but with less access to public functions */
	auto displayedMap = make_unique <DisplayedMap>(worldImpl->display.get());

	/* Create a new map object which has yet to load a level. The display is the map's observer. The amount
	of area we can see on the map corresponds to how much space we allocated in the display for drawing the map */
	Observer * map = new Map(move(displayedMap), worldImpl->display->mapWidth(), worldImpl->display->mapHeight());

	/* Every Enemy and Enemy subclass has acess to the (Observer) Map object
	in order to update the display with enemy movements */
	Enemy::map = map;

	/* Every Level has access to the map object. The Level owns the map
	and will thus be responsible for it's destruction */
	Level::map.reset(dynamic_cast <Map *> (map));

	/* Start at level one */
	worldImpl->level = make_unique <LevelOne>();

	Dialogue userInterface{ worldImpl->display.get() };

	//also pass the user interface to the enemies to give dialogue

	// Spawn the player and pass the map as one of the player's observers
	// Note static_cast up, and dynamic_cast down
	worldImpl->player = make_unique <Player>(map, worldImpl->level->getPlayerStart(), userInterface);
}


World::~World() {}

World & World::movePlayer(const int direction) {
	//cant use references because of multiple intialization of the same name in the switch statement
	Coordinate * playerPosition;

	switch (direction) {
	case VK_LEFT:
		playerPosition = &worldImpl->player->moveLeft();
		break;
	case VK_RIGHT:
		playerPosition = &worldImpl->player->moveRight();
		break;
	case VK_UP:
		playerPosition = &worldImpl->player->moveUp();
		break;
	case VK_DOWN:
		playerPosition = &worldImpl->player->moveDown();
		break;
	}

	combatPhase();

	return *this;
}

void World::animateWorld(void) {
	// make enemies continually patrol the map
	worldImpl->level->moveEnemies();

	// After moving, see if any of them are in aggro range
	combatPhase();
}

void World::combatPhase(void) {
	// The number of enemies who want to fight
	vector <Combatent *> enemies = worldImpl->level->enemiesAggrod(worldImpl->player->position());

	// If there is more than one enemy wanting to fight, start the battle
	if (!enemies.empty()) {
		Combat battle{ static_cast <Combatent *> (worldImpl->player.get()), enemies };

		// stop keeping track of the enemies and followers who died in battle
		combatAftermath(battle);
	}
}

//stop keeping track of enemies and followers that are dead
void World::combatAftermath(Combat & battle) {
	worldImpl->level->enemyCleanup(battle.reportDeadEnemies());
	//player->followerCleanup(battle.reportDeadFriendlies()); soon....
}
