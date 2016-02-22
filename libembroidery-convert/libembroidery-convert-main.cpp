#include "emb-reader-writer.h"
#include "emb-logging.h"
#include "emb-format.h"
#include "emb-stitch.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <iostream>

#define false 0
#define true 1

#ifndef M_PI
#define M_PI           3.14159265358979323846
#endif

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
    /**/
    const double line_dx = lineB->stitch.xx - lineA->stitch.xx;
    const double line_dy = lineB->stitch.yy - lineA->stitch.yy;
    const double point_dx = pointC->stitch.xx - lineA->stitch.xx;
    const double point_dy = pointC->stitch.yy - lineA->stitch.yy;
/**/
    /* Measure distance between (infinitely long) line and point*/
/*
    const double lineDistance = (point_dx * line_dy - point_dy * line_dx)/sqrt(line_dx*line_dx + line_dy*line_dy);
    */


    /* Check if the vectors lineB - lineA and pointC - lineA (the vectors mapping lineA to lineB and pointC, respectively) point in the same direction. If not, resort to rough estimation*/

    if (line_dx * point_dx + line_dy * point_dy < 0) {
        if (distance(lineA, pointC) < distance(lineB, pointC)) {
            return distance(lineA, pointC);
        }
        else {
            return distance(lineB, pointC);
        }
    }

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
    struct EmbStitchList_ * four  = 0;
    while (
           0 != currentStitch
        && 0 != currentStitch->next
        && 0 != currentStitch->next->next
        && 0 != currentStitch->next->next->next
        && 0 != currentStitch->next->next->next->next
        && 0 != currentStitch->next->next->next->next->next
        && 0 != currentStitch->next->next->next->next->next->next
        ) {
        /* Count normal stitches separately */
        if (NORMAL == currentStitch->stitch.flags) {
            normalStitchCounter++;
        }
        /* Re-name the four stitches beginning with the current stitch for clarity */
        one = currentStitch->next;
        two = one->next;
        three = two->next;
        four = three->next;

        /* Check conditions under which we want to remove the middle stitch */
        if (
               NORMAL == currentStitch->stitch.flags
            && NORMAL == one->stitch.flags /* All three stitches need to be purely normal stitches, no trimming involved */
            && NORMAL == two->stitch.flags
            && NORMAL == three->stitch.flags
            && NORMAL == four->stitch.flags
            && distance(one, three) <= maxLength /* The distance between 1 and 3 must be smaller or equal the maximum stitch length we allow */
            && lineDeviation(one, three, two) <= maxErr /* The estimated maximum distance between the middle stitch and the line segment connecting the stitches 1 and three must be smaller or equal our maximum error threshold */
        ) {
            one->next = three;
            currentStitch = three;
            removedCounter++;
            stitchCounter++;
            continue;
        }

        currentStitch = currentStitch->next;
        stitchCounter++;
    }
    printf("Stitches: %d\nNormal stitches: %d\nRemoved: %d\n", stitchCounter, normalStitchCounter, removedCounter);

}

/**
 * Count the number of stitches until the end of a list.
 *
 * @param start The beginning of a list or some stitch in the middle if you want to start counting in the middle.
 * @return The number of stitches including the start.
 */
int countStitches(struct EmbStitchList_ * const start) {
    struct EmbStitchList_ * current = start;
    int counter = 0;
    while (0 != current) {
        counter++;
        current = current->next;
    }
    return counter;
}

/**
 * Find the stitch with maximum length and return it's length.
 */
double maxLength(struct EmbStitchList_ * p) {
    struct EmbStitchList_ * currentStitch = p;
    double max_length = 0;
    double currentLength = 0;
    while (currentStitch && currentStitch->next) {
        if (NORMAL == currentStitch->stitch.flags && NORMAL == currentStitch->next->stitch.flags) {
            currentLength = distance(currentStitch, currentStitch->next);
            if (currentLength > max_length) {
                max_length = currentLength;
            }
        }
        currentStitch = currentStitch->next;
    }
    return max_length;
}

/**
 * Count the number of normal stitches until the end of a list.
 *
 * @param start The beginning of a list or some stitch in the middle if you want to start counting in the middle.
 * @return The number of normal stitches including the start.
 */
int countNormalStitches(struct EmbStitchList_ * const start) {
    struct EmbStitchList_ * current = start;
    int counter = 0;
    while (0 != current) {
        if (NORMAL == current->stitch.flags) {
            counter++;
        }
        current = current->next;
    }
    return counter;
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
    struct EmbStitchList_ * five  = 0;
    while (0 != currentStitch
        && 0 != currentStitch->next
        && 0 != currentStitch->next->next
        && 0 != currentStitch->next->next->next
        && 0 != currentStitch->next->next->next->next
        && 0 != currentStitch->next->next->next->next->next
        && 0 != currentStitch->next->next->next->next->next->next
   ) {
        /* Count normal stitches separately */
        if (NORMAL == currentStitch->stitch.flags) {
            normalStitchCounter++;
        }
        /* Re-name the four stitches beginning with the current stitch for clarity */
        one = currentStitch->next;
        two = one->next;
        three = two->next;
        four = three->next;
        five = four->next;

        if (
               NORMAL == currentStitch->stitch.flags
            && NORMAL == one->stitch.flags
            && (JUMP == two->stitch.flags   || NORMAL == two->stitch.flags)
            && (JUMP == three->stitch.flags || NORMAL == three->stitch.flags)
            && NORMAL == four->stitch.flags
            && NORMAL == five->stitch.flags
            && currentStitch->stitch.color == five->stitch.color
            && distance(two, three) < threshold) {
            /* We want to maximize the overall length of the embroidery so we don't loose detail at curves. */
            
            if (distance(one, three) + distance(three, four) > distance(one, two) + distance(two, four)) {
                one->next = three;
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
    printf("Stitches: %d\nNormal stitches: %d\nRemoved: %d\n", stitchCounter, normalStitchCounter, removedCounter);
}

/**
 * Compute the angle between two consecutive lines given by three consecutive stitches.
 *
 * @param x The first of the three consecutive stitches which should be considered.
 * @return the angle between the stitches in degree. Angles smaller than 90° mean that the motive changes direction, very small angles are a sign for a area filled by satin stitches.
 */
double angle(const struct EmbStitchList_* x) {
    double dx1, dx2, dy1, dy2, cosalpha;
    if (0 == x || 0 == x->next || 0 == x->next->next) {
        return 0;
    }

    dx1 = x->stitch.xx - x->next->stitch.xx;
    dy1 = x->stitch.yy - x->next->stitch.yy;

    dx2 = x->next->next->stitch.xx - x->next->stitch.xx;
    dy2 = x->next->next->stitch.yy - x->next->stitch.yy;

    cosalpha = (dx1 * dx2 + dy1 * dy2)/sqrt((dx1*dx1 + dy1*dy1)*(dx2*dx2 + dy2*dy2));

    return acos(cosalpha)/M_PI*180;
}

/** Detects if the current stitch is a satin stitch by testing the length of the stitch and the next stitch and computing the angle.
 *
 * @param x The stitch where the test should start. This and the two successors are considered in this function.
 * @param minSatinLength The minimum length for the detection of satin stitches.
 * @param maxAngle Maximum angle in degree between two stitches for the detection of satin stitches
 * @param minDensity The minimum density of the area in stitches per mm.
 */
int isSatinStitch(const struct EmbStitchList_* x, const double minSatinLength, const double maxAngle, const double minDensity) {
    if (0 == x || 0 == x->next || 0 == x->next->next) {
        return false;
    }

    if (NORMAL != x->stitch.flags || NORMAL != x->next->stitch.flags || NORMAL != x->next->next->stitch.flags) {
        return false;
    }

    if (distance(x, x->next) < minSatinLength || distance (x->next, x->next->next) < minSatinLength) {
        return false;
    }

    if (abs(angle(x)) > maxAngle) {
        return false;
    }
    if (x->stitch.color != x->next->stitch.color || x->stitch.color != x->next->next->stitch.color) {
        return false;
    }
    if (distance(x, x->next->next) > 2.0/minDensity) {
        return false;
    }

    return true;
}

/**
 * Check if the current stitch marks the beginning (or middle) of a area filled by satin stitches.
 *
 * @param start The stitch where the testing should start. This and the following minSatinCount + 2 Stitches are considered in the test.
 * @param minSatinLength The minimum length for the detection of satin stitches.
 * @param minSatinCount The minimum number of consecutive satin stitches for the detection of a area filled by satin stitches
 * @param maxAngle Maximum angle in degree between two stitches for the detection of satin stitches
 */
int isSatinArea(struct EmbStitchList_ * const start, const double minSatinLength, const int minSatinCount, const double maxAngle, const double minDensity) {
    struct EmbStitchList_* current = start;
    int ii = 0;
    const int flags = start->stitch.flags;
    const int color = start->stitch.color;
    for (ii = 1; ii < minSatinCount; ++ii) {
        if (0 == current || !isSatinStitch(current, minSatinLength, maxAngle, minDensity) || color != current->stitch.color || flags != current->stitch.flags) {
            return false;
        }
        current = current->next;
    }
    return true;
}

/**
 * Calculate the mean position of two stitches and store the result in a third stitch.
 *
 * @param result The result of the cacluation.
 * @param a One of the two stitches for which the mean should be calculated.
 * @param b The other of the two stitches.
 */
void calculateMean(struct EmbStitchList_* result, const struct EmbStitchList_* a, const struct EmbStitchList_* b) {
    result->stitch.xx = (a->stitch.xx + b->stitch.xx)/2;
    result->stitch.yy = (a->stitch.yy + b->stitch.yy)/2;
}

/**
 * Given two stitches move both of them closer to each other by a certain distance.
 *
 * @param a Stitch a.
 * @param b Stitch b.
 * @param distance Both stitches are moved by this distance.
 */
void moveCloser(struct EmbStitchList_* a, struct EmbStitchList_* b, const double dist) {
    double dist_b_a = distance(b,a);
    const double b_a_xx = b->stitch.xx - a->stitch.xx;
    const double b_a_yy = b->stitch.yy - a->stitch.yy;
    
    a->stitch.xx += b_a_xx * dist / dist_b_a;
    a->stitch.yy += b_a_yy * dist / dist_b_a;
    
    b->stitch.xx -= b_a_xx * dist / dist_b_a;
    b->stitch.yy -= b_a_yy * dist / dist_b_a;
}

/**
 * Allocate memory for a stitch and clone the position, color and flags content.
 *
 *
 */
struct EmbStitchList_* clone(const struct EmbStitchList_* x) {
    struct EmbStitchList_* result = (EmbStitchList_*)malloc(sizeof *result);
    result->stitch.xx = x->stitch.xx;
    result->stitch.yy = x->stitch.yy;
    result->stitch.flags = x->stitch.flags;
    result->stitch.color = x->stitch.color;
    return result;
}

struct EmbStitchList_* addSingleUnderSewing(struct EmbStitchList_* const start, const double minStitchLength, const double minSatinLength, const double maxAngle, const double safetyDistance, const double minDensity, int * addedStitches, int * satinAreaLength) {
    const int color = start->stitch.color;
    struct EmbStitchList_* firstOpposing; 
    struct EmbStitchList_* lastAdjacent; 
    struct EmbStitchList_* prevOpposing;
    struct EmbStitchList_* prevAdjacent;
    struct EmbStitchList_* newOpposing;
    struct EmbStitchList_* newAdjacent;
    struct EmbStitchList_* satinAreaBeginning = start->next; 
    struct EmbStitchList_* currentStitch = start;
    struct EmbStitchList_* tmpStitch;
    (*satinAreaLength) = 0;
    *addedStitches = 0;
    if (0 == start || 0 == start->next || 0 == start->next->next || 0 == start->next->next->next || !isSatinStitch(currentStitch, minSatinLength, maxAngle, minDensity)) {
        return start;
    }
    firstOpposing = clone(currentStitch);
    calculateMean(firstOpposing, currentStitch->next, currentStitch->next->next->next);
    
    lastAdjacent = clone(currentStitch);
    calculateMean(lastAdjacent, currentStitch, currentStitch->next->next);

    moveCloser(firstOpposing, lastAdjacent, safetyDistance);

    lastAdjacent->next = clone(firstOpposing);
    lastAdjacent->next->next = satinAreaBeginning;

    prevOpposing = firstOpposing;
    prevAdjacent = lastAdjacent;
    
    newOpposing = clone(prevOpposing);
    newAdjacent = clone(prevAdjacent);

    while (isSatinStitch(currentStitch, minSatinLength, maxAngle, minDensity) && color == currentStitch->stitch.color) {
        calculateMean(newOpposing, currentStitch->next, currentStitch->next->next->next);
        calculateMean(newAdjacent, currentStitch, currentStitch->next->next);
        moveCloser(newOpposing, newAdjacent, safetyDistance);
        if (distance(newOpposing, prevOpposing) > minStitchLength) {
            prevOpposing->next = newOpposing;
            prevOpposing = newOpposing;
            newOpposing = clone(newOpposing);   
            (*addedStitches)++;
        }
        if (distance(newAdjacent, prevAdjacent) > minStitchLength) {
            newAdjacent->next = prevAdjacent;
            prevAdjacent = newAdjacent;
            newAdjacent = clone(newAdjacent);
            (*addedStitches)++;
        }
        calculateMean(newOpposing, currentStitch->next, currentStitch->next->next->next);
        calculateMean(newAdjacent, currentStitch, currentStitch->next->next);
        moveCloser(newOpposing, newAdjacent, safetyDistance);


        currentStitch = currentStitch->next->next;
        (*satinAreaLength) += 2;
    }
    /* Add the final stitches. Don't care about their length, if they are too short they will be automatically deleted by the optimizer */
    prevOpposing->next = newOpposing;
    prevOpposing = newOpposing;
    newAdjacent->next = prevAdjacent;
    prevAdjacent = newAdjacent;
    (*addedStitches) += 5;

    /* Finally stitch the beginning and end together */
    start->next = firstOpposing;
    
    /* Check if the stitches are too far away, introduce a middle stitch if necessary */
    if (distance(prevOpposing, prevAdjacent) > 2*minStitchLength) {
        tmpStitch = clone(prevOpposing);
        calculateMean(tmpStitch, prevOpposing, prevAdjacent);
        prevOpposing->next = tmpStitch;
        tmpStitch->next = prevAdjacent;
    }
    else {
        prevOpposing->next = prevAdjacent;
    }
           
    printf("Found satin area of length %d, added %d stitches\n", *satinAreaLength, *addedStitches);

    return currentStitch;
}

/**
 * Adds under sewing to the parts of a pattern where satin stich (zick-zack) is used.
 *
 * @param p Pattern to which the under sewing should be added.
 * @param minStitchLength The minimum stitch length which should be used for under sewing.
 * @param minSatinLength The minimum length for the detection of satin stitches.
 * @param minSatinCount The minimum number of consecutive satin stitches for the detection of a area filled by satin stitches
 * @param maxAngle Maximum angle in degree between two stitches for the detection of satin stitches
 * @param safetyDistance The desired distance between the satin stitches and the under sewing lines.
 */
void addUnderSewing(EmbPattern* p, const double minStitchLength, const double minSatinLength, const int minSatinCount, const double maxAngle, const double safetyDistance, const double minDensity) {
    struct EmbStitchList_ * currentStitch = p->stitchList;
    int addedStitches = 0;
    int satinAreaLength = 0;
    int satinAreaCount = 0;
    double stitchSum = 0;
    double stitchSquareSum = 0;
    double lengthSum = 0;
    double lengthSquareSum = 0;

    struct EmbStitchList_ * listMemory = (EmbStitchList_*)malloc(sizeof *listMemory);

    while (0 != currentStitch) {
        if (isSatinArea(currentStitch, minSatinLength, minSatinCount, maxAngle, minDensity)) {
            currentStitch = addSingleUnderSewing(currentStitch, minStitchLength, minSatinLength, maxAngle, safetyDistance, minDensity, &addedStitches, &satinAreaLength);
            satinAreaCount++;
            stitchSum += addedStitches;
            stitchSquareSum += addedStitches * addedStitches;
            lengthSum += satinAreaLength;
            lengthSquareSum += satinAreaLength * satinAreaLength;
        }

        currentStitch = currentStitch->next;
    }

    printf("\nFound %d satin areas.\nMean length: %f +- %f\nMean added Stitches: %f +- %f\n", satinAreaCount, lengthSum / satinAreaCount, sqrt((lengthSquareSum - lengthSum * lengthSum / satinAreaCount)/satinAreaCount), stitchSum / satinAreaCount, sqrt((stitchSquareSum - stitchSum * stitchSum / satinAreaCount)/satinAreaCount));

}

struct EmbStitchList_* addSingleZigZagUnderSewing2(struct EmbStitchList_* const start, const double minSatinLength, const double maxAngle, const double safetyDistance, const double minDensity, int * addedStitches, int * satinAreaLength) {
    const int color = start->stitch.color;
    struct EmbStitchList_* firstOpposing; 
    struct EmbStitchList_* lastAdjacent; 
    struct EmbStitchList_* prevOpposing;
    struct EmbStitchList_* prevAdjacent;
    struct EmbStitchList_* newOpposing;
    struct EmbStitchList_* newAdjacent;
    struct EmbStitchList_* satinAreaBeginning = start->next; 
    struct EmbStitchList_* currentStitch = start;
    int leftOutCounter = 0;
    (*satinAreaLength) = 0;
    *addedStitches = 0;
    if (0 == start || 0 == start->next || 0 == start->next->next || 0 == start->next->next->next || !isSatinStitch(currentStitch, minSatinLength, maxAngle, minDensity)) {
        return start;
    }
    firstOpposing = clone(currentStitch);
    calculateMean(firstOpposing, currentStitch->next, currentStitch->next->next->next);
    
    lastAdjacent = clone(currentStitch);
    calculateMean(lastAdjacent, currentStitch, currentStitch->next->next);

    moveCloser(firstOpposing, lastAdjacent, safetyDistance);

    lastAdjacent->next = clone(firstOpposing);
    lastAdjacent->next->next = satinAreaBeginning;

    prevOpposing = firstOpposing;
    prevAdjacent = lastAdjacent;
    
    newOpposing = clone(prevOpposing);
    newAdjacent = clone(prevAdjacent);

    while (isSatinStitch(currentStitch, minSatinLength, maxAngle, minDensity) && color == currentStitch->stitch.color) {
        calculateMean(newOpposing, currentStitch->next, currentStitch->next->next->next);
        calculateMean(newAdjacent, currentStitch, currentStitch->next->next);
        moveCloser(newOpposing, newAdjacent, safetyDistance);
        if (leftOutCounter > 3 && leftOutCounter % 2 == 1) {
            prevOpposing->next = newOpposing;
            prevOpposing = newOpposing;
            newOpposing = clone(newOpposing);   
            (*addedStitches)++;
            newAdjacent->next = prevAdjacent;
            prevAdjacent = newAdjacent;
            newAdjacent = clone(newAdjacent);
            (*addedStitches)++;
            leftOutCounter = 0;
        }
        leftOutCounter++;
        calculateMean(newOpposing, currentStitch->next, currentStitch->next->next->next);
        calculateMean(newAdjacent, currentStitch, currentStitch->next->next);
        moveCloser(newOpposing, newAdjacent, safetyDistance);


        currentStitch = currentStitch->next;
        (*satinAreaLength)++;
    }
    /* Add the final stitches. Don't care about their length, if they are too short they will be automatically deleted by the optimizer */
    prevOpposing->next = newOpposing;
    prevOpposing = newOpposing;
    newAdjacent->next = prevAdjacent;
    prevAdjacent = newAdjacent;
    (*addedStitches) += 5;

    /* Finally stitch the beginning and end together */
    start->next = firstOpposing;
    
    prevOpposing->next = prevAdjacent;
           
    printf("Found satin area of length %d, added %d stitches\n", *satinAreaLength, *addedStitches);

    return currentStitch;
}

/**
 * Adds zigzag under sewing to the parts of a pattern where satin stich (zick-zack) is used.
 *
 * @param p Pattern to which the under sewing should be added.
 * @param minStitchLength The minimum stitch length which should be used for under sewing.
 * @param minSatinLength The minimum length for the detection of satin stitches.
 * @param minSatinCount The minimum number of consecutive satin stitches for the detection of a area filled by satin stitches
 * @param maxAngle Maximum angle in degree between two stitches for the detection of satin stitches
 * @param safetyDistance The desired distance between the satin stitches and the under sewing lines.
 */
void addZigZagUnderSewing2(EmbPattern* p, const double minSatinLength, const int minSatinCount, const double maxAngle, const double safetyDistance, const double minDensity) {
    struct EmbStitchList_ * currentStitch = p->stitchList;
    int addedStitches = 0;
    int satinAreaLength = 0;
    int satinAreaCount = 0;
    double stitchSum = 0;
    double stitchSquareSum = 0;
    double lengthSum = 0;
    double lengthSquareSum = 0;

    struct EmbStitchList_ * listMemory = (EmbStitchList_*)malloc(sizeof *listMemory);

    while (0 != currentStitch) {
        if (isSatinArea(currentStitch, minSatinLength, minSatinCount, maxAngle, minDensity)) {
            currentStitch = addSingleZigZagUnderSewing2(currentStitch, minSatinLength, maxAngle, safetyDistance, minDensity, &addedStitches, &satinAreaLength);
            satinAreaCount++;
            stitchSum += addedStitches;
            stitchSquareSum += addedStitches * addedStitches;
            lengthSum += satinAreaLength;
            lengthSquareSum += satinAreaLength * satinAreaLength;
        }

        currentStitch = currentStitch->next;
    }

    printf("\nFound %d satin areas.\nMean length: %f +- %f\nMean added Stitches: %f +- %f\n", satinAreaCount, lengthSum / satinAreaCount, sqrt((lengthSquareSum - lengthSum * lengthSum / satinAreaCount)/satinAreaCount), stitchSum / satinAreaCount, sqrt((stitchSquareSum - stitchSum * stitchSum / satinAreaCount)/satinAreaCount));

}

struct EmbStitchList_* addSingleZigZagUnderSewing(struct EmbStitchList_* const start, const double minStitchLength, const double minSatinLength, const double maxAngle, const double safetyDistance, const double minDensity, int * addedStitches, int * satinAreaLength) {
    const int color = start->stitch.color;
    struct EmbStitchList_* firstOpposing; 
    struct EmbStitchList_* lastAdjacent; 
    struct EmbStitchList_* prevOpposing;
    struct EmbStitchList_* prevAdjacent;
    struct EmbStitchList_* newOpposing;
    struct EmbStitchList_* newAdjacent;
    struct EmbStitchList_* satinAreaBeginning = start->next; 
    struct EmbStitchList_* currentStitch = start;
    int leftoutcounter = 0;
    (*satinAreaLength) = 0;
    *addedStitches = 0;
    if (0 == start || 0 == start->next || 0 == start->next->next || 0 == start->next->next->next || !isSatinStitch(currentStitch, minSatinLength, maxAngle, minDensity)) {
        return start;
    }
    firstOpposing = clone(currentStitch);
    calculateMean(firstOpposing, currentStitch->next, currentStitch->next->next->next);
    
    lastAdjacent = clone(currentStitch);
    calculateMean(lastAdjacent, currentStitch, currentStitch->next->next);

    moveCloser(firstOpposing, lastAdjacent, safetyDistance);

    lastAdjacent->next = clone(firstOpposing);
    lastAdjacent->next->next = satinAreaBeginning;

    prevOpposing = firstOpposing;
    prevAdjacent = lastAdjacent;
    
    newOpposing = clone(prevOpposing);
    newAdjacent = clone(prevAdjacent);

    while (isSatinStitch(currentStitch, minSatinLength, maxAngle, minDensity) && color == currentStitch->stitch.color) {
        calculateMean(newAdjacent, currentStitch, currentStitch->next->next);
        calculateMean(newOpposing, currentStitch->next, currentStitch->next->next->next);
        moveCloser(newOpposing, newAdjacent, safetyDistance);
        calculateMean(newOpposing, currentStitch->next, currentStitch->next->next);
        // This makes the (more or less) straight line to the end of the zigzag under sewing
        if (distance(newOpposing, prevOpposing) > minStitchLength) {
            prevOpposing->next = newOpposing;
            prevOpposing = newOpposing;
            newOpposing = clone(newOpposing);   
            (*addedStitches)++;
        }
        // This makes the zigzag under sewing
        if (leftoutcounter > 3 && (leftoutcounter % 2 == 1)) {
            newAdjacent->next = prevAdjacent;
            prevAdjacent = newAdjacent;
            newAdjacent = clone(newAdjacent);
            (*addedStitches)++;
            leftoutcounter = 0;
        }
        leftoutcounter++;
        calculateMean(newOpposing, currentStitch->next, currentStitch->next->next->next);
        calculateMean(newAdjacent, currentStitch, currentStitch->next->next);
        moveCloser(newOpposing, newAdjacent, safetyDistance);


        currentStitch = currentStitch->next;
        (*satinAreaLength) += 1;
    }
    /* Add the final stitches. Don't care about their length, if they are too short they will be automatically deleted by the optimizer */
    prevOpposing->next = newOpposing;
    prevOpposing = newOpposing;
    newAdjacent->next = prevAdjacent;
    prevAdjacent = newAdjacent;
    (*addedStitches) += 5;

    /* Finally stitch the beginning and end together */
    start->next = firstOpposing;
    prevOpposing->next = prevAdjacent;
           
    printf("Found satin area of length %d, added %d stitches\n", *satinAreaLength, *addedStitches);

    return currentStitch;
}

/**
 * Adds zigzag under sewing to the parts of a pattern where satin stich (zick-zack) is used.
 *
 * @param p Pattern to which the under sewing should be added.
 * @param minStitchLength The minimum stitch length which should be used for under sewing.
 * @param minSatinLength The minimum length for the detection of satin stitches.
 * @param minSatinCount The minimum number of consecutive satin stitches for the detection of a area filled by satin stitches
 * @param maxAngle Maximum angle in degree between two stitches for the detection of satin stitches
 * @param safetyDistance The desired distance between the satin stitches and the under sewing lines.
 */
void addZigZagUnderSewing(EmbPattern* p, const double minStitchLength, const double minSatinLength, const int minSatinCount, const double maxAngle, const double safetyDistance, const double minDensity) {
    struct EmbStitchList_ * currentStitch = p->stitchList;
    int addedStitches = 0;
    int satinAreaLength = 0;
    int satinAreaCount = 0;
    double stitchSum = 0;
    double stitchSquareSum = 0;
    double lengthSum = 0;
    double lengthSquareSum = 0;

    struct EmbStitchList_ * listMemory = (EmbStitchList_*)malloc(sizeof *listMemory);

    while (0 != currentStitch) {
        if (isSatinArea(currentStitch, minSatinLength, minSatinCount, maxAngle, minDensity)) {
            currentStitch = addSingleZigZagUnderSewing(currentStitch, minStitchLength, minSatinLength, maxAngle, safetyDistance, minDensity, &addedStitches, &satinAreaLength);
            satinAreaCount++;
            stitchSum += addedStitches;
            stitchSquareSum += addedStitches * addedStitches;
            lengthSum += satinAreaLength;
            lengthSquareSum += satinAreaLength * satinAreaLength;
        }

        currentStitch = currentStitch->next;
    }

    printf("\nFound %d satin areas.\nMean length: %f +- %f\nMean added Stitches: %f +- %f\n", satinAreaCount, lengthSum / satinAreaCount, sqrt((lengthSquareSum - lengthSum * lengthSum / satinAreaCount)/satinAreaCount), stitchSum / satinAreaCount, sqrt((stitchSquareSum - stitchSum * stitchSum / satinAreaCount)/satinAreaCount));

}

void removeNeedlessJumps(EmbPattern* p, const double maxLength) {
    struct EmbStitchList_ * currentStitch = p->stitchList;
    int removedCounter = 0;
    while (   0 != currentStitch
           && 0 != currentStitch->next
           && 0 != currentStitch->next->next
    ) {
        if (    NORMAL == currentStitch->stitch.flags
             && NORMAL == currentStitch->next->next->stitch.flags
             && JUMP   == currentStitch->next->stitch.flags
             && currentStitch->stitch.color == currentStitch->next->next->stitch.color
             && distance(currentStitch, currentStitch->next->next) <= maxLength
        ) {
            currentStitch->next = currentStitch->next->next;
            removedCounter++;
        }
        currentStitch = currentStitch->next;
    }
    printf("Removed %d needless jumps", removedCounter);

}

int main(int argc, const char* argv[])
{
    EmbPattern* p = 0;
    int successful = 0, i = 0;
    int formatType;
    int normalStitches1 = 0;
    int normalStitches2 = 0;
    int normalStitches3 = 0;
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


#define UNDER_SEWING_ZIGZAG 0
#define UNDER_SEWING_ZIGZAG2 1
#define UNDER_SEWING      1
#define SIMPLIFY_STRAIGHT 0
#define SIMPLIFY          1
#define REPEAT_SIMPLIFY   1
#define REMOVE_NEEDLESS_JUMPS 0
    normalStitches1 = countNormalStitches(p->stitchList);

#if REMOVE_NEEDLESS_JUMPS
    removeNeedlessJumps(p, 1.0);
#endif

#if SIMPLIFY
    /* Remove stitches shorter than threshold. */
    printf("\nAttempting to remove very short stitches\n");
    removeSmallStitches(p, threshold);
#endif


#if UNDER_SEWING_ZIGZAG
    /* Add zigzag under sewing to satin areas */

    printf("\nAdding zigzag under sewing to satin areas\n");
    /* Paramters: Pattern, minStitchLength, minSatinLength, minSatinCount, maxAngle, safetyDistance, minDensity */
    addZigZagUnderSewing(p,      2,               2.0,            5,             75,        0.75,          0.7);

    normalStitches2 = countNormalStitches(p->stitchList);
    printf("\nIn total %d stitches were added\n", normalStitches2-normalStitches1);
#endif

#if UNDER_SEWING
    /* Add under sewing to satin areas */

    printf("\nAdding under sewing to satin areas\n");
    /* Paramters: Pattern, minStitchLength, minSatinLength, minSatinCount, maxAngle, safetyDistance, minDensity */
    addUnderSewing(p,      2.0,               1.0,            5,             75,        0.35,           1.1);

    normalStitches2 = countNormalStitches(p->stitchList);
    printf("\nIn total %d stitches were added\n", normalStitches2-normalStitches1);
#endif

#if UNDER_SEWING_ZIGZAG2
    /* Add zigzag under sewing to satin areas */

    printf("\nAdding zigzag under sewing to satin areas\n");
    /* Paramters: Pattern, minSatinLength, minSatinCount, maxAngle, safetyDistance, minDensity */
    addZigZagUnderSewing2(p,      1.9,            5,             75,        0.75,          1.1);

    normalStitches2 = countNormalStitches(p->stitchList);
    printf("\nIn total %d stitches were added\n", normalStitches2-normalStitches1);
#endif


#if SIMPLIFY
    /* Remove stitches shorter than threshold. */
    printf("\nAttempting to remove very short stitches\n");
    removeSmallStitches(p, threshold);

    normalStitches3 = countNormalStitches(p->stitchList);
    printf("\nIn total %d stitches were removed (or added of the number is negative)\n", normalStitches1 - normalStitches3);
    printf("\nFinal number of stitches: %d\n", normalStitches3);

#if SIMPLIFY_STRAIGHT
    printf("\nAttempting to simplify straight lines\n");
    simplifyStraightLines(p, 0.1, 3.6),
#endif

#if REPEAT_SIMPLIFY 
    printf("\nAttempting to remove very short stitches a second time\n");
    removeSmallStitches(p, threshold);
    
    printf("\nAttempting to remove very short stitches a third time\n");
    removeSmallStitches(p, threshold);

    printf("\nAttempting to remove very short stitches a fourth time\n");
    removeSmallStitches(p, threshold);
#endif
#endif

    printf("\n Length of longest stitch: %f \n", maxLength(p->stitchList));

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
