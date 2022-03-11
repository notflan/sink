#ifndef _SINK_FEATURES_H
#define _SINK_FEATURES_H

// REPLACE_STDERR
#ifdef FEATURE_REPLACE_STDERR
#define _FEATURE_REPLACE_STDERR 1
#else
#define _FEATURE_REPLACE_STDERR 0
#endif

// NO_SEARCH_PATH
#ifdef FEATURE_NO_SEARCH_PATH
#define _FEATURE_NO_SEARCH_PATH 1
#else
#define _FEATURE_NO_SEARCH_PATH 0
#endif

// NO_ENV
#ifdef FEATURE_NO_ENV
#define _FEATURE_NO_ENV 1
#else
#define _FEATURE_NO_ENV 0
#endif

// DEBUG_IGNORE_SPLASH
#ifdef FEATURE_DEBUG_IGNORE_SPLASH
#define _FEATURE_DEBUG_IGNORE_SPLASH 1
#else
#define _FEATURE_DEBUG_IGNORE_SPLASH 0
#endif

// Modes

#ifdef DEBUG
#define _FEATURE_MODE_DEBUG 1
#else
#define _FEATURE_MODE_DEBUG 0
#endif

#ifdef RELEASE
#define _FEATURE_MODE_RELEASE 1
#else
#define _FEATURE_MODE_RELEASE 0
#endif

#define _FEATURE_NAME_(name) _FEATURE_ ## name

#define FEATURE_HAS_FLAG(name) (_FEATURE_NAME_(name) == 1)
#define FEATURE_GET_FLAG(name) (FEATURE_HAS_FLAG(name) ? CF_ ## name : 0)

enum compiled_features {
	CF_REPLACE_STDERR	= 1 << 0,
	CF_NO_SEARCH_PATH	= 1 << 1,
	CF_NO_ENV		= 1 << 2,
	CF_DEBUG_IGNORE_SPLASH	= 1 << 3,

	CF_MODE_DEBUG		= 1 << 4,
	CF_MODE_RELEASE		= 1 << 5,
	CF_MODES		= CF_MODE_DEBUG | CF_MODE_RELEASE
};

#define FEATURE_FLAGS ((enum compiled_features) \
				( FEATURE_GET_FLAG(REPLACE_STDERR) \
				| FEATURE_GET_FLAG(NO_SEARCH_PATH) \
				| FEATURE_GET_FLAG(NO_ENV) \
				| FEATURE_GET_FLAG(DEBUG_IGNORE_SPLASH) \
				| FEATURE_GET_FLAG(MODE_DEBUG) \
				| FEATURE_GET_FLAG(MODE_RELEASE) \
			))

#define FEATURE_STRING(flags, name) ((((flags) & FEATURE_GET_FLAG(name)) != 0) ? "+" # name : "-" # name )

#endif /* _SINK_FEATURES_H */
