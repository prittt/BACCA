#include "chaincode_suzuki_handmade.h"
#include "register.h"

#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>

using namespace std;
using namespace cv;



enum class Dir {CLOCKWISE = 0, COUNTERCLOCKWISE = 1};

#define DELTAS2INDEX(x, y) (4 - 2*x + x*y - 2*(1 - x*x)*(y*y-y))

static const int IndexToDeltaX[] = { 0, 1, 1, 1, 0, -1, -1, -1 };
static const int IndexToDeltaY[] = { -1, -1, 0, 1, 1, 1, 0, -1 };


static Point RoundNext(Point center, Point start, Dir dir) {
    int delta_x = start.x - center.x;
    int delta_y = start.y - center.y;

    int index = DELTAS2INDEX(delta_x, delta_y);

    int next_index = (index + 9 - 2 * static_cast<int>(dir)) % 8;

    int next_delta_x = IndexToDeltaX[next_index];
    int next_delta_y = IndexToDeltaY[next_index];

    Point next(center.x + next_delta_x, center.y + next_delta_y);
    return next;
}


static bool RoundNextForeground(Mat1i img, Point center, Point start, Dir dir, Point& next_foreground, Point through, bool& passed_through) {

    int rows = img.rows;
    int cols = img.cols;

    bool return_value = false;
    passed_through = false;

    Point current = start;
    for(int i = 0; i < 7; ++i) {

        Point next = RoundNext(center, current, dir);
        if (next.x >= 0 && next.x < cols && next.y >= 0 && next.y < rows) {
            int v = img(next);

            if (v != 0) {
                return_value = true;
                next_foreground = next;
                break;
            }
        }

        if (next == through) {
            passed_through = true;
        }

        current = next;
    }

    return return_value;
}


static bool RoundNextForeground(Mat1i img, Point center, Point start, Dir dir, Point& next_foreground) {

    int rows = img.rows;
    int cols = img.cols;

    bool return_value = false;

    Point current = start;
    for (int i = 0; i < 7; ++i) {

        Point next = RoundNext(center, current, dir);
        if (next.x >= 0 && next.x < cols && next.y >= 0 && next.y < rows) {
            int v = img(next);

            if (v != 0) {
                return_value = true;
                next_foreground = next;
                break;
            }
        }

        current = next;
    }

    return return_value;
}


static void myFindContours(Mat1b img_, vector<vector<Point>>& contours_, vector<Vec4i>& hierarchy_) {

    Mat1i img = img_.clone();

    vector<vector<Point>> contours;
    vector<Vec4i> hierarchy;
    vector<bool> contours_outer;
    vector<int> last_sons;
    int last_outermost = -1;

    int rows = img.rows;
    int cols = img.cols;

    int NBD = 1;
    int LNBD = 1;

    for (int r = 0; r < rows; ++r) {
        LNBD = 1;

        for (int c = 0; c < cols; ++c) {

            if (img(r, c) != 0) {

                Point p(c, r);
                Point p1;
                Point p2;
                Point p3;
                Point p4;

                // Find starting point of a contour
                bool start = false;
                if (img(r, c) == 1 && (c == 0 || img(r, c - 1) == 0)) { // Outer
                    p2 = Point(c - 1, r);
                    start = true;
                    contours_outer.push_back(true);
                }
                else if (img(r, c) >= 1 && (c == cols || img(r, c + 1) == 0)) { // Hole
                    p2 = Point(c + 1, r);
                    if (img(r, c) > 1) {
                        LNBD = img(r, c);
                    }
                    start = true;
                    contours_outer.push_back(false);
                }
                if (start) {

                    ++NBD;
                    vector<Point> contour;
                    contour.emplace_back(c, r);

                    // Punto 2: gerarchia
                    last_sons.push_back(-1);
                    int cur = static_cast<int>(contours.size());
                    int father = -1;
                    int prev_sibling = -1;
                    int b_primo = LNBD - 2;
                    if (contours_outer.back()) {
                        if (b_primo >= 0 && contours_outer[b_primo]) {
                            father = hierarchy[b_primo][3];
                        }
                        else {
                            father = b_primo;
                        }
                    }
                    else {
                        if (b_primo == -1 || contours_outer[b_primo]) {
                            father = b_primo;
                        }
                        else {
                            father = hierarchy[b_primo][3];
                        }
                    }
                    if (father >= 0) {
                        prev_sibling = last_sons[father];
                        if (prev_sibling < 0) {
                            hierarchy[father][2] = cur;
                        }
                        else {
                            hierarchy[prev_sibling][0] = cur;
                        }

                        last_sons[father] = cur;
                    }
                    else {
                        if (last_outermost >= 0) {
                            hierarchy[last_outermost][0] = cur;
                            prev_sibling = last_outermost;
                        }
                        last_outermost = cur;
                    }
                    hierarchy.emplace_back(-1, prev_sibling, -1, father);

                    // 3.1
                    bool found = RoundNextForeground(img, p, p2, Dir::CLOCKWISE, p1);
                    if (found) {

                        // 3.2
                        p2 = p1;
                        p3 = p;

                        // 3.3
                        while (true) {
                            Point through(p3.x + 1, p3.y);
                            bool passed_through;
                            found = RoundNextForeground(img, p3, p2, Dir::COUNTERCLOCKWISE, p4, through, passed_through);
                            if (!found) {
                                p4 = p2;
                            }

                            // 3.4
                            if (passed_through) {
                                img(p3) = -NBD;
                            }
                            else if (img(p3) == 1) {
                                img(p3) = NBD;
                            }

                            // 3.5
                            if (p4 == p && p3 == p1) {
                                // We're back to the starting point
                                break;
                            }

                            contour.push_back(p4);
                            p2 = p3;
                            p3 = p4;

                        }

                    }
                    else {
                        img(r, c) = -NBD;
                    }

                    contours.push_back(move(contour));
                }

                // 4
                if (img(r, c) != 1) {
                    LNBD = abs(img(r, c));
                }
            }
        }
    }

    contours_ = contours;
    hierarchy_ = hierarchy;

}


void SuzukiHandmadeTopology::PerformChainCode() {
    with_hierarchy_ = true;

    vector<vector<Point>> cv_contours;
    myFindContours(img_, cv_contours, hierarchy_);
    chain_code_ = ChainCode(cv_contours, true);
}


REGISTER_CHAINCODEALG(SuzukiHandmadeTopology)
