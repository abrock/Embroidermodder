#pragma once

#include <cmath>
#include <opencv2/core.hpp>

template<class T, class P>
T minimum_distance(P v, P w) {
    // Return minimum distance between line segment vw and point p
    const T l2 = (v-w).dot(v-w);  // i.e. |w-v|^2 -  avoid a sqrt
    if (l2 <= 0.0) return std::sqrt(v.dot(v));   // v == w case
    // Consider the line extending the segment, parameterized as v + t (w - v).
    // We find projection of point p onto the line.
    // It falls where t = [(p-v) . (w-v)] / |w-v|^2
    // We clamp t from [0,1] to handle points outside the segment vw.
    const T t = std::max(T(0), std::min(T(1), (v).dot(v-w) / l2));
    const P projection = v + t * (w - v);  // Projection falls on the segment
    return std::sqrt(projection.dot(projection));
}

/**
 * @brief minimum_distance Calculates the distance between a straight line segment given by
 * start and end points a and b and a point x.
 * @param a
 * @param b
 * @param x
 * @return
 */
double minimum_distance(cv::Point2d a, cv::Point2d b, cv::Point2d x) {
    a -= x;
    b -= x;
    return minimum_distance<double>(a,b);
}

double max_nonlinearity(const std::vector<cv::Point2d>& points) {
    if (points.size() <= 2) {
        return 0;
    }
    const cv::Point2d start = points.front();
    const cv::Point2d end = points.back();
    double max_dist = 0.0;
    for (size_t ii = 1; ii+1 < points.size(); ++ii) {
        max_dist = std::max(max_dist, minimum_distance(start, end, points[ii]));
    }

    return max_dist;
}
