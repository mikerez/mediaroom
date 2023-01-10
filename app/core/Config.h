// Copyright: https://github.com/mikerez/mediaroom/blob/main/LICENSE

#pragma once

// place for all flow addresses
#define CONFIG_FLOWADDR_SIZE 32
// flows stored per bucket
#define CONFIG_FLOWBUCKET_SIZE 7

#define PRESET_SMALL

#ifdef PRESET_LARGE
// place before/after packet to store attributes
#define CONFIG_PACKET_RESERVE 64
// flows hash size per core
#define CONFIG_FLOWHASH_SIZE (128*1024)
// hash bulk before sessions decoding
//#define CONFIG_HASHBULK_DEFAULT_SIZE (16*1024)
#endif

#ifdef PRESET_SMALL
// place before/after packet to store attributes
#define CONFIG_PACKET_RESERVE 64
// flows hash size per core
#define CONFIG_FLOWHASH_SIZE (8*1024)
// hash bulk before sessions decoding
//#define CONFIG_HASHBULK_DEFAULT_SIZE (1024)
#endif
