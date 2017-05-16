#pragma once

/* Represents the coordinates of a single tile
contained in the map */
struct Coordinate final {
	Coordinate(const int initialX, const int initialY) :
		x{ initialX }, y{ initialY } {};

	int x;
	int y;
};
