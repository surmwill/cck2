#pragma once

#include <string>
#include <fstream>
#include <vector>

class Fstream final {
public:
	Fstream(const std::string & txtFile);
	~Fstream();

	int firstLineLength(void);
	int numLines(void);
	std::vector <std::vector <char>> readTxTBlock(
		const int fromChar,
		const int charsToRead,
		const int startLine,
		const int linesToRead);

	std::vector <std::vector <char>> readRectContent(void);

private:
	std::fstream ifstream; //An ifstream to add functionality to
	std::string txtFile; //The current txt file we are working with
};

