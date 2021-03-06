#include "Display.h"
#include "Iostream.h"
#include "DisplayImpl.h"
#include "Coordinate.h"
#include "DisplayCommands.h"
#include <utility>
#include <cwchar>
#include <string>
#include <cmath>
#include <array>

//#define NOMINMAX
//#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

//this is one line of code added

using std::make_unique;
using std::vector;
using std::move;
using std::make_pair;
using std::wcscpy;
using std::string;
using std::array;

//to delete
#include "Iostream.h"
#include "Debug.h"
using std::to_string;
using std::unique_ptr;

/* Intializes the console screen by setting its dimensions and 
hiding the flashing cursor */
Display::Display(unique_ptr <DisplayCommands> cmd): displayImpl(make_unique <DisplayImpl> (std::move(cmd))) {
	adjustTextProperties();
	setConsoleProperties();
	setConsoleDimensions();
	disableConsoleCursor();
	drawUI();
	clearDialogue();

	formatOptions({
		{"aaaaaaaaaa", "b", "cccc"},
		{"d", "aaaaaaaaaaa", "fffffffff"},
		{"gfewfaEda", "dewdwe", "dewdew", "dwedwedwewef" } });

	/* drawDialogue("sabrina", "hey");
	drawDialogue("will", "01234567890 1234567890 1234567890 1234567890 1234567890 12345678904 ad dsa gred frefre fwedew 01234567890 1234567890 12 frefre fwedew 01234567890 1234567890 12 frefre fwedew 01234567890 1234567890 12 frefre fwedew 01234567890 1234567890 12 frefre fwedew 01234567890 1234567890 12 1234567890 12");
	drawDialogue("will", "hi hi hi h ih ih ih ih hi hihi hi hiih ih hiih ih ih hi ihih ");
	drawDialogue("sabrina", "hey");
	drawDialogue("tom", "yo", true); */
}

Display::~Display() {}

/* Hides the flashing console cursor */
void Display::disableConsoleCursor(void) {
	CONSOLE_CURSOR_INFO cci;
	GetConsoleCursorInfo(displayImpl->hOut, &cci);
	cci.bVisible = FALSE;
	SetConsoleCursorInfo(displayImpl->hOut, &cci);
}

/* Sets the console's x and y position to the top left of the screen.
Also disables resizing the console, and going fullscreen */
void Display::setConsoleProperties(void) {
	HWND hConsole;
	//MENUITEMINFO mii; 
	const int updateTime = 10;
	const int dontCareDim = 0;

	/* Sets the title and allows time for the console to update */
	SetConsoleTitle(displayImpl->title);
	Sleep(updateTime);

	// Gets a handle to the console window
	hConsole = FindWindow(NULL, displayImpl->title);

	/* Sets the console to appear in the top left (0, 0) of the screen. The
	screen will be resized later so the dimensions don't matter */
	MoveWindow(hConsole, 0, 0, dontCareDim, dontCareDim, FALSE);

	// Gets a handle to the menu in order to adjust console options
	HMENU hMenu = GetSystemMenu(hConsole, FALSE);
	
	DeleteMenu(hMenu, 1, MF_BYPOSITION); // disable 'Move'
	DeleteMenu(hMenu, 1, MF_BYPOSITION); // disable 'Size'

	DeleteMenu(hMenu, 2, MF_BYPOSITION); // disable 'Maximize'
	//DeleteMenu(hMenu, 3, MF_BYPOSITION); // disable 'Close or X'
}

/* Adjusts the console screen size and buffer size according to displayImpl::consoleWidth
and displayImpl::consoleHeight. The buffer is adjusted to perfectly match the screen size,
eliminating the scroll bar */
void Display::setConsoleDimensions(void) {
	// Specifies the dimensions of the buffer
	COORD bufferSize{ displayImpl->consoleWidth , displayImpl->consoleHeight };

	/* Specifies the dimensions of the console screen in lines.
	The parameters are left, top, right, bottom respectivly. Note the
	area of the console is the same as that of that of the bufferSize. An
	equal sized console and buffer eliminates the scrollbar. */
	SMALL_RECT consoleBounds{
		0,
		0,
		displayImpl->consoleWidth - 1,
		displayImpl->consoleHeight - 1 };

	// Sets the buffer size and console size
	SetConsoleScreenBufferSize(displayImpl->hOut, bufferSize);
	SetConsoleWindowInfo(displayImpl->hOut, TRUE, &consoleBounds);
}

/* Adjusts the text size and font */
void Display::adjustTextProperties(void) {
	CONSOLE_FONT_INFOEX cfi;

	//size of the structure
	cfi.cbSize = sizeof(cfi);

	//index of the front of the console's font table
	cfi.nFont = 0;

	//set to 24pt font
	cfi.dwFontSize.X = 0; 
	cfi.dwFontSize.Y = 24;

	//we are using the default font
	cfi.FontFamily = FF_DONTCARE;

	//no bold
	cfi.FontWeight = FW_NORMAL;

	/* setting the font type to "Consolas". Any other font screws 
	with the printing */
	wcscpy_s(cfi.FaceName, L"Consolas");

	//applying the font changes
	SetCurrentConsoleFontEx(displayImpl->hOut, FALSE, &cfi);
}

/* shorthand for FillConsoleOutput since displayImpl->hOut
and &charsWritten are always the same. Also updates the cursor to
the next open position in the buffer */
void Display::writeConsole(const WCHAR toWrite, const DWORD length) {
	DWORD charsWritten;

	FillConsoleOutputCharacter(
		displayImpl->hOut,
		toWrite,
		length,
		displayImpl->cursor,
		&charsWritten
	);

	updateCursorPos(length);
}

/* Draws a string in the console, starting wherever the cursor
is located */
void Display::writeStringToConsole(const string strToWrite, bool slowType) {
	char toWrite = displayImpl->tokenTile;
	int numWrites = 0;

	/* the amount of time we wait (ms) between printing batches of characters. Since we don't
	expect many repeated adjacent characters in dialogue, it should display smoothly */
	const DWORD slowTypeSpeed = 10;

	/* The same algorithim as redrawScreen. Store as many repeats of the
	same character in the buffer as possible and write them all at once when
	a different character is encountered */
	for (auto c : strToWrite) {
		if (toWrite == displayImpl->tokenTile) toWrite = c;

		if (c == toWrite) numWrites++;
		else {
			writeConsole(toWrite, numWrites);

			// pause for a brief moment between outputs for a "typewriter effect"
			if (slowType) Sleep(slowTypeSpeed); 

			toWrite = c;
			numWrites = 1;
		}
	}

	/* Again, the buffer is once step behind what is actually drawn of the screen.
	Print whatever's left in the buffer */
	writeConsole(toWrite, numWrites);
}

/* updates the cursor position based on how many characters we
have just written to the screen */
void Display::updateCursorPos(const int numWrites) {
	const int rowsToAdd = (displayImpl->cursor.X + numWrites) / displayImpl->consoleWidth;

	displayImpl->cursor.Y += rowsToAdd;
	displayImpl->cursor.X = 
		(displayImpl->cursor.X + numWrites) - (rowsToAdd * displayImpl->consoleWidth);
}

/* Updates the portion of the map we are displaying */ //maybe rename to refresh screen
void Display::drawMap(vector <vector <char>> & newTiles) {
	DWORD numWrites = 0;

	//We begin redrawing the screen from the top left
	setNextDrawPosition(0, 0);

	//inital value that will NEVER appear on a map. Only ' ' will be used for whitespace
	WCHAR toWrite = '\t';

	/* lambda to set numWrites and toWrite values. Main purpose is to help clean the code */
	auto adjustWriteProperties = [&numWrites, &toWrite](const int newNum, const char newChar) {
		numWrites = newNum;
		toWrite = newChar;
	};

	/* this will be updated as newTiles get written to the screen, prevDisplay
	will eventually be set equal to this */
	vector <vector <char>> prevDraw(
		displayImpl->mapHeight, 
		vector <char>(displayImpl->consoleWidth, ' '));

	/* Go through the previous visible area and redraw only the
	characters that differ */
	for (int i = 0; i < displayImpl->mapHeight; i++) {
		/* Special case: If the visible area is smaller than the entire screen (height wise) which 
		newTiles covers, populate the rows of the map we cannot see with spaces */
		if (i >= newTiles.size()) {
			const int rows = static_cast <int> (displayImpl->prevDisplay.size() - newTiles.size());
			const int numSpaces = rows * displayImpl->consoleWidth;

			/* If spaces are already in the buffer simply add on to the number of
			spaces we print */
			if (toWrite == ' ') numWrites += numSpaces;
			else { 
				//otherwise print what's in the buffer
				writeConsole(toWrite, numWrites);

				// and add the correct number of spaces to printed to the buffer
				adjustWriteProperties(numSpaces, ' ');
			}

			//print the spaces
			writeConsole(toWrite, numWrites);

			// resets toWrite and numWrites to their default values
			adjustWriteProperties(0, displayImpl->tokenTile);

			//Since these are the final rows in the console, there is nothing left to print after
			break; 
		}

		for (int j = 0; j < displayImpl->mapWidth; j++) {
			/* Special case: If the visible area is smaller than the entire screen (width wise), 
			populate the columns at the end of the row with spaces*/
			if (j >= newTiles[0].size()) {
				int numSpaces = static_cast <int> (displayImpl->prevDisplay[0].size() - newTiles[0].size());

				if (toWrite == ' ') numWrites += numSpaces; 
				else { 
					writeConsole(toWrite, numWrites);
					adjustWriteProperties(numSpaces, ' ');
				}

				writeConsole(toWrite, numWrites);
				adjustWriteProperties(0, displayImpl->tokenTile);

				//continue on to printing the next row
				break;
			}

			/* as we print the newTiles we also store them as what was previously drawn 
			on the screen, for the next set of newTiles. This was not needed for the cases 
			before b/c prevDraw is already initialized to spaces. */
			prevDraw[i][j] = newTiles[i][j];

			//Main case: For every newTile that differs from its previous draw
			if (newTiles[i][j] != displayImpl->prevDisplay[i][j]) {

				// initializes toWrite to the first character that needs to be redrawn
				if (toWrite == displayImpl->tokenTile) toWrite = newTiles[i][j];

				// If we have multiple of the same character, draw the batch all at once
				if (newTiles[i][j] == toWrite) numWrites++; //note that numWrites starts at 0
				else {
					/* Draws the updated tile(s) and adjusts the cursor to the next drawing poistion */
					writeConsole(toWrite, numWrites);

					/* Sets the buffer to the next tile design to be printed. Since we already 
					encountered 1 of the tile, it follos that we print at least 1 */
					adjustWriteProperties(1, newTiles[i][j]);
				} 
			}
			else { //Main case: for every newTile that is the same as its previous draw:
				/* If there are tile(s) to be printed in the buffer. Print them first
				as we will be updating the cursor to jump over the un-changed tile */
				if (numWrites != 0) {
					writeConsole(toWrite, numWrites);
					adjustWriteProperties(0, displayImpl->tokenTile);
				}

				/* moves the cursor one forward, skipping the tile. If the tile
				is the same there as the last draw, there is no need to redraw it */
				updateCursorPos(1);
			} 
		} 

	} 

	/* The screen's output is always one step behind the buffer. Thus, one final
	write is needed to print the final char(s) left in the buffer */
	writeConsole(toWrite, numWrites);

	// Our previous display becomes what was just drawn
	displayImpl->prevDisplay = move(prevDraw);
}

/* Draws our U.I. The U.I. is not redrawn unless a menu screen is pulled up */
void Display::drawUI(void) {
	/* start drawing the line after the bottom of the map. The map is in charge
	of updating it's own section of the display */
	setNextDrawPosition(displayImpl->uiStarts);

	writeConsole(' ', displayImpl->consoleWidth); //draw a line of blank spaces for clarity
	writeConsole('~', displayImpl->consoleWidth); //draw a line of '~' to section off the menu

	//3 spaces on either side to center the u.i
	const string uiLine = "   (I)nventory | (S)elf | (M)ap | (T)alents | (C)raft | (R)eligion | (O)ptions   ";
	writeStringToConsole(uiLine);

	writeConsole('~', displayImpl->consoleWidth); //draw a line of blank spaces for clarity
}

/* Clears the dialogue section with spaces and sets the
next draw position to the first line of the dialogue section */
void Display::clearDialogue(void) {
	// start at the beginning of where dialogue is drawn
	setNextDrawPosition(displayImpl->dialogueStarts);

	const int numRows = displayImpl->currentDialogueLine - displayImpl->dialogueStarts;
	const int numWrites = numRows * displayImpl->consoleWidth;

	//clear the entire dialogue section
	writeConsole(' ', numWrites);

	// our next draw position is at the beginning of where dialogue is drawn
	setNextDrawPosition(displayImpl->dialogueStarts);
	displayImpl->currentDialogueLine = displayImpl->dialogueStarts;
}

/* Clears previous dialogue, displays the new dialogue on the bottom the screen,
waits for a space press */
void Display::drawDialogue(
	const string & name, 
	const string & dialogue,
	const int line,
	const int indent,
	bool batchWrite,
	bool slowType) {
	// Set our next draw position relative to the start of the text box
	const int startLine = displayImpl->dialogueStarts + line;

	// Ensure we are writting within the text box
	if (startLine >= displayImpl->consoleHeight || indent > displayImpl->consoleWidth) {
		Debug::write("Text out of bounds");
		return;
	}

	displayImpl->currentDialogueLine = startLine;
	setNextDrawPosition(startLine, indent);

	// the text to be displayed
	string toWrite;

	// add the corresponding number of spaces to indent
	for (int i = 0; i < indent; i++) {
		toWrite += " ";
	}

	if(name != "") toWrite += name + ": " + dialogue; // ": " is Appended after a name to pretty print the dialogue
	else toWrite = dialogue; // Otherwise, there is no need to format anything

	int nextLineBreak = displayImpl->consoleWidth; // The character that needs to be a space in it's respected line
	unsigned int lineBreak = nextLineBreak; // The actual character we are testing to see if it's a space. unsigned b/c I don't like warnings

	// additional spaces may be padded so a word is not split between 2 lines
	int spacesNeeded = 0;

	while (toWrite.length() > lineBreak) {
		/* If the current dialogue toWrite (with formatted spaces) is longer 
		than what can be drawn on an entirely screen, print an error */
		if (toWrite.length() > displayImpl->maxDialogueLength) {
			Debug::write("TEXT ERROR: formatted dialogue length is: " + to_string(toWrite.length())
				+ " characters, the maximum length it  can be is: " + to_string(displayImpl->maxDialogueLength) 
				+ " characters.");
			return;
		}
	
		/* If a word is split between two lines, find the last character of the
		last full word that fits on the current line. -1 to adjust for the 0th index
		in a string */
		while (toWrite[lineBreak - 1] != ' ') {
			lineBreak--;
			spacesNeeded++;
		}

		// A word is split between lines, pad with spaces
		if (spacesNeeded != 0) {
			/* copy toWrite up to and including the (lineBreak - 1)th index 
			(the last character of the last word to be printed on the line)  */
			string dialogueOne = toWrite.substr(0, (lineBreak));

			// pad spaces to correctly split the dialogue
			for (int i = 0; i < spacesNeeded; i++) {
				dialogueOne += ' ';
			}

			/* dialogueOne copies characters: 0 to lineBreak - 1, dialogueTwo copies
			the rest of the characters starting from lineBreak */
			string dialogueTwo = toWrite.substr(lineBreak);
	
			toWrite = dialogueOne + dialogueTwo; // reconstruct the string
			spacesNeeded = 0;
		}

		// check if the next line needs to be split up
		nextLineBreak += displayImpl->consoleWidth;
		lineBreak = nextLineBreak;
	}
	
	// the 2 double casts are neeeded to avoid the truncating of the division before ceil()
	const int linesNeeded = static_cast <int> (ceil(
		static_cast <double> (toWrite.length()) / static_cast <double> (displayImpl->consoleWidth)));
	
	// adjust where the next empty line for dialgoue drawing is, so we know how many lines to clear in clearDialogue()
	displayImpl->currentDialogueLine += linesNeeded;

	// draw the dialogue
	writeStringToConsole(toWrite, slowType);

	// short dialogue gets drawn so quickly that one space press may skip multiple short lines of dialogue. Add a slight pause so this doesn't happen
	if(static_cast <int> (toWrite.length()) < displayImpl->consoleWidth) Sleep(100);

	/* What for the player to press space before printing further dialogue */
	if (!batchWrite) waitForSpacePressToClear();
}

/* Displays a list of options in the text box. Each option may not extend past
the line it's displayed on for clairty, i.e. we want an easy to read menu */
void Display::formatOptions(const vector <vector <string>> options) {
	clearDialogue();

	// we only have 6 lines to work with
	if (options.size() > displayImpl->dialogueLines) Debug::write("Too many menu lines, the maximum number of lines is " +
		to_string(displayImpl->dialogueLines));

	// lambda that returns a string with numSpaces spaces
	auto addSpaces = [](const int numSpaces) {
		string spaces;

		for (int i = 0; i < numSpaces; i++) {
			spaces += ' ';
		}

		return spaces;
	};
	
	vector <int> optionIndents; // indents for every column of options
	const int baseFormat = 4; //4 characters needed for base formatting, a bullet point like "1. " and a " " at the end of each menu option
	int totalOptions = 0; // keep track of how many options we have processed

	for (const auto & line : options) {
		int indent = 0; // how far right in a line the option will be drawn
		if (optionIndents.size() < line.size()) optionIndents.resize(line.size(), 0); // are we adding another column?

		for (int option = 0; option < line.size(); option++) {
			if (indent > optionIndents[option]) optionIndents[option] = indent; // choose the greatest indent for every column
			else indent = optionIndents[option];

			// calculate the indent need for the following option based on the length of our already processed options
			indent += static_cast <int> (line[option].size()) + baseFormat;

			totalOptions++;
		}

		// We have keys 0 - 9 there is no "10" key, thus a maximum of 10 options
		if (totalOptions > 10) {
			Debug::write("Cannot have more than 10 options (0 - 9)");
			return;
		}

		/* The indent needed for the next option corresponds to the total length of the line, we
		subtract 1 to account for removing the " " at the end of the righmost option, as this is 
		not part of the line to be drawn */
		int totalLineLength = indent - 1;

		if (totalLineLength > displayImpl->consoleWidth) {
			Debug::write("The menu's options are too long to display on the console. The longest line with formatting is " +
			to_string(totalLineLength) + " characters. The maxmimum number of characters in a line is " + to_string(displayImpl->consoleWidth));
			return;
		}
	}

	/* add formatting, to be put into another function 
	TODO work on vision around walls in the map and processing options */
	totalOptions = 0;
	vector <string> menu;
	menu.reserve(options.size());

	for (int line = 0; line < options.size(); line++) {
		string formattedLine;

		for (int option = 0; option < options[line].size(); option++) {
			const int numSpaces = optionIndents[option] - formattedLine.size();

			formattedLine += addSpaces(numSpaces) + to_string(totalOptions) + ". " + options[line][option];
			totalOptions++;
		}

		drawDialogue("", formattedLine, line, 0, true, true);
	}

	waitForSpacePressToClear();

}

/* Sets the cursor position which corresponds to the next place a character is drawn */
void Display::setNextDrawPosition(const int row, const int col) {
	displayImpl->cursor.Y = row;
	displayImpl->cursor.X = col;
}

int Display::mapHeight(void) {
	return displayImpl->mapHeight;
}

int Display::mapWidth(void) {
	return displayImpl->mapWidth;
}

void Display::waitForSpacePressToClear(void) {
	/* Wait for the player to press space */
	while (!displayImpl->cmd->keyPressed(VK_SPACE)) {}
	clearDialogue();
}