<img src="doc/BACCA.png" height="150px" align="right">

# BACCA: Benchmark Another Chain Code Algorithm
<!-- [![release](https://img.shields.io/github/v/release/prittt/BACCA)](https://github.com/prittt/BACCA/releases/latest/) -->
[![license](https://img.shields.io/github/license/prittt/BACCA)](https://github.com/prittt/BACCA/blob/master/LICENSE)

<p align="justify">
BACCA is an open source <i>C++</i> chain code benchmarking framework, developed using <a href="https://github.com/prittt/YACCLAB">YACCLAB</a> <a href="https://github.com/prittt/YACCLAB/releases/tag/v2.0">v2.0</a> as a starting point. BACCA provides <i>correctness</i> and average run-time (<i>average</i>) tests for chain code (also known as boundary tracing) algorithms over a collection of real world datasets.
</p>



<table>
<thead>
<tr>
    <th>OS</th>
    <th>Build</th>
    <th>Compiler</th>
    <th>OpenCV</th>
    <th>CMake</th>
    <!--<th width="200px">Travis CI</th>-->
    <th width="200px">GitHub Actions</th>
</tr>
<thead>
<tbody>
<tr>
    <td align="center">Ubuntu<br/>16.04 LTS</td>
    <td align="center">x32</td>
    <td align="center">gcc 5.4.0</td>
    <td align="center">3.0.0</td>
    <td align="center">3.13.5</td>
    <td align="center"><a href="https://github.com/prittt/BACCA/actions"><img src="https://github.com/prittt/BACCA/workflows/linux32/badge.svg?branch=master" alt="Build Status"/></a></td>
</tr>
<tr>
    <td align="center">Ubuntu<br/>18.04 LTS</td>
    <td align="center">x64</td>
    <td align="center">gcc 9.3.0</td>
    <td align="center">4.1.2</td>
    <td align="center">3.13.5</td><td align="center"><a href="https://github.com/prittt/BACCA/actions"><img src="https://github.com/prittt/BACCA/workflows/linux64/badge.svg?branch=master" alt="Build Status"/></a></td>
</tr>
<tr>
    <td align="center">MacOS<br/>(Darwin 19.6.0)</td>
    <td align="center">x64</td>
    <td align="center">AppleClang 12.0.0<br/>(Xcode-12)</td>
    <td align="center">3.1.0</td>
    <td align="center">3.13.0</td>
    <td align="center"><a href="https://github.com/prittt/BACCA/actions"><img src="https://github.com/prittt/BACCA/workflows/macos/badge.svg?branch=master" alt="Build Status"/></a></td>
</tr>
</tbody>
</table>


## Requirements

<p align="justify">To correctly install and run BACCA the following packages, libraries and utilities are needed:</p>

- CMake 3.13 or higher (https://cmake.org),
- OpenCV 3.0 or higher (http://opencv.org),
- Gnuplot (http://www.gnuplot.info/),
- One of your favourite IDE/compiler: Visual Studio 2013 or higher, Xcode 5.0.1, gcc 4.7 or higher, .. (with C++14 support)

Notes for gnuplot:
- on Windows system: be sure of adding gnuplot to the system path to allow the automatic charts generation.
- on MacOS system: 'pdf terminal' seems to be not available, 'postscript' is used instead.

<a name="inst"></a>
## Installation

<p align="justify">Clone the GitHub repository (HTTPS clone URL: https://github.com/prittt/BACCA.git) or simply download the full master branch zip file and extract it (e.g BACCA folder).</p>
<p align="justify">Install software in BACCA/bin subfolder (suggested) or wherever you want using CMake. Note that CMake should automatically find the OpenCV path if correctly installed on your OS, download the BACCA Dataset (be sure to check the box if you want to download it or to select the correct path if the dataset is already on your file system, and create a C++ project for the selected IDE/compiler.</p>

<p align="justify">Set the <a href="#conf">configuration file (config.yaml)</a> placed in the installation folder (bin in this example) in order to select desired tests.</p>

<p align="justify">Open the project, compile and run it: the work is done!</p>

### CMake Configuration Variables

| Name                                 | Meaning                     | Default | 
| ------------------------------------ |-----------------------------| --------|
| `BACCA_DOWNLOAD_DATASET`           | whether to automatically download the BACCA dataset or not  | `OFF` |
| `BACCA_INPUT_DATASET_PATH`         | path to the `input` dataset folder, where to find test datasets  | `${CMAKE_INSTALL_PREFIX}/input` |
| `BACCA_OUTPUT_RESULTS_PATH`        | path to the `output` folder, where to save output results  | `${CMAKE_INSTALL_PREFIX}/output` |
| `OpenCV_DIR`                         | OpenCV installation path    |  -      |


<a name="conf"></a>
## Configuration File
<p align="justify">A <tt>YAML</tt> configuration file placed in the installation folder lets you to specify which kind of tests should be performed, on which datasets and on which algorithms. A complete description of all configuration parameters is reported below.</p>

- <i>perform</i> - dictionary which specifies the kind of tests to perform:
```yaml
perform:
  correctness:        false
  average:            true
  average_with_steps: false
```

- <i>correctness_tests</i> - dictionary indicating the kind of correctness tests to perform:
```yaml
correctness_tests:
  standard: true
  steps:    true
```

- <i>tests_number</i> - dictionary which sets the number of runs for each test available:
```yaml
tests_number:
  average:            10
  average_with_steps: 10
```

- <i>algorithms</i> - list of algorithms on which to apply the chosen tests, along with display name and reference for correctness check:
```yaml
algorithms:
  - Suzuki                          , Suzuki85(OpenCV)      ; Suzuki
  - Cederberg_LUT                   , Cederberg_LUT         ; Suzuki
  - Cederberg_LUT_PRED              , Cederberg_LUT_PRED    ; Suzuki
  - Cederberg_Tree                  , Cederberg_Tree        ; Suzuki
  - Cederberg_Spaghetti             , Cederberg_Spaghetti   ; Suzuki
  - Cederberg_Spaghetti_FREQ_All    , Cederberg_SpaghettiF  ; Suzuki
```

- <i>check_datasets</i>, <i>average_datasets</i>, <i>average_ws_datasets</i> - lists of datasets on which, respectively, correctness, average, and average_ws tests should be run:
```yaml
...
average_datasets: ["3dpes", "fingerprints", "hamlet", "medical", "mirflickr", "tobacco800", "xdocs"]
...
```

- <i>paths</i> - dictionary with both input (datasets) and output (results) paths. It is automatically filled by CMake during the creation of the project:
```yaml
paths: {input: "<datasets_path>", output: "<output_results_path>"}
```

- <i>save_middle_tests</i> - dictionary specifying, separately for every test, whether to save the output of single runs, or only a summary of the whole test:
```yaml
save_middle_tests: {average: false, average_with_steps: false}
```
