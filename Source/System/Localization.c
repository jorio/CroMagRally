// LOCALIZATION.C
// (C) 2022 Iliyas Jorio
// This file is part of Cro-Mag Rally. https://github.com/jorio/cromagrally

#include "game.h"
#include <string.h>
#include <SDL.h>

#define SEPARATOR ','
#define QUOTE_DELIMITER '"'
#define CSV_PATH ":system:strings.csv"

#define MAX_STRINGS (NUM_LOCALIZED_STRINGS + 1)

extern FSSpec gDataSpec;

static GameLanguageID	gCurrentStringsLanguage = LANGUAGE_ILLEGAL;
static Ptr				gStringsBuffer = nil;
static const char*		gStringsTable[MAX_STRINGS];

static const char kLanguageCodesISO639_1[NUM_LANGUAGES][3] =
{
	[LANGUAGE_ENGLISH	] = "en",
	[LANGUAGE_FRENCH	] = "fr",
	[LANGUAGE_GERMAN	] = "de",
	[LANGUAGE_SPANISH	] = "es",
	[LANGUAGE_ITALIAN	] = "it",
	[LANGUAGE_SWEDISH	] = "sv",
};

enum
{
	kCSVState_Stop,
	kCSVState_WithinQuotedString,
	kCSVState_WithinUnquotedString,
	kCSVState_AwaitingSeparator,
};

char* CSVNextColumn(char** csvCursor, bool* eolOut)
{
	GAME_ASSERT(csvCursor);
	GAME_ASSERT(*csvCursor);

	const char* reader = *csvCursor;
	char* writer = *csvCursor;		// we'll write over the column as we read it
	char* columnStart = writer;
	bool eol = false;

	if (reader[0] == '\0')
	{
		reader = NULL;			// signify the parser should stop
		columnStart = NULL;		// signify nothing more to read
		eol = true;
	}
	else
	{
		int state;

		if (*reader == QUOTE_DELIMITER)
		{
			state = kCSVState_WithinQuotedString;
			reader++;
		}
		else
		{
			state = kCSVState_WithinUnquotedString;
		}

		while (*reader && state != kCSVState_Stop)
		{
			if (reader[0] == '\r' && reader[1] == '\n')
			{
				// windows CRLF -- skip the \r; we'll look at the \n later
				reader++;
				continue;
			}

			switch (state)
			{
				case kCSVState_WithinQuotedString:
					if (*reader == QUOTE_DELIMITER)
					{
						state = kCSVState_AwaitingSeparator;
					}
					else
					{
						*writer = *reader;
						writer++;
					}
					break;

				case kCSVState_WithinUnquotedString:
					if (*reader == SEPARATOR)
					{
						state = kCSVState_Stop;
					}
					else if (*reader == '\n')
					{
						eol = true;
						state = kCSVState_Stop;
					}
					else
					{
						*writer = *reader;
						writer++;
					}
					break;

				case kCSVState_AwaitingSeparator:
					if (*reader == SEPARATOR)
					{
						state = kCSVState_Stop;
					}
					else if (*reader == '\n')
					{
						eol = true;
						state = kCSVState_Stop;
					}
					else
					{
						GAME_ASSERT_MESSAGE(false, "unexpected token in CSV file");
					}
					break;
			}

			reader++;
		}
	}

	*writer = '\0';	// terminate string

	if (reader != NULL)
	{
		GAME_ASSERT_MESSAGE(reader >= writer, "writer went past reader!!!");
	}

	*csvCursor = (char*) reader;
	*eolOut = eol;
	return columnStart;
}

void LoadLocalizedStrings(GameLanguageID languageID)
{
	// Don't bother reloading strings if we've already loaded this language
	if (languageID == gCurrentStringsLanguage)
	{
		return;
	}

	// Free previous strings buffer
	if (gStringsBuffer != nil)
	{
		SafeDisposePtr(gStringsBuffer);
		gStringsBuffer = nil;
	}

	GAME_ASSERT(languageID >= 0);
	GAME_ASSERT(languageID < NUM_LANGUAGES);

	long count = 0;
	gStringsBuffer = LoadTextFile(CSV_PATH, &count);

	for (int i = 0; i < MAX_STRINGS; i++)
		gStringsTable[i] = nil;
	gStringsTable[STR_NULL] = "???";
	GAME_ASSERT(STR_NULL == 0);

	int row = 1;	// start row at 1, so that 0 is an illegal index (STR_NULL)

	char* csvReader = gStringsBuffer;
	while (csvReader != NULL)
	{
		char* myPhrase = NULL;
		bool eol = false;

		for (int x = 0; !eol; x++)
		{
			char* phrase = CSVNextColumn(&csvReader, &eol);

			if (phrase &&
				phrase[0] &&
				(x == languageID || !myPhrase))
			{
				myPhrase = phrase;
			}
		}

		if (myPhrase != NULL)
		{
			gStringsTable[row] = myPhrase;
			row++;
		}
	}

//	for (int i = 0; i < MAX_STRINGS; i++) printf("Str%d-%d: %s\n", languageID, i, gStringsTable[i]);
}

const char* Localize(LocStrID stringID)
{
	if (!gStringsBuffer)
		return "STRINGS NOT LOADED";

	if (stringID < 0 || stringID >= MAX_STRINGS)
		return "ILLEGAL STRING ID";

	if (!gStringsTable[stringID])
		return "";

	return gStringsTable[stringID];
}

GameLanguageID GetBestLanguageIDFromSystemLocale(void)
{
	GameLanguageID languageID = LANGUAGE_ENGLISH;

#if !(SDL_VERSION_ATLEAST(2,0,14))
	#warning Please upgrade to SDL 2.0.14 or later for SDL_GetPreferredLocales. Will default to English for now.
#else
	SDL_Locale* localeList = SDL_GetPreferredLocales();
	if (!localeList)
		return languageID;

	for (SDL_Locale* locale = localeList; locale->language; locale++)
	{
		for (int i = 0; i < NUM_LANGUAGES; i++)
		{
			if (0 == strncmp(locale->language, kLanguageCodesISO639_1[i], 2))
			{
				languageID = i;
				goto foundLocale;
			}
		}
	}

foundLocale:
	SDL_free(localeList);
#endif

	return languageID;
}
