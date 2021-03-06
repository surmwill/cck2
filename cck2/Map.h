//***************************************************************************80
#pragma once

#include <string>
#include <memory>
#include "Observer.h"
#include <ostream>
#include <fstream>
#include <utility>
#include <map>

struct MapImpl;
struct Coordinate;
class DisplayedMap;

class Map : public Observer {
public:
	Map(std::unique_ptr <DisplayedMap> display, const int mapWidth, const int mapHeight);

	~Map();
	
	virtual bool addressTileChange(
		const Coordinate & tile,
		const char newDesign) override;

	void readLevel(const std::string & mapTxtFile);
	void placePlayer(const Coordinate playerStart);

private:
	void notifyVisibleArea(void);
	void updateVisibleArea(void);
	bool validMove(const Coordinate & newOrigin);
	bool addressMovementChange(const Coordinate & tile);
	void addEnemies(std::map <Coordinate, char>); // levelOne will call addEnemies which will draw enemies on the map according to their coordinate and design

	std::unique_ptr<MapImpl> mapImpl;
	void printVisibleArea(void) const;
	friend std::ostream & operator<<(std::ostream & out, const Map & map);
};

