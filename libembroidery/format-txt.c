#include "format-txt.h"
#include "emb-file.h"
#include "emb-logging.h"
#include "helpers-misc.h"
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

/*! Reads a file with the given \a fileName and loads the data into \a pattern.
 *  Returns \c true if successful, otherwise returns \c false. */
int readTxt(EmbPattern* pattern, const char* fileName)
{
    FILE* input;
#define maxLineLength 4096
    char line[maxLineLength];
    size_t lineNumber = 0;
    int flags = 0, color = 0;
    float x = 0.0, y = 0.0;
    int lastColor = -1;
    if(!pattern) { embLog_error("format-txt.c readTxt(), pattern argument is null\n file %s \n line %u\n", __FILE__, __LINE__); return 0; }
    if(!fileName) { embLog_error("format-txt.c readTxt(), fileName argument is null\n file %s \n line %u\n", __FILE__, __LINE__); return 0; }

    input = fopen(fileName, "r");
    if (!input) {
        embLog_error("Input file %s could not be opened for reading\n file %s \n line %u\n", fileName, __FILE__, __LINE__);
    }
    /* We ignore the first line, it only contains the number of stitches, we don't need that. */
    fgets(line, maxLineLength, input);

    while (NULL != fgets(line, maxLineLength, input)) {

        lineNumber++;
        if (strlen(line)+1 >= maxLineLength) {
            embLog_error("Line number %u in input file %s exceeds %u characters\n file %s \n line %u\n", lineNumber, fileName, maxLineLength, __FILE__, __LINE__);
            return 0;
        }
        /* Example line: 0.0,-0.0 color:0 flags:1 */
        sscanf(line, "%f,%f color:%d flags:%d", &x, &y, &color, &flags);
        if(color != lastColor) {
            lastColor = color;
            embPattern_changeColor(pattern, color);
            flags |= STOP | TRIM;
        }
        embPattern_addStitchAbs(pattern, x, y, flags, 0);
    }

    /* Check for an END stitch and add one if it is not present */
    if(pattern->lastStitch->stitch.flags != END)
        embPattern_addStitchAbs(pattern, x, y, END, 1);
    return 1; /*TODO: finish readTxt */
}

/*! Writes the data from \a pattern to a file with the given \a fileName.
 *  Returns \c true if successful, otherwise returns \c false. */
int writeTxt(EmbPattern* pattern, const char* fileName)
{
    EmbStitchList* pointer = 0;
    EmbFile* file = 0;

    if(!pattern) { embLog_error("format-txt.c writeTxt(), pattern argument is null\n"); return 0; }
    if(!fileName) { embLog_error("format-txt.c writeTxt(), fileName argument is null\n"); return 0; }

    if(!embStitchList_count(pattern->stitchList))
    {
        embLog_error("format-txt.c writeTxt(), pattern contains no stitches\n");
        return 0;
    }

    /* Check for an END stitch and add one if it is not present */
    if(pattern->lastStitch->stitch.flags != END)
        embPattern_addStitchRel(pattern, 0, 0, END, 1);

    file = embFile_open(fileName, "w");
    if(!file)
    {
        embLog_error("format-txt.c writeTxt(), cannot open %s for writing\n", fileName);
        return 0;
    }
    pointer = pattern->stitchList;
    embFile_printf(file, "%u\n", (unsigned int) embStitchList_count(pointer));

    while(pointer)
    {
        EmbStitch s = pointer->stitch;
        embFile_printf(file, "%.1f,%.1f color:%i flags:%i\n", s.xx, s.yy, s.color, s.flags);
        pointer = pointer->next;
    }

    embFile_close(file);
    return 1;
}

/* kate: bom off; indent-mode cstyle; indent-width 4; replace-trailing-space-save on; */
