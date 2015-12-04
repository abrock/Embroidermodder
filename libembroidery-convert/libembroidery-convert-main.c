#include "emb-reader-writer.h"
#include "emb-logging.h"
#include "emb-format.h"
#include "emb-stitch.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

void usage(void)
{
    EmbFormatList* formatList = 0;
    EmbFormatList* curFormat = 0;
    const char* extension = 0;
    const char* description = 0;
    char readerState;
    char writerState;
    int numReaders = 0;
    int numWriters = 0;
    printf(" _____________________________________________________________________________ \n");
    printf("|          _   _ ___  ___ _____ ___  ___   __  _ ___  ___ ___   _ _           |\n");
    printf("|         | | | | _ \\| __|     | _ \\| _ \\ /  \\| |   \\| __| _ \\ | | |          |\n");
    printf("|         | |_| | _ <| __| | | | _ <|   /| () | | |) | __|   / |__ |          |\n");
    printf("|         |___|_|___/|___|_|_|_|___/|_|\\_\\\\__/|_|___/|___|_|\\_\\|___|          |\n");
    printf("|                    ___  __  ___ _ _    _ ___ ___  _____                     |\n");
    printf("|                   / __|/  \\|   | | \\  / | __| _ \\|_   _|                    |\n");
    printf("|                  ( (__| () | | | |\\ \\/ /| __|   /  | |                      |\n");
    printf("|                   \\___|\\__/|_|___| \\__/ |___|_|\\_\\ |_|                      |\n");
#if defined(EMSCRIPTEN)
    printf("|                                                                             |\n");
    printf("|                           * Emscripten Version *                            |\n");
#endif
    printf("|_____________________________________________________________________________|\n");
    printf("|                                                                             |\n");
    printf("| Usage: libembroidery-convert fileToRead filesToWrite ...                    |\n");
    printf("|_____________________________________________________________________________|\n");
    printf("                                                                               \n");
    printf(" _____________________________________________________________________________ \n");
    printf("|                                                                             |\n");
    printf("| List of Formats                                                             |\n");
    printf("|_____________________________________________________________________________|\n");
    printf("|                                                                             |\n");
    printf("| 'S' = Yes, and is considered stable.                                        |\n");
    printf("| 'U' = Yes, but may be unstable.                                             |\n");
    printf("| ' ' = No.                                                                   |\n");
    printf("|_____________________________________________________________________________|\n");
    printf("|        |       |       |                                                    |\n");
    printf("| Format | Read  | Write | Description                                        |\n");
    printf("|________|_______|_______|____________________________________________________|\n");
    printf("|        |       |       |                                                    |\n");

    formatList = embFormatList_create();
    if(!formatList) { embLog_error("libembroidery-convert-main.c usage(), cannot allocate memory for formatList\n"); return; }
    curFormat = formatList;
    while(curFormat)
    {
        extension = embFormat_extension(curFormat);
        description = embFormat_description(curFormat);
        readerState = embFormat_readerState(curFormat);
        writerState = embFormat_writerState(curFormat);

        numReaders += readerState != ' '? 1 : 0;
        numWriters += writerState != ' '? 1 : 0;
        printf("|  %-4s  |   %c   |   %c   |  %-49s |\n", extension, readerState, writerState, description);

        curFormat = curFormat->next;
    }
    embFormatList_free(formatList);
    formatList = 0;

    printf("|________|_______|_______|____________________________________________________|\n");
    printf("|        |       |       |                                                    |\n");
    printf("| Total: |  %3d  |  %3d  |                                                    |\n", numReaders, numWriters);
    printf("|________|_______|_______|____________________________________________________|\n");
    printf("|                                                                             |\n");
    printf("|                         http://embroidermodder.org                          |\n");
    printf("|_____________________________________________________________________________|\n");
    printf("\n");
}

/* TODO: Add capability for converting multiple files of various types to a single format. Currently, we only convert a single file to multiple formats. */

/*! Developers incorporating libembroidery into another project should use the SHORT_WAY of using libembroidery. It uses
 *  convenience functions and is approximately 20 lines shorter than the long way.
 *
 *  Developers who are interested improving libembroidery or using it to its fullest extent should use the LONG_WAY.
 *  It essentially does the same work the convenience function do.
 *
 *  Both methods are acceptable and it is up to you which to choose. Both will stay here for regression testing.
 */

#define SHORT_WAY

/** 
 * Calculate euclidean distance of two stitches given as EmbStitchList_*
 */
double distance(const struct EmbStitchList_ * a, const struct EmbStitchList_ * b) {
    const double dx = a->stitch.xx - b->stitch.xx;
    const double dy = a->stitch.yy - b->stitch.yy;
    return sqrt(dx*dx + dy*dy);
}

/**
 * Calculate the distance between a point and a straight line segment.
 *
 * @param lineA Beginning of the line segment.
 * @param lineB End of the line segment.
 * @param pointC
 */
double lineDeviation(const struct EmbStitchList_ * lineA, const struct EmbStitchList_ * lineB, const struct EmbStitchList_ * pointC) {
    /* All this is for exact calculation, until we figure it out we just use triangle inequality at the bottom */
    /*
    const double line_dx = lineB->stitch.xx - lineA->stitch.xx;
    const double line_dy = lineB->stitch.yy - lineA->stitch.yy;
    const double point_dx = pointC->stitch.xx - lineA->stitch.xx;
    const double point_dy = pointC->stitch.yy - lineA->stitch.yy;
*/
    /* Measure distance between (infinitely long) line and point*/
/*
    const double lineDistance = (point_dx * line_dy - point_dy * line_dx)/sqrt(line_dx*line_dx + line_dy*line_dy);
    */


    /* This is an upper limit for the distance, the formula is obtained by the triangle inequality */
    return (distance(lineA, pointC) + distance(pointC, lineB) - distance(lineA, lineB))/2;
}

/**
 * Replace multiple short stitches which (approximately) lie on a straight line by fewer stitches.
 *
 * @param p Pattern for which the optimization shall be done.
 * @param maxErr Maximum deviation from the straight line we are going to tolerate.
 * @param maxLength Maximum stitch length we want to create by removing stitches.
 */
void simplifyStraightLines(EmbPattern* p, const double maxErr, const double maxLength) {
    struct EmbStitchList_ * currentStitch = p->stitchList;
    int stitchCounter = 0;
    int normalStitchCounter = 0;
    int removedCounter = 0;
    struct EmbStitchList_ * one   = 0;
    struct EmbStitchList_ * two   = 0;
    struct EmbStitchList_ * three = 0;
    while (0 != currentStitch) {
        /* Count normal stitches separately */
        if (NORMAL == currentStitch->stitch.flags) {
            normalStitchCounter++;
        }
        /* Re-name the four stitches beginning with the current stitch for clarity */
        one = currentStitch;
        two = one->next;
        if (0 == two) {
            stitchCounter++;
            currentStitch = currentStitch->next;
            continue;
        }
        three = two->next;
        if (0 == three) {
            stitchCounter++;
            currentStitch = currentStitch->next;
            continue;
        }

        /* Check conditions under which we want to remove the middle stitch */
        if (
               NORMAL == one->stitch.flags /* All three stitches need to be purely normal stitches, no trimming involved */
            && NORMAL == two->stitch.flags
            && NORMAL == three->stitch.flags
            && distance(one, three) <= maxLength /* The distance between 1 and 3 must be smaller or equal the maximum stitch length we allow */
            && lineDeviation(one, three, two) <= maxErr /* The estimated maximum distance between the middle stitch and the line segment connecting the stitches 1 and three must be smaller or equal our maximum error threshold */
        ) {
            currentStitch->next = three;
            removedCounter++;
            stitchCounter++;
            continue;
        }

        currentStitch = currentStitch->next;
        stitchCounter++;
    }
    printf("Stiches: %d\nNormal stitches: %d\nRemoved: %d\n", stitchCounter, normalStitchCounter, removedCounter);

}

/** This function deletes stitches from an EmbPattern such that the resulting pattern does not contain any stitches shorter than a given threshold.
 * @param p The pattern which might contain very short stitches. These stitches are removed.
 * @param threshold The minimum stitch length to be enforced.
 */
void removeSmallStitches(EmbPattern* p, const double threshold) {
    struct EmbStitchList_ * currentStitch = p->stitchList;
    int stitchCounter = 0;
    int normalStitchCounter = 0;
    int removedCounter = 0;
    struct EmbStitchList_ * one   = 0;
    struct EmbStitchList_ * two   = 0;
    struct EmbStitchList_ * three = 0;
    struct EmbStitchList_ * four  = 0;
    while (0 != currentStitch) {
        /* Count normal stitches separately */
        if (NORMAL == currentStitch->stitch.flags) {
            normalStitchCounter++;
        }
        /* Re-name the four stitches beginning with the current stitch for clarity */
        one = currentStitch;
        two = one->next;
        if (0 == two) {
            stitchCounter++;
            currentStitch = currentStitch->next;
            continue;
        }
        three = two->next;
        if (0 == three) {
            stitchCounter++;
            currentStitch = currentStitch->next;
            continue;
        }
        four = three->next;
        if (0 == four) {
            stitchCounter++;
            currentStitch = currentStitch->next;
            continue;
        }

        if (NORMAL == one->stitch.flags && NORMAL == two->stitch.flags && NORMAL == three->stitch.flags && NORMAL == four->stitch.flags && distance(two, three) < threshold) {
            /* We want to maximize the overall length of the embroidery so we don't loose detail at curves. */
            if (distance(one, three) + distance(three, four) > distance(one, two) + distance(two, four)) {
                currentStitch->next = three;
            }
            else {
                two->next = four;
            }
            removedCounter++;
            stitchCounter++;
            continue;
        }

        stitchCounter++;
        currentStitch = currentStitch->next;
    }
    printf("Stiches: %d\nNormal stitches: %d\nRemoved: %d\n", stitchCounter, normalStitchCounter, removedCounter);
}

int main(int argc, const char* argv[])
{
    EmbPattern* p = 0;
    int successful = 0, i = 0;
    int formatType;
    const double threshold = 0.7;
#ifdef SHORT_WAY
    if(argc < 3)
    {
        usage();
        exit(0);
    }

    p = embPattern_create();
    if(!p) { embLog_error("libembroidery-convert-main.c main(), cannot allocate memory for p\n"); exit(1); }

    successful = embPattern_read(p, argv[1]);
    if(!successful)
    {
        embLog_error("libembroidery-convert-main.c main(), reading file %s was unsuccessful\n", argv[1]);
        embPattern_free(p);
        exit(1);
    }
    /* Remove stitches shorter than threshold. */
    printf("\nAttempting to remove very short stitches\n");
    removeSmallStitches(p, threshold);

    printf("\nAttempting to simplify straight lines\n");
    simplifyStraightLines(p, 0.1, 3.6),
    
    printf("\nAttempting to remove very short stitches a second time\n");
    removeSmallStitches(p, threshold);
    
    printf("\nAttempting to remove very short stitches a third time\n");
    removeSmallStitches(p, threshold);

    printf("\nAttempting to remove very short stitches a fourth time\n");
    removeSmallStitches(p, threshold);


    formatType = embFormat_typeFromName(argv[1]);
    if(formatType == EMBFORMAT_OBJECTONLY && argc == 3) /* TODO: fix this to work when writing multiple files */
    {
        formatType = embFormat_typeFromName(argv[2]);
        if(formatType == EMBFORMAT_STITCHONLY)
            embPattern_movePolylinesToStitchList(p);
    }

    i = 2;
    for(i = 2; i < argc; i++)
    {
        successful = embPattern_write(p, argv[i]);
        if(!successful)
            embLog_error("libembroidery-convert-main.c main(), writing file %s was unsuccessful\n", argv[i]);
    }

    embPattern_free(p);
    return 0;
#else /* LONG_WAY */

    EmbReaderWriter* reader = 0, *writer = 0;

    if(argc < 3)
    {
        usage();
        exit(0);
    }

    p = embPattern_create();
    if(!p) { embLog_error("libembroidery-convert-main.c main(), cannot allocate memory for p\n"); exit(1); }

    successful = 0;
    reader = embReaderWriter_getByFileName(argv[1]);
    if(!reader)
    {
        successful = 0;
        embLog_error("libembroidery-convert-main.c main(), unsupported read file type: %s\n", argv[1]);
    }
    else
    {
        successful = reader->reader(p, argv[1]);
        if(!successful) embLog_error("libembroidery-convert-main.c main(), reading file was unsuccessful: %s\n", argv[1]);
    }
    free(reader);
    if(!successful)
    {
        embPattern_free(p);
        exit(1);
    }

    formatType = embFormat_typeFromName(argv[1]);
    if(formatType == EMBFORMAT_OBJECTONLY && argc == 3) /* TODO: fix this to work when writing multiple files */
    {
        formatType = embFormat_typeFromName(argv[2]);
        if(formatType == EMBFORMAT_STITCHONLY)
            embPattern_movePolylinesToStitchList(p);
    }

    i = 2;
    for(i = 2; i < argc; i++)
    {
        writer = embReaderWriter_getByFileName(argv[i]);
        if(!writer)
        {
            embLog_error("libembroidery-convert-main.c main(), unsupported write file type: %s\n", argv[i]);
        }
        else
        {
            successful = writer->writer(p, argv[i]);
            if(!successful)
                embLog_error("libembroidery-convert-main.c main(), writing file %s was unsuccessful\n", argv[i]);
        }
        free(writer);
    }

    embPattern_free(p);
    return 0;
#endif /* SHORT_WAY */

}

/* kate: bom off; indent-mode cstyle; indent-width 4; replace-trailing-space-save on; */
