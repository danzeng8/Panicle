# Panicle primary branch and stem identification tool
## Introduction

This software extends the method of https://nph.onlinelibrary.wiley.com/doi/full/10.1111/nph.16533 to identify primary branches and the rachises of sorghum panicles, while measuring their traits. The pipeline is similar to that of TopoRoot (https://github.com/danzeng8/TopoRoot), except that during the final stage of the algorithm, primary branches are measured as opposed to the whole hierarchy.

## Installation and Execution

Open up command prompt, clone the repository (git clone https://github.com/danzeng8/Panicle.git.git), then navigate to the main directory. Panicle.exe can be run as follows. The first three arguments are required, while the rest are optional:

Panicle --in [input_file] --out [output_file] --S [shape] --K [kernel] --N [neighborhood] --d [downsampling rate] --lowerRadius [decimal/float value] --upperRadius [decimal/float value] 

Required arguments: 

--in : directory where image slices are located (e.g. C:/data/image_slices/), or a .raw file. If the input is a .raw file, an accompanying .dat file must be specified in the command (--dat [file.dat]).

--out  : Specifies the location and names of the output files. If the argument value is directory/output_file_name, then the four output files are named as directory/output_file_name.ply (geometric skeleton), directory/output_file_annotations.txt (annotation file), directory/output_file_traits.csv (hierarchy measurements), and directory/output_file.off (Surface mesh for visualization)

Highly recommended if not running in batch processing mode:

--S : Shape iso-value. The global iso-value used to produce an initial iso-surface. See "picking shape, kernel, and neighborhood" section below. 

Optional arguments:

--K (recommended, especially for applications besides Maize CT): Kernel iso-value. See "picking shape, kernel, and neighborhood" section below. 

--N (recommended, especially for applications besides Maize CT): Neighborhood iso-value. See "picking shape, kernel, and neighborhood" section below. 

--d : downsampling rate: offers a tradeoff between speed and resolution. A reasonable value for the downsampling rate would be one which produces a volume with dimensions of around 400^3, which typically results in a running time of 5-10 minutes. For example, if the original image volume is of size 1600^3, then a downsampling rate of 4 could be used. Further details on the timing can be found in the paper. 

--lowerRadius : threshold for the thickness of the thinnest part of the rachis. Only vertices greater than this threshold can potentially be greater (default = 0.15 * of thickest vertex). If parts of the rachis are missing, decrease this threshold.

--upperRadius: threshold for the thickness of the thickest part of the rachis. The rachis identification algorithm begins with vertices whose thickness is greater than this value (default=0.95). Typically does not need to be adjusted, unless the rachis entirely fails to be identified.

--branchSizeMin: all branches greater than this length are considered in the measurements. Default = 50.

--radiusTolerance: As a proportion of the stem radius, how far beyond the radius of the stem should a primary branch be identified. The default is 2.0, because due to the resolution branches may appear merged at first when close to the stem. Raise this value if there are branches close to the stem which fail to be identified, and lower it if higher-order branches are mistakenly included.

--fromSkel : run the tool starting from the intermediate skeleton (before cycles are broken)

If you encounter any issues, please contact me (Dan Zeng) at danzeng8@gmail.com or file an issue on Github. 

## Selecting a shape, kernel, and neighborhood threshold

If you choose to use a shape, kernel, and/or neighborhood threshold, the criteria for picking these thresholds are very similar to that of TopoRoot (please see https://github.com/danzeng8/TopoRoot). The main difference is that the shape threshold should be picked above the intensity where the seeds appear (large, potentially hollow balls), which cause excessive cycles between the branches.

## Troubleshooting

Occasionally, the primary branches will not be identified completely after a first run, especially when running the batch processing mode. A few errors may arise, including:
* Missing primary branches, especially near the top of the panicle
* Stem is too short, only being traced from the base to midway up the panicle
* Stem is too long, and includes parts of branches
* Missing primary branches due to the shape of the branches being merged, or disconnections in the shape

The first three errors can usually be addressed by re-running the tool on the intermediate skeleton file. This saves time compared to having to re-run the tool on the original input volume.

Panicle --fromSkel --out [outputFile : same name as used in the first run] --lowerRadius [float value between 0 and 1: default 0.15] --radiusTolerance [float value: default 2.0]

* Missing primary branches near the top of the panicle can be addressed either by lowering the --lowerRadius parameter (if the stem is not fully traced to the very top), or by raising the --radiusTolerance parameter (though at the risk of mistakenly identifying higher-order branches as primary branches).
* The stem being too short can be addressed by lowering the --lowerRadius parameter (try 0.1)
* The stem being too long can be addressed by raising the --lowerRadius parameter (try 0.25)

If there are missing primary branches due to a problem with the identified surface mesh (e.g. missing primary branches due to the branches not being separated on the 3D surface), then the tool needs to be re-run from the beginning, as described in the "Installation and Execution" section. If the branches appear merged together, open up UCSF Chimera (see TopoRoot tutorial), and pick a higher shape (--S) threshold which would separate the branches. If there are disconnections along the stem or primary branches, try lowering "--S" to a threshold which would connect those branches.

## Understanding the output

For each sample, we output the following traits in a .csv file:

* Number of primary branches
* Average branch length
* Total branch length
* Average emergence angle
* Average tip angle
* Longest internode distance
* Second longest internode distance
* First primary branch length
* Rachis (Stem) Length

All length measurements are in voxels and need to be converted to world measurements (e.g. mm or cm) for analysis.

There is also an output .ply file which contains the primary branch and stem geometry, and an annotation file which assigns each vertex to be a primary branch or part of the stem.

Please see https://github.com/danzeng8/TopoRoot for instructions on visualization

## Batch Processing

The software will automatically pick S to be the air intensity plus 25, and N and K are chosen as before. The Panicle code can then be run in a batch process mode on an entire folder of image slices (or .raw files, but slices is preferred) using the panicle_batch.py script as follows:

python panicle_batch.py -i myfolder/ -o outputfolder/ -d downsampling rate [downsampling rate (e.g. 4)]

## Downsampling

Downsampling to a resolution of approximately 300x300x800 is recommended to provide a good balance between resolution and efficiency. This can be done in image processing software such as ImageJ. Alternatively, a script is provided in this repository to perform downsampling:

python downsample.py -i input_dir/ -o output_dir/ -d [downsampling rate (e.g. 4)]

Downsampling can also be performed in batch, on a folder with several subfolders of image slices:

python downsample_batch.py -i input_dir/ -o output_dir/ -d [downsampling rate (e.g. 4)]

Before running downsampling in batch mode, please create folders with the subfolder names. For example if the input folder has a subfolder of image slices called "slices1", then the output folder should also have a subfolder called "slices1". The downsampled result for that data will go into that subfolder.
