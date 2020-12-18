// Copyright (c) 2020, the BACCA contributors, as
// shown by the AUTHORS file. All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#ifndef	BACCA_LATEX_GENERATOR_H_
#define BACCA_LATEX_GENERATOR_H_

#include <map>
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>

#include "file_manager.h"

void GenerateLatexTable(const filesystem::path& output_path, const std::string& latex_file, const cv::Mat1d& all_res,
    const std::vector<cv::String>& alg_name, const std::vector<cv::String>& ccl_algorithms);

void GenerateMemoryLatexTable(const filesystem::path& output_path, const std::string& latex_file, const cv::Mat1d& accesses,
    const std::string& dataset, const std::vector<cv::String>& ccl_mem_algorithms);

void GenerateLatexCharts(const filesystem::path& output_path, const std::string& latex_charts, const std::string& latex_folder,
    const std::vector<cv::String>& datasets_name);

void LatexGenerator(const std::map<std::string, bool>& test_to_perform, const filesystem::path& output_path, const std::string& latex_file,
    const cv::Mat1d& all_res, const std::vector<cv::String>& datasets_name, const std::vector<cv::String>& ccl_algorithms,
    const std::vector<cv::String>& ccl_mem_algorithms, const std::map<std::string, cv::Mat1d>& accesses);

#endif // !BACCA_LATEX_GENERATOR_H_