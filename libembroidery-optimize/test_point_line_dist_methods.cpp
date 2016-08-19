#include <gtest/gtest.h>
#include <opencv2/core.hpp>

#include "point_line_dist.cpp"

TEST(line_dist, simple1) {
    for (size_t ii = 0; ii < 15; ++ii) {
        {
            cv::Point2d a(ii,0), b(ii,0), x(0,0);
            EXPECT_NEAR(minimum_distance(a,b,x), ii, 1e-10);
        }
        {
            cv::Point2d a(ii,0), b(ii,ii), x(0,0);
            EXPECT_NEAR(minimum_distance(a,b,x), ii, 1e-10);
        }
        {
            cv::Point2d a(ii,0), b(0,0), x(0,ii);
            EXPECT_NEAR(minimum_distance(a,b,x), ii, 1e-10);
        }
        {
            cv::Point2d a(ii,0), b(0,0), x((double)ii/2, ii);
            EXPECT_NEAR(minimum_distance(a,b,x), ii, 1e-10);
        }
        {
            cv::Point2d a(ii,0), b(0,ii), x(0,0);
            EXPECT_NEAR(minimum_distance(a,b,x), (double)ii * M_SQRT1_2, 1e-10);
        }
        {
            cv::Point2d a(ii,0), b(0,ii), x(ii,ii);
            EXPECT_NEAR(minimum_distance(a,b,x), (double)ii * M_SQRT1_2, 1e-10);
        }
    }
}

TEST(line_dist, nonlinearity_complex) {
    for (size_t scale = 0; scale < 30; ++scale) {
        for (size_t numPoints = 0; numPoints < 5; ++numPoints) {
            std::vector<cv::Point2d> curve;
            curve.push_back(cv::Point2d(0,0));
            for (size_t ii = 1; ii <= numPoints; ++ii) {
                curve.push_back(cv::Point2d(ii, ii*scale));
            }
            curve.push_back(cv::Point2d(numPoints, 0));
            EXPECT_NEAR(max_nonlinearity(curve), numPoints * scale, 1e-10);
        }
    }
}

TEST(line_dist, nonlinearity_simple) {
    for (size_t scale = 0; scale < 30; ++scale) {
        std::vector<cv::Point2d> curve;
        curve.push_back(cv::Point2d(scale,0));
        curve.push_back(cv::Point2d(scale, scale));
        curve.push_back(cv::Point2d(0, scale));
        EXPECT_NEAR(max_nonlinearity(curve), M_SQRT1_2 * scale, 1e-10);
    }
}

int main(int argc, char** argv)
{
    testing::InitGoogleTest(&argc, argv);
    std::cout << "RUN_ALL_TESTS return value: " << RUN_ALL_TESTS() << std::endl;
}
