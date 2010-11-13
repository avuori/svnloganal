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

/*
 * TODO:
 *         - css styles from external file
 *         - take path exceptions from cmdline args
 *         - header IFNDEFs...
 */

#define _GNU_SOURCE 1
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <getopt.h>
#include <stdarg.h>
#include <libxml/encoding.h>
#include <libxml/xmlwriter.h>
#include "logAnalyze.h"



/*
 * project title
 */
static char* svn_title;

/*
 * comment lines in currently parsed log entry
 */ 
static int parser_comment_lines;


static int _compare_monthstats(const void *a, const void *b)
{
    struct svnmonthstats **arg1 = (struct svnmonthstats **)a;
    struct svnmonthstats **arg2 = (struct svnmonthstats **)b;
    return (*arg2)->revisions - (*arg1)->revisions;
}

static int _compare_revisioncount(const void *a, const void *b)
{
    struct svnuser **arg1 = (struct svnuser **)a;
    struct svnuser **arg2 = (struct svnuser **)b;
    return (*arg2)->revisions - (*arg1)->revisions;
}

/*
static int _compare_addfilecount(const void *a, const void *b)
{
    struct svnuser **arg1 = (struct svnuser **)a;
    struct svnuser **arg2 = (struct svnuser **)b;
    return (*arg2)->addfilecount - (*arg1)->addfilecount;
}
*/
/*
static int _compare_delfilecount(const void *a, const void *b)
{
    struct svnuser **arg1 = (struct svnuser **)a;
    struct svnuser **arg2 = (struct svnuser **)b;
    return (*arg2)->delfilecount - (*arg1)->delfilecount;
}
*/
/*
static int _compare_commentcount(const void *a, const void *b)
{
    struct svnuser **arg1 = (struct svnuser **)a;
    struct svnuser **arg2 = (struct svnuser **)b;
    return (*arg2)->commentcount - (*arg1)->commentcount;
}
*/
/*
static int _compare_modifyfilecount(const void *a, const void *b)
{
    struct svnuser **arg1 = (struct svnuser **)a;
    struct svnuser **arg2 = (struct svnuser **)b;
    return (*arg2)->modifyfilecount - (*arg1)->modifyfilecount;
}
*/
/*
static int _compare_commentlinecount(const void *a, const void *b)
{
    struct svnuser **arg1 = (struct svnuser **)a;
    struct svnuser **arg2 = (struct svnuser **)b;
    return (*arg2)->commentlines - (*arg1)->commentlines;
}
*/
/*
static int _compare_commentcharcount(const void *a, const void *b)
{
    struct svnuser **arg1 = (struct svnuser **)a;
    struct svnuser **arg2 = (struct svnuser **)b;
    return (*arg2)->commentcharcount - (*arg1)->commentcharcount;
}
*/

static int _compare_fileactioncount(const void *a, const void *b)
{
    Node **arg1 = (Node **)a;
    Node **arg2 = (Node **)b;
    //return *(int*)(*arg2)->custom - *(int*)(*arg1)->custom;
    struct nodeCustomData *data_1 
        = (struct nodeCustomData *)((*arg1)->custom);
    struct nodeCustomData *data_2 
        = (struct nodeCustomData *)((*arg2)->custom);
    
    return data_2->actioncount - data_1->actioncount;
}

static void init_svnuser_array(struct svnuser *array[], 
                               struct svnuser *list)
{
    struct svnuser *p = list;
    int i = 0;
    while (p != NULL)
    {
        array[i] = p;
        i++;
        p = p->next;
    }
}

/*
static unsigned int _get_revisioncount(struct svnuser *u)
{
    return u->revisions;
}
*/
/*
static unsigned int _get_addfilecount(struct svnuser *u)
{
    return u->addfilecount;
}
*/
/*
static unsigned int _get_delfilecount(struct svnuser *u)
{
    return u->delfilecount;
}
*/
/*
static unsigned int _get_commentcount(struct svnuser *u)
{
    return u->commentcount;
}
*/
/*
static unsigned int _get_modifyfilecount(struct svnuser *u)
{
    return u->modifyfilecount;
}
*/
/*
static unsigned int _get_commentlinecount(struct svnuser *u)
{
    return u->commentlines;
}
*/
/*
static unsigned int _get_commentcharcount(struct svnuser *u)
{
    return u->commentcharcount;
}
*/
/*
static unsigned int _get_total_revisioncount(struct svnstats *st)
{
    return st->revisions > 0 ? st->revisions : 1;
}
*/
/*
static unsigned int _get_total_addfilecount(struct svnstats *st)
{
    return st->addfilecount > 0 ? st->addfilecount : 1;
}
*/
/*
static unsigned int _get_total_delfilecount(struct svnstats *st)
{
    return st->delfilecount > 0 ? st->delfilecount : 1;
}
*/
/*
static unsigned int _get_total_modifyfilecount(struct svnstats *st)
{
    return st->modifyfilecount > 0 ? st->modifyfilecount : 1;
}
*/
/*
static unsigned int _get_total_commentcount(struct svnstats *st)
{
    return st->commentcount > 0 ? st->commentcount : 1;
}
*/
/*
static unsigned int _get_total_commentlinecount(struct svnstats *st)
{
    return st->commentlines > 0 ? st->commentlines : 1;
}
*/
/*
static unsigned int _get_total_commentcharcount(struct svnstats *st)
{
    return st->commentcharcount > 0 ? st->commentcharcount : 1;
}
*/

/*
static void printRanking(struct svnuser *ranking[], 
                         struct svnstats *stats, 
                         const char *rankingName, 
                         unsigned int (*get_field_func)(struct svnuser *),
                         unsigned int (*get_total_field_func)(struct svnstats *))
{
    printf("Ranking by %s\n", rankingName);
    int i;
    for (i = 0; i < stats->usercount; i++)
    {
        printf("%d. %s\t%u (%.2f%%)\n", (i+1), 
               ranking[i]->username, 
               get_field_func(ranking[i]),
               ((get_field_func(ranking[i]) 
               / (float)_divisor(get_total_field_func(stats)))*100));
    }
    puts("");
}
*/

static void init_file_array(Node ***ptr, Node *n, int *filecount)
{
    if (!n)
        return;

    if (!n->children)
    {
//        printf("[INIT][0x%x] %s\n", (int)*ptr, n->path);
        **ptr = n;
        *ptr += 1;
        (*filecount)++;
        return;
    }
   
    Node *c = n->children;
    while (c)
    {
        init_file_array(ptr, c, filecount); 
        c = c->sibling; 
    }
}

static void init_datepair_array(struct svnmonthstats **ptr, 
                                struct svnmonthstats *ms, 
                                int paircount)
{
    if (ms == NULL)
        return;

    // rewind
    while (ms->prev != NULL)
        ms = ms->prev;
    
    int i;
    for (i = 0; i < paircount; i++)
    {
        *(ptr+i) = ms;
        ms = ms->next;
    }
}

static int doStats(const char *inFile, 
                   const char *outFile,
                   struct svnstats **stats,
                   struct svnuser **users,
                   struct svnmonthstats **mstats,
                   struct svnmonthstats **dstats,
                   struct fileAges *fileAges)
{
    int errcode;
    int i;

    if ((errcode = collectStats(inFile, 
                                &(*stats), 
                                &(*users), 
                                &(*mstats),
                                &(*dstats),
                                fileAges)) < 0)
    {
        fprintf(stderr, "Error collecting data: %i\n", errcode);
        return errcode;
    }

    //indexes for sorting.
    struct svnuser *revisionindex[(*stats)->usercount];
    /*
    struct svnuser *addindex[(*stats)->usercount];
    struct svnuser *delindex[(*stats)->usercount];
    struct svnuser *modifyindex[(*stats)->usercount];
    struct svnuser *commentsindex[(*stats)->usercount];
    struct svnuser *commentlinesindex[(*stats)->usercount];
    struct svnuser *commentcharsindex[(*stats)->usercount];
    */

    init_svnuser_array(revisionindex, *users);
    /*
    init_svnuser_array(addindex, *users);
    init_svnuser_array(delindex, *users);
    init_svnuser_array(modifyindex, *users);
    init_svnuser_array(commentsindex, *users);
    init_svnuser_array(commentlinesindex, *users);
    init_svnuser_array(commentcharsindex, *users);
    */

    qsort(revisionindex, 
          (*stats)->usercount, 
          sizeof(struct svnuser *), 
          &_compare_revisioncount);
    /*
    qsort(addindex, 
          (*stats)->usercount, 
          sizeof(struct svnuser *), 
          &_compare_addfilecount);
    
    qsort(delindex, 
          (*stats)->usercount, 
          sizeof(struct svnuser *), 
          &_compare_delfilecount);
     
    qsort(modifyindex, 
          (*stats)->usercount, 
          sizeof(struct svnuser *), 
          &_compare_modifyfilecount);

    qsort(commentsindex, 
          (*stats)->usercount, 
          sizeof(struct svnuser *), 
          &_compare_commentcount);

    qsort(commentlinesindex, 
          (*stats)->usercount, 
          sizeof(struct svnuser *), 
          &_compare_commentlinecount);

    qsort(commentcharsindex, 
          (*stats)->usercount, 
          sizeof(struct svnuser *), 
          &_compare_commentcharcount);
    */

    Node *fileactionindex[(*stats)->unique_node_count];
    Node **ptr = &fileactionindex[0];
    int unique_file_count = 0;
    init_file_array(&ptr, (*stats)->fileroot, &unique_file_count);

    qsort(fileactionindex,
          unique_file_count,
          sizeof(Node *),
          &_compare_fileactioncount);
    
    struct svnmonthstats *monthstatsindex[(*stats)->unique_datepair_count];
    struct svnmonthstats **msPtr = &monthstatsindex[0];
    init_datepair_array(msPtr, *mstats, (*stats)->unique_datepair_count);
    qsort(monthstatsindex,
          (*stats)->unique_datepair_count,
          sizeof(struct svnmonthstats *),
          &_compare_monthstats);


    /*
    printRanking(revisionindex, 
                 *stats, 
                 "committed revisions", 
                 &_get_revisioncount,
                 &_get_total_revisioncount);
    
    printRanking(addindex, *stats, 
                 "added files", 
                 &_get_addfilecount, 
                 &_get_total_addfilecount);
    
    printRanking(modifyindex, 
                 *stats, 
                 "file modifications", 
                 &_get_modifyfilecount, 
                 &_get_total_modifyfilecount);

    printRanking(delindex, 
                 *stats, 
                 "file deletions", 
                 &_get_delfilecount, 
                 &_get_total_delfilecount);

    printRanking(commentsindex, 
                 *stats, 
                 "comments", 
                 &_get_commentcount, 
                 &_get_total_commentcount);

    printRanking(commentlinesindex, 
                 *stats, 
                 "comment lines", 
                 &_get_commentlinecount, 
                 &_get_total_commentlinecount);

    printRanking(commentcharsindex, 
                 *stats, 
                 "comment chars", 
                 &_get_commentcharcount, 
                 &_get_total_commentcharcount);
    */

//    printf("Ignored path nodes: %d\n", (*stats)->ignored_nodes);



    int rc;
    xmlTextWriterPtr writer;
    // a few buffers for converting types to string
    char buf1[1024];
    char buf2[1024];
    char buf3[1024];
    char buf4[1024];
    char buf5[1024];
    char buf6[1024];
    char buf7[1024];
    char buf8[1024];
    char buf9[1024];
    char buf10[1024];
    //FIXME encoding must be alterable.
    xmlCharEncodingHandlerPtr encoding = xmlFindCharEncodingHandler(MY_ENCODING);
    if (outFile == NULL)
    {
        writer = xmlNewTextWriter(xmlOutputBufferCreateFile(stdout, encoding));
    } else {
        writer = xmlNewTextWriterFilename(outFile, 0);
    }
    if (writer == NULL)
    {
        printf("Error creating the xml writer\n");
        //return 1;
    }

    rc = xmlTextWriterStartDocument(writer, NULL, MY_ENCODING, NULL);
    if (rc < 0) {
        printf
            ("testXmlwriterFilename: Error at xmlTextWriterStartDocument\n");
//        return 1;
    }

    xmlTextWriterWriteDTD(writer,
                          BAD_CAST "html",
                          BAD_CAST "-//W3C//DTD XHTML 1.0 Strict//EN",
                          BAD_CAST "http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd",
                          NULL);

    rc = xmlTextWriterStartElement(writer, BAD_CAST "html");
    if (rc < 0) {
        printf
            ("testXmlwriterFilename: Error at xmlTextWriterStartElement\n");
        //      return 1;
    }
    rc = xmlTextWriterWriteAttribute(writer,
                                      BAD_CAST "xmlns",
                                      BAD_CAST "http://www.w3.org/1999/xhtml");
    rc = xmlTextWriterWriteAttribute(writer,
                                      BAD_CAST "xml:lang",
                                      BAD_CAST "en");

    rc = xmlTextWriterStartElement(writer, BAD_CAST "head");
    if (rc < 0) {
        printf
            ("testXmlwriterFilename: Error at xmlTextWriterStartElement\n");
//        return 1;
    }

    rc = xmlTextWriterStartElement(writer, BAD_CAST "meta");
    rc = xmlTextWriterWriteAttribute(writer, 
                                     BAD_CAST "http-equiv", 
                                     BAD_CAST "Content-Type");

    rc = xmlTextWriterWriteAttribute(writer, 
                                     BAD_CAST "content", 
                                     BAD_CAST "application/xhtml+xml; charset=utf-8"); //FIXME use MY_ENCODING
    rc = xmlTextWriterEndElement(writer); // meta

    
    rc = xmlTextWriterStartElement(writer, BAD_CAST "title");
    rc = xmlTextWriterWriteFormatString(writer,
                                        "SVN Log Analysis for %s",
                                        BAD_CAST svn_title);
    rc = xmlTextWriterEndElement(writer); // title


    rc = xmlTextWriterStartElement(writer, BAD_CAST "style");
    rc = xmlTextWriterWriteAttribute(writer, 
                                     BAD_CAST "type", 
                                     BAD_CAST "text/css");

    //FIXME extra css from external file.
    // __NOTE__: style specs for table must be defined second!
    // (because of javascript hacks below)
    rc = xmlTextWriterWriteString(writer,
BAD_CAST "\nbody {\n\
    font-size: 0.9em;\n\
    margin-top: 5px;\n\
    margin-left: 5px;\n\
}\n\
table {\n\
    border-collapse: collapse;\n\
    display: inline-table;\n\
    margin-right: 20px;\n\
}\n\
table.users {\n\
   margin: 0px;\n\
}\n\
td,th {\n\
    font-size: 0.9em;\n\
    border-bottom: 1px solid black;\n\
    border-right: 1px solid black;\n\
    padding: 3px;\n\
    text-align: left;\n\
}\n\
div.d {\n\
    float: left;\n\
    margin-right: 1em;\n\
}\n\
div#foot {\n\
    clear: left;\n\
    border-top: 1px solid black;\n\
    font-size: 0.8em;\n\
    margin-top: 1em;\n\
}\n\
h1,h2 {\n\
    margin-top: 0;\n\
    margin-bottom: 0.5em;\n\
    padding-left: 0.5em;\n\
    padding-right: 0.5em;\n\
    background-color: #333333;\n\
    color: white;\n\
}\n\
h2 {\n\
    margin-top: 0.5em;\n\
    font-size: 1em;\n\
}\n\
\n\
div.histogram_1,\n\
div.histogram_2,\n\
div.histogram_3,\n\
div.histogram_4,\n\
div.histogram_5,\n\
div.histogram_6 {\n\
   width: 30px;\n\
   float: left;\n\
   position: relative;\n\
}\n\
div.histogram_1 {\n\
  background-color: red;\n\
}\n\
div.histogram_2 {\n\
  background-color: yellow;\n\
}\n\
div.histogram_3 {\n\
  background-color: #899F00;\n\
}\n\
div.histogram_4 {\n\
  background-color: #699F00;\n\
}\n\
div.histogram_5 {\n\
  background-color: #00BF00;\n\
}\n\
div.histogram_6 {\n\
 background-color: #00FF00;\n\
}\n");

    rc = xmlTextWriterEndElement(writer); // style

// Browser compatibility hacking :-)
    rc = xmlTextWriterWriteRaw(writer,
BAD_CAST "<script type='text/javascript'>\n\
var myStyle = document.styleSheets[0].cssRules[1].style;\n\
if (navigator.userAgent.indexOf('Firefox') >= 0) {\n\
                                                      myStyle.display = 'inline';\n\
                                                                                     }\n\
</script>\n");
    
    rc = xmlTextWriterEndElement(writer); // head
    
    rc = xmlTextWriterStartElement(writer, BAD_CAST "body");

    rc = xmlTextWriterStartElement(writer, BAD_CAST "h1");

    rc = xmlTextWriterWriteFormatString(writer,
                                        "SVN Log Analysis for %s",
                                        BAD_CAST svn_title);    

    rc = xmlTextWriterEndElement(writer); // h1

    rc = xmlTextWriterStartElement(writer, BAD_CAST "div");

    rc = xmlTextWriterWriteAttribute(writer, 
                                     BAD_CAST "class", 
                                     BAD_CAST "d");
    
    rc = xmlTextWriterStartElement(writer, BAD_CAST "h2");
    rc = xmlTextWriterWriteString(writer, BAD_CAST "General Statistics");
    rc = xmlTextWriterEndElement(writer); // h2

    /* totals table */
    //TODO total comments, commentlines, commentchars, files... etc.

    rc = xmlTextWriterStartElement(writer, BAD_CAST "table");
    rc = xmlTextWriterStartElement(writer, BAD_CAST "tbody");
    
    sprintf(buf1, "%d", (*stats)->totalrevisions);
    _xmlWriteRow(writer, 
                 HEADER_VERTICAL,
                 2,
                 BAD_CAST "Revisions",
                 buf1);

    sprintf(buf1, "%d (%.0f%%)", 
            (*stats)->revisions,
            (*stats)->revisions / (float)_divisor((*stats)->totalrevisions) * 100);
    _xmlWriteRow(writer, 
                 HEADER_VERTICAL,
                 2,
                 BAD_CAST "Sample Size",
                 buf1);

    struct svnmonthstats *tail = _monthstats_tail(*dstats);
    sprintf(buf1, "%d-%02d-%02d", 
            tail->year + 1900,
            tail->month + 1,
            tail->day);
    _xmlWriteRow(writer, 
                 HEADER_VERTICAL,
                 2,
                 BAD_CAST "First Commit",
                 buf1);

    struct svnmonthstats *head = _monthstats_head(*dstats);
    sprintf(buf1, "%d-%02d-%02d", 
            head->year + 1900,
            head->month + 1,
            head->day);
    _xmlWriteRow(writer, 
                 HEADER_VERTICAL,
                 2,
                 BAD_CAST "Last Commit",
                 buf1);

     sprintf(buf1, "%d", (*stats)->usercount);
    _xmlWriteRow(writer, 
                 HEADER_VERTICAL,
                 2,
                 BAD_CAST "Contributors",
                 buf1);

    sprintf(buf1, "%d", (*stats)->workday_count);
    _xmlWriteRow(writer, 
                 HEADER_VERTICAL,
                 2,
                 BAD_CAST "Workdays",
                 buf1);

    sprintf(buf1, "%d", (*stats)->overlapping_workday_count);
    _xmlWriteRow(writer, 
                 HEADER_VERTICAL,
                 2,
                 BAD_CAST "Combined Workdays",
                 buf1);

     sprintf(buf1, "%d", unique_file_count);
    _xmlWriteRow(writer, 
                 HEADER_VERTICAL,
                 2,
                 BAD_CAST "Number of Files",
                 buf1);

     sprintf(buf1, "%d", (*stats)->modifyfilecount);
    _xmlWriteRow(writer, 
                 HEADER_VERTICAL,
                 2,
                 BAD_CAST "File Modifications",
                 buf1);

     sprintf(buf1, "%d", (*stats)->addfilecount);
    _xmlWriteRow(writer, 
                 HEADER_VERTICAL,
                 2,
                 BAD_CAST "Files Added",
                 buf1);

     sprintf(buf1, "%d", (*stats)->delfilecount);
    _xmlWriteRow(writer, 
                 HEADER_VERTICAL,
                 2,
                 BAD_CAST "Files Deleted",
                 buf1);

     sprintf(buf1, "%d", (*stats)->commentcount);
    _xmlWriteRow(writer, 
                 HEADER_VERTICAL,
                 2,
                 BAD_CAST "Comments",
                 buf1);

    sprintf(buf1, "%.5f", 
            (*stats)->commentcount 
            / (float)_divisor((*stats)->revisions));
    _xmlWriteRow(writer, 
                 HEADER_VERTICAL,
                 2,
                 BAD_CAST "Comments / Revisions",
                 buf1);

     sprintf(buf1, "%ld", (*stats)->commentlines);
    _xmlWriteRow(writer, 
                 HEADER_VERTICAL,
                 2,
                 BAD_CAST "Comment Lines",
                 buf1);

     sprintf(buf1, "%ld", (*stats)->commentcharcount);
    _xmlWriteRow(writer, 
                 HEADER_VERTICAL,
                 2,
                 BAD_CAST "Comment Characters",
                 buf1);

    sprintf(buf1, "%.1f chars", 
            (*stats)->commentcharcount 
            / (float)_divisor((*stats)->commentcount));
    _xmlWriteRow(writer, 
                 HEADER_VERTICAL,
                 2,
                 BAD_CAST "Avg. Comment Length",
                 buf1);

    rc = xmlTextWriterEndElement(writer); //tbody
    rc = xmlTextWriterEndElement(writer); //table

    /* day table */
    rc = xmlTextWriterStartElement(writer, BAD_CAST "table");
    rc = xmlTextWriterStartElement(writer, BAD_CAST "tbody");
    _xmlWriteRow(writer, 
                 HEADER_HORIZONTAL,
                 2, 
                 BAD_CAST "Day", 
                 BAD_CAST "Revisions");
    
    for (i = 0; i < 7; i++) {
        sprintf(buf1,
                "%d",
                (*stats)->revision_day_stats[i]);
        sprintf(buf2,
                "%.0f%%",
                100 * (*stats)->revision_day_stats[i] / (float)_divisor((*stats)->revisions));
        _xmlWriteRow(writer,
                     HEADER_NONE,
                     3,
                     _iDayToStr(i), //weekday
                     buf1, // revisions
                     buf2); // percent
    }
    
    rc = xmlTextWriterEndElement(writer); //tbody
    rc = xmlTextWriterEndElement(writer); //table

    /* month table */
    
    rc = xmlTextWriterStartElement(writer, BAD_CAST "table");
    rc = xmlTextWriterStartElement(writer, BAD_CAST "tbody");
    _xmlWriteRow(writer, 
                 HEADER_HORIZONTAL,
                 3, 
                 BAD_CAST "Rank",
                 BAD_CAST "Month", 
                 BAD_CAST "Revisions");
    int datepairs = (*stats)->unique_datepair_count;
    //FIXME list length from config
    int visibleMonths = datepairs > GENERAL_RANKING_LENGTH ? GENERAL_RANKING_LENGTH : datepairs;
    for (i = 0; i < visibleMonths; i++) {
        sprintf(buf1, 
                "%d, %s", 
                monthstatsindex[i]->year + 1900,
                _iMonthToStr(monthstatsindex[i]->month));
         sprintf(buf2, 
                "%d", 
                monthstatsindex[i]->revisions);
         sprintf(buf3, "%d.", i+1);
         sprintf(buf4, 
                 "%.1f%%", 
                 100 * monthstatsindex[i]->revisions / (float)_divisor((*stats)->revisions));
               
        _xmlWriteRow(writer,
                     HEADER_NONE,
                     4,
                     buf3,  //rank
                     buf1,   //month
                     buf2,  //revisions
                     buf4); //percent
    }
    
    rc = xmlTextWriterEndElement(writer); //tbody
    rc = xmlTextWriterEndElement(writer); //table

    rc = xmlTextWriterEndElement(writer); // div
    
    rc = xmlTextWriterStartElement(writer, 
                                   BAD_CAST "div");

    rc = xmlTextWriterWriteAttribute(writer, 
                                     BAD_CAST "class", 
                                     BAD_CAST "d");

    rc = xmlTextWriterStartElement(writer, BAD_CAST "h2");
    rc = xmlTextWriterWriteString(writer,
                                  BAD_CAST "Project Files");

    rc = xmlTextWriterEndElement(writer); // h1
    
    /* file ages */

    rc = xmlTextWriterStartElement(writer, BAD_CAST "h3");
    rc = xmlTextWriterWriteString(writer,
                                  BAD_CAST "Last updated since...");
    rc = xmlTextWriterEndElement(writer); // h2

    const int maxPixelSize = 150;
    unsigned int *ageFields[] = { &(fileAges->year_or_more),
                                  &(fileAges->half_a_year),
                                  &(fileAges->quarter),
                                  &(fileAges->month),
                                  &(fileAges->week),
                                  &(fileAges->recent) };
    const int fieldCount = sizeof(ageFields) / sizeof(int *);
    unsigned int *ageField;
    int relativeSize;
    int maxHeight = 0;
    int totalTimedFiles = 0;
    for (i = 0; i <fieldCount; i++) {
        ageField = ageFields[i];
        totalTimedFiles += *ageField;
    }
    
    rc = xmlTextWriterWriteFormatRaw(
        writer,
"\n\
<div class='histogram_1' style='float:none;position:static;display:inline;height:20px'>&nbsp;</div>&nbsp;a year or more (%.1f%%)\n\
<div class='histogram_2' style='float:none;position:static;display:inline;height:20px'>&nbsp;</div>&nbsp;6 months (%.1f%%)\n\
<div class='histogram_3' style='float:none;position:static;display:inline;height:20px'>&nbsp;</div>&nbsp;a quarter (%.1f%%)\n\
<br />\n\
<div class='histogram_4' style='float:none;position:static;display:inline;height:20px'>&nbsp;</div>&nbsp;a month (%.1f%%)\n\
<div class='histogram_5' style='float:none;position:static;display:inline;height:20px'>&nbsp;</div>&nbsp;a week (%.1f%%)\n\
<div class='histogram_6' style='float:none;position:static;display:inline;height:20px'>&nbsp;</div>&nbsp;under a week (%.1f%%)\n",
        *ageFields[0] / (float)totalTimedFiles * 100,
        *ageFields[1] / (float)totalTimedFiles * 100,
        *ageFields[2] / (float)totalTimedFiles * 100,
        *ageFields[3] / (float)totalTimedFiles * 100,
        *ageFields[4] / (float)totalTimedFiles * 100,
        *ageFields[5] / (float)totalTimedFiles * 100
        );

    for (i = 0; i <fieldCount; i++) {
        ageField = ageFields[i];
        relativeSize = max(*ageField / (float)totalTimedFiles * maxPixelSize,
                           1);

        if (relativeSize > maxHeight) {
            maxHeight = relativeSize;
        }

        rc = xmlTextWriterWriteFormatRaw(
            writer,
            "<div class='histogram_1' style='height:%dpx;background-color:#D3D3D3;'></div>\n",
            maxPixelSize-relativeSize);
    }

    rc = xmlTextWriterWriteRaw(writer,
                               BAD_CAST "<div style='clear:left'></div>\n");

    for (i = 0; i < fieldCount; i++) {
        ageField = ageFields[i];
        relativeSize = max(*ageField / (float)totalTimedFiles * maxPixelSize,
                           1);
        rc = xmlTextWriterWriteFormatRaw(
            writer,
            "<div class='histogram_%d' style='height:%dpx;top:-%dpx;'></div>\n",
            (i+1), relativeSize, relativeSize);
    }

    /* file table */
    rc = xmlTextWriterStartElement(writer, BAD_CAST "table");
    sprintf(buf1,
            "clear:left;display:block;position:relative;top:-%dpx;",
            maxHeight);
    rc = xmlTextWriterWriteAttribute(writer, 
                                     BAD_CAST "style", 
                                     BAD_CAST buf1);
    rc = xmlTextWriterStartElement(writer, BAD_CAST "tbody");
    _xmlWriteRow(writer, 
                 HEADER_HORIZONTAL,
                 3, 
                 BAD_CAST "Rank",
                 BAD_CAST "File", 
                 BAD_CAST "Actions");

//    int uniqueFiles = (*stats)->unique_node_count;

    //FIXME list maxlength should be alterable
    int visibleFiles = unique_file_count > GENERAL_RANKING_LENGTH 
        ? GENERAL_RANKING_LENGTH : unique_file_count;

    for (i = 0; i < visibleFiles; i++) {
        sprintf(buf2, "%d.", i+1);
        node_pathncpy(buf5, 
                      fileactionindex[i], 
                      sizeof(buf5));
        //sprintf(buf1, "%d", *(int*)fileactionindex[i]->custom);
        sprintf(buf1, "%u",
                ((struct nodeCustomData*)(fileactionindex[i]->custom))->actioncount);
        sprintf(buf3, 
                "%.1f%%",
                100 * ((struct nodeCustomData*)(fileactionindex[i]->custom))->actioncount
                / (float)_divisor(((*stats)->addfilecount 
                                   + (*stats)->modifyfilecount
                                   + (*stats)->delfilecount)));


//        printf(":::%s:::\n", buf5);

       _xmlWriteRow(writer,
                    HEADER_NONE,
                    4,
                    buf2,   //rank
                    buf5,   //path
                    buf1,    //edits
                    buf3);  //percent
    }
    

    rc = xmlTextWriterEndElement(writer); //tbody
    rc = xmlTextWriterEndElement(writer); //table


    rc = xmlTextWriterEndElement(writer); // div
    
    rc = xmlTextWriterStartElement(writer, 
                                   BAD_CAST "div");

    rc = xmlTextWriterWriteAttribute(writer, 
                                     BAD_CAST "class", 
                                     BAD_CAST "d");
    
    rc = xmlTextWriterStartElement(writer, 
                                   BAD_CAST "h2");
    sprintf(buf1,
            "clear:left;display:block;position:relative;top:-%dpx;",
            maxHeight);
    rc = xmlTextWriterWriteAttribute(writer, 
                                     BAD_CAST "style", 
                                     BAD_CAST buf1);
    rc = xmlTextWriterWriteString(writer, 
                                  BAD_CAST "User Statistics");
    rc = xmlTextWriterEndElement(writer); // h2

    /* user table */
    rc = xmlTextWriterStartElement(writer, 
                                   BAD_CAST "table");
    sprintf(buf1,
            "clear:left;display:block;position:relative;top:-%dpx;",
            maxHeight);
    rc = xmlTextWriterWriteAttribute(writer, 
                                     BAD_CAST "style", 
                                     BAD_CAST buf1);
    rc = xmlTextWriterWriteAttribute(writer, 
                                     BAD_CAST "class", 
                                     BAD_CAST "users");
    rc = xmlTextWriterStartElement(writer, BAD_CAST "tbody");
    _xmlWriteRow(writer, 
                 HEADER_HORIZONTAL,
                 11, 
                 BAD_CAST "Contributor",
                 BAD_CAST "Revisions", 
                 BAD_CAST "Workdays", 
                 BAD_CAST "File Modifications",
                 BAD_CAST "Files Added",
                 BAD_CAST "Files Deleted",
                 BAD_CAST "Comments",
                 BAD_CAST "Comments / Revisions",
                 BAD_CAST "Comment Lines",
                 BAD_CAST "Comment Characters",
                 BAD_CAST "Avg. Comment Length");

    for (i = 0; i < (*stats)->usercount; i++) {

        sprintf(buf1, "%d (%.1f%%)", 
                revisionindex[i]->revisions,
                revisionindex[i]->revisions 
                / (float)_divisor((*stats)->revisions) * 100);

        sprintf(buf2, "%d (%.1f%%)", 
                revisionindex[i]->modifyfilecount,
                revisionindex[i]->modifyfilecount 
                / (float)_divisor((*stats)->modifyfilecount) * 100);

        sprintf(buf3, "%d (%.1f%%)", 
                revisionindex[i]->addfilecount,
                revisionindex[i]->addfilecount 
                / (float)_divisor((*stats)->addfilecount) * 100);

        sprintf(buf4, "%d (%.1f%%)", 
                revisionindex[i]->delfilecount,
                revisionindex[i]->delfilecount 
                / (float)_divisor((*stats)->delfilecount) * 100);

        sprintf(buf5, "%d (%.1f%%)", 
                revisionindex[i]->commentcount,
                revisionindex[i]->commentcount 
                / (float)_divisor((*stats)->commentcount) * 100);

        sprintf(buf6, "%ld (%.1f%%)", 
                revisionindex[i]->commentlines,
                revisionindex[i]->commentlines 
                / (float)_divisor((*stats)->commentlines) * 100);
        
        sprintf(buf7, "%ld (%.1f%%)", 
                revisionindex[i]->commentcharcount,
                revisionindex[i]->commentcharcount 
                / (float)_divisor((*stats)->commentcharcount) * 100);

        sprintf(buf8, "%.1f chars", 
                revisionindex[i]->commentcharcount 
                / (float)_divisor(revisionindex[i]->revisions));

        sprintf(buf9, "%d (%.1f%%)", 
                revisionindex[i]->workdays, 
                revisionindex[i]->workdays
                / (float)_divisor((*stats)->overlapping_workday_count) * 100);

        sprintf(buf10, "%.5f", 
                revisionindex[i]->commentcount 
                / (float)_divisor(revisionindex[i]->revisions));

        _xmlWriteRow(writer,
                     HEADER_NONE,
                     11,
                     revisionindex[i]->username,
                     buf1, //commits
                     buf9, //workdays
                     buf2, //file modif.
                     buf3, //file adds
                     buf4, //file dels
                     buf5, //comments
                     buf10,//comments / revisions
                     buf6, //comment lines
                     buf7, //comment chars
                     buf8);//avg. comment length
    }

    //revisionindex, stats->usercount
    rc = xmlTextWriterEndElement(writer); //tbody
    rc = xmlTextWriterEndElement(writer); //table

    rc = xmlTextWriterEndElement(writer); // div

    rc = xmlTextWriterStartElement(writer, BAD_CAST "div");

    rc = xmlTextWriterWriteAttribute(writer, 
                                     BAD_CAST "id", 
                                     BAD_CAST "foot");


    time_t rawtime;
    struct tm* timeinfo;
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    
    int analyzeDuration = time(NULL) - (*stats)->analyzeStartTime;
    rc = xmlTextWriterWriteFormatRaw(writer, 
                                        "<i>Generated</i> at %s\
                                        <i>in</i> ~%d seconds\
                                        by <a href='%s'>svnloganal</a> %s.",
                                     asctime(timeinfo),
                                     analyzeDuration,
                                     LOGANAL_URI,
                                     LOGANAL_VERSION);
    rc = xmlTextWriterWriteRaw(writer,
            BAD_CAST "<p>\
                <a href=\"http://validator.w3.org/check?uri=referer\"><img\
                   src=\"http://www.w3.org/Icons/valid-xhtml10\"\
                   alt=\"Valid XHTML 1.0 Strict\" height=\"31\" width=\"88\" /></a>\
                      </p>");

    rc = xmlTextWriterEndElement(writer); // div

    rc = xmlTextWriterEndElement(writer); // body

    rc = xmlTextWriterEndElement(writer); // html

    rc = xmlTextWriterEndDocument(writer);
    if (rc < 0) {
        printf
            ("testXmlwriterFilename: Error at xmlTextWriterEndDocument\n");
        //      return 1;
    }

    xmlFreeTextWriter(writer);
    return 1;
}

inline static int max(int a, int b) {
    if (a > b)
        return a;
    return b;
}

static int collectStats(const char *inFile, 
                        struct svnstats **stats, 
                        struct svnuser **users,
                        struct svnmonthstats **mstats,
                        struct svnmonthstats **dstats,
                        struct fileAges *fileAges)
{
    
    FILE *in;
    if (inFile) {
        in = fopen(inFile, "r");
        if (in == NULL)
        {
            fprintf(stderr, "Failed opening '%s'\n", inFile);
            return LOGANAL_CANNOT_OPEN_FILE;
        }
    } else {
        in = stdin;
    }
    char buf[BUFSIZ];
    int errcode = 0;
    
    *stats = malloc(sizeof(struct svnstats));

    if (*stats == NULL)
    {
        return LOGANAL_MEMORY_EXHAUSTED;
    }
   
    (*stats)->analyzeStartTime = time(NULL);
    
    (*stats)->totalrevisions = 0;
    (*stats)->revisions = 0;
    (*stats)->commentcount = 0;
    (*stats)->commentlines = 0;
    (*stats)->usercount = 0;
    (*stats)->modifyfilecount = 0;
    (*stats)->addfilecount = 0;
    (*stats)->delfilecount = 0;
    (*stats)->commentcharcount = 0;
    (*stats)->unique_node_count = 0;
    (*stats)->ignored_nodes = 0;
    (*stats)->unique_datepair_count = 0;
    (*stats)->workday_count = 0;
    (*stats)->overlapping_workday_count = 0;
    (*stats)->fileroot = NULL;
    (*stats)->parseState = SVN_PARSER_IDLE;

    int i;
    for (i = 0; i < 7; i++)
        (*stats)->revision_day_stats[i] = 0;

    struct svnuser *currentUser = NULL;
    
    int line = 0;
    while (fgets(buf, sizeof(buf), in))
    {
        line++;
        if ((errcode = _parseSvnStats(buf, 
                                      stats, 
                                      users, 
                                      mstats, 
                                      dstats,
                                      &currentUser,
                                      fileAges)) < 0)
        {
            fprintf(stderr, "Parse error in %s at line %d.\n",
                    (inFile ? inFile : "stdin"), 
                    line);
            break;
        }
    }

    currentUser = NULL;
    
    fclose(in);
    return (errcode < 0 ? errcode : 0);
}

static int _parseSvnStats(char *line, 
                          struct svnstats **stats,
                          struct svnuser **users,
                          struct svnmonthstats **mstats,
                          struct svnmonthstats **dstats,
                          struct svnuser **currentUser,
                          struct fileAges *fileAges)
{
    char *token = NULL, *token2 = NULL;
    int tokenindex = 0;
    int commentlength = 0;
    int pathLineLength;
    char* username;
    bool parseTime = false;
    struct tm *tm;
    static struct tm last_tm;
    static bool last_tm_ok;
    line = trim(line);
//    printf("%s\n", line);
    switch ((*stats)->parseState)
    {
    case SVN_PARSER_OPENED:
        // a stats-line should follow
        token = strtok(line, "|");
        if (token != NULL)
        {
            (*stats)->parseState = SVN_PARSER_OPEN;
        } else {
            fprintf(stderr, "An info line expected.\n");
            return LOGANAL_PARSE_ERROR;
        }
        
        while (token != NULL)
        {
            switch(tokenindex++)
            {
            case 0: // rev n.
//                        printf("Analyzing %s...", trim(token));
                if ((*stats)->revisions == 0)
                {
                    char *p = strpbrk(trim(token), "r");
                    if (p != NULL)
                    {
                        (*stats)->totalrevisions = atoi(p+1);
                    } else {
                        fprintf(stderr, "A revision number expected.\n");
                        return LOGANAL_PARSE_ERROR;
                    }
                }
                break;
            case 1: // username
                username = trim(token);
                break;
            case 2: // commit time
                tm = malloc(sizeof *tm);
                if (!tm) {
                    return LOGANAL_MEMORY_EXHAUSTED;
                }
                token = trim(token);
                parseTime = false;
                if (strptime(token, "%Y-%m-%d", tm))
                {
                    /*
                            printf("token %s\n", token);
                            printf("year: %d\n", tm->tm_year);
                            printf("month: %d\n", tm->tm_mon);
                            printf("day: %d\n", tm->tm_mday);
                    */
                    (*stats)->revision_day_stats[tm->tm_wday]++;
                    _updateMonthStats(mstats, tm, *stats, false);
                    _updateMonthStats(dstats, tm, *stats, true);
                    /* make a copy */
                    last_tm = *tm;
                    last_tm_ok = true;
                    parseTime = true;
                } else {
                    fprintf(stderr, "Could not parse date from '%s'\n", 
                            token);
                    last_tm_ok = false;
                    //FIXME stop here as well?
                }
                if (!parseTime) {
                    fprintf(stderr, "A commit time expected.\n");
                    free(tm);
                    return LOGANAL_PARSE_ERROR;
                }
                break;
            case 3: // comment lines
//                        commentlines = atol(trim(token)) - 1;
                parser_comment_lines = atoi(token);
                break;
            }
            token = strtok(NULL, "|");
        }

        if (tokenindex != 4) {
            fprintf(stderr, "Unexpected number of tokens in info line.\n");
            free(tm);
            return LOGANAL_PARSE_ERROR;
        }
        
        int x;
        *currentUser = add_userdata(users, username, &x);
        if (x == 1)
        {
            (*stats)->usercount++;
        } else if (x == -1) {
            free(tm);
            return LOGANAL_MEMORY_EXHAUSTED;
        }
        (*stats)->revisions++;
        (*currentUser)->tmp_commentcharcount = (*currentUser)->commentcharcount;
        /*
        printf("line: %s\n", line);
        printf("year: %d\n", tm->tm_year);
        printf("month: %d\n", tm->tm_mon);
        printf("day: %d\n", tm->tm_mday); */
        if (_updateMonthStats(&((*currentUser)->daystats), 
                              tm, 
                              NULL, 
                              true) == 1) {
            // a new working day was found for this user.
            (*currentUser)->workdays++;
            (*stats)->overlapping_workday_count++;
        }
        free(tm);
        return 0;
        
    case SVN_PARSER_OPEN:
        // changed paths (possibly), comments.
        if (strstr(line, "Changed paths:") != NULL)
        {
            (*stats)->parseState = SVN_PARSER_CHANGEDPATHS;
        } else {
            (*stats)->parseState = SVN_PARSER_COMMENT;
        }
        return 0;
        
    case SVN_PARSER_CHANGEDPATHS:
        // changed paths
        if ((pathLineLength = strlen(line)) == 0)
        {
            (*stats)->parseState = SVN_PARSER_COMMENT;
        } else {
            // found a changed path info line
            // first parse the file action (add, modify,...etc.)
            token2 = strtok(line, " ");
            if (token2 != NULL)
            {
                char flag;
                if (sscanf(token2, "%c", &flag) == 1)
                {
                    switch (flag)
                    {
                    case 'A': //add
                        (*currentUser)->addfilecount++;
                        (*stats)->addfilecount++;
                        // FIXME detect moves
// FORMAT: A /foo/bar (from /bar:100) <-- rev.nr.
/*
  Now, move /bar to /foo/bar
  (store; delete; add new with stored properties)
*/
                        break;
                    case 'M': //modify
                    case 'G': //merge
                    case 'R': //replace
                        (*currentUser)->modifyfilecount++;
                        (*stats)->modifyfilecount++;
                        break;
                    case 'D': //delete
                        (*currentUser)->delfilecount++;
                        (*stats)->delfilecount++;
                        break;
                    case 'C': //conflict
                        //todo
                        break;
                    }
                } else {
                    fprintf(stderr, "Expected a file-action symbol. Did not get it.\n");
                    return LOGANAL_PARSE_ERROR;
                }
            } else {
                // parse error
                fprintf(stderr, "Unexpectedly no tokens found in a file-action row.\n");
                return LOGANAL_PARSE_ERROR;
            }
            // then parse the file path
            token2 = strtok(NULL, " ");
            if (token2 == NULL)
            {
                // parse error: file path empty.
                fprintf(stderr, "Empty file path.\n");
                return LOGANAL_PARSE_ERROR;
            }
            
            // If path contains spaces, 
            // we have to obtain the remaining tokens as well.
            char pathBuf[pathLineLength];
            *pathBuf = '\0';
            int tokenLength = strlen(token2);
            strncat(pathBuf, token2, tokenLength);
            char *pathBufPtr = &pathBuf[tokenLength];
            while ((token2 = strtok(NULL, " ")) != NULL) {
                if (strstr(token2, "(from")) {
                    // It's not the path that is continuing.
                    // It's other information not needed here.
                    break;
                }
                *pathBufPtr++ = ' ';
                *pathBufPtr = '\0';
                tokenLength = strlen(token2);
                strncat(pathBuf, token2, tokenLength);
                pathBufPtr += tokenLength;
            }
            
//                printf("[PATH]: %s\n", pathBuf);
            
            /*
              if (strstr(token2, "src/web/help")) // todo get this from args
              {
                  (*stats)->ignored_nodes++;
                  break;
              }
            */
            Node *far_end = NULL;
            if (node_add(&((*stats)->fileroot), 
                         pathBuf, 
                         &far_end) > 0) {
                (*stats)->unique_node_count += 1;
            }
            
            if (far_end->custom == NULL)
            {
                //far_end->custom = malloc(sizeof(int));
                //*(int*)(far_end->custom) = 0;
                far_end->custom = malloc(sizeof(struct nodeCustomData));
                ((struct nodeCustomData*)(far_end->custom))->actioncount = 0;
                if (last_tm_ok) {
                    _updateFileAges(&last_tm, fileAges);
                }
//                ((struct nodeCustomData*)(far_end->custom))->newestRevisionChecked = false;
            } else {
            }
            if (far_end->custom)
            {
                //(*(int*)(far_end->custom))++;
                ((struct nodeCustomData*)(far_end->custom))->actioncount++;
            }
        }
        return 0;
        
    case SVN_PARSER_COMMENT:
        // possible comment
        //if (strcmp(SVN_LOG_SEPARATOR, line) != 0)
        if (parser_comment_lines > 0)
        {
            commentlength = strlen(line);
            (*stats)->commentcharcount += commentlength;
            (*currentUser)->commentcharcount += commentlength;
            (*stats)->commentlines++;
            (*currentUser)->commentlines++;
        }
        parser_comment_lines--;     
        break;
    case SVN_PARSER_IDLE:
        if (strcmp(SVN_LOG_SEPARATOR, line) != 0) {
            fprintf(stderr, "The log should start with a log separator.\n");
            return LOGANAL_PARSE_ERROR;
        }
        break;
    }
    
    if (strcmp(SVN_LOG_SEPARATOR, line) == 0)
    {
        switch ((*stats)->parseState)
        {
        case SVN_PARSER_IDLE:
            (*stats)->parseState = SVN_PARSER_OPENED;
            return 0;
        case SVN_PARSER_OPENED:
            (*stats)->parseState = SVN_PARSER_OPEN;
            return 0;
        case SVN_PARSER_COMMENT:
            if (parser_comment_lines >=0) {
                //there was a SVN_LOG_SEPARATOR in the comment itself
                break;
            }

            if ((*currentUser)->commentcharcount 
                > (*currentUser)->tmp_commentcharcount)
            {
                (*currentUser)->commentcount++;
                (*stats)->commentcount++;
            }
        case SVN_PARSER_OPEN:
        case SVN_PARSER_CHANGEDPATHS:
            (*stats)->parseState = SVN_PARSER_OPENED;
            return 0;
        }
    }
    return 0;
}

static int _updateFileAges(struct tm *tm,
                           struct fileAges *fileAges) {
    if (!tm || !fileAges)
        return 0;

    /* filling the missing fields */
    tm->tm_sec  = 0;
    tm->tm_min  = 0;
    tm->tm_hour = 0;
    tm->tm_isdst  = 0;

    time_t t = mktime(tm);
    if (t == -1) {
        return 0;
    }

    unsigned int *field;
    int age = time(NULL) - t;

    if (age < DAY_SECONDS*7) {
        field = &(fileAges->recent);
    } else if (age < MONTH_SECONDS) {
        field = &(fileAges->week);
    } else if (age < MONTH_SECONDS*3) {
        field = &(fileAges->month);
    } else if (age < MONTH_SECONDS*6) {
        field = &(fileAges->quarter);
    } else if (age < YEAR_SECONDS) {
        field = &(fileAges->half_a_year);
    } else {
        field = &(fileAges->year_or_more);
    }

    (*field)++;
    return 1;
}


static int _updateMonthStats(struct svnmonthstats **mstats,
                             struct tm *tm,
                             struct svnstats *stats,
                             bool uniqueDay)
    {
        struct svnmonthstats *ms;
        bool ret;
        if (*mstats == NULL)
        {
            // first addition
            if (!(ret = _createYearMonthPair(mstats, 
                                             tm, 
                                             stats,
                                             uniqueDay)))
            {
                return 0;
            }
            ms = *mstats;
        } else {

            bool dayCondition = true;
            if (uniqueDay) {
                dayCondition = (*mstats)->day == tm->tm_mday;
            }

            // check the queue tail for a match
            if ((*mstats)->year == tm->tm_year 
                && (*mstats)->month == tm->tm_mon
                && dayCondition)
            {
                ms = *mstats;
                ret = false; // no new date created
            } else {
            // new year-month pair was found
                if (!(ret = _createYearMonthPair(&ms, 
                                                 tm, 
                                                 stats,
                                                 uniqueDay)))
            {
                return 0;
            }
            // attaching pair to the list
            ms->prev = *mstats;
            (*mstats)->next = ms;
            // re-positioning list's tail to the new item
            *mstats = ms;
        }
    }
    ms->revisions++;
    return (ret ? 1 : -1);
}

static bool _createYearMonthPair(struct svnmonthstats **ms, 
                                 struct tm *tm,
                                 struct svnstats *stats,
                                 bool uniqueDay)
{
    *ms = malloc(sizeof(struct svnmonthstats));
    if (ms == NULL)
    {
        fprintf(stderr, "Couldn't malloc.\n");
        return false;
    }
    (*ms)->year = tm->tm_year;
    (*ms)->month = tm->tm_mon;
    (*ms)->day = tm->tm_mday;
    (*ms)->revisions = 0;
    (*ms)->next = NULL;
    (*ms)->prev = NULL;

    if (!stats) {
        // We are not collecting global stats.
        return true;
    }
    
    if (uniqueDay) {
        stats->workday_count++;
    } else {
        stats->unique_datepair_count++;
    }

    return true;
}

static struct svnuser *add_userdata(struct svnuser **users, 
                                    char *username, 
                                    int *retflag)
{
    struct svnuser *prev = NULL;
    struct svnuser *p = *users;
    struct svnuser *new = NULL;
    while (p != NULL && strcmp(p->username, username) != 0)
    {
        prev = p;
        p = p->next;
    }
    
    if (p == NULL)
    {
        // a new user found
        new = malloc(sizeof(struct svnuser));
        p = new;
        if (new == NULL)
        {
            *retflag = -1;
            return NULL;
        }
        if (prev == NULL)
        {
            *users = new;
        } else {
            prev->next = new;
        }
        new->username = malloc(strlen(username)+1);
        if (new->username == NULL)
        {
            free(new);
            *retflag = -1;
            return NULL;
        }
        strcpy(new->username, username);
        new->revisions = 0;
        new->commentcount = 0;
        new->commentlines = 0;
        new->addfilecount = 0;
        new->delfilecount = 0;
        new->modifyfilecount = 0;
        new->commentcharcount = 0;
        new->tmp_commentcharcount = 0;
        new->commentcount = 0;
        new->workdays = 0;
        new->daystats = NULL;
        new->next = NULL;
    }

    p->revisions++;

    *retflag = (new == NULL ? 0 : 1);
    return p;
}

char *trim(char *str)
{
    char *start;
    char *end = NULL;
    //leading
    while (isspace(*str++));
    
    start = --str;
    
    //trailing
    while (*str != '\0')
    {
        if (isspace(*str))
        {
            if (end == NULL)
            {
                end = str;
            }
        } else {
            end = NULL;
        }
        str++;
    }
    
    str = start;

    if (end != NULL) 
    {
        *end = '\0';
    }
    
    return str;
}

/*
static void _printUsers(struct svnuser *list)
{
    puts("------------------------");
    struct svnuser *p = list;
    
    while (p != NULL)
    {
        printf("[%s (revisions=%u, comments=%u, commentlines=%lu, commentchars=%lu, adds=%u, modifications=%u)]\n",
                p->username,
                p->revisions,
                p->commentcount,
                p->commentlines,
                p->commentcharcount,
                p->addfilecount,
                p->modifyfilecount);
        p = p->next;
    }
    puts("------------------------");
}
*/

/*
static void _printStats(struct svnstats *stats)
{
    printf("totalrevisions=%u, analyzedrevisions=%d, comments=%u, commentlines=%lu\ncommentchars=%lu, usercount=%d, adds=%u\nmodifications=%u\n",
            stats->totalrevisions, stats->revisions, stats->commentcount, 
            stats->commentlines, stats->commentcharcount, stats->usercount, 
            stats->addfilecount, stats->modifyfilecount);
    puts("------------------------");
} 
*/

static int _xmlWriteRow(xmlTextWriterPtr writer, 
                        int headermode, 
                        int cellcount, 
                        ...)
{
    va_list ap;
    char *val;
    int rc;
    int i;

    /* the second arg here is the last named argument of this function */
    va_start(ap, cellcount);
    
    rc = xmlTextWriterStartElement(writer, BAD_CAST "tr");
    
    for (i = 0; i < cellcount; i++) {
        val = va_arg(ap, char *);
        bool useHeader = false;
        if (i == 0) {
            if (headermode != HEADER_NONE) {
                useHeader = true;
            }
        } else {
            if (headermode == HEADER_HORIZONTAL) {
                useHeader = true;
            }
        }
        rc = xmlTextWriterStartElement(writer,
                                       useHeader ? BAD_CAST "th" : BAD_CAST "td");
        rc = xmlTextWriterWriteString(writer, BAD_CAST val);
        rc = xmlTextWriterEndElement(writer); //th or td
    }
    
    rc = xmlTextWriterEndElement(writer); //tr

    va_end(ap);

    return 1;
}

static const char* _iMonthToStr(int month) {
    return monthsAlpha[month % 12];
}

static const char* _iDayToStr(int day) {
    return daysAlpha[day % 7];
}

static int _divisor(int n) {
    return n == 0 ? 1 : n;
}

static struct svnmonthstats* _monthstats_tail(struct svnmonthstats *mstats) 
{
    if (!mstats) {
        return NULL;
    }

    while (mstats->next)
        mstats = mstats->next;

    return mstats;
}

static struct svnmonthstats* _monthstats_head(struct svnmonthstats *mstats) 
{
    if (!mstats) {
        return NULL;
    }

    while (mstats->prev)
        mstats = mstats->prev;

    return mstats;
}

static void init_fileAges(struct fileAges *fileAges) {
    fileAges->year_or_more = 0;
    fileAges->half_a_year = 0;
    fileAges->quarter = 0;
    fileAges->month = 0;
    fileAges->week = 0;
    fileAges->recent = 0;
}

void printUsage(const char* program) {
    puts("Analyzes Subversion (SVN) logs \
and produces XHTML output.\n");
    printf("Usage: %s [OPTIONS]\n",
           program);
    puts("OPTIONS may include:");
    puts("-f [INFILE], --file [INFILE]       Input SVN log file");
    puts("                                   If omitted, reads from stdin.");
    puts("-o [OUTFILE], --output [OUTFILE]   XHTML output will be written here.");
    puts("                                   If omitted, writes into stdout.");
    puts("-t [title], --title [TITLE]        Project title");
    puts("                                   If omitted, Anonymous is used.");
    puts("-h, --help                         Shows help and exits.");
    puts("");
    puts("Examples:");
    printf("  i)  svn -v log svn://localhost/MyProject|%s -t MyProject -o ~/public_html/myproject_stats.html\n",
           program);
    printf(" ii)  svn -v log svn://localhost/MyProject|%s -t MyProject > ~/public_html/myproject_stats.html\n",
            program);
    printf("iii) %s -t MyProject -f mylog.log -o ~/public_html/myproject_stats.html\n",
           program);
}

static void _freeMonthStats(struct svnmonthstats *mstats) {

    while (mstats->prev != NULL)
        mstats = mstats->prev;

    struct svnmonthstats *d;

    while (mstats != NULL)
    {
        d = mstats;
        mstats = mstats->next;
        free(d);
    }
}

int main(int argc, char **argv)
{
    char *inFilePath = NULL;
    char *outFilePath = NULL;

    /*
     * this initializes the library and check potential ABI mismatches
     * between the version it was compiled for and the actual shared
     * library used.
     */
    //LIBXML_TEST_VERSION    

    int c;
    while (1)
    {
        static struct option long_options[] =
        {
            {"file",    required_argument,  0,  'f'},
            {"output",  required_argument,  0,  'o'},
            {"title",   required_argument,  0,  't'},
            {"help",    no_argument,        0,  'h'},
            {0, 0, 0, 0}
        };

        int option_index = 0;
        c = getopt_long(argc, argv, "f:o:t:h", long_options, &option_index);

        if (c == -1)
        {
            break;
        }

        switch(c)
        {
        case 'f':
            inFilePath = optarg;
            break;
        case 'o':
            outFilePath = optarg;
            break;
        case 't':
            svn_title = optarg;
            break;
        case 'h':
            printUsage(argv[0]);
            return 0;
            break;
        case '?':
            return 1;
        default:
            abort();
        }
    }

    if (!svn_title) {
        if (inFilePath) {
            svn_title = inFilePath;
        } else {
            svn_title = "Anonymous";
        }
    }
    
    struct svnuser *users = NULL;
    struct svnstats *stats = NULL;
    struct svnmonthstats *mstats = NULL;
    struct svnmonthstats *dstats = NULL;
    struct fileAges *fileAges = malloc(sizeof(struct fileAges));

    if (!fileAges) {
        return LOGANAL_MEMORY_EXHAUSTED;
    }

    init_fileAges(fileAges);

    doStats(inFilePath, 
            outFilePath, 
            &stats, 
            &users, 
            &mstats,
            &dstats,
            fileAges);
    //FIXME OMG resources don't get freed
    //if a parse error occurred :P

    if (stats->fileroot)
    {
        node_delete(&stats->fileroot);
    }
    free(stats);
    stats = NULL;

    _freeMonthStats(mstats);
    _freeMonthStats(dstats);
    mstats = NULL;
    dstats = NULL;
    
    // cleanup users
    struct svnuser *p = users;
    while (users != NULL)
    {
        p = users;
        users = p->next;
        free(p->username);
        p->username = NULL;
        _freeMonthStats(p->daystats);
        p->daystats = NULL;
        free(p);
        p = NULL;
    }
    users = NULL;
    free(fileAges);
    return 0;
}
