/*
 * svnloganal - Reads Subversion logs and writes statistics in xml format.
 * Copyright (C) 2007  Arto Vuori
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */


/* general constants */

#define LOGANAL_VERSION "51"
#define LOGANAL_URI "http://www.cs.helsinki.fi/u/avuori/projects/svnloganal/"
#define MY_ENCODING "UTF-8"
#define GENERAL_RANKING_LENGTH 15
#define SVN_LOG_SEPARATOR "------------------------------------------------------------------------"
#define YEAR_SECONDS 31536000
#define MONTH_SECONDS 2592000
#define DAY_SECONDS 86400

/* error codes */

#define LOGANAL_PARSE_ERROR -1
#define LOGANAL_MEMORY_EXHAUSTED -2
#define LOGANAL_CANNOT_OPEN_FILE -3

#include "pathtree.h"

enum { SVN_PARSER_IDLE, SVN_PARSER_OPENED, 
       SVN_PARSER_OPEN, SVN_PARSER_COMMENT, 
       SVN_PARSER_CHANGEDPATHS };

/* structs */

struct svnstats {
    char lastcommit[19];
    unsigned int totalrevisions; /* revisions in the version control */
    unsigned int revisions; /* number of revisions that were analyzed */
    unsigned long commentlines;
    unsigned int usercount;
    unsigned int modifyfilecount;
    unsigned int addfilecount;
    unsigned int delfilecount;
    unsigned long commentcharcount;
    unsigned int commentcount;
    unsigned int unique_node_count;
    unsigned int ignored_nodes;
    unsigned int unique_datepair_count;
    unsigned int workday_count; /* working days */
    unsigned int overlapping_workday_count; /* combined working days of all workers*/
    Node *fileroot;
    short int parseState;
    unsigned int revision_day_stats[7]; /* revision count per weekday */
    time_t analyzeStartTime;
};

struct svnuser {
    char *username;
    char lastcommit[19];
    unsigned int revisions;
    unsigned long commentlines;
    unsigned int modifyfilecount;
    unsigned int addfilecount;
    unsigned int delfilecount;
    unsigned long commentcharcount;
    unsigned int tmp_commentcharcount;
    unsigned int commentcount;
    unsigned int workdays;
    struct svnmonthstats *daystats;
    struct svnuser *next;
};

struct svnmonthstats {
    unsigned short int year;
    unsigned short int month;
    unsigned short int day;
    unsigned int revisions;
    struct svnmonthstats *next;
    struct svnmonthstats *prev;
};

struct fileAges {
    unsigned int year_or_more;
    unsigned int half_a_year;
    unsigned int quarter;
    unsigned int month;
    unsigned int week;
    unsigned int recent; /* under a week old */
};

struct nodeCustomData {
    unsigned int actioncount;
};

/* functions */

static void init_fileAges(struct fileAges *);
inline static int max(int a, int b);

static int doStats(const char *inFile, 
                   const char *outFile,
                   struct svnstats **,
                   struct svnuser **,
                   struct svnmonthstats **mstats,
                   struct svnmonthstats **dstats,
                   struct fileAges *fileAges);
static int collectStats(const char *, 
                        struct svnstats **, 
                        struct svnuser **,
                        struct svnmonthstats **mstats,
                        struct svnmonthstats **dstats,
                        struct fileAges *fileAges);
static int _parseSvnStats(char *, 
                          struct svnstats **,
                          struct svnuser **,
                          struct svnmonthstats **mstats,
                          struct svnmonthstats **dstats,
                          struct svnuser **,
                          struct fileAges *fileAges);

static int _compare_revisioncount(const void *a, const void *b);
static int _compare_monthstats(const void *a, const void *b);
static int _compare_fileactioncount(const void *a, const void *b);
//static int _compare_addfilecount(const void *a, const void *b);
//static int _compare_modifyfilecount(const void *a, const void *b);

static void init_svnuser_array(struct svnuser *array[], 
                               struct svnuser *list);
static void init_file_array(Node ***ptr, Node *n, int *filecount);
static void init_datepair_array(struct svnmonthstats **ptr, 
                                struct svnmonthstats *ms, 
                                int paircount);

/**
 * Updates linkedlist 'users' with given data
 * If the list does not contain the referred user, a new node is created first.
 * 
 * Returns: -1 if error occurred
 *           0 if succeed but no new users were created
 *           1 if succeed and a new user was created
 */ 
static struct svnuser *add_userdata(struct svnuser **, char *, int *);

/**
 * Updates fileAges struct according to the tm struct
 * In failure: returns 0. Otherwise returns 1.
 */
static int _updateFileAges(struct tm *,
                           struct fileAges *);

/**
 * Returns 1 if a new date was created.
 * Returns -1 if a new date was not created.
 * In failure: returns 0.
 */
static int _updateMonthStats(struct svnmonthstats **,
                              struct tm *,
                              struct svnstats *,
                              bool uniqueDay);
static bool _createYearMonthPair(struct svnmonthstats **, 
                                 struct tm *,
                                 struct svnstats *,
                                 bool uniqueDay);

/**
 * Trims whitespaces from both ends of str
 * Returns trimmed str.
 */
char *trim(char *);

/**
 * Prints users and their stats
 * This is for debugging
 */
//static void _printUsers(struct svnuser *);

/**
 * Prints general stats
 * This is for debuggin
 */
//static void _printStats(struct svnstats *stats);

//static unsigned int _get_revisioncount(struct svnuser *);
//static unsigned int _get_addfilecount(struct svnuser *);
//static unsigned int _get_modifyfilecount(struct svnuser *);

/**
 * Writes a table row
 *
 * headermode: HEADER_NONE | HEADER_HORIZONTAL | _HEADER_VERTICAL
 * varargs are for cell values (char *)
 */
static int _xmlWriteRow(xmlTextWriterPtr writer, int headermode, int cellcount, ...);
/*
 * header modes for _xmlWriteRow
 */
enum { HEADER_NONE, HEADER_HORIZONTAL, HEADER_VERTICAL };

/*
static void printRanking(struct svnuser *ranking[], 
                         struct svnstats *, 
                         const char *rankingName, 
                         unsigned int (*getfield_fnc)(struct svnuser *),
                         unsigned int (*gettotalfield_fnc)(struct svnstats*));
*/

static const char* monthsAlpha[] = {"January",
                                    "February",
                                    "March",
                                    "April",
                                    "May",
                                    "June",
                                    "July",
                                    "August",
                                    "September",
                                    "October",
                                    "November",
                                    "December"};
static const char* _iMonthToStr(int month);
static const char* daysAlpha[] = {"Sunday",
                                  "Monday",
                                  "Tuesday",
                                  "Wednesday",
                                  "Thursday",
                                  "Friday",
                                  "Saturday"};
static const char* _iDayToStr(int day);

static struct svnmonthstats* _monthstats_tail(struct svnmonthstats *mstats);
static struct svnmonthstats* _monthstats_head(struct svnmonthstats *mstats);

/**
 * If d equals zero, returns 1. Otherwise returns d.
 */
static int _divisor(int d);

/**
 * Frees a svnmonthstats linked list
 */
static void _freeMonthStats(struct svnmonthstats *mstats);

/**
 * Prints help
 */
void printUsage(const char* program);
