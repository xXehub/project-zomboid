#pragma once

// FontAwesome 5 Free Solid - Icon Unicode Codepoints
// Used for sidebar tab icons. Values come from the FontAwesome icon
// reference: https://fontawesome.com/v5/search?ic=free&s=solid
//
// The encoding is UTF-8 for each codepoint. Quick converter for
// U+F0xy codepoints: 0xEF 0x(80|(xy>>6)) 0x(80|(xy&0x3F)).

#define ICON_MIN_FA 0xf000
#define ICON_MAX_FA 0xf8ff

// Tab / weapon / sidebar icons
#define ICON_FA_CROSSHAIRS    "\xef\x81\x9b"  // U+F05B - crosshairs
#define ICON_FA_SHIELD_ALT    "\xef\x8f\xad"  // U+F3ED - shield
#define ICON_FA_MOUSE_POINTER "\xef\x89\x85"  // U+F245 - mouse pointer
#define ICON_FA_EYE           "\xef\x81\xae"  // U+F06E - eye (visuals)
#define ICON_FA_PALLETE       "\xef\x94\xbf"  // U+F53F - palette
#define ICON_FA_COG           "\xef\x80\x93"  // U+F013 - cog / gear (settings)
#define ICON_FA_PAINT_BRUSH   "\xef\x87\xbc"  // U+F1FC - paint brush
#define ICON_FA_USERS         "\xef\x83\x80"  // U+F0C0 - users
#define ICON_FA_BOMB          "\xef\x87\xa2"  // U+F1E2 - bomb
#define ICON_FA_SKULL         "\xef\x9c\x85"  // U+F705 - skull (zombie esp)
#define ICON_FA_BOX           "\xef\x91\xa6"  // U+F466 - box (item spawner)
#define ICON_FA_MOON          "\xef\x86\x86"  // U+F186 - moon (night vision)
#define ICON_FA_SKULL_CROSSBONES "\xef\x9c\x94" // U+F714 - skull & crossbones
