#include <harfbuzz/hb.h> // HarfBuzz header
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#define strcasecmp _stricmp
#endif

#define GDIP_DEBUG_HARFBUZZ_CAIRO_FREETYPE 0
#define dbgHbCrFt GDIP_DEBUG_HARFBUZZ_CAIRO_FREETYPE

// Define the enumerator for supported languages.
typedef enum {
    LANG_UNKNOWN,
    LANG_ENGLISH,
    LANG_FRENCH,
    LANG_GERMAN,
    LANG_SPANISH,
    LANG_PORTUGUESE,
    LANG_RUSSIAN,
    LANG_CHINESE,
    LANG_JAPANESE,
    LANG_KOREAN,
    LANG_ARABIC,
    LANG_TAMIL,
    LANG_SINHALESE,
    LANG_HINDI,
    // Add more languages as needed
} HarfBuzzLanguage;

// A helper struct to map various aliases to the above enumerated values.
typedef struct {
    const char *alias;
    HarfBuzzLanguage lang;
} LanguageMap;

// Array of maps for allowed aliases (both full names and shortened versions).
static const LanguageMap languageMap[] = {
    {"en", LANG_ENGLISH},
    {"eng", LANG_ENGLISH},
    {"english", LANG_ENGLISH},
    {"fr", LANG_FRENCH},
    {"fra", LANG_FRENCH},
    {"french", LANG_FRENCH},
    {"de", LANG_GERMAN},
    {"ger", LANG_GERMAN},
    {"german", LANG_GERMAN},
    {"es", LANG_SPANISH},
    {"spa", LANG_SPANISH},
    {"spanish", LANG_SPANISH},
    {"pt", LANG_PORTUGUESE},
    {"por", LANG_PORTUGUESE},
    {"portuguese", LANG_PORTUGUESE},
    {"ru", LANG_RUSSIAN},
    {"rus", LANG_RUSSIAN},
    {"russian", LANG_RUSSIAN},
    {"zh", LANG_CHINESE},
    {"chi", LANG_CHINESE},
    {"chinese", LANG_CHINESE},
    {"ja", LANG_JAPANESE},
    {"jpn", LANG_JAPANESE},
    {"japanese", LANG_JAPANESE},
    {"ko", LANG_KOREAN},
    {"kor", LANG_KOREAN},
    {"korean", LANG_KOREAN},
    {"ar", LANG_ARABIC},
    {"ara", LANG_ARABIC},
    {"arabic", LANG_ARABIC},
    {"ta", LANG_TAMIL},
    {"tam", LANG_TAMIL},
    {"tamil", LANG_TAMIL},
    {"si", LANG_SINHALESE},
    {"sin", LANG_SINHALESE},
    {"sinhala", LANG_SINHALESE},
    {"sinhalese", LANG_SINHALESE},
    {"hi", LANG_HINDI},
    {"hin", LANG_HINDI},
    {"hindi", LANG_HINDI},
};

#define NUM_LANGUAGES (sizeof(languageMap) / sizeof(languageMap[0]))

// Function that reads the environment variable and returns the corresponding
// language enum.
HarfBuzzLanguage parse_gdiplus_harfbuzz_language() {
    char *env = getenv("GDIPLUS_HARFBUZZ_LANGUAGE");
    if (env == NULL) {
        return LANG_TAMIL;
    }

    for (size_t i = 0; i < NUM_LANGUAGES; i++) {
        if (strcasecmp(env, languageMap[i].alias) == 0) {
            return languageMap[i].lang;
        }
    }
    return LANG_UNKNOWN;
}

// Optional helper to get a human-readable string for the enumerated language.
const char *language_to_str(HarfBuzzLanguage lang) {
    switch (lang) {
    case LANG_ENGLISH:
        return "English";
    case LANG_FRENCH:
        return "French";
    case LANG_GERMAN:
        return "German";
    case LANG_SPANISH:
        return "Spanish";
    case LANG_PORTUGUESE:
        return "Portuguese";
    case LANG_RUSSIAN:
        return "Russian";
    case LANG_CHINESE:
        return "Chinese";
    case LANG_JAPANESE:
        return "Japanese";
    case LANG_KOREAN:
        return "Korean";
    case LANG_ARABIC:
        return "Arabic";
    case LANG_TAMIL:
        return "Tamil";
    case LANG_SINHALESE:
        return "Sinhalese";
    case LANG_HINDI:
        return "Hindi";
    default:
        return "Unknown";
    }
}

// Global HarfBuzz variables for language and script.
hb_language_t g_hb_language;
hb_script_t g_hb_script;

// The init function sets g_hb_language and g_hb_script based on the environment
// variable.
void init_hb_languages(void) {
    HarfBuzzLanguage langEnum = parse_gdiplus_harfbuzz_language();

    switch (langEnum) {
    case LANG_ENGLISH:
    case LANG_FRENCH:
    case LANG_GERMAN:
    case LANG_SPANISH:
    case LANG_PORTUGUESE:
        // For these languages, we use Latin script.
        g_hb_language = hb_language_from_string("en", -1);
        g_hb_script = HB_SCRIPT_LATIN;
        break;
    case LANG_RUSSIAN:
        g_hb_language = hb_language_from_string("ru", -1);
        g_hb_script = HB_SCRIPT_CYRILLIC;
        break;
    case LANG_CHINESE:
        // Default to Simplified Chinese script.
        g_hb_language = hb_language_from_string("zh", -1);
        g_hb_script = HB_SCRIPT_HAN;
        break;
    case LANG_JAPANESE:
        g_hb_language = hb_language_from_string("ja", -1);
        g_hb_script = HB_SCRIPT_HIRAGANA;
        break;
    case LANG_KOREAN:
        g_hb_language = hb_language_from_string("ko", -1);
        g_hb_script = HB_SCRIPT_HAN;
        break;
    case LANG_ARABIC:
        g_hb_language = hb_language_from_string("ar", -1);
        g_hb_script = HB_SCRIPT_ARABIC;
        break;
    case LANG_TAMIL:
        g_hb_language = hb_language_from_string("ta", -1);
        g_hb_script = HB_SCRIPT_TAMIL;
        break;
    case LANG_SINHALESE:
        g_hb_language = hb_language_from_string("si", -1);
        g_hb_script = HB_SCRIPT_SINHALA;
        break;
    case LANG_HINDI:
        g_hb_language = hb_language_from_string("hi", -1);
        g_hb_script = HB_SCRIPT_DEVANAGARI;
        break;
    case LANG_UNKNOWN:
    default:
        // Fallback/default settings (Latin script)
        g_hb_language = hb_language_from_string("ta", -1);
        g_hb_script = HB_SCRIPT_TAMIL;
        break;
    }
}
