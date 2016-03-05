#include "format-sew.h"
#include "format-jef.h"
#include "emb-file.h"
#include "emb-logging.h"
#include "helpers-binary.h"
#include <math.h>
#include <stdlib.h>

static char sewDecode(unsigned char inputByte)
{
    return (inputByte >= 0x80) ? (char) (-~(inputByte - 1)) : (char) inputByte; /* TODO: fix return statement */
}

/*! Reads a file with the given \a fileName and loads the data into \a pattern.
 *  Returns \c true if successful, otherwise returns \c false. */
int readSew(EmbPattern* pattern, const char* fileName)
{
    EmbFile* file = 0;
    int i;
    int fileLength;
    char dx = 0, dy = 0;
    int flags;
    int numberOfColors;
    char thisStitchIsJump = 0;

    if(!pattern) { embLog_error("format-sew.c readSew(), pattern argument is null\n"); return 0; }
    if(!fileName) { embLog_error("format-sew.c readSew(), fileName argument is null\n"); return 0; }

    file = embFile_open(fileName, "rb");
    if(!file)
    {
        embLog_error("format-sew.c readSew(), cannot open %s for reading\n", fileName);
        return 0;
    }

    embFile_seek(file, 0x00, SEEK_END);
    fileLength = embFile_tell(file);
    embFile_seek(file, 0x00, SEEK_SET);
    numberOfColors = binaryReadByte(file);
    numberOfColors += (binaryReadByte(file) << 8);

    for(i = 0; i < numberOfColors; i++)
    {
        embPattern_addThread(pattern, jefThreads[binaryReadInt16(file)]);
    }
    embFile_seek(file, 0x1D78, SEEK_SET);

    for(i = 0; embFile_tell(file) < fileLength; i++)
    {
        unsigned char b0 = binaryReadByte(file);
        unsigned char b1 = binaryReadByte(file);

        flags = EM_NORMAL;
        if(thisStitchIsJump)
        {
            flags = TRIM;
            thisStitchIsJump = 0;
        }
        if(b0 == 0x80)
        {
            if(b1 == 1)
            {
                b0 = binaryReadByte(file);
                b1 = binaryReadByte(file);
                flags = STOP;
            }
            else if((b1 == 0x02) || (b1 == 0x04))
            {
                thisStitchIsJump = 1;
                b0 = binaryReadByte(file);
                b1 = binaryReadByte(file);
                flags = TRIM;
            }
            else if(b1 == 0x10)
            {
               break;
            }
        }
        dx = sewDecode(b0);
        dy = sewDecode(b1);
        if(abs(dx) == 127 || abs(dy) == 127)
        {
            thisStitchIsJump = 1;
            flags = TRIM;
        }

        embPattern_addStitchRel(pattern, dx / 10.0, dy / 10.0, flags, 1);
    }
    printf("current position: %ld\n", embFile_tell(file));
    embFile_close(file);

    /* Check for an END stitch and add one if it is not present */
    if(pattern->lastStitch->stitch.flags != END)
        embPattern_addStitchRel(pattern, 0, 0, END, 1);

    return 1;
}

/*! Writes the data from \a pattern to a file with the given \a fileName.
 *  Returns \c true if successful, otherwise returns \c false. */
int writeSew(EmbPattern* pattern, const char* fileName)
{
    if(!pattern) { embLog_error("format-sew.c writeSew(), pattern argument is null\n"); return 0; }
    if(!fileName) { embLog_error("format-sew.c writeSew(), fileName argument is null\n"); return 0; }

    if(!embStitchList_count(pattern->stitchList))
    {
        embLog_error("format-sew.c writeSew(), pattern contains no stitches\n");
        return 0;
    }

    /* Check for an END stitch and add one if it is not present */
    if(pattern->lastStitch->stitch.flags != END)
        embPattern_addStitchRel(pattern, 0, 0, END, 1);

    /* TODO: embFile_open() needs to occur here after the check for no stitches */

    return 0; /*TODO: finish writeSew */
}

/* kate: bom off; indent-mode cstyle; indent-width 4; replace-trailing-space-save on; */
