#ifndef DRAWING_CPP
#define DRAWING_CPP

#include "emb-reader-writer.h"
#include "emb-logging.h"
#include "emb-format.h"
#include "emb-stitch.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <iostream>

#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>

/**
 * @brief The Drawing class provides a set of drawing functions highlighting different aspects of
 * an embroidery file like stitch length or overlap.
 */
class Drawing {

public:
    void initializeImage(const struct EmbStitchList_ * start, const struct EmbStitchList_ * stop = 0) {
        getLimits(start, stop);
        const int nrows = std::ceil((maxY-minY)*scale);
        const int ncols = std::ceil((maxX-minX)*scale);
        img = cv::Mat_<float>(nrows, ncols, 0.0);
        mask = cv::Mat_<uint8_t>(nrows, ncols);
    }
    void initializeImage(EmbPattern* p, const struct EmbStitchList_ * stop = 0) {
        initializeImage(p->stitchList, stop);
    }
    void save(const std::string& filename) {
        cv::Mat output;
        cv::normalize(img, output, 0, 255, cv::NORM_MINMAX, CV_8UC1);
        cv::imwrite(filename + "-gray.png", output);
        cv::applyColorMap(output, output, cv::COLORMAP_JET);
        cv::imwrite(filename + "-jet.png", output);
    }

    cv::Point2i getPos(const struct EmbStitchList_ * s) {
        return cv::Point2i (
                    std::round(scale * (s->stitch.xx-minX)),
                    std::round(img.rows - scale * (s->stitch.yy-minY) - 1)
                    );
    }

    void draw(const struct EmbStitchList_ * a, const struct EmbStitchList_ * b, const float intensity) {
        if (EM_NORMAL != a->stitch.flags && EM_NORMAL != b->stitch.flags) {
            return;
        }
        // The circles for the two stitches are only drawn if the flag is "EM_NORMAL",
        // thereby we avoid drawing circles for intermediate jumps, trims etc.
        if (isNormal(a)) {
            cv::circle(img, getPos(a), std::round(radius*scale), cv::Scalar(intensity), -1);
            cv::circle(mask, getPos(a), std::round(radius*scale), 255, 0);
        }
        if (isNormal(b)) {
            cv::circle(img, getPos(b), std::round(radius*scale), cv::Scalar(intensity), -1);
            cv::circle(mask, getPos(b), std::round(radius*scale), 255, -1);
        }
        cv::line(img, getPos(a), getPos(b), cv::Scalar(intensity), std::round(scale * lineWidth));
        cv::line(mask, getPos(a), getPos(b), 255, std::round(scale * lineWidth));
    }

    bool isNormal(const struct EmbStitchList_ * s) {
        return NULL != s && (EM_NORMAL == s->stitch.flags);
    }

    /**
     * Calculate euclidean distance of two stitches given as EmbStitchList_*
     */
    double distance(const struct EmbStitchList_ * a, const struct EmbStitchList_ * b) {
        const double dx = a->stitch.xx - b->stitch.xx;
        const double dy = a->stitch.yy - b->stitch.yy;
        return sqrt(dx*dx + dy*dy);
    }

    void drawStitchLengthes(const struct EmbStitchList_ * start, const struct EmbStitchList_ * stop = 0) {
        if (NULL == start || NULL == start->next) {
            return;
        }
        struct EmbStitchList_ * currentStitch = start->next;
        draw(start, start->next, distance(start, start->next));
        for (;NULL != currentStitch
             && NULL != currentStitch->next
             && stop != currentStitch->next
             && stop != currentStitch; currentStitch = currentStitch->next) {
            struct EmbStitchList_ * nextStitch = currentStitch->next;
            if (isNormal(currentStitch) && isNormal(nextStitch)) {
                draw(currentStitch, nextStitch, distance(currentStitch, nextStitch));
                continue;
            }
            if (TRIM == nextStitch->stitch.flags) {
                continue;
            }
            if (isNormal(currentStitch)) {
                for (size_t ii = 0; ii < 1; ++ii) {
                    nextStitch = nextStitch->next;
                    if (isNormal(nextStitch) && distance(currentStitch, nextStitch) < 20) {
                        draw(currentStitch, nextStitch, distance(currentStitch, nextStitch));
                        continue;
                    }
                }
            }
        }
    }

    void drawStitchLengthes(EmbPattern* p, const struct EmbStitchList_ * stop = 0) {
        drawStitchLengthes(p->stitchList, stop);
    }

    void getLimits(const struct EmbStitchList_ * start, const struct EmbStitchList_ * stop = 0) {
        if (NULL == start) {
            return;
        }
        minX = maxX = start->stitch.xx;
        minY = maxY = start->stitch.yy;
        struct EmbStitchList_ * currentStitch = start->next;

        while (NULL != currentStitch && stop != currentStitch) {
            minX = std::min(minX, currentStitch->stitch.xx);
            maxX = std::max(maxX, currentStitch->stitch.xx);
            minY = std::min(minY, currentStitch->stitch.yy);
            maxY = std::max(maxY, currentStitch->stitch.yy);
            currentStitch = currentStitch->next;
        }
    }

    double minX = 0;
    double minY = 0;
    double maxX = 0;
    double maxY = 0;

    double radius = .3;
    double lineWidth = .33;
    double scale = 30;

    /**
     * @brief offset Safety offset at image borders
     */
    int offset = 10;

    cv::Mat_<float> img;
    cv::Mat_<uint8_t> mask;

};

#endif // DRAWING_CPP
