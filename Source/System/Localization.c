// LOCALIZATION.C
// (C) 2022 Iliyas Jorio
// This file is part of Cro-Mag Rally. https://github.com/jorio/cromagrally

#include "game.h"
#include <string.h>
#include <SDL.h>

#define CSV_PATH ":System:strings.csv"

#define MAX_STRINGS (NUM_LOCALIZED_STRINGS + 1)

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

void LoadLocalizedStrings(GameLanguageID languageID)
{
	// Don't bother reloading strings if we've already loaded this language
	if (languageID == gCurrentStringsLanguage)
	{
		return;
	}

	// Free previous strings buffer
	DisposeLocalizedStrings();

	GAME_ASSERT(languageID >= 0);
	GAME_ASSERT(languageID < NUM_LANGUAGES);

	long count = 0;
	gStringsBuffer = LoadTextFile(CSV_PATH, &count);

	for (int i = 0; i < MAX_STRINGS; i++)
		gStringsTable[i] = nil;
	gStringsTable[STR_NULL] = "???";
	_Static_assert(STR_NULL == 0, "STR_NULL must be 0!");

	int row = 1;	// start row at 1, so that 0 is an illegal index (STR_NULL)

	char* csvReader = gStringsBuffer;
	while (csvReader != NULL)
	{
		char* myPhrase = NULL;
		bool eol = false;

		for (int x = 0; !eol; x++)
		{
			char* phrase = CSVIterator(&csvReader, &eol);

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

void DisposeLocalizedStrings(void)
{
	if (gStringsBuffer != nil)
	{
		SafeDisposePtr(gStringsBuffer);
		gStringsBuffer = nil;
	}
}
