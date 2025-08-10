// utilities
#define _REPLACE_SPACE(chr) ((chr) == ' ' ? '0' : (chr))
#define _DIGIT(chr, mult)   (((chr) - '0') * mult)
#define _MONTH_EQUALS(a, b) (((a)[0] == (b)[0]) && \
                             ((a)[1] == (b)[1]) && \
                             ((a)[2] == (b)[2]))

// __TIME__ expands to string in the format "HH:MM:SS":
#define BUILD_HOUR          (_DIGIT(__TIME__[0], 10) + _DIGIT(__TIME__[1], 1))
#define BUILD_MINUTE        (_DIGIT(__TIME__[3], 10) + _DIGIT(__TIME__[4], 1))
#define BUILD_SECOND        (_DIGIT(__TIME__[6], 10) + _DIGIT(__TIME__[7], 1))

// __DATE__ expands to string like "Aug 10 2025" (mmm dd yyyy).
#define BUILD_MONTH         (_MONTH_EQUALS(__DATE__, "Jan") ? 1 : \
                             _MONTH_EQUALS(__DATE__, "Feb") ? 2 : \
                             _MONTH_EQUALS(__DATE__, "Mar") ? 3 : \
                             _MONTH_EQUALS(__DATE__, "Apr") ? 4 : \
                             _MONTH_EQUALS(__DATE__, "May") ? 5 : \
                             _MONTH_EQUALS(__DATE__, "Jun") ? 6 : \
                             _MONTH_EQUALS(__DATE__, "Jul") ? 7 : \
                             _MONTH_EQUALS(__DATE__, "Aug") ? 8 : \
                             _MONTH_EQUALS(__DATE__, "Sep") ? 9 : \
                             _MONTH_EQUALS(__DATE__, "Oct") ? 10 : \
                             _MONTH_EQUALS(__DATE__, "Nov") ? 11 : \
                             _MONTH_EQUALS(__DATE__, "Dec") ? 12 : \
                             -1 )

// Day is padded with spaces:
#define BUILD_DAY           (_DIGIT(_REPLACE_SPACE(__DATE__[4]), 10) + \
                             _DIGIT(__DATE__[5], 1))

#define BUILD_YEAR          (_DIGIT(__DATE__[ 7], 1000) + \
                             _DIGIT(__DATE__[ 8], 100) + \
                             _DIGIT(__DATE__[ 9], 10) + \
                             _DIGIT(__DATE__[10], 1))
