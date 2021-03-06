// Copyright (c) 2020, the BACCA contributors, as
// shown by the AUTHORS file. All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.


#include "utilities.h"

#include <time.h>

#include <iostream>
#include <map>

#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

#include "config_data.h"
#include "file_manager.h"
#include "progress_bar.h"
#include "system_info.h"

using namespace cv;
using namespace filesystem;
using namespace std;

#ifdef BACCA_APPLE
const string kTerminal = "postscript";
const string kTerminalExtension = ".ps";
#else
const string kTerminal = "pdf";
const string kTerminalExtension = ".pdf";
#endif

StreamDemultiplexer dmux::cout(std::cout);

void RemoveCharacter(string& s, const char c)
{
    s.erase(std::remove(s.begin(), s.end(), c), s.end());
}

unsigned ctoi(const char& c)
{
    return ((int)c - 48);
}

string GetDatetime()
{
    time_t rawtime;
    char buffer[80];
    time(&rawtime);

    //Initialize buffer to empty string
    for (auto& c : buffer)
        c = '\0';

#if defined(BACCA_WINDOWS) && defined(_MSC_VER)

    struct tm timeinfo;
    localtime_s(&timeinfo, &rawtime);
    strftime(buffer, 80, "%Y-%m-%d %H:%M:%S", &timeinfo);

#elif defined(BACCA_WINDOWS) || defined(BACCA_LINUX) || defined(BACCA_UNIX) || defined(BACCA_APPLE)

    struct tm * timeinfo;
    timeinfo = localtime(&rawtime);
    strftime(buffer, 80, "%Y-%m-%d %H:%M:%S", timeinfo);

#endif
    string datetime(buffer);

    return datetime;
}

string GetDatetimeWithoutSpecialChars()
{
    string datetime(GetDatetime());
    std::replace(datetime.begin(), datetime.end(), ' ', '_');
    std::replace(datetime.begin(), datetime.end(), ':', '.');
    return datetime;
}

bool GetBinaryImage(const string& filename, Mat1b& binary_mat)
{
    // Image load
    Mat1b image;
    image = imread(filename, IMREAD_GRAYSCALE);   // Read the file
    // Check if image exist
    if (image.empty()) {
        return false;
    }
    // Adjust the threshold to force the image be {0,1}
    threshold(image, binary_mat, 0, 1, THRESH_BINARY);
   
    return true;
}

bool GetBinaryImage(const path& p, Mat1b& binary_mat) {
    return GetBinaryImage(p.string(), binary_mat);
}

bool CompareMat(const Mat1i& mat_a, const Mat1i& mat_b)
{
    // Get a matrix with non-zero values at points where the
    // two matrices have different values
    cv::Mat diff = mat_a != mat_b;
    // Equal if no elements disagree
    return cv::countNonZero(diff) == 0;
}

void HideConsoleCursor()
{
#ifdef BACCA_WINDOWS
    HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);

    CONSOLE_CURSOR_INFO cursor_info;

    GetConsoleCursorInfo(out, &cursor_info);
    cursor_info.bVisible = false; // set the cursor visibility
    SetConsoleCursorInfo(out, &cursor_info);

#elif defined(BACCA_LINUX) || defined(BACCA_UNIX) || defined(BACCA_APPLE)
    int unused; // To avoid "return unused" and "variable unused" warnings 
    unused = system("setterm -cursor off");
#endif
    return;
}

int RedirectCvError(int status, const char* func_name, const char* err_msg, const char* file_name, int line, void*)
{
    OutputBox ob;
    ob.Cerror(err_msg);
    return 0;
}

std::string GetGnuplotTitle(ConfigData& cfg)
{
    SystemInfo s_info(cfg);
    string s = "\"{/:Bold CPU}: " + s_info.cpu() + " {/:Bold BUILD}: " + s_info.build() + " {/:Bold OS}: " + s_info.os() +
        " {/:Bold COMPILER}: " + s_info.compiler_name() + " " + s_info.compiler_version() + "\" font ', 9'";
    return s;
}

string EscapeUnderscore(const string& s)
{
    string s_escaped;
    unsigned i = 0;
    for (const char& c : s) {
        if (c == '_' && i > 0 && s[i - 1] != '\\') {
            s_escaped += '\\';
        }
        s_escaped += c;
        ++i;
    }
    return s_escaped;
}

// Gnuplot requires double-escaped name when underscores are encountered
string DoubleEscapeUnderscore(const string& s)
{
    string s_escaped{ s };
    size_t found = s_escaped.find_first_of("_");
    while (found != std::string::npos) {
        s_escaped.insert(found, "\\\\");
        found = s_escaped.find_first_of("_", found + 3);
    }
    return s_escaped;
}