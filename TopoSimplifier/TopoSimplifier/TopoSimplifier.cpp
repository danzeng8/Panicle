/****************************************************
This code implements the topological simplification algorithm published in:

"To cut or to fill: a global optimization approach to topological simplification."

Dan Zeng, Erin Chambers, David Letscher, Tao Ju. 
ACM Transactions on Graphics (Proc. ACM Siggraph Asia 2020), 39(6): No. 201

Paper link: https://www.cse.wustl.edu/~taoju/research/cutfill.pdf

Code Author: Dan Zeng, Washington University in St. Louis, Department of Computer Science and Engineering
Email: danzeng8@gmail.com

****************************************************/

#include <stdint.h>
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image.h"
#include "stb_image_write.h"
#include <iostream>
#include <queue>
#include <chrono>
#define BOOST_TYPEOF_EMULATION
#include <boost/config.hpp>
#include <boost/graph/connected_components.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <algorithm>
#include <iterator>
#include <chrono>
#include <boost/heap/pairing_heap.hpp>
#include <typeinfo>
#include <stdlib.h>
#include <boost/graph/dijkstra_shortest_paths.hpp>
#include <boost/numeric/ublas/io.hpp>
#include <boost/property_map/property_map.hpp>
#include <boost/graph/properties.hpp>
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/biconnected_components.hpp>
#include <boost/graph/lookup_edge.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/timer/timer.hpp>
#include <map>
#include <stack>
#include <unordered_map>
#include <bitset>
#include "stats.h"
#include "options.h"
#include "procstatus.h"
#include "timer.h"
#include "ds.h"
#include "bbtree.h"
#include "prep.h"
#include <stdio.h>
#include "util.h"
#include "tbb/parallel_for.h"
#include "tbb/concurrent_vector.h"
#include <tiffio.h>
#include <boost/filesystem.hpp>

using namespace std;
using namespace boost::heap;
using namespace boost::filesystem;

#pragma region 
typedef boost::property<boost::edge_weight_t, float> EdgeWeightProperty; // define edge weight property
typedef boost::property<boost::vertex_name_t, float> VertexWeightProperty; // define node weight property; note that: vertex_index_t is not mutable
typedef boost::adjacency_list<boost::vecS, boost::vecS, boost::undirectedS, VertexWeightProperty, EdgeWeightProperty> grapht; // define all the graph properties
typedef boost::graph_traits<grapht>::adjacency_iterator AdjacencyIterator;
typedef boost::graph_traits<grapht >::vertex_descriptor vertex_t;

typedef boost::adjacency_list<boost::listS, boost::vecS, boost::undirectedS, VertexWeightProperty, EdgeWeightProperty> graphL; // define all the graph properties
typedef boost::graph_traits<graphL>::adjacency_iterator AdjacencyIteratorL;
typedef boost::graph_traits<graphL>::vertex_descriptor vertex_tL;

#pragma endregion define graph property 2018Äê3ÔÂ23ÈÕ20:15:27

#include <fstream>


struct node {
	unsigned char type;
	unsigned char inFg;
	int64_t labelCost; //intensity costs: only needed for cuts and fills
	float floatCost;
	int64_t v = 0; int64_t e = 0;  int64_t f = 0;  int64_t c = 0; //cell complex costs: only needed for cuts and fills
	int index;
	bool valid = true;
	bool isArticulate = false;
	bool isNew = false;
	int compIndex = -1;
	int tin = 0;
	int low = 0;
	int64_t intensity;
	int overallCompIndexFg = -1; int overallCompIndexBg = -1;
};

//#pragma endregion define heaps
struct VertexProps {
	float weight;
};

struct EdgeProps {
	bool strong;
};
typedef boost::adjacency_list <boost::vecS, boost::vecS, boost::undirectedS, VertexProps, EdgeProps> Graph;

struct coord {
	short x, y, z;
};

//Use 26-connectivity for foreground, 6-connectivity for background 
std::vector<std::vector<int>> structCube = { 
	{-1, -1, -1},
	{-1, -1, 0},
	{-1, -1, 1},
	{-1, 0, -1}, 
	{-1, 0, 0}, 
	{-1,0, 1}, 
	{-1, 1, -1},
	{-1, 1, 0},
	{-1, 1, 1}, 
	{0, -1, -1},
	{0, -1, 0},
	{0, -1, 1},
	{0, 0, -1}, 
	{0, 0, 1},
	{0, 1, -1},
	{0, 1, 0},
	{0, 1,1}, 
	{1, -1, -1}, 
	{1, -1, 0}, 
	{1, -1, 1}, 
	{1, 0, -1}, 
	{1, 0, 0}, 
	{1,0, 1}, 
	{1, 1, -1}, 
	{1, 1, 0}, 
	{1, 1, 1} };

std::vector<std::vector<int>> struct18 = { 
	{-1, -1, 0}, 
	{-1, 0, -1},
	{-1, 0, 0},
	{-1,0, 1}, 
	{-1, 1, 0}, 
	{0, -1, -1}, 
	{0, -1, 0},
	{0, -1, 1},
	{0, 0, -1}, 
	{0, 0, 1}, 
	{0, 1, -1},
	{0, 1, 0},
	{0, 1,1},
	{1, -1, 0},
	{1, 0, -1},
	{1, 0, 0}, 
	{1,0, 1},
	{1, 1, 0} 
};

std::vector< std::vector<int>> structCross3D = {
	{-1, 0, 0},
	{0, -1, 0},
	{0, 0, -1}, 
	{0, 0, 1}, 
	{0, 1, 0}, 
	{1, 0, 0} 
};

std::vector<std::vector<int>> structCubeFull = { 
	{-1, -1, -1}, 
	{-1, -1, 0}, 
	{-1, -1, 1}, 
	{-1, 0, -1}, 
	{-1, 0, 0},
	{-1,0, 1}, 
	{-1, 1, -1},
	{-1, 1, 0}, 
	{-1, 1, 1}, 
	{0, -1, -1}, 
	{0, -1,0}, 
	{0, -1, 1},
	{0, 0, -1},
	{0, 0, 0}, 
	{0, 0, 1}, 
	{0, 1, -1},
	{0, 1,0},
	{0, 1, 1},
	{1, -1, -1},
	{1, -1, 0},
	{1, -1, 1}, 
	{1, 0, -1},
	{1, 0, 0},
	{1, 0, 1}, 
	{1, 1, -1},
	{1, 1, 0}, 
	{1, 1, 1} 
};

std::vector<std::vector<int>> structSquare = {
	{-1, -1, 0},
	{-1, 0, 0},
	{-1, 1, 0},
	{0, -1, 0},
	{0, 1, 0},
	{1, -1, 0},
	{1, 0, 0},
	{1, 1, 0},
};


bool inStruct18[3][3][3] = {
	{{false,true,false},{true,true,true},{false,true,false}},
	{{true,true,true},{true,true,true},{true,true,true}},
	{{false,true,false},{true,true,true},{false,true,false}}
};

struct weightedCoord {
	short x, y, z;  float intensityDiff; //Records intensity difference of voxel compared to shape
	bool operator<(const weightedCoord& rhs) const
	{
		return intensityDiff < rhs.intensityDiff;
	}
};

std::vector< std::vector<int> > cubeFrontMask = {
	{0,0,0},
	{1,0,0},
	{0,1,0},
	{1,1,0},
	{1,0,1},
	{1,1,1},
	{0,1,1},
	{0,0,1} 
};

typedef std::tuple<int, int, int, int, int> coor;

struct key_hash : public std::unary_function<coor, int>
{
	std::size_t operator()(const coor& k) const
	{
		return std::get<0>(k) ^ std::get<1>(k) ^ std::get<2>(k);
	}
};

typedef std::unordered_map<const coor, int, key_hash, std::equal_to<coor>> mapC;

struct Coordinate {
	int x, y, z;
	Coordinate(int x, int y, int z) : x(x), y(y), z(z) {}

	bool operator<(const Coordinate& coord) const {
		if (x < coord.x) return true;
		if (x > coord.x) return false;
		//x == coord.x
		if (y < coord.y) return true;
		if (y > coord.y) return false;
		//x == coord.x && y == coord.y
		if (z < coord.z) return true;
		if (z > coord.z) return false;
		//*this == coord
		return false;
	}

	bool operator==(const Coordinate& coord) const {
		if (x == coord.x && y == coord.y && z == coord.z)
			return true;
		return false;
	}

	inline bool isInRange(Coordinate coord, int range) const {
		if (pow(coord.x - this->x, 2) + pow(coord.y - this->y, 2) + pow(coord.z - this->z, 2) <= range * range)
			return true;
		return false;
	}
};

uint32_t unvisited = 2097152;

class Compare
{
public:
	bool operator() (node &a, node &b)
	{
		return abs(a.labelCost) > abs(b.labelCost);
	}
};

enum type {
	CORE = 0, 
	N = 1, 
	CUT = 2, 
	FILL = 3, 
	MIDT = 4, 
	PINODE = 5, 
	HYPERNODE = 6
};

int numDigits(int x)
{
	//Accomodate image volumes with up to 10^5 slices
	x = abs(x);
	return (x < 10 ? 1 :
		(x < 100 ? 2 :
		(x < 1000 ? 3 :
			(x < 10000 ? 4 : 5))));
}

int coordToIndex(int x, int y, int z, int width, int height, int depth) {
	return x + height * (y + depth * z);
}

void getCoordFromIndex(int index, int& x, int& y, int& z, int width, int height, int depth) {
	x = index - (height * (y + depth * z));
	y = ((index - x) / height) - (depth * z);
	z = (((index - x) / height) - y) / depth;
}


bool IsBitSet(uint32_t num, int bit)
{
	return 1 == ((num >> bit) & 1);
}

bool Visited(uint32_t num)
{
	return 1 == ((num >> 31) & 1);
}

//Extract k bits from position p
uint32_t bitExtracted(uint32_t number, int k, int p)
{
	uint32_t mask;
	mask = ((1 << k) - 1) << p;
	return number & mask;
}

uint32_t Label(uint32_t number) {
	return bitExtracted(number, 22, 0);
}

void nBitToX(uint32_t& number, int n, int x) {
	number ^= (-x ^ number) & (1UL << n);
}

void changeLabel(uint32_t &n1, uint32_t &n2) {
	for (int i = 0; i < 23; i++) {
		//Set ith bit of n2 to ith bit of n1
		nBitToX(n2, i, ((n1 >> i) & 1));
	}
}

void setVisitedFlag(uint32_t &n, int i) {
	nBitToX(n, 31, i);
}


void loadImages(std::vector<stbi_uc*> & g_Image3D, int & numSlices, std::string dir, int &width, int &height, int inFileType, std::vector< std::vector<float> > & tiffImages,float & K, float & N,
	float & S, float & minVal, float & maxVal, bool cutOnly, bool fillOnly, bool & rootShape, string & rootShapeFile, float autoOffset
	) {
	bool boundingBox = (K == INFINITY) && (N == -INFINITY);
	if (boundingBox) {
		K = -INFINITY;
		N = INFINITY;
	}
	if (cutOnly) {
		K = -INFINITY;
	}
	if (fillOnly) {
		N = INFINITY;
	}
	//File format is .dist
	if (inFileType == 2) {
		std::ifstream infile;
		infile.open(dir, ios::binary | ios::in);
		infile.read((char *)&width, 4);
		infile.read((char *)&height, 4);
		infile.read((char *)&numSlices, 4);

		double boxCoordinates;
		infile.read((char *)&boxCoordinates, 8);
		infile.read((char *)&boxCoordinates, 8);
		infile.read((char *)&boxCoordinates, 8);
		infile.read((char *)&boxCoordinates, 8);
		infile.read((char *)&boxCoordinates, 8);
		infile.read((char *)&boxCoordinates, 8);
		width *= -1;
		width += 1;
		height += 1;
		for (int s = 0; s < numSlices; s++) {
			std::vector<float> img(width*height, 0.0);
			stbi_uc * g_image = (stbi_uc*)calloc(width*height, sizeof(stbi_uc));
			for (int j = 0; j < height; j++) {
				for (int i = 0; i < width; i++) {
					float data;
					infile.read((char *)&data, 4);
					if (boundingBox) {
						K = max(K, data);
						N = min(N, data);
					}
					minVal = min(minVal, data);
					maxVal = max(maxVal, data);
					img[i + j * width] = data;
					g_image[i + j * width] = (stbi_uc)img[i + j * width];
				}
			}
			g_Image3D.push_back(g_image);
			tiffImages.push_back(img);
		}
		if (boundingBox) {
			K -= 0.00001;
			N += 0.00001;
		}
		return;
	}

	//File format is .tif
	if (inFileType == 1) {
		string filename = dir;

		TIFF* tif = TIFFOpen(filename.c_str(), "r");
		if (tif) {
			int dircount = 0;
			do {
				tdata_t buf;
				uint32 row;
				TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &width);
				TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &height);
				buf = _TIFFmalloc(TIFFScanlineSize(tif));
				std::vector<float> imageF(width * height, 0.0);
				for (row = 0; row < height; row++) {
					TIFFReadScanline(tif, buf, row);
					float* imgF = (float*)buf;
					for (int i = 0; i < width; i++) {
						imageF[i + row * width] = imgF[i];
						if (boundingBox) {
							K = max(K, imgF[i]);
							N = min(N, imgF[i]);
						}
						if (cutOnly) {
							K = max(K, imgF[i]);
						}
						if (fillOnly) {
							N = min(N, imgF[i]);
						}

						minVal = min(minVal, imgF[i]);
						maxVal = max(maxVal, imgF[i]);
					}
				}
				tiffImages.push_back(imageF);
				_TIFFfree(buf);

				dircount++;
			} while (TIFFReadDirectory(tif));
			TIFFClose(tif);
		}
		else {
			std::cout << "Error Reading Tif File  (not existent, not accessible or no TIFF file)" << std::endl;
		}
		if (boundingBox) {
			K -= 0.00001;
			N += 0.00001;
		}
		if (cutOnly) {
			K -= 0.00001;
		}
		if (fillOnly) {
			N += 0.00001;
		}
		return;
	}

	
	if (numSlices > 0) {
		//File Format is .png
		std::vector<int> histogram(255, 0);
		for (int s = 0; s < numSlices; s++) {

			int bpp;
			int digits = numDigits(s);
			string numStr = "";
			for (int n = 0; n < 4 - digits; n++) {
				numStr += "0";

			}
			numStr += std::to_string(s);
			string filename;

			stbi_uc* g_image = NULL;

			std::vector< float > imageF;
			if (inFileType == 0) {
				filename = dir + numStr + ".png";
				g_image = stbi_load(filename.c_str(), &width, &height, &bpp, 1);
			}
			
			for (int i = 0; i < width; i++) {
				for (int j = 0; j < height; j++) {
					if (boundingBox) {
						K = max(K, (float)g_image[i + j * width]);
						N = min(N, (float)g_image[i + j * width]);
					}
					if (cutOnly) {
						K = max(K, (float)g_image[i + j * width]);
					}
					if (fillOnly) {
						N = min(N, (float)g_image[i + j * width]);
					}
					if (rootShape) {
						histogram[(int)g_image[i + j * width]] ++;
					}
				}
			}
			g_Image3D.push_back(g_image);

		}
		if (boundingBox) {
			K -= 1;
			N += 1;
		}
		if (cutOnly) {
			K -= 1;
		}
		if (fillOnly) {
			N += 1;
		}
		if (rootShape) {
			int rootS = 0;
			for (int i = 0; i < 255; i++) {
				if (histogram[i] > histogram[S]) {
					rootS = i;
				}
			}
			//Sorghum settings
			//rootS += 15;
			//K = rootS + 30;
			//N = rootS;

			//Root settings
			rootS += 6;
			K = rootS + 4;
			N = rootS - 4;
			
			S = rootS;

			std::ofstream ofS;
			ofS.open(rootShapeFile);
			cout << "Writing " << S << " to " << rootShapeFile << endl;
			ofS << S << endl;
			ofS.close();
		}
	}
	else {
		std::vector<int> histogram(256, 0);
		path p(dir);
		cout << "Loading files " << endl;
		for (auto i = directory_iterator(p); i != directory_iterator(); i++)
		{
			if (!is_directory(i->path()))
			{
				stbi_uc* g_image = NULL;
				std::vector< float > imageF;
				int bpp;
				g_image = stbi_load(i->path().string().c_str(), &width, &height, &bpp, 1);

				for (int i = 0; i < width; i++) {
					for (int j = 0; j < height; j++) {
						if (boundingBox) {
							K = max(K, (float)g_image[i + j * width]);
							N = min(N, (float)g_image[i + j * width]);
						}
						if (cutOnly) {
							K = max(K, (float)g_image[i + j * width]);
						}
						if (fillOnly) {
							N = min(N, (float)g_image[i + j * width]);
						}
						if (rootShape) {
							histogram[(int)g_image[i + j * width]] ++;
						}
					}
				}
				g_Image3D.push_back(g_image);
			}
			else {
				continue;
			}
		}
		numSlices = g_Image3D.size();
		cout << "Done loading " << endl;
		if (rootShape) {
			int rootS = 0;
			for (int i = 0; i < 255; i++) {
				if (histogram[i] > histogram[rootS]) {
					rootS = i;
				}
			}
			//Sorghum settings
			//rootS += autoOffset;
			
			//K = rootS + 30;
			//N = rootS;
			//S = rootS;
			//Root settings
			rootS += 6;
			K = rootS + 4;
			N = rootS - 4;
			S = rootS;
			
			std::ofstream ofS;
			ofS.open(rootShapeFile);
			cout << "Writing " << S << " to " << rootShapeFile << ", auto offset " << autoOffset << " K " << K << " N " << N << endl;
			ofS << S << endl;
			ofS.close();
		}
		
	}
	if (boundingBox) {
		K -= 1;
		N += 1;
	}
	if (cutOnly) {
		K -= 1;
	}
	if (fillOnly) {
		N += 1;
	}
	
}

void parseArguments(int argc, string &inFile, string& outFile, int &numSlices, char ** argv, float & N, float & S, float & C, float & vizEps, int & hypernodeSize, int & productThresh, int & globalSteinerTime, int & localSteinerTime,
	int & greedyMode, int & nodesToFix, int & bbTime, int & genMode, int & openItr, int & closeItr,int & inFileType, int & exportGen, int & greedy,
	int & se, bool & shapeTopo, bool & showGeomCost, bool & help, bool & thresholdOut, bool & finalTopo, 
	bool & rootShape, string & rootShapeFile, bool & rootMorpho, float & autoOffset) {

	for (int i = 0; i < argc; i++) {
		if (((string)argv[i]).substr(0, 2) == "--") {
			string arg = (string)argv[i];
			arg.erase(0, 2);
			if (i < argc - 1) {
				if (arg == "in") {
					inFile = (string)argv[i + 1]; //Input image slices directory
				}
				if (arg == "out") {
					outFile = (string)argv[i + 1]; //Output image slices directory
				}
				if (arg == "numSlices") {
					numSlices = std::stoi(argv[i + 1]); //Number of slices in directory
				}
				if (arg == "K") {
					C = stof(argv[i + 1]);
				}
				if (arg == "S") {
					S = stof(argv[i + 1]);
				}
				if (arg == "N") {
					N = stof(argv[i + 1]);
				}
				if (arg == "vizEps") {
					vizEps = stof(argv[i + 1]);
				}
				if (arg == "hypernodeSize") {
					hypernodeSize = stoi(argv[i + 1]);
				}
				if (arg == "P") {
					productThresh = stoi(argv[i + 1]);
				}
				if (arg == "globalTime") {
					globalSteinerTime = stoi(argv[i + 1]);
				}
				if (arg == "localTime") {
					localSteinerTime = stoi(argv[i + 1]);
				}
				if (arg == "bbTime") {
					bbTime = stoi(argv[i + 1]);
				}
				if (arg == "greedy") {
					greedyMode = stoi(argv[i + 1]);
				}
				if (arg == "nodesToFix") {
					nodesToFix = stoi(argv[i + 1]);
				}
				if (arg == "genMode") {
					genMode = stoi(argv[i + 1]);
				}
				if (arg == "open") {
					openItr = stoi(argv[i + 1]);
				}
				if (arg == "close") {
					closeItr = stoi(argv[i + 1]);
				}
				if (arg == "inType") {
					inFileType = stoi(argv[i + 1]);
				}
				if (arg == "se") {
					se = stoi(argv[i + 1]);
				}
				if (arg == "finalTopo") {
					finalTopo = (bool)stoi(argv[i + 1]);
				}
				if (arg == "rootShape") {
					rootShape = true;
					rootShapeFile = argv[i + 1];
					std::cout << "root shape set to true " << rootShapeFile << std::endl;
				}
				if (arg == "rootMorpho") {
					rootMorpho = true;
				}
				if (arg == "autoOffset") {
					autoOffset = stoi(argv[i + 1]);
				}
			}
			
			if (arg == "shapeTopo") {
				shapeTopo = true;
			}
			
			if (arg == "showGeomCost") {
				showGeomCost = true;
			}

			if (arg == "exportGen") {
				exportGen = 1;
			}


			if (arg == "allGreedy") {
				greedy = 1;
			}
			if (arg == "thresholdOut") {
				thresholdOut = true;
			}

			if (arg == "help") {
				std::cout << "Showing help menu. Required and optional parameters: " << std::endl;
				std::cout << "Required: " << std::endl;
				std::cout << "	--in	<Input file>	Format: .tif file, .dist file, or directory with .png image slices" << std::endl;
				std::cout << "	--out	<Output file>	Format: If input file is .tif, output must be .tif." << std::endl;
				std::cout << "									If input file is .dist, output must be .dist." << std::endl;
				std::cout << "									If input is directory of image slices, output must be name of output directory" << std::endl;
				std::cout << "Required for .png image slices (e.g. biomedical / plant images): " << std::endl;
				std::cout << "	--S	<Float or Integer> : Shape threshold for the input surface being simplified. Defines an iso-surface / level set. Default: 0" << std::endl;

				std::cout << "Optional: " << std::endl;
				std::cout << "	Simplification level (Kernel and Neighborhood): " << std::endl;
				std::cout << "		--K <Float or Integer> : Kernel threshold (Upper level set). Default: Highest value in image volume." << std::endl;
				std::cout << "		--N <Float or Integer> : Neighborhood threshold (Lower level set). Default: Lowest value in image volume." << std::endl;
				std::cout << "	Visualization: " << std::endl;
				std::cout << "		--vizEps <Float or Integer> : Visualization offset: the larger this value, the larger the applied changes will appear. Default: 0.01 for .tif, 10 for .png" << std::endl;
				std::cout << "		--exportGen: flag which exports all computed cuts and fills, and selected cuts and fills as separate .ply mesh files. " << std::endl;
				std::cout << "	Type of simplification (Minimal or Smooth Morphological): " << std::endl;
				std::cout << "		By default, minimal geometric simplifications are performed. If either parameter below is specified to be not 0, then morphological opening/closing will be used instead." << std::endl;
				std::cout << "			--open <Integer> : Number of opening iterations (Specifies kernel). Default: 0" << std::endl;
				std::cout << "			--close <Integer> : Number of closing iterations (Specifies neighborhood). Default: 0" << std::endl;
				std::cout << "	Quality / Time tradeoff: " << std::endl;
				std::cout << "		--P <Integer> : The higher this is, the faster our algorithm, with no sacrifice in quality. Local groups of cuts and fills whose product of connected kernel and neighborhood terminal nodes is less than this value will be processed as local clusters. Default: INFINITY" << std::endl;
				std::cout << "		--globalTime <Float or Integer> : Time threshold for the global steiner tree solver. The higher this value, the more optimal, but more time consuming. Default: 8." << std::endl;
				std::cout << "		--localTime <Float or Integer> : Time threshold for the local steiner tree solver. The higher this value, the more optimal, but more time consuming. Default: 3." << std::endl;
				std::cout << "		--hypernodeSize <Integer> : The maximum # of connected cuts / fills in a single hypernode of our augmented hypergraph. The higher this value, the more optimal, but more time consuming. Default: 1." << std::endl;
				std::cout << "		--se <Integer> : structuring element used for morphological opening and closing (0 for 3D cross, 1 for 18-connected neighborhood, 2 for 3D cube). Default: 0 (3D cross)." << std::endl;
				std::cout << "		--greedy: <Integer> : Perform greedy step at the end of our algorithm to monotonically improve our results. 1 for on, 0 for off. Default: 1." << std::endl;
				std::cout << "	Stats: " << std::endl;
				std::cout << "		--shapeTopo : (flag) Print in the console topology numbers of the input shape (as specified by --S, which is 0 by default)" << std::endl;
				std::cout << "		--finalTopo : (flag) Print in the console topology numbers of the final shape" << std::endl;
				std::cout << "		--showGeomCost : (flag) Print out the geometry cost of the result of our algorithm, as well as hypothetically applying all cuts, or all fills instead." << std::endl;
				std::cout << "	Comparing with Greedy Approach: " << std::endl;
				std::cout << "		--allGreedy: (flag) Instead of performing our algorithm, run a completely greedy approach." << std::endl;

				help = true;
				return;
			}
		}
	}
}

//Flood fill component inside core, starting from above the core threshold C 
void floodFillCore(std::vector<stbi_uc*> & g_Image3D, std::vector<uint32_t *> &  labels, int numSlices, int width, int height, float C, float S, int x, int y, int z, int& labelCtI, priority_queue<weightedCoord> & corePQ,
	std::vector<std::vector<float> > & floatImg
	) {
	std::queue<coord> coordinates;
	coord c = { x,y,z };
	std::vector<std::vector<int>> mask = structCross3D;
	coordinates.push(c);
	uint32_t labelCt = (uint32_t)labelCtI;
	labels[z][x + y * width] = labelCt;
	setVisitedFlag(labels[z][x + width * y], 1);
	while (!coordinates.empty()) {
		coord v = coordinates.front();
		setVisitedFlag(labels[v.z][v.x + width * v.y], 1);
		coordinates.pop();
		for (int i = 0; i < mask.size(); i++) { //Using 26-connectivity for foreground
			coord n = { v.x + mask[i][0], v.y + mask[i][1], v.z + mask[i][2] };
			if (n.x >= 0 && n.x < width && n.y >= 0 && n.y < height && n.z >= 0 && n.z < numSlices) { //Bounds check
				if (Label(labels[n.z][n.x + n.y * width]) == unvisited) {
					if (floatImg.size() > 0) {
						if ((float)floatImg[n.z][n.x + n.y*width] > C) { //neighboring voxel still in core
							coordinates.push(n);
							uint32_t label32 = labelCt;
							changeLabel(label32, labels[n.z][n.x + n.y * width]);
						}
						else {
							if ((float)floatImg[n.z][n.x + n.y*width] > S) { //neighboring voxel in shape, to be expanded during core inflation
								if (Visited(labels[n.z][n.x + width * n.y]) == false) { //place in core priority queue if not in a priority queue already
									weightedCoord wc = { n.x, n.y, n.z, ((float)floatImg[n.z][n.x + width * n.y]-S) };
									corePQ.push(wc);
									setVisitedFlag(labels[n.z][n.x + width * n.y], 1);
								}
							}
						}
					}
					else {

						if ((float)g_Image3D[n.z][n.x + n.y*width] > C) { //neighboring voxel still in core
							coordinates.push(n);
							uint32_t label32 = labelCt;
							changeLabel(label32, labels[n.z][n.x + n.y * width]);
						}
						else {
							if ((float)g_Image3D[n.z][n.x + n.y*width] > S) { //neighboring voxel in shape, to be expanded during core inflation
								if (Visited(labels[n.z][n.x + width * n.y]) == false) { //place in core priority queue if not in a priority queue already
									weightedCoord wc = { n.x, n.y, n.z, ((float)g_Image3D[n.z][n.x + width * n.y]-S) };
									corePQ.push(wc);
									setVisitedFlag(labels[n.z][n.x + width * n.y], 1);
								}
							}
						}
					}
				}
			}
		}
	}
}

//Flood fill component inside complement of neighborhood, starting from below the neighborhood threshold N 
void floodFillNeighborhood(std::vector<stbi_uc*> & g_Image3D, std::vector<uint32_t *>& labels, int numSlices, int width, int height, float S, float N, int x, int y, int z, int& labelCtI,  priority_queue< weightedCoord> & nPQ,
	std::vector< std::vector<float> > & floatImg
	) {
	std::queue<coord> coordinates; //std::vector<bool *> & inPQ,
	coord c = { x,y,z };
	std::vector<std::vector<int>> mask = structCube;
	coordinates.push(c);
	uint32_t labelCt = (uint32_t)labelCtI;
	labels[z][x + y * width] = labelCt;
	setVisitedFlag(labels[z][x + width * y], 1);
	while (!coordinates.empty()) {
		coord v = coordinates.front();
		coordinates.pop();
		setVisitedFlag(labels[v.z][v.x + width * v.y], 1);
		for (int i = 0; i < mask.size(); i++) { //Using 6-connectivity for background
			coord n = { v.x + mask[i][0], v.y + mask[i][1], v.z + mask[i][2] };
			if (n.x >= 0 && n.x < width && n.y >= 0 && n.y < height && n.z >= 0 && n.z < numSlices) { //Bounds check
				if (Label(labels[n.z][n.x + n.y * width]) == unvisited) {
					if (floatImg.size() > 0) {
						if ((float)floatImg[n.z][n.x + width * n.y] <= N) { //neighboring voxel still in core
							coordinates.push(n);
							uint32_t label32 = labelCt;
							changeLabel(label32, labels[n.z][n.x + n.y * width]);
						}

						else {
							if ((float)floatImg[n.z][n.x + width * n.y] <= S) { //neighboring voxel in shape, to be deflated to during neighborhood deflation
								if (Visited(labels[n.z][n.x + width * n.y]) == false) { //place in core priority queue if not in a priority queue already
									weightedCoord wc = { n.x, n.y, n.z, abs((float)floatImg[n.z][n.x + width * n.y] - S) }; //-abs((float)floatImg[n.z][n.x + width * n.y] - S) };
									nPQ.push(wc);
									setVisitedFlag(labels[n.z][n.x + width * n.y], 1);
								}
							}
						}
					}
					else {
						if ((float)g_Image3D[n.z][n.x + width * n.y] <= N) { //neighboring voxel still in core
							coordinates.push(n);
							uint32_t label32 = labelCt;
							changeLabel(label32, labels[n.z][n.x + n.y * width]);
						}

						else {
							if ((float)g_Image3D[n.z][n.x + width * n.y] <= S) { //neighboring voxel in shape, to be deflated to during neighborhood deflation
								if (Visited(labels[n.z][n.x + width * n.y]) == false) { //place in core priority queue if not in a priority queue already
									weightedCoord wc = { n.x, n.y, n.z, abs((float)g_Image3D[n.z][n.x + width * n.y] - S) }; //-abs((float)g_Image3D[n.z][n.x + width * n.y] - S) };
									nPQ.push(wc);
									setVisitedFlag(labels[n.z][n.x + width * n.y], 1);
								}
							}
						}
					}
				}
			}
		}
	}
}

void floodFillCAndN(std::vector<stbi_uc*> & g_Image3D, std::vector<uint32_t *>& labels, int numSlices, int width, int height, float C, float S, float N, int& labelCt, std::vector<node> & nodes, priority_queue<weightedCoord> & corePQ, priority_queue<weightedCoord>  & nPQ, 
	std::vector<std::vector<float> > & floatImg
	) {
	//flood fill core and neighborhood components in one sweep through volume
	for (int i = 0; i < width; i++) {
		for (int j = 0; j < height; j++) {
			for (int k = 0; k < numSlices; k++) {
				if (labels[k][i + j * width] == unvisited) {

					if(floatImg.size() > 0) {
						//Flood fill core component
						if ((float)floatImg[k][i + j * width] > C) {
							labelCt += 1; //Found new component, increment new node index
							node n; n.type = 0; n.inFg = 1; n.index = labelCt;
							nodes.push_back(n);
							floodFillCore(g_Image3D, labels, numSlices, width, height, C, S, i, j, k, labelCt, corePQ, floatImg);
						}

						//Flood fill neighborhood component
						if ((float)floatImg[k][i + j * width] <= N) {
							labelCt += 1; //Found new component, increment new node index
							node n; n.type = 1; n.inFg = 0; n.index = labelCt;
							nodes.push_back(n); // inPQ,
							floodFillNeighborhood(g_Image3D, labels, numSlices, width, height, S, N, i, j, k, labelCt, nPQ, floatImg);
						}
					}
					else {
						//Flood fill core component
						if ((float)g_Image3D[k][i + j * width] > C) {
							labelCt += 1; //Found new component, increment new node index
							node n; n.type = 0; n.inFg = 1; n.index = labelCt;
							nodes.push_back(n);
							floodFillCore(g_Image3D, labels, numSlices, width, height, C, S, i, j, k, labelCt, corePQ, floatImg);
						}

						//Flood fill neighborhood component
						if ((float)g_Image3D[k][i + j * width] <= N) {
							labelCt += 1; //Found new component, increment new node index
							node n; n.type = 1; n.inFg = 0; n.index = labelCt;
							nodes.push_back(n); // inPQ,
							floodFillNeighborhood(g_Image3D, labels, numSlices, width, height, S, N, i, j, k, labelCt, nPQ, floatImg);
						}
					}

				}
			}
		}
	}
}


bool ccNeighborFill6Conn(const std::vector<node>& nodes, int px, int py, int pz, int nx, int ny, int nz, const std::vector<uint32_t *>& labels, int width) {
	//is a six neighbor
	bool c1 = abs(nx - px) + abs(ny - py) + abs(nz - pz) == 1;
	if (c1) {
		return true;
	}
	if ((abs(nx - px) + abs(ny - py) + abs(nz - pz) == 2)) {
		if (pz == nz) {

			bool isNFillOrN1 = true;
			if (Label(labels[pz][nx + (width*py)]) != unvisited) {
				if (((int)nodes[Label(labels[pz][nx + (width*py)])].type) == 1) {
					isNFillOrN1 = false;
				}
			}

			bool isNFillOrN2 = true;
			if (Label(labels[pz][px + (width*ny)]) != unvisited) {
				if (((int)nodes[Label(labels[pz][px + (width*ny)])].type) == 1) {
					isNFillOrN2 = false;
				}
			}

			//2D diagonals are not fill or neighbor
			return isNFillOrN1 && isNFillOrN2;
		}

		if (px == nx) {
			bool isNFillOrN1 = true;
			if (Label(labels[nz][px + (width*py)]) != unvisited) {
				if (((int)nodes[Label(labels[nz][px + (width*py)])].type) == 1) {
					isNFillOrN1 = false;
				}
			}

			bool isNFillOrN2 = true;
			if (Label(labels[pz][px + (width*ny)]) != unvisited) {
				if (((int)nodes[Label(labels[pz][px + (width*ny)])].type) == 1) {
					isNFillOrN2 = false;
				}
			}
			//2D diagonals are not fill or neighbor
			return isNFillOrN1 && isNFillOrN2;
		}

		if (py == ny) {
			bool isNFillOrN1 = true;
			if (Label(labels[nz][px + (width*py)]) != unvisited) {
				if (((int)nodes[Label(labels[nz][px + (width*py)])].type) == 1) {
					isNFillOrN1 = false;
				}
			}

			bool isNFillOrN2 = true;
			if (Label(labels[pz][nx + (width*py)]) != unvisited) {
				if (((int)nodes[Label(labels[pz][nx + (width*py)])].type) == 1) {
					isNFillOrN2 = false;
				}
			}
			return isNFillOrN1 && isNFillOrN2;
		}
	}

	if ((abs(nx - px) + abs(ny - py) + abs(nz - pz) == 3)) {
		bool isNFillOrN = true;
		if (Label(labels[nz][px + (width*ny)]) != unvisited) {
			if (((int)nodes[Label(labels[nz][px + (width*ny)])].type) == 1) {
				isNFillOrN = false;
			}
		}
		if (Label(labels[nz][nx + (width*py)]) != unvisited) {
			if (((int)nodes[Label(labels[nz][nx + (width*py)])].type) == 1) {
				isNFillOrN = false;
			}
		}
		if (Label(labels[pz][nx + (width*py)]) != unvisited) {
			if (((int)nodes[Label(labels[pz][nx + (width*py)])].type) == 1) {
				isNFillOrN = false;
			}
		}
		if (Label(labels[nz][px + (width*py)]) != unvisited) {
			if (((int)nodes[Label(labels[nz][px + (width*py)])].type) == 1) {
				isNFillOrN = false;
			}
		}
		if (Label(labels[pz][px + (width*ny)]) != unvisited) {
			if (((int)nodes[Label(labels[pz][px + (width*ny)])].type) == 1) {
				isNFillOrN = false;
			}
		}
		if (Label(labels[pz][nx + (width*ny)]) != unvisited) {
			if (((int)nodes[Label(labels[pz][nx + (width*ny)])].type) == 1) {
				isNFillOrN = false;
			}
		}
		return isNFillOrN;
	}
	cout << "case not addressed for fill connectivity, with coordinate " << px << " " << py << " " << pz << " and neighbor " << nx << " " << ny << " " << nz << endl;
	return false;
}

bool ccNeighborOpenClose(const std::vector<node>& nodes, int px, int py, int pz, int nx, int ny, int nz, const std::vector<uint32_t *>& labels, int width, int height, int numSlices,
	const std::vector<std::vector<int> >& g_Image3D
) {
	//is a six neighbor
	bool c1 = abs(nx - px) + abs(ny - py) + abs(nz - pz) == 1;
	if (c1) {
		return true;
	}

	if ((abs(nx - px) + abs(ny - py) + abs(nz - pz) == 2)) {
		if (pz == nz) {
			bool isNFillOrN1 = true;
			if (((int)g_Image3D[pz][nx + (width*py)]) == 0) {
				isNFillOrN1 = false;
			}

			bool isNFillOrN2 = true;
			if (((int)g_Image3D[pz][px + (width*ny)]) == 0) {
				isNFillOrN2 = false;
			}
			//2D diagonals are not fill or neighbor
			return isNFillOrN1 && isNFillOrN2;
		}

		if (px == nx) {
			bool isNFillOrN1 = true;
			if (((int)g_Image3D[nz][px + (width*py)]) == 0) {
				isNFillOrN1 = false;
			}

			bool isNFillOrN2 = true;
			if (((int)g_Image3D[pz][px + (width*ny)]) == 0) {
				isNFillOrN2 = false;
			}
			//2D diagonals are not fill or neighbor
			return isNFillOrN1 && isNFillOrN2;
		}

		if (py == ny) {
			bool isNFillOrN1 = true;
			if (((int)g_Image3D[nz][px + (width*py)]) == 0) {
				isNFillOrN1 = false;
			}

			bool isNFillOrN2 = true;
			if (((int)g_Image3D[pz][nx + (width*py)]) == 0) {
				isNFillOrN2 = false;
			}
			return isNFillOrN1 && isNFillOrN2;
		}
	}

	if (abs(nx - px) + abs(ny - py) + abs(nz - pz) == 3) {

		bool isNFillOrN = true;
		if (((float)g_Image3D[nz][px + (width*ny)]) == 0) {
			isNFillOrN = false;
		}
		if (((float)g_Image3D[nz][nx + (width*py)]) == 0) {
			isNFillOrN = false;
		}
		if (((float)g_Image3D[pz][nx + (width*py)]) == 0) {
			isNFillOrN = false;
		}
		if (((float)g_Image3D[nz][px + (width*py)]) == 0) {
			isNFillOrN = false;
		}
		if (((float)g_Image3D[pz][px + (width*ny)]) == 0) {
			isNFillOrN = false;
		}
		if (((float)g_Image3D[pz][nx + (width*ny)]) == 0) {
			isNFillOrN = false;
		}
		return isNFillOrN;
	}
	cout << "case not addressed (bug) at coordinate " << px << " " << py << " " << pz << " with neighbor " << nx << " " << ny << " " << nz << endl;
	//3D diagonals are not core
	return false;
}

void shrink(std::vector< std::vector<int> >& shape, int label, int numSlices, int width, int height, int se) {
	std::vector< std::vector<int> > structE;
	if (se == 0) {
		structE = structCross3D;
	}
	else {
		if (se == 1) {
			structE = struct18;
		}
		else {
			structE = structCube;
		}
	}
	std::vector<std::vector<int>> newShape = shape;
	for (int i = 0; i < width; i++) {
		for (int j = 0; j < height; j++) {
			for (int k = 0; k < numSlices; k++) {
				if (shape[k][i + j * width] == label) {
					for (int s = 0; s < structE.size(); s++) {
						int x = i + structE[s][0]; int y = j + structE[s][1]; int z = k + structE[s][2];
						if (x >= 0 && x < width && y >= 0 && y < height && z >= 0 && z < numSlices) {
							if ((int)shape[z][x + y * width] != label) {
								newShape[k][i + j * width] = 1 - label;
							}
						}
					}
				}
			}
		}
	}
	shape = newShape;
}

void erode(std::vector<std::vector<int>>& shape, int numSlices, int width, int height, int se) {
	shrink(shape, 1, numSlices, width, height, se);
}

void dilate(std::vector<std::vector<int>>& shape, int numSlices, int width, int height, int se) {
	shrink(shape, 0, numSlices, width, height, se);
}

void open(std::vector<std::vector<int>>& shape, int numSlices, int width, int height, int k, int se) {
	for (int i = 0; i < k; i++) {
		erode(shape, numSlices, width, height, se);
	}
	for (int i = 0; i < k; i++) {
		dilate(shape, numSlices, width, height, se);
	}
}

void close(std::vector<std::vector<int>>& shape, int numSlices, int width, int height, int k, int se) {

	for (int i = 0; i < k; i++) {
		dilate(shape, numSlices, width, height, se);
	}
	for (int i = 0; i < k; i++) {
		erode(shape, numSlices, width, height, se);
	}
}


void addEdge(Graph & G, map<std::vector<int>, int>& edgeWt, int i1, int i2, int px, int py, int pz, int nx, int ny, int nz) {
	if (edgeWt.find({ i1,i2 }) != edgeWt.end()) { //Indicate edge is 6-connected
		int wt = edgeWt[{i1, i2}];
		if (wt == 0 && (abs(nx - px) + abs(ny - py) + abs(nz - pz) == 1)) {
			edgeWt[{i1, i2}] = 1; edgeWt[{i2, i1}] = 1;
		}
	}
	else { // Edge doesnt exist
		add_edge(i1, i2, G);
		if (abs(nx - px) + abs(ny - py) + abs(nz - pz) == 1) {
			edgeWt[{i1, i2}] = 1; edgeWt[{i2, i1}] = 1;
		}
		else {
			edgeWt[{i1, i2}] = 0; edgeWt[{i2, i1}] = 0;
		}

	}
}

void floodFillBinOpenClose(std::vector<std::vector<int> > & shape, std::vector<uint32_t *> &  labels, int numSlices, int width, int height, int x, int y, int z, int labelCtI, std::vector<node> & nodes) {
	std::queue<coord> coordinates;
	coord c = { x,y,z };
	std::vector<std::vector<int>> mask = structCross3D;
	coordinates.push(c);
	uint32_t labelCt = (uint32_t)labelCtI;

	while (!coordinates.empty()) {
		coord v = coordinates.front();
		setVisitedFlag(labels[v.z][v.x + width * v.y], 1);
		if (((int)nodes[labelCtI].type) == CUT || ((int)nodes[labelCtI].type) == FILL) {
			nodes[labelCtI].labelCost += 1;
		}

		labels[v.z][v.x + width * v.y] = labelCt;
		coordinates.pop();
		for (int i = 0; i < mask.size(); i++) { //Using 6-connectivity for foreground
			coord n = { v.x + mask[i][0], v.y + mask[i][1], v.z + mask[i][2] };
			if (n.x >= 0 && n.x < width && n.y >= 0 && n.y < height && n.z >= 0 && n.z < numSlices) { //Bounds check
				if (Label(labels[n.z][n.x + n.y * width]) == unvisited) {
					if (shape[n.z][n.x + n.y * width] == 1) {
						coordinates.push(n);
						uint32_t label32 = labelCt;
						changeLabel(label32, labels[n.z][n.x + n.y * width]);
					}
				}
			}
		}

	}
}

//Writes out cell complex as a binary file, to be read into Dipha program to measure min persistence values
void writeComplex(std::string fName, const std::vector<std::vector<int> >& g_Image3D, int width, int height, int numSlices) {
	std::ofstream complexFile(fName, std::ios::binary);
	int64_t magic = 8067171840;
	complexFile.write(reinterpret_cast<const char *>(&magic), sizeof(magic));
	int64_t type = 1;
	complexFile.write(reinterpret_cast<const char *>(&type), sizeof(type));
	int64_t numPts = width * height*numSlices;
	complexFile.write(reinterpret_cast<const char *>(&numPts), sizeof(numPts));
	int64_t dim = 3;
	complexFile.write(reinterpret_cast<const char *>(&dim), sizeof(dim));
	int64_t w = width; int64_t h = height; int64_t d = numSlices;
	complexFile.write(reinterpret_cast<const char *>(&w), sizeof(w));
	complexFile.write(reinterpret_cast<const char *>(&h), sizeof(h));
	complexFile.write(reinterpret_cast<const char *>(&d), sizeof(d));
	string dataStr = "";

	for (int k = 0; k < numSlices; k++) {
		for (int j = 0; j < height; j++) {
			for (int i = 0; i < width; i++) {
				double data = 255.0 - ((double)g_Image3D[k][i + j * width]);
				complexFile.write((char*)&data, sizeof(double));
			}
		}
	}

	complexFile.close();
}

//First set shape to 1
void generatingOpenCloseGenerators(std::vector<std::vector<int>>& shape, std::vector<uint32_t*>& labels, int numSlices, int width, int height, std::vector<node>& nodes,
	int openItr, int closeItr, Graph& G, map<std::vector<int>, int>& edgeWt, int se) {

	std::vector<std::vector<int>> openedImg;
	std::vector<std::vector<int>> closedImg;
	std::vector<std::vector<int>> cutImg;
	std::vector<std::vector<int>> fillImg;
	std::vector<std::vector<int>> nImg;
	for (int k = 0; k < numSlices; k++) {
		std::vector<int> slice(width * height, 0);
		for (int i = 0; i < width; i++) {
			for (int j = 0; j < height; j++) {
				int val = shape[k][i + j * width];

				slice[i + j * width] = val;

			}
		}
		std::vector<int> sliceN(width * height, 1);
		openedImg.push_back(slice); closedImg.push_back(slice);
		cutImg.push_back(slice); fillImg.push_back(slice); nImg.push_back(sliceN);
	}

	open(openedImg, numSlices, width, height, openItr, se);
	close(closedImg, numSlices, width, height, closeItr, se);

	for (int i = 0; i < width; i++) {
		for (int j = 0; j < height; j++) {
			for (int k = 0; k < numSlices; k++) {
				cutImg[k][i + j * width] = shape[k][i + j * width] - openedImg[k][i + j * width];
				fillImg[k][i + j * width] = closedImg[k][i + j * width] - shape[k][i + j * width];
				shape[k][i + j * width] = shape[k][i + j * width] - cutImg[k][i + j * width];
				nImg[k][i + j * width] = nImg[k][i + j * width] - shape[k][i + j * width] - cutImg[k][i + j * width] - fillImg[k][i + j * width];


			}
		}
	}

	for (int i = 0; i < width; i++) {
		for (int j = 0; j < height; j++) {
			for (int k = 0; k < numSlices; k++) {
				if (Label(labels[k][i + (width * j)]) == unvisited) {
					if (shape[k][i + (width * j)] == 1) {
						int numNodes = nodes.size();
						node n; n.index = numNodes; n.type = CORE; n.inFg = 1; n.labelCost = 0;
						nodes.push_back(n);
						floodFillBinOpenClose(shape, labels, numSlices, width, height, i, j, k, numNodes, nodes);

					}
					else {
						if (cutImg[k][i + (width * j)] == 1) {
							int numNodes = nodes.size();
							node n; n.index = numNodes; n.type = CUT; n.inFg = 1; n.labelCost = 0;
							nodes.push_back(n);
							floodFillBinOpenClose(cutImg, labels, numSlices, width, height, i, j, k, numNodes, nodes);

						}
						else {
							if (fillImg[k][i + (width * j)] == 1) {
								int numNodes = nodes.size();
								node n; n.index = numNodes; n.type = FILL; n.inFg = 0; n.labelCost = 0;
								nodes.push_back(n);
								floodFillBinOpenClose(fillImg, labels, numSlices, width, height, i, j, k, numNodes, nodes);
								nodes[nodes.size() - 1].labelCost = -nodes[nodes.size() - 1].labelCost;
							}
							else {
								if (nImg[k][i + (width * j)] == 1) {
									int numNodes = nodes.size();
									node n; n.index = numNodes; n.type = N; n.inFg = 0; n.labelCost = 0;
									nodes.push_back(n);
									floodFillBinOpenClose(nImg, labels, numSlices, width, height, i, j, k, numNodes, nodes);

								}
							}
						}
					}



				}

			}
		}
	}

	for (int i = 0; i < nodes.size(); i++) {
		add_vertex(G);
	}
	for (int i = 0; i < width; i++) {
		for (int j = 0; j < height; j++) {
			for (int k = 0; k < numSlices; k++) {

				if (((int)nodes[Label(labels[k][i + (width * j)])].type) == CUT || ((int)nodes[Label(labels[k][i + (width * j)])].type) == FILL) {
					for (int n = 0; n < structCube.size(); n++) {
						int nx = i + structCube[n][0]; int ny = j + structCube[n][1]; int nz = k + structCube[n][2];
						if (nx >= 0 && nx < width && ny >= 0 && ny < height && nz >= 0 && nz < numSlices) {
							int v1 = Label(labels[k][i + (width * j)]); int v2 = Label(labels[nz][nx + (width * ny)]);

							if (v1 != v2) {
								addEdge(G, edgeWt, v1, v2, i, j, k, nx, ny, nz);
							}
						}
					}

				}
			}
		}
	}

}


int neighborhoodToIndex(std::vector<std::vector<std::vector<int>>>& neighborhood) {
	int ct = 0;
	uint32_t index = 0;
	for (int i = 0; i < 3; i++) {
		for (int j = 0; j < 3; j++) {
			for (int k = 0; k < 3; k++) {
				nBitToX(index, ct, neighborhood[i][j][k]);
				ct += 1;
			}
		}
	}
	return index;
}

bool simple3DDictionary(const std::vector<uint32_t*>& labels, const std::vector<node> & nodes, int x, int y, int z, int numSlices, int width, int height, const std::vector<unsigned char> & simpleDictionary3D, int lType, bool inFg, int conn) {
	int minX = max(x - 1, 0); int minY = max(y - 1, 0); int minZ = max(z - 1, 0);
	int maxX = min(x + 2, width); int maxY = min(y + 2, height); int maxZ = min(z + 2, numSlices);
	//Correct version
	std::vector< std::vector< std::vector<int>>> cubeN(3, std::vector<std::vector<int>>(3, std::vector<int>(3, 0)));
	for (int i = minX; i < maxX; i++) {
		for (int j = minY; j < maxY; j++) {
			for (int k = minZ; k < maxZ; k++) {
				if (Label(labels[k][i + j * width]) != unvisited) {

					if (((int)nodes[Label(labels[k][i + j * width])].type) == lType) {
						cubeN[i - x + 1][j - y + 1][k - z + 1] = 1;

					}
					else {
						cubeN[i - x + 1][j - y + 1][k - z + 1] = 0;
					}
				}
				else {
					cubeN[i - x + 1][j - y + 1][k - z + 1] = 0;
				}
			}
		}
	}


	if (conn == 0) {
		for (int i = 0; i < 3; i++) {
			for (int j = 0; j < 3; j++) {
				for (int k = 0; k < 3; k++) {
					cubeN[i][j][k] = 1 - cubeN[i][j][k];
				}
			}
		}
	}
	return ((int)simpleDictionary3D[neighborhoodToIndex(cubeN)]) == 49;

}

float gradient(int x, int y, int z, const std::vector<stbi_uc*>& g_Image3D, int width, int height, int numSlices) {
	int xA = min(x + 1, width - 1); int xB = max(x - 1, 0);
	float gX = (float)g_Image3D[z][xA + y * width] - (float)g_Image3D[z][xB + y * width];

	int yA = min(y + 1, height - 1); int yB = max(y - 1, 0);
	float gY = (float)g_Image3D[z][x + yA * width] - (float)g_Image3D[z][x + yB * width];

	int zA = min(z + 1, numSlices - 1); int zB = max(z - 1, 0);
	float gZ = (float)g_Image3D[zA][x + y * width] - (float)g_Image3D[zB][x + y * width];
	return sqrtf((gX*gX) + (gY*gY) + (gZ*gZ));
}

float gradientF(int x, int y, int z, const std::vector< std::vector<float> >& g_Image3DF, int width, int height, int numSlices) {
	int xA = min(x + 1, width - 1); int xB = max(x - 1, 0);
	float gX = (float)g_Image3DF[z][xA + y * width] - (float)g_Image3DF[z][xB + y * width];

	int yA = min(y + 1, height - 1); int yB = max(y - 1, 0);
	float gY = (float)g_Image3DF[z][x + yA * width] - (float)g_Image3DF[z][x + yB * width];

	int zA = min(z + 1, numSlices - 1); int zB = max(z - 1, 0);
	float gZ = (float)g_Image3DF[zA][x + y * width] - (float)g_Image3DF[zB][x + y * width];
 	return sqrtf((gX*gX) + (gY*gY) + (gZ*gZ));
}

void inflateCore(std::vector<uint32_t *>& labels, const std::vector<stbi_uc*>& g_Image3D, std::vector<node> & nodes, priority_queue<weightedCoord> & corePQ, int numSlices, int width,
	int height, float C, float S, const std::vector<unsigned char> & simpleDictionary3D, const std::vector< std::vector<float> >& floatImg) {
	priority_queue<weightedCoord> unsimpleVoxels;
	std::vector<std::vector<int>> mask = structCube;
	while (!corePQ.empty()) {
		weightedCoord wc = corePQ.top(); //Pop voxel on core boundary with closest intensity to shape
		corePQ.pop();
		auto sTime = std::chrono::high_resolution_clock::now();
		if (simple3DDictionary(labels, nodes, wc.x, wc.y, wc.z, numSlices, width, height, simpleDictionary3D, 0, true, 1)) {
			auto sTime2 = std::chrono::high_resolution_clock::now();
			std::chrono::duration<double> elapsed = sTime2 - sTime;
			bool wasSet = false;
			for (int i = 0; i < mask.size(); i++) { //Since 3 x3 neighborhood is simple with respect to inflated core, can pick label of any neighbor to be current one,
				Coordinate n(wc.x + mask[i][0], wc.y + mask[i][1], wc.z + mask[i][2]);
				if (n.x >= 0 && n.y >= 0 && n.z >= 0 && n.x < width && n.y < height && n.z < numSlices) { //bounds check
					if (abs(mask[i][0]) + abs(mask[i][1]) + abs(mask[i][2]) == 1) {
						if (Label(labels[n.z][n.x + n.y * width]) != unvisited) {
							if (((int)nodes[Label(labels[n.z][n.x + n.y * width])].type) == 0) { //current voxel on boundary, which means one of neighbors is already in core
								if (Label(labels[wc.z][wc.x + wc.y * width]) == unvisited) {
									labels[wc.z][wc.x + wc.y * width] = labels[n.z][n.x + n.y * width];
								}
							}
						}
					}

					if (floatImg.size() > 0) {
						if ((float)floatImg[n.z][n.x + n.y * width] > S && (float)floatImg[n.z][n.x + n.y * width] <= C && Visited(labels[n.z][n.x + n.y * width]) == false) {

							weightedCoord wnc = { n.x, n.y, n.z,  ((float)floatImg[n.z][n.x + n.y * width]-S) };
							corePQ.push(wnc);
							setVisitedFlag(labels[n.z][n.x + n.y * width], 1);
						}
					}
					else {
						//Continue expanding to unvisited neighbors in correct intensity range
						if ((float)g_Image3D[n.z][n.x + n.y * width] > S && (float)g_Image3D[n.z][n.x + n.y * width] <= C && Visited(labels[n.z][n.x + n.y * width]) == false) {

							weightedCoord wnc = { n.x, n.y, n.z,  ((float)g_Image3D[n.z][n.x + n.y * width]-S) };
							corePQ.push(wnc);
							setVisitedFlag(labels[n.z][n.x + n.y * width], 1);
						}
					}
				}
			}
		}
		else {
			//Pixel not simple, but may become simple later, so unflag in inPQ table to allow for a later visit
			auto sTime2 = std::chrono::high_resolution_clock::now();
			std::chrono::duration<double> elapsed = sTime2 - sTime;
			setVisitedFlag(labels[wc.z][wc.x + wc.y * width], 0);
		}
	}
}

void inflateCoreBB(std::vector<uint32_t *>& labels, std::vector<node> & nodes, priority_queue<weightedCoord> & corePQ, int numSlices, int width,
	int height, float S, const std::vector<unsigned char> & simpleDictionary3D, const std::vector<stbi_uc*>& g_Image3D, const std::vector< std::vector<float> >& floatImg) {
	priority_queue<weightedCoord> unsimpleVoxels;
	std::vector<std::vector<int>> mask = structCube;
	while (!corePQ.empty()) {
		weightedCoord wc = corePQ.top(); //Pop voxel on core boundary with closest intensity to shape
		corePQ.pop();
		auto sTime = std::chrono::high_resolution_clock::now();
		if (simple3DDictionary(labels, nodes, wc.x, wc.y, wc.z, numSlices, width, height, simpleDictionary3D, 0, true, 1)) {
			auto sTime2 = std::chrono::high_resolution_clock::now();
			std::chrono::duration<double> elapsed = sTime2 - sTime;
			bool wasSet = false;
			for (int i = 0; i < mask.size(); i++) { //Since 3 x3 neighborhood is simple with respect to inflated core, can pick label of any neighbor to be current one,
				Coordinate n(wc.x + mask[i][0], wc.y + mask[i][1], wc.z + mask[i][2]);
				if (n.x >= 0 && n.y >= 0 && n.z >= 0 && n.x < width && n.y < height && n.z < numSlices) { //bounds check
					if (abs(mask[i][0]) + abs(mask[i][1]) + abs(mask[i][2]) == 1) {
						if (Label(labels[n.z][n.x + n.y * width]) != unvisited) {
							if (((int)nodes[Label(labels[n.z][n.x + n.y * width])].type) == 0) { //current voxel on boundary, which means one of neighbors is already in core
								if (Label(labels[wc.z][wc.x + wc.y * width]) == unvisited) {
									labels[wc.z][wc.x + wc.y * width] = labels[n.z][n.x + n.y * width];
								}
							}
						}
					}
					if (floatImg.size() > 0) {
						if ((float)floatImg[n.z][n.x + n.y * width] > S && Visited(labels[n.z][n.x + n.y * width]) == false) {

							weightedCoord wnc = { n.x, n.y, n.z,  ((float)floatImg[n.z][n.x + n.y * width] - S) };
							corePQ.push(wnc);
							setVisitedFlag(labels[n.z][n.x + n.y * width], 1);
						}
					}
					else {
						if ((float)g_Image3D[n.z][n.x + n.y * width] > S && Visited(labels[n.z][n.x + n.y * width]) == false) {

							weightedCoord wnc = { n.x, n.y, n.z,  ((float)g_Image3D[n.z][n.x + n.y * width] - S) };
							corePQ.push(wnc);
							setVisitedFlag(labels[n.z][n.x + n.y * width], 1);
						}
					}
				}
			}
		}
		else {
			//Pixel not simple, but may become simple later, so unflag in inPQ table to allow for a later visit
			auto sTime2 = std::chrono::high_resolution_clock::now();
			std::chrono::duration<double> elapsed = sTime2 - sTime;
			setVisitedFlag(labels[wc.z][wc.x + wc.y * width], 0);
		}
	}
}

void deflateNeighborhood(const std::vector<uint32_t *>& labels, const std::vector<stbi_uc*>& g_Image3D, std::vector<node> & nodes,
	priority_queue<weightedCoord> & nPQ, int numSlices, int width, int height, float N, float S,
	const std::vector<unsigned char> & simpleDictionary3D, const std::vector< std::vector<float> >& floatImg) {
	while (!nPQ.empty()) {
		weightedCoord wc = nPQ.top(); //Pop voxel on neighborhood boundary with closest intensity to shape
		nPQ.pop();
		auto sTime = std::chrono::high_resolution_clock::now();
		if (simple3DDictionary(labels, nodes, wc.x, wc.y, wc.z, numSlices, width, height, simpleDictionary3D, 1, false, 0)) {
			auto sTime2 = std::chrono::high_resolution_clock::now();
			std::chrono::duration<double> elapsed = sTime2 - sTime;
			for (int i = 0; i < structCube.size(); i++) { //Since 3 x3 neighborhood is simple with respect to inflated core, can pick label of any neighbor to be current one,
				Coordinate n(wc.x + structCube[i][0], wc.y + structCube[i][1], wc.z + structCube[i][2]);
				if (n.x >= 0 && n.y >= 0 && n.z >= 0 && n.x < width && n.y < height && n.z < numSlices) { //bounds check
					if (Label(labels[n.z][n.x + (n.y * width)]) != unvisited) {
						if (((int)nodes[Label(labels[n.z][n.x + (n.y * width)])].type) == 1) { //current voxel on boundary, which means one of neighbors is already in core
							if (Label(labels[wc.z][wc.x + wc.y * width]) == unvisited) {
								labels[wc.z][wc.x + wc.y * width] = labels[n.z][n.x + (n.y * width)];
							}
						}
					}
					if (floatImg.size() > 0) {
						if ((float)floatImg[n.z][n.x + n.y * width] <= S && (float)floatImg[n.z][n.x + n.y * width] > N && Visited(labels[n.z][n.x + (n.y * width)]) == false) {

							weightedCoord wnc = { n.x, n.y, n.z, abs((float)floatImg[n.z][n.x + width * n.y] - S) };
							nPQ.push(wnc);
							setVisitedFlag(labels[n.z][n.x + (n.y * width)], 1);
						}
					}
					else {
						if ((float)g_Image3D[n.z][n.x + n.y * width] <= S && (float)g_Image3D[n.z][n.x + n.y * width] > N && Visited(labels[n.z][n.x + (n.y * width)]) == false) {

							weightedCoord wnc = { n.x, n.y, n.z, abs((float)g_Image3D[n.z][n.x + width * n.y] - S) };
							nPQ.push(wnc);
							setVisitedFlag(labels[n.z][n.x + (n.y * width)], 1);
						}
					}
				}
			}
		}
		else {
			auto sTime2 = std::chrono::high_resolution_clock::now();
			std::chrono::duration<double> elapsed = sTime2 - sTime;
			//Voxel not simple, but may become simple later, so unflag in inPQ table to allow for a later visit
			setVisitedFlag(labels[wc.z][wc.x + wc.y * width], 0);
		}
	}
}

bool ccNeighbor(const std::vector<node>& nodes, int px, int py, int pz, int nx, int ny, int nz, const std::vector<uint32_t *>& labels, int width, int height, int numSlices, const std::vector<stbi_uc*>& g_Image3D, 
	const std::vector< std::vector<float> > & g_Image3DF,
	float S) {
	//is a six neighbor
	bool c1 = abs(nx - px) + abs(ny - py) + abs(nz - pz) == 1;
	if (c1) {
		return true;
	}

	if ((abs(nx - px) + abs(ny - py) + abs(nz - pz) == 2)) {
		bool useFloat = g_Image3DF.size() > 0;

		if (pz == nz) {

			bool isNFillOrN1 = true;
			if (Label(labels[pz][nx + (width*py)]) != unvisited) {
				if (useFloat) {
					if (((float)g_Image3DF[pz][nx + (width*py)]) <= S) {
						isNFillOrN1 = false;
					}
				}
				else {
					if (((float)g_Image3D[pz][nx + (width*py)]) <= S) {
						isNFillOrN1 = false;
					}
				}
			}

			bool isNFillOrN2 = true;
			if (Label(labels[pz][px + (width*ny)]) != unvisited) {
				if (useFloat) {
					if (((float)g_Image3DF[pz][px + (width*ny)]) <= S) {
						isNFillOrN2 = false;
					}
				}
				else {
					if (((float)g_Image3D[pz][px + (width*ny)]) <= S) {
						isNFillOrN2 = false;
					}
				}
			}
			//2D diagonals are not fill or neighbor
			return isNFillOrN1 && isNFillOrN2;
		}

		if (px == nx) {
			bool isNFillOrN1 = true;
			if (Label(labels[nz][px + (width*py)]) != unvisited) {
				if (useFloat) {
					if (((float)g_Image3DF[nz][px + (width*py)]) <= S) {
						isNFillOrN1 = false;
					}
				}
				else {
					if (((float)g_Image3D[nz][px + (width*py)]) <= S) {
						isNFillOrN1 = false;
					}
				}
			}

			bool isNFillOrN2 = true;
			if (Label(labels[pz][px + (width*ny)]) != unvisited) {
				if (useFloat) {
					if (((float)g_Image3DF[pz][px + (width*ny)]) <= S) {
						isNFillOrN2 = false;
					}
				}
				else {
					if (((float)g_Image3D[pz][px + (width*ny)]) <= S) {
						isNFillOrN2 = false;
					}
				}
			}
			//2D diagonals are not fill or neighbor
			return isNFillOrN1 && isNFillOrN2;
		}

		if (py == ny) {
			bool isNFillOrN1 = true;
			if (Label(labels[nz][px + (width*py)]) != unvisited) {
				if (useFloat) {
					if (((float)g_Image3DF[nz][px + (width*py)]) <= S) {
						isNFillOrN1 = false;
					}
				}
				else {
					if (((float)g_Image3D[nz][px + (width*py)]) <= S) {
						isNFillOrN1 = false;
					}
				}
			}

			bool isNFillOrN2 = true;
			if (Label(labels[pz][nx + (width*py)]) != unvisited) {
				if (useFloat) {
					if (((float)g_Image3DF[pz][nx + (width*py)]) <= S) {
						isNFillOrN2 = false;
					}
				}
				else {
					if (((float)g_Image3D[pz][nx + (width*py)]) <= S) {
						isNFillOrN2 = false;
					}
				}
			}
			return isNFillOrN1 && isNFillOrN2;
		}
	}

	if (abs(nx - px) + abs(ny - py) + abs(nz - pz) == 3) {
		bool useFloat = g_Image3DF.size() > 0;
		bool isNFillOrN = true;
		if (Label(labels[nz][px + (width*ny)]) != unvisited) {
			if (useFloat) {
				if ((g_Image3DF[nz][px + (width*ny)]) <= S) {
					isNFillOrN = false;
				}
			}
			else {
				if (((float)g_Image3D[nz][px + (width*ny)]) <= S) {
					isNFillOrN = false;
				}
			}
		}
		if (Label(labels[nz][nx + (width*py)]) != unvisited) {
			if (useFloat) {
				if (((float)g_Image3DF[nz][nx + (width*py)]) <= S) {
					isNFillOrN = false;
				}
			}
			else {
				if (((float)g_Image3D[nz][nx + (width*py)]) <= S) {
					isNFillOrN = false;
				}
			}
		}
		if (Label(labels[pz][nx + (width*py)]) != unvisited) {
			if (useFloat) {
				if (((float)g_Image3DF[pz][nx + (width*py)]) <= S) {
					isNFillOrN = false;
				}
			}
			else {
				if (((float)g_Image3D[pz][nx + (width*py)]) <= S) {
					isNFillOrN = false;
				}
			}
		}
		if (Label(labels[nz][px + (width*py)]) != unvisited) {
			if (useFloat) {
				if (((float)g_Image3DF[nz][px + (width*py)]) <= S) {
					isNFillOrN = false;
				}
			}
			else {
				if (((float)g_Image3D[nz][px + (width*py)]) <= S) {
					isNFillOrN = false;
				}
			}
		}
		if (Label(labels[pz][px + (width*ny)]) != unvisited) {
			if (useFloat) {
				if (((float)g_Image3DF[pz][px + (width*ny)]) <= S) {
					isNFillOrN = false;
				}
			}
			else {
				if (((float)g_Image3D[pz][px + (width*ny)]) <= S) {
					isNFillOrN = false;
				}
			}
		}
		if (Label(labels[pz][nx + (width*ny)]) != unvisited) {
			if (useFloat) {
				if (((float)g_Image3DF[pz][nx + (width*ny)]) <= S) {
					isNFillOrN = false;
				}
			}
			else {
				if (((float)g_Image3D[pz][nx + (width*ny)]) <= S) {
					isNFillOrN = false;
				}
			}
		}
		return isNFillOrN;
	}
	cout << "case not addressed (bug) at coordinate " << px << " " << py << " " << pz << " with neighbor " << nx << " " << ny << " " << nz << endl;
	//3D diagonals are not core
	return false;
}



void identifyCutFromPixel3D(std::vector<uint32_t *>& labels, std::vector<stbi_uc*>& g_Image3D, Graph & G, int x, int y, int z, int width, int height,
	int numSlices, int labelCtI, node &n, float S, std::vector<node> & nodes, map<std::vector<int>, int>& edgeWt, int genMode, std::vector< std::vector<float> > & floatImg) {
	uint32_t labelCt = (uint32_t)labelCtI;
	std::queue<tuple<int, int, int>> q; q.push({ x,y,z });
	n.labelCost = 0; n.floatCost = 0.0;  n.intensity = 0.0;
	while (!q.empty()) {
		tuple<int, int, int> pt = q.front(); q.pop();
		int px = get<0>(pt); int py = get<1>(pt); int pz = get<2>(pt);
		if (Label(labels[pz][px + (width*py)]) == unvisited) {
			if (floatImg.size() > 0) {
				n.floatCost += 1.0;
			}
			else {
				n.floatCost += (gradient(x, y, z, g_Image3D, width, height, numSlices));
			}
			changeLabel(labelCt, labels[pz][px + (width*py)]);
			setVisitedFlag(labels[pz][px + (width*py)], 1);
			//flood fill to nearby neighbor depending on priority.
			for (int i = 0; i < structCube.size(); i++) {
				int nx = px + structCube[i][0]; int ny = py + structCube[i][1]; int nz = pz + structCube[i][2];

				if (nx >= 0 && nx < width && ny >= 0 && ny < height && nz >= 0 && nz < numSlices) {
					if (floatImg.size() > 0) {
						if (floatImg[nz][nx + (width*ny)] > S) {
							if (Label(labels[nz][nx + (width*ny)]) == unvisited) { //unvisited
								if (ccNeighbor(nodes, px, py, pz, nx, ny, nz, labels, width, height, numSlices, g_Image3D, floatImg, S)) {
									q.push(std::make_tuple(nx, ny, nz));
								}
							}
						}
					}
					else {
						if ((float)g_Image3D[nz][nx + (width*ny)] > S) {
							if (Label(labels[nz][nx + (width*ny)]) == unvisited) { //unvisited
								if (ccNeighbor(nodes, px, py, pz, nx, ny, nz, labels, width, height, numSlices, g_Image3D, floatImg, S)) {
									q.push(std::make_tuple(nx, ny, nz));
								}
							}
						}
					}
				}
			}

			for (int i = 0; i < structCube.size(); i++) {
				int xn = px + structCube[i][0]; int yn = py + structCube[i][1]; int zn = pz + structCube[i][2];
				if (xn >= 0 && xn < width && yn >= 0 && yn < height && zn >= 0 && zn < numSlices) {
					if (Label(labels[zn][xn + (width*yn)]) != unvisited && Label(labels[pz][px + (width*py)]) != Label(labels[zn][xn + (width*yn)])) { //neighboring voxel has a label, is potential edge
						if (ccNeighbor(nodes, px, py, pz, xn, yn, zn, labels, width, height, numSlices, g_Image3D, floatImg, S)) {
							addEdge(G, edgeWt, labelCt, Label(labels[zn][xn + (width*yn)]), px, py, pz, xn, yn, zn);
						}
					}
				}
			}
		}
	}
}

void identifyFillFromPixel3D(std::vector<uint32_t *>& labels, std::vector<stbi_uc*>& g_Image3D, Graph & G, int x, int y, int z, int width, int height,
	int numSlices, int labelCtI, node &n, float S, std::vector<node> & nodes, map<std::vector<int>, int>& edgeWt, int genMode, std::vector< std::vector<float> > & floatImg) {
	uint32_t labelCt = (uint32_t)labelCtI;
	std::queue<tuple<int, int, int>> q; q.push(std::make_tuple(x, y, z));
	n.labelCost = 0; n.floatCost = 0.0;
	std::vector<float> diffs;
	while (!q.empty()) {
		tuple<int, int, int> pt = q.front(); q.pop();
		int px = std::get<0>(pt); int py = std::get<1>(pt); int pz = std::get<2>(pt);
		if (Label(labels[pz][px + (width*py)]) == unvisited) {
			if (floatImg.size() > 0) {
				n.floatCost += (-1.0);
			}
			else {
				n.floatCost += (-gradient(x, y, z, g_Image3D, width, height, numSlices));
			}
			changeLabel(labelCt, labels[pz][px + (width*py)]);
			setVisitedFlag(labels[pz][px + (width*py)], 1);

			//flood fill to nearby neighbor depending on priority.
			for (int i = 0; i < structCube.size(); i++) {
				int xn = px + structCube[i][0]; int yn = py + structCube[i][1]; int zn = pz + structCube[i][2];
				if (xn >= 0 && xn < width && yn >= 0 && yn < height && zn >= 0 && zn < numSlices) {
					if (floatImg.size() > 0) {
						if (floatImg[zn][xn + (width*yn)] <= S) {
							if (Label(labels[zn][xn + (width*yn)]) == unvisited) { //unvisited
								if (ccNeighborFill6Conn(nodes, px, py, pz, xn, yn, zn, labels, width)) {
									q.push(std::make_tuple(xn, yn, zn));

								}
							}
						}

					}
					else {
						if ((float)g_Image3D[zn][xn + (width*yn)] <= S) {
							if (Label(labels[zn][xn + (width*yn)]) == unvisited) { //unvisited
								//, g_Image3D, S
								if (ccNeighborFill6Conn(nodes, px, py, pz, xn, yn, zn, labels, width)) {

									q.push(std::make_tuple(xn, yn, zn));

								}
							}
						}
					}
				}
			}

			//Add edge depending on how well connected
			for (int i = 0; i < structCube.size(); i++) {
				int xn = px + structCube[i][0]; int yn = py + structCube[i][1]; int zn = pz + structCube[i][2];
				if (xn >= 0 && xn < width && yn >= 0 && yn < height && zn >= 0 && zn < numSlices) {
					if (Label(labels[zn][xn + (width*yn)]) != unvisited && Label(labels[pz][px + (width*py)]) != Label(labels[zn][xn + (width*yn)])) { //neighboring voxel has a label, is potential edge
						if (ccNeighborFill6Conn(nodes, px, py, pz, xn, yn, zn, labels, width)) {//, g_Image3D, S
							addEdge(G, edgeWt, labelCt, Label(labels[zn][xn + (width*yn)]), px, py, pz, xn, yn, zn);
						}
					}
				}
			}
		}
	}
}

void dfs(grapht & g, int v, std::vector<bool> & visited, int & timer, int p, int & overallIndex, std::vector<node> & nodes) {

	visited[v] = true;
	nodes[v].compIndex = overallIndex;
	nodes[v].tin = timer; nodes[v].low = timer;
	timer += 1;
	int children = 0;
	auto neighbours = adjacent_vertices(v, g);
	for (auto u : make_iterator_range(neighbours)) {
		if (u != p) {
			if (visited[u]) {
				nodes[v].low = min(nodes[v].low, nodes[u].tin);
			}
			else {
				dfs(g, u, visited, timer, v, overallIndex, nodes);
				nodes[v].low = min(nodes[v].low, nodes[u].low);
				if (nodes[u].low >= nodes[v].tin) {
					if (nodes[u].tin > 1) {
						nodes[u].isNew = true;
					}
					if (p != -1) {
						nodes[v].isArticulate = true;
					}
				}
				children += 1;
			}
		}
	}
	if (p == -1 && children > 1) {
		nodes[v].isArticulate = true;
	}
}

void dfsOverall(grapht & g, int v, std::vector<bool> & visited, int & timer, int p, int & overallIndex, std::vector<node> & nodes, bool inFg) {
	visited[v] = true;
	if (inFg) {
		nodes[v].overallCompIndexFg = overallIndex;
	}
	else {
		nodes[v].overallCompIndexBg = overallIndex;
	}
	//nodes[v].tin = timer; nodes[v].low = timer;
	timer += 1;
	int children = 0;
	auto neighbours = adjacent_vertices(v, g);

	for (auto u : make_iterator_range(neighbours)) {
		if (!nodes[u].valid) { continue; }
		if (u != p) {
			if (!visited[u]) {
				dfsOverall(g, u, visited, timer, v, overallIndex, nodes, inFg);
			}
		}
	}
}


bool compareByTime(const node &a, const node &b)
{
	return a.tin < b.tin;
}


//DFS to find fg and bg connectivity
map<int, map<int, int> > findComponents(grapht & g, std::vector<node> & nodes, int fg) {
	int nV = nodes.size();
	std::vector<bool> visited(nV, false); int overallIndex = 0;
	std::vector<bool> visited1(nV, false); int overallIndex1 = 0;
	for (int i = 0; i < nodes.size(); i++) {
		if (((int)nodes[i].inFg) == fg) {
			if (!visited[i]) {
				int timer = 0; int timer1 = 0;
				dfs(g, i, visited, timer, -1, overallIndex, nodes);

				overallIndex += 1;
			}
		}
	}
	//Find component indices for isolated comps
	for (int i = 0; i < nodes.size(); i++) {
		if (((int)nodes[i].inFg) == fg) {
			if (nodes[i].compIndex == -1) {
				nodes[i].compIndex = overallIndex;
				overallIndex += 1;
			}
		}
	}
	map<int, map<int, int> > nodeConnectivity;
	for (int i = 0; i < nodes.size(); i++) {
		if (nodes[i].isArticulate) {
			auto neighbourItr = adjacent_vertices(i, g);
			std::vector<node> neighbours;
			map<int, int> componentMapping; componentMapping[0] = 0;
			for (auto u : make_iterator_range(neighbourItr)) {
				neighbours.push_back(nodes[u]);
				componentMapping[u] = 0;
			}
			std::sort(neighbours.begin(), neighbours.end(), compareByTime);
			int compIndex = 0;
			map<int, int> nodeToComp;
			for (int j = 0; j < neighbours.size(); j++) {
				if (neighbours[j].low < nodes[i].tin) {
					componentMapping[neighbours[j].tin] = componentMapping[neighbours[j].low]; //Use time of ancestor
				}
				else {
					if (neighbours[j].isNew) { //reporting node
						compIndex += 1;
						componentMapping[neighbours[j].tin] = compIndex;
					}
					else { //Inherit component of nearest left neighbor
						if (j - 1 > 0) {
							componentMapping[neighbours[j].tin] = componentMapping[neighbours[j - 1].tin];
						}
					}
				}
				nodeToComp[neighbours[j].index] = componentMapping[neighbours[j].tin];
			}
			nodeConnectivity[i] = nodeToComp;
		}
	}

	return nodeConnectivity;
}

map<int, map<int, int> > findTermComponents(grapht& g, std::vector<node>& nodes, int type) {
	int nV = nodes.size();
	std::vector<bool> visited(nV, false); int overallIndex = 0;
	std::vector<bool> visited1(nV, false); int overallIndex1 = 0;
	for (int i = 0; i < nodes.size(); i++) {
		if (((int)nodes[i].type) == type) {
			if (!visited[i]) {
				int timer = 0; int timer1 = 0;
				dfs(g, i, visited, timer, -1, overallIndex, nodes);

				overallIndex += 1;
			}
		}
	}
	//Find component indices for isolated comps
	for (int i = 0; i < nodes.size(); i++) {
		if (((int)nodes[i].type) == type) {
			if (nodes[i].compIndex == -1) {
				nodes[i].compIndex = overallIndex;
				overallIndex += 1;
			}
		}
	}
	map<int, map<int, int> > nodeConnectivity;
	for (int i = 0; i < nodes.size(); i++) {
		if (nodes[i].isArticulate) {
			auto neighbourItr = adjacent_vertices(i, g);
			std::vector<node> neighbours;
			map<int, int> componentMapping; componentMapping[0] = 0;
			for (auto u : make_iterator_range(neighbourItr)) {
				neighbours.push_back(nodes[u]);
				componentMapping[u] = 0;
			}
			std::sort(neighbours.begin(), neighbours.end(), compareByTime);
			int compIndex = 0;
			map<int, int> nodeToComp;
			for (int j = 0; j < neighbours.size(); j++) {
				if (neighbours[j].low < nodes[i].tin) {
					componentMapping[neighbours[j].tin] = componentMapping[neighbours[j].low]; //Use time of ancestor
				}
				else {
					if (neighbours[j].isNew) { //reporting node
						compIndex += 1;
						componentMapping[neighbours[j].tin] = compIndex;
					}
					else { //Inherit component of nearest left neighbor
						if (j - 1 > 0) {
							componentMapping[neighbours[j].tin] = componentMapping[neighbours[j - 1].tin];
						}
					}
				}
				nodeToComp[neighbours[j].index] = componentMapping[neighbours[j].tin];
			}
			nodeConnectivity[i] = nodeToComp;
		}
	}
	return nodeConnectivity;
}


void findOverallComponents(grapht & g, std::vector<node> & nodes, int fg) {
	int nV = nodes.size();
	std::vector<bool> visited(nV, false); int overallIndex = 0;

	if (fg == 1) {
		for (int i = 0; i < nodes.size(); i++) {
			if (!nodes[i].valid) { continue; }
			if (((int)nodes[i].inFg) == 1 || ((int)nodes[i].type == FILL)) {
				if (!visited[i]) {
					int timer = 0;
					dfsOverall(g, i, visited, timer, -1, overallIndex, nodes, true);
					overallIndex += 1;
				}
			}
		}
		
		for (int i = 0; i < nodes.size(); i++) {
			if (!nodes[i].valid) { continue; }
			if (((int)nodes[i].inFg) == 1 || ((int)nodes[i].type == FILL)) {
				if (nodes[i].overallCompIndexFg == -1) {
					nodes[i].overallCompIndexFg = overallIndex;
					overallIndex += 1;
				}
			}
		}
	}
	else {
		for (int i = 0; i < nodes.size(); i++) {
			if (!nodes[i].valid) { continue; }
			if (((int)nodes[i].inFg) == 0 || ((int)nodes[i].type == CUT)) {
				if (!visited[i]) {
					int timer = 0;
					dfsOverall(g, i, visited, timer, -1, overallIndex, nodes, false);
					overallIndex += 1;
				}
			}
		}
		for (int i = 0; i < nodes.size(); i++) {
			if (!nodes[i].valid) { continue; }
			if (((int)nodes[i].inFg) == 0 || ((int)nodes[i].type == CUT)) {
				if (nodes[i].overallCompIndexBg == -1) {
					nodes[i].overallCompIndexBg = overallIndex;
					overallIndex += 1;
				}
			}
		}
	}
	//Find component indices for isolated comps
	
	
}

void findGenVoxels(const std::vector<uint32_t*> & labels, std::vector <Coordinate> & voxels, int labelIndex, int width, int height, int numSlices,
	int x, int y, int z
) {
	map<Coordinate, bool> visited;
	queue<Coordinate> q;
	Coordinate cp(x, y, z);
	q.push(cp);
	while (!q.empty()) {
		Coordinate c = q.front(); q.pop();
		visited[c] = true;
		voxels.push_back(c);
		for (int i = 0; i < structCube.size(); i++) {
			int nx = c.x + structCube[i][0]; int ny = c.y + structCube[i][1]; int nz = c.z + structCube[i][2];
			if (nx >= 0 && nx < width && ny >= 0 && ny < height && nz >= 0 && nz < numSlices) {
				if (Label(labels[nz][nx + ny * width]) == labelIndex) {
					Coordinate cn(nx, ny, nz);
					if (visited.find(cn) == visited.end()) {
						q.push(cn);
						visited[cn] = true;

					}
				}
			}
		}
	}

}

bool compareByIntensity(const weightedCoord &a, const weightedCoord &b)
{
	return a.intensityDiff > b.intensityDiff;
}

int getParent(map<int, int> & parentComp, int node) {
	int parent = parentComp[node];
	while (parent != parentComp[parent]) {
		parent = parentComp[parent];

	}
	return parent;

}
int minigraphComps(const grapht & miniG, const std::vector<bool> & miniGValid, int numNodes) {
	std::vector<int> nodeToComp(numNodes);
	int n = (int)boost::connected_components(miniG, &nodeToComp[0]);
	int numComps = 0;
	std::vector<bool> isCompIndex(n, false);
	for (int i = 0; i < nodeToComp.size(); i++) {
		// cout << i << " " << nodeToComp[i] << endl;
		if (miniGValid[i]) {
			if (!isCompIndex[nodeToComp[i]]) {
				isCompIndex[nodeToComp[i]] = true;
				numComps += 1;
			}
		}
	}
	return numComps;
}

std::vector< std::vector< std::vector<int> > >  adjFaces6Conn = {
   {{1, 0, 0}, {1, 0, 1}, {0, 0, 1}},
   {{1, 0, 0}, {1, 0, -1}, {0, 0, -1}},
   {{-1, 0, 0}, {-1, 0, -1}, {0, 0, -1}},
   {{-1, 0, 0}, {-1, 0, 1}, {0, 0, 1}},
   {{1, 0, 0}, {1, 1, 0}, {0, 1, 0}},
   {{-1, 0, 0}, {-1, 1, 0}, {0, 1, 0}},
   {{-1, 0, 0}, {-1, -1, 0}, {0, -1, 0}},
   {{1, 0, 0}, {1, -1, 0}, {0, -1, 0}},
   {{0, 0, 1}, {0, 1, 1}, {0, 1, 0}},
   {{0, 0, -1}, {0, 1, -1}, {0, 1, 0}},
   {{0, 0, -1}, {0, -1, -1}, {0, -1, 0}},
   {{0, 0, 1}, {0, -1, 1}, {0, -1, 0}}
};

std::vector< std::vector< std::vector<int> > > adjCubes6Conn = { {{0, 0, 1}, {0, 1, 0}, {0, 1, 1}, {1, 0, 0}, {1, 0, 1}, {1, 1,
   0}, {1, 1, 1}}, {{0, 0, -1}, {0, 1, -1}, {0, 1, 0}, {1, 0, -1}, {1,
	0, 0}, {1, 1, -1}, {1, 1, 0}}, {{0, -1, 0}, {0, -1, 1}, {0, 0,
   1}, {1, -1, 0}, {1, -1, 1}, {1, 0, 0}, {1, 0,
   1}}, {{0, -1, -1}, {0, -1, 0}, {0, 0, -1}, {1, -1, -1}, {1, -1,
   0}, {1, 0, -1}, {1, 0, 0}}, {{-1, 0, 0}, {-1, 0, 1}, {-1, 1,
   0}, {-1, 1, 1}, {0, 0, 1}, {0, 1, 0}, {0, 1, 1}}, {{-1,
   0, -1}, {-1, 0, 0}, {-1, 1, -1}, {-1, 1, 0}, {0, 0, -1}, {0,
   1, -1}, {0, 1, 0}}, {{-1, -1, 0}, {-1, -1, 1}, {-1, 0, 0}, {-1, 0,
   1}, {0, -1, 0}, {0, -1, 1}, {0, 0, 1}}, {{-1, -1, -1}, {-1, -1,
   0}, {-1, 0, -1}, {-1, 0, 0}, {0, -1, -1}, {0, -1, 0}, {0, 0, -1}} };

bool simple3DLabel(const std::vector<uint32_t*>& labels, const std::vector<node> & nodes, int x, int y, int z, int numSlices, int width, int height, const std::vector<unsigned char> & simpleDictionary3D, int labelIndex, bool inFg, int & conn) {
	int minX = max(x - 1, 0); int minY = max(y - 1, 0); int minZ = max(z - 1, 0);
	int maxX = min(x + 2, width); int maxY = min(y + 2, height); int maxZ = min(z + 2, numSlices);
	//Check core simplicity in 26-neighborhood
	std::vector< std::vector< std::vector<int>>> cubeN(3, std::vector<std::vector<int>>(3, std::vector<int>(3, 0)));
	for (int i = minX; i < maxX; i++) {
		for (int j = minY; j < maxY; j++) {
			for (int k = minZ; k < maxZ; k++) {
				if (Label(labels[k][i + j * width]) != unvisited) {

					if (((int)Label(labels[k][i + j * width])) == labelIndex ||
						((int)nodes[((int)Label(labels[k][i + j * width]))].type) == 0 ||
						((int)nodes[((int)Label(labels[k][i + j * width]))].type) == 2
						) {
						cubeN[i - x + 1][j - y + 1][k - z + 1] = 1;
					}
					else {
						cubeN[i - x + 1][j - y + 1][k - z + 1] = 0;
					}
				}
				else {
					cubeN[i - x + 1][j - y + 1][k - z + 1] = 0;
				}
			}
		}
	}

	if (conn == 0) {
		for (int i = 0; i < 3; i++) {
			for (int j = 0; j < 3; j++) {
				for (int k = 0; k < 3; k++) {
					cubeN[i][j][k] = 1 - cubeN[i][j][k];
				}
			}
		}
	}
	return ((int)simpleDictionary3D[neighborhoodToIndex(cubeN)]) == 49;
}

bool simple3DLabelBg(const std::vector<uint32_t*>& labels, const std::vector<node> & nodes, int x, int y, int z, int numSlices, int width, int height, const std::vector<unsigned char> & simpleDictionary3D, int labelIndex, bool inFg, int & conn) {
	int minX = max(x - 1, 0); int minY = max(y - 1, 0); int minZ = max(z - 1, 0);
	int maxX = min(x + 2, width); int maxY = min(y + 2, height); int maxZ = min(z + 2, numSlices);
	//Check core simplicity in 26-neighborhood
	//Correct version
	std::vector< std::vector< std::vector<int>>> cubeN(3, std::vector<std::vector<int>>(3, std::vector<int>(3, 0)));
	for (int i = minX; i < maxX; i++) {
		for (int j = minY; j < maxY; j++) {
			for (int k = minZ; k < maxZ; k++) {
				if (Label(labels[k][i + j * width]) != unvisited) {

					if (((int)Label(labels[k][i + j * width])) == labelIndex ||
						((int)nodes[((int)Label(labels[k][i + j * width]))].type) == 1 ||
						((int)nodes[((int)Label(labels[k][i + j * width]))].type) == 3
						) {
						cubeN[i - x + 1][j - y + 1][k - z + 1] = 1;

					}
					else {
						cubeN[i - x + 1][j - y + 1][k - z + 1] = 0;
					}
				}
				else {
					cubeN[i - x + 1][j - y + 1][k - z + 1] = 0;
				}
			}
		}
	}


	if (conn == 0) {
		for (int i = 0; i < 3; i++) {
			for (int j = 0; j < 3; j++) {
				for (int k = 0; k < 3; k++) {
					cubeN[i][j][k] = 1 - cubeN[i][j][k];
				}
			}
		}
	}
	return ((int)simpleDictionary3D[neighborhoodToIndex(cubeN)]) == 49;
}

//G, labels, g_Image3D, fgG, bgG, fgLocalArtConnectivity, bgLocalArtConnectivity, numSlices, width, height, fgConn, nodes, Label(labels[k][i + j * width]), edgeWt, S, simpleDictionary3D, nodeVoxels
void simplifyGeneratorRoots(Graph& G, std::vector<uint32_t*>& labels, const std::vector<stbi_uc*>& g_Image3D, const std::vector< std::vector<float> >& g_Image3DF,
	map<int, map<int, int> >& fgLocalArtConnectivity,
	map<int, map<int, int> >& bgLocalArtConnectivity, int numSlices, int width, int height, int fgConn,
	std::vector<node>& nodes, int nodeIndex, map<std::vector<int>, int>& edgeWt, float S,
	const std::vector<unsigned char>& simpleDictionary3D, const std::vector<Coordinate>& nodeVoxels, int genMode, map<int, int>& parentCompFg, map<int, int>& parentCompBg, bool& generatorChanged
	, grapht& fgGWithFills, grapht& bgGWithCuts, grapht& coreG, grapht& nG, float& minigraphT, float& addETime, float& otherTime
) {
	//grapht & fgG, grapht & bgG, 
	auto t1 = std::chrono::high_resolution_clock::now();
	std::vector< std::vector<int> > fgMask, bgMask;
	if (fgConn == 0) {
		fgMask = structCube;
		bgMask = structCross3D;
	}
	else {
		fgMask = structCross3D;
		bgMask = structCube;
	}

	bool isFloat = g_Image3DF.size() > 0;
	if (((int)nodes[nodeIndex].type) == 3) { //Fill
		auto neighbours = adjacent_vertices(nodeIndex, G);
		std::vector<int> compIndices; int nodeAdjFg = 0;
		for (auto u : make_iterator_range(neighbours)) {

			if (((int)nodes[u].inFg) == 1) {
				if (edgeWt[{nodeIndex, (int)u}] == 1) {
					if (((int)nodes[u].type) == 0 || ((int)nodes[u].type) == 2) {
						if (nodes[u].valid) {
							if (nodes[u].overallCompIndexFg != -1) {
								compIndices.push_back(nodes[u].overallCompIndexFg);
								nodeAdjFg += 1;
							}
						}
					}
				}
			}
		}
		//Delete duplicate connected components
		std::sort(compIndices.begin(), compIndices.end());
		compIndices.erase(unique(compIndices.begin(), compIndices.end()), compIndices.end());

		//Fill not connected to any fg comps
		int vx, vy, vz, nx, ny, nz;

		map<Coordinate, int> vtxToGNode;
		int  newVtxIndex = 1;
		map<int, int> fgCompToMini;
		if (compIndices.size() > 0) {
			for (int i = 0; i < compIndices.size(); i++) {
				fgCompToMini[compIndices[i]] = i;
			}
			newVtxIndex = compIndices.size();
		}
		grapht miniFgG = grapht();
		std::vector<weightedCoord> P;
		//Create vertex indices
		std::vector<bool> miniFgValid(newVtxIndex + nodeVoxels.size(), false);

		//std::vector<node> minigraphNodes;
		for (int i = 0; i < nodeVoxels.size(); i++) {
			vx = nodeVoxels[i].x; vy = nodeVoxels[i].y; vz = nodeVoxels[i].z;
			vtxToGNode[Coordinate(vx, vy, vz)] = newVtxIndex;
			miniFgValid[newVtxIndex] = true;
			newVtxIndex += 1;

			if (isFloat) {
				weightedCoord wc = { vx, vy, vz, abs((float)g_Image3DF[vz][vx + width * vy] - S) };
				P.push_back(wc);
			}
			else {
				weightedCoord wc = { vx, vy, vz, abs((float)g_Image3D[vz][vx + width * vy] - S) };
				P.push_back(wc);
			}
		}


		for (int i = 0; i < compIndices.size(); i++) {
			miniFgValid[fgCompToMini[compIndices[i]]] = true;
		}
		for (int i = 0; i < newVtxIndex; i++) {
			add_vertex(miniFgG);
		}
		//Construct internal and external FG minigraph edges
		for (int i = 0; i < nodeVoxels.size(); i++) {
			vx = nodeVoxels[i].x; vy = nodeVoxels[i].y; vz = nodeVoxels[i].z;
			Coordinate vC(vx, vy, vz);
			for (int j = 0; j < fgMask.size(); j++) {
				nx = vx + fgMask[j][0]; ny = vy + fgMask[j][1]; nz = vz + fgMask[j][2];
				Coordinate vN(nx, ny, nz);
				if (nx >= 0 && ny >= 0 && nz >= 0 && nx < width && ny < height && nz < numSlices) {
					if (Label(labels[nz][nx + (width * (ny))]) == Label(labels[vz][vx + (width * (vy))])) { //edge within voxel
						if (!boost::edge(vtxToGNode[vC], vtxToGNode[vN], miniFgG).second) {
							add_edge(vtxToGNode[vC], vtxToGNode[vN], miniFgG);
						}
					}
					else {
						if (((int)nodes[Label(labels[nz][nx + (width * (ny))])].type) == 0
							|| ((int)nodes[Label(labels[nz][nx + (width * (ny))])].type) == 2
							) {
							if (!boost::edge(vtxToGNode[vC], fgCompToMini[nodes[Label(labels[nz][nx + (width * (ny))])].overallCompIndexFg], miniFgG).second) {
								add_edge(vtxToGNode[vC], fgCompToMini[nodes[Label(labels[nz][nx + (width * (ny))])].overallCompIndexFg], miniFgG);
							}
						}
					}
				}
			}
		}

		int h0 = 1; int h2;
		map<int, int> parentComp;
		map<int, int> nodeToComp;
		auto neighboursN = adjacent_vertices(nodeIndex, G);
		std::vector<int> uniqueComps;
		for (auto u : make_iterator_range(neighboursN)) {
			if (((int)nodes[u].type) == N) {
				uniqueComps.push_back(getParent(parentCompBg, nodes[u].compIndex));
			}
		}
		std::sort(uniqueComps.begin(), uniqueComps.end());
		uniqueComps.erase(unique(uniqueComps.begin(), uniqueComps.end()), uniqueComps.end());

		h2 = uniqueComps.size();
		//Next step sort voxels by intensity and go thru them
		std::sort(P.begin(), P.end(), compareByIntensity);
		int voxelsDeleted = 0;
		std::vector<vertex_t> art_points;
		auto t1 = std::chrono::high_resolution_clock::now();
		auto t21 = std::chrono::high_resolution_clock::now();
		std::chrono::duration<double> elapsed1 = t21 - t1;
		otherTime += elapsed1.count();

		for (int i = 0; i < P.size(); i++) {
			weightedCoord wc = P[i];

			if (Label(labels[wc.z][wc.x + (width * (wc.y))]) == nodeIndex) {
				//auto t1 = std::chrono::high_resolution_clock::now();
				Coordinate wcC(wc.x, wc.y, wc.z);
				int changeH2;
				std::vector<int> nBgNodes;
				for (int j = 0; j < bgMask.size(); j++) {
					nx = wc.x + bgMask[j][0]; ny = wc.y + bgMask[j][1]; nz = wc.z + bgMask[j][2];
					if (nx >= 0 && ny >= 0 && nz >= 0 && nx < width && ny < height && nz < numSlices) {
						if (Label(labels[nz][nx + (width * (ny))]) != nodeIndex) { //edge within voxel
							if (((int)nodes[Label(labels[nz][nx + (width * (ny))])].type) == 1
								) {
								nBgNodes.push_back(getParent(parentCompBg, nodes[Label(labels[nz][nx + (width * (ny))])].compIndex));
							}
						}
					}
				}
				//Number of neighboring bg components
				std::sort(nBgNodes.begin(), nBgNodes.end());
				nBgNodes.erase(unique(nBgNodes.begin(), nBgNodes.end()), nBgNodes.end());

				changeH2 = 1 - nBgNodes.size();
				int h0N; int changeH0;

				if (P.size() == 1 && nodeAdjFg == 0) {
					changeH0 = -1;
				}
				else {

					clear_vertex(vtxToGNode[wcC], miniFgG);
					miniFgValid[vtxToGNode[wcC]] = false;

					auto t1 = std::chrono::high_resolution_clock::now();
					h0N = minigraphComps(miniFgG, miniFgValid, newVtxIndex);
					auto t2 = std::chrono::high_resolution_clock::now();
					std::chrono::duration<double> elapsed = t2 - t1;
					minigraphT += elapsed.count();
					changeH0 = h0N - h0;
				}

				int dV = 1, dE = 0, dF = 0, dC = 0;
				for (int j = 0; j < fgMask.size(); j++) {
					nx = wc.x + fgMask[j][0]; ny = wc.y + fgMask[j][1]; nz = wc.z + fgMask[j][2];
					if (nx >= 0 && ny >= 0 && nz >= 0 && nx < width && ny < height && nz < numSlices) {
						if (((int)nodes[Label(labels[nz][nx + (width * (ny))])].type) == 0 ||
							((int)nodes[Label(labels[nz][nx + (width * (ny))])].type) == 3 ||
							((int)nodes[Label(labels[nz][nx + (width * (ny))])].type) == 2
							) {
							dE += 1;
						}
					}
				}
				for (int j = 0; j < adjFaces6Conn.size(); j++) {
					std::vector< std::vector<int> > adjFace = adjFaces6Conn[j];
					bool hasN = false;
					for (int k = 0; k < adjFace.size(); k++) {
						nx = wc.x + adjFace[k][0]; ny = wc.y + adjFace[k][1]; nz = wc.z + adjFace[k][2];
						if (nx >= 0 && ny >= 0 && nz >= 0 && nx < width && ny < height && nz < numSlices) {
							if (((int)nodes[Label(labels[nz][nx + (width * (ny))])].type) == 1) {
								hasN = true;
							}
						}
					}
					if (!hasN) {
						dF += 1;
					}
				}
				for (int j = 0; j < adjCubes6Conn.size(); j++) {
					std::vector< std::vector<int> > adjCube = adjCubes6Conn[j];
					bool hasN = false;
					for (int k = 0; k < adjCube.size(); k++) {
						nx = wc.x + adjCube[k][0]; ny = wc.y + adjCube[k][1]; nz = wc.z + adjCube[k][2];
						if (nx >= 0 && ny >= 0 && nz >= 0 && nx < width && ny < height && nz < numSlices) {
							if (((int)nodes[Label(labels[nz][nx + (width * (ny))])].type) == 1) {
								hasN = true;
							}
						}
					}
					if (!hasN) {
						dC += 1;
					}
				}
				int dEuler = -(dV - dE + dF - dC);
				int changeH1 = changeH0 + changeH2 - dEuler;
				if ((changeH0 <= 0 && changeH2 <= 0 && changeH1 <= 0 && changeH0 + changeH2 + changeH1 < 0) || changeH2 > 0) {
					generatorChanged = true;
					h2 = h2 + changeH2;
					for (int j = 0; j < bgMask.size(); j++) {
						nx = wc.x + bgMask[j][0]; ny = wc.y + bgMask[j][1]; nz = wc.z + bgMask[j][2];
						if (nx >= 0 && ny >= 0 && nz >= 0 && nx < width && ny < height && nz < numSlices) {
							if (((int)nodes[Label(labels[nz][nx + (width * (ny))])].type) == 1) {
								labels[wc.z][wc.x + (width * (wc.y))] = labels[nz][nx + (width * (ny))];
							}
						}
					}

					voxelsDeleted += 1;
					//voxel has now been relabeled to a background component, create potential new edges for background component
					for (int j = 0; j < bgMask.size(); j++) {
						nx = wc.x + bgMask[j][0]; ny = wc.y + bgMask[j][1]; nz = wc.z + bgMask[j][2];
						if (nx >= 0 && ny >= 0 && nz >= 0 && nx < width && ny < height && nz < numSlices) {
							if (Label(labels[wc.z][wc.x + (width * (wc.y))]) != Label(labels[nz][nx + (width * (ny))])) {
								if (((int)nodes[Label(labels[nz][nx + (width * (ny))])].type) == 1 ||
									((int)nodes[Label(labels[nz][nx + (width * (ny))])].type) == 3
									) {
									if (ccNeighborFill6Conn(nodes, wc.x, wc.y, wc.z, nx, ny, nz, labels, width)) { //, g_Image3D, S
										if (edgeWt.find({ (int)Label(labels[wc.z][wc.x + (width * (wc.y))]), (int)Label(labels[nz][nx + (width * (ny))]) }) == edgeWt.end()) {
											add_edge(Label(labels[nz][nx + (width * (ny))]), Label(labels[wc.z][wc.x + (width * (wc.y))]), G);
											if (abs(nx - wc.x) + abs(ny - wc.y) + abs(nz - wc.z) == 1) {
												if (
													(((int)nodes[Label(labels[wc.z][wc.x + wc.y * width])].type) == CORE || ((int)nodes[Label(labels[wc.z][wc.x + wc.y * width])].type) == CUT || ((int)nodes[Label(labels[wc.z][wc.x + wc.y * width])].type) == FILL)
													&&
													(((int)nodes[Label(labels[nz][nx + ny * width])].type) == CORE || ((int)nodes[Label(labels[nz][nx + ny * width])].type) == CUT || ((int)nodes[Label(labels[nz][nx + ny * width])].type) == FILL)

													) {
													add_edge(Label(labels[wc.z][wc.x + wc.y * width]), Label(labels[nz][nx + ny * width]), fgGWithFills);

												}
												if (
													(((int)nodes[Label(labels[wc.z][wc.x + wc.y * width])].type) == CORE)
													&&
													(((int)nodes[Label(labels[nz][nx + ny * width])].type) == CORE)

													) {
													add_edge(Label(labels[wc.z][wc.x + wc.y * width]), Label(labels[nz][nx + ny * width]), coreG);

												}
											}
											if (
												(((int)nodes[Label(labels[wc.z][wc.x + wc.y * width])].type) == N || ((int)nodes[Label(labels[wc.z][wc.x + wc.y * width])].type) == CUT || ((int)nodes[Label(labels[wc.z][wc.x + wc.y * width])].type) == FILL)
												&&
												(((int)nodes[Label(labels[nz][nx + ny * width])].type) == N || ((int)nodes[Label(labels[nz][nx + ny * width])].type) == CUT || ((int)nodes[Label(labels[nz][nx + ny * width])].type) == FILL)

												) {
												add_edge(Label(labels[wc.z][wc.x + wc.y * width]), Label(labels[nz][nx + ny * width]), bgGWithCuts);

											}
											if (
												(((int)nodes[Label(labels[wc.z][wc.x + wc.y * width])].type) == N)
												&&
												(((int)nodes[Label(labels[nz][nx + ny * width])].type) == N)

												) {
												add_edge(Label(labels[wc.z][wc.x + wc.y * width]), Label(labels[nz][nx + ny * width]), nG);

											}
										}
									}

									if (abs(nx - wc.x) + abs(ny - wc.y) + abs(nz - wc.z) == 1) {
										if (edgeWt[{(int)Label(labels[wc.z][wc.x + (width * (wc.y))]), (int)Label(labels[nz][nx + (width * (ny))])}] == 0) {
											if (
												(((int)nodes[Label(labels[wc.z][wc.x + wc.y * width])].type) == CORE || ((int)nodes[Label(labels[wc.z][wc.x + wc.y * width])].type) == CUT || ((int)nodes[Label(labels[wc.z][wc.x + wc.y * width])].type) == FILL)
												&&
												(((int)nodes[Label(labels[nz][nx + ny * width])].type) == CORE || ((int)nodes[Label(labels[nz][nx + ny * width])].type) == CUT || ((int)nodes[Label(labels[nz][nx + ny * width])].type) == FILL)

												) {
												add_edge(Label(labels[wc.z][wc.x + wc.y * width]), Label(labels[nz][nx + ny * width]), fgGWithFills);

											}
										}
										edgeWt[{(int)Label(labels[wc.z][wc.x + (width * (wc.y))]), (int)Label(labels[nz][nx + (width * (ny))])}] = 1;
										edgeWt[{(int)Label(labels[nz][nx + (width * (ny))]), (int)Label(labels[wc.z][wc.x + (width * (wc.y))])}] = 1;


									}
									else {
										if (ccNeighborFill6Conn(nodes, wc.x, wc.y, wc.z, nx, ny, nz, labels, width)) { //, g_Image3D, S
											if (edgeWt.find({ (int)Label(labels[wc.z][wc.x + (width * (wc.y))]), (int)Label(labels[nz][nx + (width * (ny))]) }) == edgeWt.end()) {
												edgeWt[{(int)Label(labels[wc.z][wc.x + (width * (wc.y))]), (int)Label(labels[nz][nx + (width * (ny))])}] = 0;
												edgeWt[{(int)Label(labels[nz][nx + (width * (ny))]), (int)Label(labels[wc.z][wc.x + (width * (wc.y))])}] = 0;
											}
										}
									}
								}
								if (((int)nodes[Label(labels[nz][nx + (width * (ny))])].type) == 1 //|| ((int)nodes[Label(labels[nz][nx + (width*(ny))])].type) == 3
									) {
									if (getParent(parentCompBg, nodes[Label(labels[wc.z][wc.x + (width * (wc.y))])].compIndex)
										!= getParent(parentCompBg, nodes[Label(labels[nz][nx + (width * (ny))])].compIndex)
										) {
										parentCompBg[getParent(parentCompBg, nodes[Label(labels[nz][nx + (width * (ny))])].compIndex)] = getParent(parentCompBg, nodes[Label(labels[wc.z][wc.x + (width * (wc.y))])].compIndex);

									}
								}
							}

						}


					}
					queue<Coordinate> q;
					//Iteratively remove simple voxels
					for (int j = 0; j < structCube.size(); j++) {
						nx = wc.x + structCube[j][0]; ny = wc.y + structCube[j][1]; nz = wc.z + structCube[j][2];
						if (nx >= 0 && ny >= 0 && nz >= 0 && nx < width && ny < height && nz < numSlices) {
							if (Label(labels[nz][nx + (width * (ny))]) == nodeIndex) {
								q.push(Coordinate(nx, ny, nz));
							}
						}
					}
					map<Coordinate, int> genVisited;
					for (int j = 0; j < P.size(); j++) {

						if (Label(labels[P[j].z][P[j].x + (width * (P[j].y))]) == nodeIndex) {
							Coordinate pNow(P[j].x, P[j].y, P[j].z);
							genVisited[pNow] = 0;
						}
					}

					Coordinate wCoord(wc.x, wc.y, wc.z);
					genVisited[wCoord] = 1;
					while (!q.empty()) {
						Coordinate p = q.front();
						int px = p.x; int py = p.y; int pz = p.z;
						q.pop();
						genVisited[p] = 1;
						if (simple3DLabel(labels, nodes, px, py, pz, numSlices, width, height, simpleDictionary3D, nodeIndex, true, fgConn)) {
							if ((int)Label(labels[pz][px + (width * (py))]) == nodeIndex) {
								voxelsDeleted += 1;
								for (int j = 0; j < structCube.size(); j++) {
									nx = px + structCube[j][0]; ny = py + structCube[j][1]; nz = pz + structCube[j][2];
									if (nx >= 0 && ny >= 0 && nz >= 0 && nx < width && ny < height && nz < numSlices) {
										if (abs(nx - px) + abs(ny - py) + abs(nz - pz) == 1) {
											if (((int)nodes[Label(labels[nz][nx + (width * (ny))])].type) == 1) {
												labels[pz][px + (width * (py))] = labels[nz][nx + (width * (ny))];
												clear_vertex(vtxToGNode[p], miniFgG);
												miniFgValid[vtxToGNode[p]] = false;
												for (int a = 0; a < structCube.size(); a++) {
													int nxi = px + structCube[a][0];
													int nyi = py + structCube[a][1];
													int nzi = pz + structCube[a][2];
													if (nxi >= 0 && nyi >= 0 && nzi >= 0 && nxi < width && nyi < height && nzi < numSlices) {
														if (Label(labels[pz][px + (width * (py))]) != Label(labels[nzi][nxi + (width * (nyi))])) {
															if (((int)nodes[Label(labels[nzi][nxi + (width * (nyi))])].type) == 1 ||
																((int)nodes[Label(labels[nzi][nxi + (width * (nyi))])].type) == 2 ||
																((int)nodes[Label(labels[nzi][nxi + (width * (nyi))])].type) == 3
																) {
																if (ccNeighborFill6Conn(nodes, px, py, pz, nxi, nyi, nzi, labels, width)) {
																	if (edgeWt.find({ (int)Label(labels[pz][px + (width * (py))]), (int)Label(labels[nzi][nxi + (width * (nyi))]) }) == edgeWt.end()) {
																		add_edge(Label(labels[nzi][nxi + (width * (nyi))]), Label(labels[pz][px + (width * (py))]), G);
																		if (abs(nxi - px) + abs(nyi - py) + abs(nzi - pz) == 1) {
																			if (
																				(((int)nodes[Label(labels[pz][px + py * width])].type) == CORE || ((int)nodes[Label(labels[pz][px + py * width])].type) == CUT || ((int)nodes[Label(labels[pz][px + py * width])].type) == FILL)
																				&&
																				(((int)nodes[Label(labels[nzi][nxi + nyi * width])].type) == CORE || ((int)nodes[Label(labels[nzi][nxi + nyi * width])].type) == CUT || ((int)nodes[Label(labels[nzi][nxi + nyi * width])].type) == FILL)

																				) {
																				add_edge(Label(labels[pz][px + py * width]), Label(labels[nzi][nxi + nyi * width]), fgGWithFills);

																			}
																			if (
																				(((int)nodes[Label(labels[pz][px + py * width])].type) == CORE)
																				&&
																				(((int)nodes[Label(labels[nzi][nxi + nyi * width])].type) == CORE)

																				) {
																				add_edge(Label(labels[pz][px + py * width]), Label(labels[nzi][nxi + nyi * width]), coreG);

																			}
																		}
																		if (
																			(((int)nodes[Label(labels[pz][px + py * width])].type) == N)
																			&&
																			(((int)nodes[Label(labels[nzi][nxi + nyi * width])].type) == N)

																			) {
																			add_edge(Label(labels[pz][px + py * width]), Label(labels[nzi][nxi + nyi * width]), nG);

																		}

																	}
																	if (abs(nxi - px) + abs(nyi - py) + abs(nzi - pz) == 1) {
																		edgeWt[{(int)Label(labels[pz][px + (width * (py))]), (int)Label(labels[nzi][nxi + (width * (nyi))])}] = 1;
																		edgeWt[{(int)Label(labels[nzi][nxi + (width * (nyi))]), (int)Label(labels[pz][px + (width * (py))])}] = 1;
																		if (edgeWt[{(int)Label(labels[pz][px + (width * (py))]), (int)Label(labels[nz][nx + (width * (ny))])}] == 0) {
																			if (
																				(((int)nodes[Label(labels[pz][px + py * width])].type) == CORE || ((int)nodes[Label(labels[pz][px + py * width])].type) == CUT || ((int)nodes[Label(labels[pz][px + py * width])].type) == FILL)
																				&&
																				(((int)nodes[Label(labels[nzi][nxi + nyi * width])].type) == CORE || ((int)nodes[Label(labels[nzi][nxi + nyi * width])].type) == CUT || ((int)nodes[Label(labels[nzi][nxi + nyi * width])].type) == FILL)

																				) {
																				add_edge(Label(labels[pz][px + py * width]), Label(labels[nzi][nxi + nyi * width]), fgGWithFills);

																			}
																		}
																		edgeWt[{(int)Label(labels[pz][px + (width * (py))]), (int)Label(labels[nzi][nxi + (width * (nyi))])}] = 1;
																		edgeWt[{(int)Label(labels[nzi][nxi + (width * (nyi))]), (int)Label(labels[pz][px + (width * (py))])}] = 1;



																	}
																	else {
																		if (edgeWt.find({ (int)Label(labels[pz][px + (width * (py))]), (int)Label(labels[nzi][nxi + (width * (nyi))]) }) == edgeWt.end()) {
																			edgeWt[{(int)Label(labels[pz][px + (width * (py))]), (int)Label(labels[nzi][nxi + (width * (nyi))])}] = 0;
																			edgeWt[{(int)Label(labels[nzi][nxi + (width * (nyi))]), (int)Label(labels[pz][px + (width * (py))])}] = 0;
																		}
																	}
																}
															}

														}

														if (((int)nodes[Label(labels[nzi][nxi + (width * (nyi))])].type) == N) {
															if (getParent(parentCompBg, nodes[Label(labels[pz][px + (width * (py))])].compIndex)
																!= getParent(parentCompBg, nodes[Label(labels[nzi][nxi + (width * (nyi))])].compIndex)
																) {
																parentCompBg[getParent(parentCompBg, nodes[Label(labels[nzi][nxi + (width * (nyi))])].compIndex)] = getParent(parentCompBg, nodes[Label(labels[pz][px + (width * (py))])].compIndex);

															}
														}
													}

												}
											}
										}
										if (Label(labels[nz][nx + (width * (ny))]) == nodeIndex) {
											Coordinate np(nx, ny, nz);
											if (genVisited[np] == 0) {
												q.push(np);
												genVisited[np] = 1;
											}
										}
									}
								}
							}
						}
						else {
							genVisited[p] = 0;
						}

					}

				}
				else {
					//if h0, h1, h2 total does not decrease, add back to graph
					Coordinate wCoord(wc.x, wc.y, wc.z);
					miniFgValid[vtxToGNode[wCoord]] = true;
					for (int j = 0; j < fgMask.size(); j++) {
						nx = wc.x + fgMask[j][0]; ny = wc.y + fgMask[j][1]; nz = wc.z + fgMask[j][2];
						if (nx >= 0 && ny >= 0 && nz >= 0 && nx < width && ny < height && nz < numSlices) {
							if (Label(labels[nz][nx + (width * (ny))]) == nodeIndex) {
								if (!boost::edge(vtxToGNode[{nx, ny, nz}], vtxToGNode[{wc.x, wc.y, wc.z}], miniFgG).second) {
									add_edge(vtxToGNode[Coordinate(nx, ny, nz)], vtxToGNode[wCoord], miniFgG);
								}
							}
							else {
								if (((int)nodes[Label(labels[nz][nx + (width * (ny))])].type) == 0
									|| ((int)nodes[Label(labels[nz][nx + (width * (ny))])].type) == 2
									) {
									if (!boost::edge(vtxToGNode[{wc.x, wc.y, wc.z}], fgCompToMini[nodes[Label(labels[nz][nx + (width * (ny))])].overallCompIndexFg], miniFgG).second) {
										add_edge(vtxToGNode[wCoord], fgCompToMini[nodes[Label(labels[nz][nx + (width * (ny))])].overallCompIndexFg], miniFgG);
									}
								}

							}
						}
					}
				}

			}
		}

		if (voxelsDeleted >= P.size()) {
			nodes[nodeIndex].valid = false;
			auto neighbours = adjacent_vertices(nodeIndex, G);
			for (auto u : make_iterator_range(neighbours)) {
				edgeWt.erase({ nodeIndex, (int)u }); edgeWt.erase({ (int)u , nodeIndex });
			}
			clear_vertex(nodeIndex, G);
			for (int j = 0; j < P.size(); j++) {
				int px = P[j].x; int py = P[j].y; int pz = P[j].z;
				for (int k = 0; k < structCube.size(); k++) {
					int nx = px + structCube[k][0]; int ny = py + structCube[k][1]; int nz = pz + structCube[k][2];
					if (nx >= 0 && ny >= 0 && nz >= 0 && nx < width && ny < height && nz < numSlices) {
						if (Label(labels[pz][px + py * width]) != Label(labels[nz][nx + ny * width])) {
							if (((int)nodes[Label(labels[nz][nx + ny * width])].type) != CORE) {
								if (edgeWt.find({ (int)Label(labels[pz][px + py * width]) , (int)Label(labels[nz][nx + ny * width]) }) == edgeWt.end()) {
									if (ccNeighborFill6Conn(nodes, px, py, pz, nx, ny, nz, labels, width)) {
										add_edge(Label(labels[pz][px + py * width]), Label(labels[nz][nx + ny * width]), G);
										if (
											(((int)nodes[Label(labels[pz][px + py * width])].type) == N)
											&&
											(((int)nodes[Label(labels[nz][nx + ny * width])].type) == N)

											) {
											add_edge(Label(labels[pz][px + py * width]), Label(labels[nz][nx + ny * width]), nG);

										}

										if (abs(nx - px) + abs(ny - py) + abs(nz - pz) == 1) {
											edgeWt[{(int)Label(labels[pz][px + py * width]), (int)Label(labels[nz][nx + ny * width])}] = 1;
											edgeWt[{(int)Label(labels[nz][nx + ny * width]), (int)Label(labels[pz][px + py * width])}] = 1;
											if (
												(((int)nodes[Label(labels[pz][px + py * width])].type) == CORE || ((int)nodes[Label(labels[pz][px + py * width])].type) == CUT || ((int)nodes[Label(labels[pz][px + py * width])].type) == FILL)
												&&
												(((int)nodes[Label(labels[nz][nx + ny * width])].type) == CORE || ((int)nodes[Label(labels[nz][nx + ny * width])].type) == CUT || ((int)nodes[Label(labels[nz][nx + ny * width])].type) == FILL)

												) {
												add_edge(Label(labels[pz][px + py * width]), Label(labels[nz][nx + ny * width]), fgGWithFills);

											}
											if (
												(((int)nodes[Label(labels[pz][px + py * width])].type) == CORE)
												&&
												(((int)nodes[Label(labels[nz][nx + ny * width])].type) == CORE)

												) {
												add_edge(Label(labels[pz][px + py * width]), Label(labels[nz][nx + ny * width]), coreG);

											}
										}
										else {
											edgeWt[{(int)Label(labels[pz][px + py * width]), (int)Label(labels[nz][nx + ny * width])}] = 0;
											edgeWt[{(int)Label(labels[nz][nx + ny * width]), (int)Label(labels[pz][px + py * width])}] = 0;
										}
									}
								}
								else {
									if (abs(nx - px) + abs(ny - py) + abs(nz - pz) == 1) {
										edgeWt[{(int)Label(labels[pz][px + py * width]), (int)Label(labels[nz][nx + ny * width])}] = 1;
										edgeWt[{(int)Label(labels[nz][nx + ny * width]), (int)Label(labels[pz][px + py * width])}] = 1;
									}
								}
							}
						}
					}
				}


			}
		}

		else {
			if (voxelsDeleted > 0) {
				auto t1 = std::chrono::high_resolution_clock::now();

				map< std::vector<int>, int> edgeWtTemp = edgeWt;
				auto neighbours = adjacent_vertices(nodeIndex, G);
				for (auto u : make_iterator_range(neighbours)) {
					edgeWtTemp.erase({ nodeIndex, (int)u }); edgeWtTemp.erase({ (int)u , nodeIndex });
				}
				clear_vertex(nodeIndex, G);
				map<Coordinate, bool> visited;
				for (int j = 0; j < P.size(); j++) {
					visited[Coordinate(P[j].x, P[j].y, P[j].z)] = false;
				}
				int currentLabelIndex = nodeIndex;

				for (int j = 0; j < P.size(); j++) {
					Coordinate pNow(P[j].x, P[j].y, P[j].z);
					if (!visited[pNow]) {
						if (Label(labels[pNow.z][pNow.x + (width * (pNow.y))]) == nodeIndex) {
							node n; n.labelCost = 0; n.floatCost = 0.0;  n.type = 3; n.inFg = 0; n.index = currentLabelIndex;

							queue<Coordinate> q;
							q.push(pNow);
							if (currentLabelIndex != nodeIndex) {
								add_vertex(G);
								nodes.push_back(n);
							}
							else {
								nodes[currentLabelIndex] = n;
							}
							std::vector<float> diffs;
							while (!q.empty()) {
								Coordinate p = q.front();
								int px = p.x; int py = p.y; int pz = p.z;
								q.pop();
								visited[p] = true;
								if (g_Image3DF.size() > 0) {
									nodes[currentLabelIndex].floatCost += -1.0;
								}
								else {
									nodes[currentLabelIndex].floatCost += (-gradient(px, py, pz, g_Image3D, width, height, numSlices));
								}

								uint32_t label32 = currentLabelIndex;
								changeLabel(label32, labels[pz][px + py * width]);
								for (int k = 0; k < structCube.size(); k++) {
									nx = px + structCube[k][0]; ny = py + structCube[k][1]; nz = pz + structCube[k][2];
									if (nx >= 0 && ny >= 0 && nz >= 0 && nx < width && ny < height && nz < numSlices) {
										if (Label(labels[pz][px + py * width]) != Label(labels[nz][nx + ny * width])) {
											if (nodeIndex != Label(labels[nz][nx + ny * width])) {
												if (ccNeighborFill6Conn(nodes, px, py, pz, nx, ny, nz, labels, width)) { //, g_Image3D, S
													if (edgeWtTemp.find({ (int)Label(labels[pz][px + py * width]),(int)Label(labels[nz][nx + ny * width]) }) == edgeWtTemp.end()) {
														add_edge(Label(labels[pz][px + py * width]), Label(labels[nz][nx + ny * width]), G);
														if (
															(((int)nodes[Label(labels[pz][px + py * width])].type) == N || ((int)nodes[Label(labels[pz][px + py * width])].type) == CUT || ((int)nodes[Label(labels[pz][px + py * width])].type) == FILL)
															&&
															(((int)nodes[Label(labels[nz][nx + ny * width])].type) == N || ((int)nodes[Label(labels[nz][nx + ny * width])].type) == CUT || ((int)nodes[Label(labels[nz][nx + ny * width])].type) == FILL)

															) {
															add_edge(Label(labels[pz][px + py * width]), Label(labels[nz][nx + ny * width]), bgGWithCuts);

														}
														if (
															(((int)nodes[Label(labels[pz][px + py * width])].type) == N)
															&&
															(((int)nodes[Label(labels[nz][nx + ny * width])].type) == N)

															) {
															add_edge(Label(labels[pz][px + py * width]), Label(labels[nz][nx + ny * width]), nG);
														}
														if (abs(nx - px) + abs(ny - py) + abs(nz - pz) == 1) {
															edgeWtTemp[{(int)Label(labels[pz][px + py * width]), (int)Label(labels[nz][nx + ny * width])}] = 1;
															edgeWtTemp[{(int)Label(labels[nz][nx + ny * width]), (int)Label(labels[pz][px + py * width])}] = 1;
															if (
																(((int)nodes[Label(labels[pz][px + py * width])].type) == CORE || ((int)nodes[Label(labels[pz][px + py * width])].type) == CUT || ((int)nodes[Label(labels[pz][px + py * width])].type) == FILL)
																&&
																(((int)nodes[Label(labels[nz][nx + ny * width])].type) == CORE || ((int)nodes[Label(labels[nz][nx + ny * width])].type) == CUT || ((int)nodes[Label(labels[nz][nx + ny * width])].type) == FILL)

																) {
																add_edge(Label(labels[pz][px + py * width]), Label(labels[nz][nx + ny * width]), fgGWithFills);

															}

															if (
																(((int)nodes[Label(labels[pz][px + py * width])].type) == CORE)
																&&
																(((int)nodes[Label(labels[nz][nx + ny * width])].type) == CORE)

																) {
																add_edge(Label(labels[pz][px + py * width]), Label(labels[nz][nx + ny * width]), coreG);

															}
														}
														else {
															edgeWtTemp[{(int)Label(labels[pz][px + py * width]), (int)Label(labels[nz][nx + ny * width])}] = 0;
															edgeWtTemp[{(int)Label(labels[nz][nx + ny * width]), (int)Label(labels[pz][px + py * width])}] = 0;

														}
													}
													else {
														if (abs(nx - px) + abs(ny - py) + abs(nz - pz) == 1) {
															edgeWtTemp[{(int)Label(labels[pz][px + py * width]), (int)Label(labels[nz][nx + ny * width])}] = 1;
															edgeWtTemp[{(int)Label(labels[nz][nx + ny * width]), (int)Label(labels[pz][px + py * width])}] = 1;
														}
													}
												}
											}
										}
										if (Label(labels[nz][nx + ny * width]) == nodeIndex) {
											Coordinate np(nx, ny, nz);
											if (!visited[np]) {
												if (ccNeighborFill6Conn(nodes, px, py, pz, nx, ny, nz, labels, width)) {
													q.push(np);
													visited[np] = true;
												}
											}
										}
									}
								}
							}
							//If empty, do lexicographical ordering if necessary
							if (currentLabelIndex == nodeIndex) {
								currentLabelIndex = nodes.size();
							}
							else {
								currentLabelIndex += 1;
							}
						}
					}
				}
				edgeWt = edgeWtTemp;


				auto t2 = std::chrono::high_resolution_clock::now();
				std::chrono::duration<double> elapsed = t2 - t1;
				addETime += elapsed.count();
			}
		}
	}


}

//G, labels, g_Image3D, fgG, bgG, fgLocalArtConnectivity, bgLocalArtConnectivity, numSlices, width, height, fgConn, nodes, Label(labels[k][i + j * width]), edgeWt, S, simpleDictionary3D, nodeVoxels
void simplifyGenerator(Graph & G, std::vector<uint32_t*> & labels, const std::vector<stbi_uc*>& g_Image3D, const std::vector< std::vector<float> >& g_Image3DF, 
	map<int, map<int, int> > & fgLocalArtConnectivity,
	map<int, map<int, int> > & bgLocalArtConnectivity, int numSlices, int width, int height, int fgConn,
	std::vector<node> & nodes, int nodeIndex, map<std::vector<int>, int> & edgeWt, float S,
	const std::vector<unsigned char> & simpleDictionary3D, const std::vector<Coordinate> & nodeVoxels, int genMode, map<int, int> & parentCompFg, map<int, int> & parentCompBg, bool & generatorChanged
	, grapht& fgGWithFills, grapht& bgGWithCuts, grapht & coreG, grapht & nG, float & minigraphT, float & addETime, float & otherTime
) {
	//grapht & fgG, grapht & bgG, 
	auto t1 = std::chrono::high_resolution_clock::now();
	std::vector< std::vector<int> > fgMask, bgMask;
	if (fgConn == 0) {
		fgMask = structCube;
		bgMask = structCross3D;
	}
	else {
		fgMask = structCross3D;
		bgMask = structCube;
	}

	bool isFloat = g_Image3DF.size() > 0;
	if (((int)nodes[nodeIndex].type) == 3) { //Fill
		auto neighbours = adjacent_vertices(nodeIndex, G);
		std::vector<int> compIndices; int nodeAdjFg = 0;
		for (auto u : make_iterator_range(neighbours)) {

			if (((int)nodes[u].inFg) == 1) {
				if (edgeWt[{nodeIndex, (int)u}] == 1 ) {
					if (((int)nodes[u].type) == 0 || ((int)nodes[u].type) == 2) {
						if (nodes[u].valid) {
							if (nodes[u].overallCompIndexFg != -1) {
								compIndices.push_back(nodes[u].overallCompIndexFg);
								nodeAdjFg += 1;
							}
						}
					}
				}
			}
		}
		//Delete duplicate connected components
		std::sort(compIndices.begin(), compIndices.end());
		compIndices.erase(unique(compIndices.begin(), compIndices.end()), compIndices.end());
		
		//Fill not connected to any fg comps
		int vx, vy, vz, nx, ny, nz;

		map<Coordinate, int> vtxToGNode;
		int  newVtxIndex = 1;
		map<int, int> fgCompToMini;
		if (compIndices.size() > 0) {
			for (int i = 0; i < compIndices.size(); i++) {
				fgCompToMini[compIndices[i]] = i;
			}
			newVtxIndex = compIndices.size();
		}
		grapht miniFgG = grapht();
		std::vector<weightedCoord> P;
		//Create vertex indices
		std::vector<bool> miniFgValid(newVtxIndex + nodeVoxels.size(), false);

		//std::vector<node> minigraphNodes;
		for (int i = 0; i < nodeVoxels.size(); i++) {
			vx = nodeVoxels[i].x; vy = nodeVoxels[i].y; vz = nodeVoxels[i].z;
			vtxToGNode[Coordinate(vx, vy, vz)] = newVtxIndex;
			miniFgValid[newVtxIndex] = true;
			newVtxIndex += 1;

			if (isFloat) {
				weightedCoord wc = { vx, vy, vz, abs((float)g_Image3DF[vz][vx + width * vy] - S) };
				P.push_back(wc);
			}
			else {
				weightedCoord wc = { vx, vy, vz, abs((float)g_Image3D[vz][vx + width * vy] - S) };
				P.push_back(wc);
			}
		}


		for (int i = 0; i < compIndices.size(); i++) {
			miniFgValid[fgCompToMini[compIndices[i]]] = true;
		}
		for (int i = 0; i < newVtxIndex; i++) {
			add_vertex(miniFgG);
		}
		 //Construct internal and external FG minigraph edges
		for (int i = 0; i < nodeVoxels.size(); i++) {
			vx = nodeVoxels[i].x; vy = nodeVoxels[i].y; vz = nodeVoxels[i].z;
			Coordinate vC(vx, vy, vz);
			for (int j = 0; j < fgMask.size(); j++) {
				nx = vx + fgMask[j][0]; ny = vy + fgMask[j][1]; nz = vz + fgMask[j][2];
				Coordinate vN(nx, ny, nz);
				if (nx >= 0 && ny >= 0 && nz >= 0 && nx < width && ny < height && nz < numSlices) {
					if (Label(labels[nz][nx + (width*(ny))]) == Label(labels[vz][vx + (width*(vy))])) { //edge within voxel
						if (!boost::edge(vtxToGNode[vC], vtxToGNode[vN], miniFgG).second) {
							add_edge(vtxToGNode[vC], vtxToGNode[vN], miniFgG);
						}
					}
					else {
						if (((int)nodes[Label(labels[nz][nx + (width*(ny))])].type) == 0
							|| ((int)nodes[Label(labels[nz][nx + (width*(ny))])].type) == 2
							) {
							if (!boost::edge(vtxToGNode[vC], fgCompToMini[nodes[Label(labels[nz][nx + (width*(ny))])].overallCompIndexFg], miniFgG).second) {
								add_edge(vtxToGNode[vC], fgCompToMini[nodes[Label(labels[nz][nx + (width*(ny))])].overallCompIndexFg], miniFgG);
							}
						}
					}
				}
			}
		}

		int h0 = 1; int h2;
		map<int, int> parentComp;
		map<int, int> nodeToComp;
		auto neighboursN = adjacent_vertices(nodeIndex, G);
		std::vector<int> uniqueComps;
		for (auto u : make_iterator_range(neighboursN)) {
			if (((int)nodes[u].type) == N) {
				uniqueComps.push_back(getParent(parentCompBg, nodes[u].compIndex));
			}
		}
		std::sort(uniqueComps.begin(), uniqueComps.end());
		uniqueComps.erase(unique(uniqueComps.begin(), uniqueComps.end()), uniqueComps.end());

		h2 = uniqueComps.size();
		 //Next step sort voxels by intensity and go thru them
		std::sort(P.begin(), P.end(), compareByIntensity);
		int voxelsDeleted = 0;
		std::vector<vertex_t> art_points;
		auto t1 = std::chrono::high_resolution_clock::now();
		auto t21 = std::chrono::high_resolution_clock::now();
		std::chrono::duration<double> elapsed1 = t21 - t1;
		otherTime += elapsed1.count();

		for (int i = 0; i < P.size(); i++) {
			weightedCoord wc = P[i];

			if (Label(labels[wc.z][wc.x + (width*(wc.y))]) == nodeIndex) {
				//auto t1 = std::chrono::high_resolution_clock::now();
				Coordinate wcC(wc.x, wc.y, wc.z);
				int changeH2;
				std::vector<int> nBgNodes;
					for (int j = 0; j < bgMask.size(); j++) {
						nx = wc.x + bgMask[j][0]; ny = wc.y + bgMask[j][1]; nz = wc.z + bgMask[j][2];
						if (nx >= 0 && ny >= 0 && nz >= 0 && nx < width && ny < height && nz < numSlices) {
							if (Label(labels[nz][nx + (width*(ny))]) != nodeIndex) { //edge within voxel
								if (((int)nodes[Label(labels[nz][nx + (width*(ny))])].type) == 1
									) {
									nBgNodes.push_back(getParent(parentCompBg, nodes[Label(labels[nz][nx + (width*(ny))])].compIndex));
								}
							}
						}
					}
					//Number of neighboring bg components
					std::sort(nBgNodes.begin(), nBgNodes.end());
					nBgNodes.erase(unique(nBgNodes.begin(), nBgNodes.end()), nBgNodes.end());

					changeH2 = 1 - nBgNodes.size();
				int h0N; int changeH0;
				
				if (P.size() == 1 && nodeAdjFg == 0) {
					changeH0 = -1;
				}
				else {
						
					clear_vertex(vtxToGNode[wcC], miniFgG);
					miniFgValid[vtxToGNode[wcC]] = false;
					
					auto t1 = std::chrono::high_resolution_clock::now();
					h0N = minigraphComps(miniFgG, miniFgValid, newVtxIndex);
					auto t2 = std::chrono::high_resolution_clock::now();
					std::chrono::duration<double> elapsed = t2 - t1;
					minigraphT += elapsed.count();
					changeH0 = h0N - h0;
				}

				int dV = 1, dE = 0, dF = 0, dC = 0;
				for (int j = 0; j < fgMask.size(); j++) {
					nx = wc.x + fgMask[j][0]; ny = wc.y + fgMask[j][1]; nz = wc.z + fgMask[j][2];
					if (nx >= 0 && ny >= 0 && nz >= 0 && nx < width && ny < height && nz < numSlices) {
						if (((int)nodes[Label(labels[nz][nx + (width*(ny))])].type) == 0 ||
							((int)nodes[Label(labels[nz][nx + (width*(ny))])].type) == 3 ||
							((int)nodes[Label(labels[nz][nx + (width*(ny))])].type) == 2 
							) {
							dE += 1;
						}
					}
				}
				for (int j = 0; j < adjFaces6Conn.size(); j++) {
					std::vector< std::vector<int> > adjFace = adjFaces6Conn[j];
					bool hasN = false;
					for (int k = 0; k < adjFace.size(); k++) {
						nx = wc.x + adjFace[k][0]; ny = wc.y + adjFace[k][1]; nz = wc.z + adjFace[k][2];
						if (nx >= 0 && ny >= 0 && nz >= 0 && nx < width && ny < height && nz < numSlices) {
							if (((int)nodes[Label(labels[nz][nx + (width*(ny))])].type) == 1) {
								hasN = true;
							}
						}
					}
					if (!hasN) {
						dF += 1;
					}
				}
				for (int j = 0; j < adjCubes6Conn.size(); j++) {
					std::vector< std::vector<int> > adjCube = adjCubes6Conn[j];
					bool hasN = false;
					for (int k = 0; k < adjCube.size(); k++) {
						nx = wc.x + adjCube[k][0]; ny = wc.y + adjCube[k][1]; nz = wc.z + adjCube[k][2];
						if (nx >= 0 && ny >= 0 && nz >= 0 && nx < width && ny < height && nz < numSlices) {
							if (((int)nodes[Label(labels[nz][nx + (width*(ny))])].type) == 1) {
								hasN = true;
							}
						}
					}
					if (!hasN) {
						dC += 1;
					}
				}
				int dEuler = -(dV - dE + dF - dC);
				int changeH1 = changeH0 + changeH2 - dEuler;
				if (changeH0 <= 0 && changeH2 <= 0 && changeH1 <= 0 && changeH0 + changeH2 + changeH1 < 0) {
					generatorChanged = true;
					h2 = h2 + changeH2;
					for (int j = 0; j < bgMask.size(); j++) {
						nx = wc.x + bgMask[j][0]; ny = wc.y + bgMask[j][1]; nz = wc.z + bgMask[j][2];
						if (nx >= 0 && ny >= 0 && nz >= 0 && nx < width && ny < height && nz < numSlices) {
							if (((int)nodes[Label(labels[nz][nx + (width*(ny))])].type) == 1) {
								labels[wc.z][wc.x + (width*(wc.y))] = labels[nz][nx + (width*(ny))];
							}
						}
					}

					voxelsDeleted += 1;
					//voxel has now been relabeled to a background component, create potential new edges for background component
					for (int j = 0; j < bgMask.size(); j++) {
						nx = wc.x + bgMask[j][0]; ny = wc.y + bgMask[j][1]; nz = wc.z + bgMask[j][2];
						if (nx >= 0 && ny >= 0 && nz >= 0 && nx < width && ny < height && nz < numSlices) {
							if (Label(labels[wc.z][wc.x + (width*(wc.y))]) != Label(labels[nz][nx + (width*(ny))])) {
								if (((int)nodes[Label(labels[nz][nx + (width*(ny))])].type) == 1 ||
									((int)nodes[Label(labels[nz][nx + (width*(ny))])].type) == 3
									) {
									if (ccNeighborFill6Conn(nodes, wc.x, wc.y, wc.z, nx, ny, nz, labels, width)) { //, g_Image3D, S
										if (edgeWt.find({ (int)Label(labels[wc.z][wc.x + (width*(wc.y))]), (int)Label(labels[nz][nx + (width*(ny))]) }) == edgeWt.end()) {
											add_edge(Label(labels[nz][nx + (width*(ny))]), Label(labels[wc.z][wc.x + (width*(wc.y))]), G);
											if (abs(nx - wc.x) + abs(ny - wc.y) + abs(nz - wc.z) == 1) {
												if (
													(((int)nodes[Label(labels[wc.z][wc.x + wc.y * width])].type) == CORE || ((int)nodes[Label(labels[wc.z][wc.x + wc.y * width])].type) == CUT || ((int)nodes[Label(labels[wc.z][wc.x + wc.y * width])].type) == FILL)
													&&
													(((int)nodes[Label(labels[nz][nx + ny * width])].type) == CORE || ((int)nodes[Label(labels[nz][nx + ny * width])].type) == CUT || ((int)nodes[Label(labels[nz][nx + ny * width])].type) == FILL)

													) {
													add_edge(Label(labels[wc.z][wc.x + wc.y * width]), Label(labels[nz][nx + ny * width]), fgGWithFills);

												}
												if (
													(((int)nodes[Label(labels[wc.z][wc.x + wc.y * width])].type) == CORE)
													&&
													(((int)nodes[Label(labels[nz][nx + ny * width])].type) == CORE)

													) {
													add_edge(Label(labels[wc.z][wc.x + wc.y * width]), Label(labels[nz][nx + ny * width]), coreG);

												}
											}
											if (
												(((int)nodes[Label(labels[wc.z][wc.x + wc.y * width])].type) == N || ((int)nodes[Label(labels[wc.z][wc.x + wc.y * width])].type) == CUT || ((int)nodes[Label(labels[wc.z][wc.x + wc.y * width])].type) == FILL)
												&&
												(((int)nodes[Label(labels[nz][nx + ny * width])].type) == N || ((int)nodes[Label(labels[nz][nx + ny * width])].type) == CUT || ((int)nodes[Label(labels[nz][nx + ny * width])].type) == FILL)

												) {
												add_edge(Label(labels[wc.z][wc.x + wc.y * width]), Label(labels[nz][nx + ny * width]), bgGWithCuts);

											}
											if (
												(((int)nodes[Label(labels[wc.z][wc.x + wc.y * width])].type) == N)
												&&
												(((int)nodes[Label(labels[nz][nx + ny * width])].type) == N)

												) {
												add_edge(Label(labels[wc.z][wc.x + wc.y * width]), Label(labels[nz][nx + ny * width]), nG);

											}
										}
									}

									if (abs(nx - wc.x) + abs(ny - wc.y) + abs(nz - wc.z) == 1) {
										if (edgeWt[{(int)Label(labels[wc.z][wc.x + (width * (wc.y))]), (int)Label(labels[nz][nx + (width * (ny))])}] == 0) {
											if (
												(((int)nodes[Label(labels[wc.z][wc.x + wc.y * width])].type) == CORE || ((int)nodes[Label(labels[wc.z][wc.x + wc.y * width])].type) == CUT || ((int)nodes[Label(labels[wc.z][wc.x + wc.y * width])].type) == FILL)
												&&
												(((int)nodes[Label(labels[nz][nx + ny * width])].type) == CORE || ((int)nodes[Label(labels[nz][nx + ny * width])].type) == CUT || ((int)nodes[Label(labels[nz][nx + ny * width])].type) == FILL)

												) {
												add_edge(Label(labels[wc.z][wc.x + wc.y * width]), Label(labels[nz][nx + ny * width]), fgGWithFills);

											}
										}
										edgeWt[{(int)Label(labels[wc.z][wc.x + (width*(wc.y))]), (int)Label(labels[nz][nx + (width*(ny))])}] = 1;
										edgeWt[{(int)Label(labels[nz][nx + (width*(ny))]), (int)Label(labels[wc.z][wc.x + (width*(wc.y))])}] = 1;


									}
									else {
										if (ccNeighborFill6Conn(nodes, wc.x, wc.y, wc.z, nx, ny, nz, labels, width)) { //, g_Image3D, S
											if (edgeWt.find({ (int)Label(labels[wc.z][wc.x + (width*(wc.y))]), (int)Label(labels[nz][nx + (width*(ny))]) }) == edgeWt.end()) {
												edgeWt[{(int)Label(labels[wc.z][wc.x + (width*(wc.y))]), (int)Label(labels[nz][nx + (width*(ny))])}] = 0;
												edgeWt[{(int)Label(labels[nz][nx + (width*(ny))]), (int)Label(labels[wc.z][wc.x + (width*(wc.y))])}] = 0;
											}
										}
									}
								}
								if (((int)nodes[Label(labels[nz][nx + (width*(ny))])].type) == 1 //|| ((int)nodes[Label(labels[nz][nx + (width*(ny))])].type) == 3
									) {
									if (getParent(parentCompBg, nodes[Label(labels[wc.z][wc.x + (width*(wc.y))])].compIndex)
										!= getParent(parentCompBg, nodes[Label(labels[nz][nx + (width*(ny))])].compIndex)
										) {										
										parentCompBg[getParent(parentCompBg, nodes[Label(labels[nz][nx + (width*(ny))])].compIndex)] = getParent(parentCompBg, nodes[Label(labels[wc.z][wc.x + (width*(wc.y))])].compIndex);

									}
								}
							}

						}


					}
					queue<Coordinate> q;
					//Iteratively remove simple voxels
					for (int j = 0; j < structCube.size(); j++) {
						nx = wc.x + structCube[j][0]; ny = wc.y + structCube[j][1]; nz = wc.z + structCube[j][2];
						if (nx >= 0 && ny >= 0 && nz >= 0 && nx < width && ny < height && nz < numSlices) {
							if (Label(labels[nz][nx + (width*(ny))]) == nodeIndex) {
								q.push(Coordinate(nx, ny, nz));
							}
						}
					}
					map<Coordinate, int> genVisited;
					for (int j = 0; j < P.size(); j++) {

						if (Label(labels[P[j].z][P[j].x + (width*(P[j].y))]) == nodeIndex) {
							Coordinate pNow(P[j].x, P[j].y, P[j].z);
							genVisited[pNow] = 0;
						}
					}
					
					Coordinate wCoord(wc.x, wc.y, wc.z);
					genVisited[wCoord] = 1;
					while (!q.empty()) {
						Coordinate p = q.front();
						int px = p.x; int py = p.y; int pz = p.z;
						q.pop();
						genVisited[p] = 1;
						if (simple3DLabel(labels, nodes, px, py, pz, numSlices, width, height, simpleDictionary3D, nodeIndex, true, fgConn)) {
							if ((int)Label(labels[pz][px + (width*(py))]) == nodeIndex) {
								voxelsDeleted += 1;
								for (int j = 0; j < structCube.size(); j++) {
									nx = px + structCube[j][0]; ny = py + structCube[j][1]; nz = pz + structCube[j][2];
									if (nx >= 0 && ny >= 0 && nz >= 0 && nx < width && ny < height && nz < numSlices) {
										if (abs(nx - px) + abs(ny - py) + abs(nz - pz) == 1) {
											if (((int)nodes[Label(labels[nz][nx + (width*(ny))])].type) == 1) {
												labels[pz][px + (width*(py))] = labels[nz][nx + (width*(ny))];
												clear_vertex(vtxToGNode[p], miniFgG);
												miniFgValid[vtxToGNode[p]] = false;
												for (int a = 0; a < structCube.size(); a++) {
													int nxi = px + structCube[a][0];
													int nyi = py + structCube[a][1];
													int nzi = pz + structCube[a][2];
													if (nxi >= 0 && nyi >= 0 && nzi >= 0 && nxi < width && nyi < height && nzi < numSlices) {
														if (Label(labels[pz][px + (width*(py))]) != Label(labels[nzi][nxi + (width*(nyi))])) {
															if (((int)nodes[Label(labels[nzi][nxi + (width*(nyi))])].type) == 1 ||
																((int)nodes[Label(labels[nzi][nxi + (width*(nyi))])].type) == 2 ||
																((int)nodes[Label(labels[nzi][nxi + (width*(nyi))])].type) == 3
																) {
																if (ccNeighborFill6Conn(nodes, px, py, pz, nxi, nyi, nzi, labels, width)) {
																	if (edgeWt.find({ (int)Label(labels[pz][px + (width*(py))]), (int)Label(labels[nzi][nxi + (width*(nyi))]) }) == edgeWt.end()) {
																		add_edge(Label(labels[nzi][nxi + (width*(nyi))]), Label(labels[pz][px + (width*(py))]), G);
																		if (abs(nxi - px) + abs(nyi - py) + abs(nzi - pz) == 1) {
																			if (
																				(((int)nodes[Label(labels[pz][px + py * width])].type) == CORE || ((int)nodes[Label(labels[pz][px + py * width])].type) == CUT || ((int)nodes[Label(labels[pz][px + py * width])].type) == FILL)
																				&&
																				(((int)nodes[Label(labels[nzi][nxi + nyi * width])].type) == CORE || ((int)nodes[Label(labels[nzi][nxi + nyi * width])].type) == CUT || ((int)nodes[Label(labels[nzi][nxi + nyi * width])].type) == FILL)

																				) {
																				add_edge(Label(labels[pz][px + py * width]), Label(labels[nzi][nxi + nyi * width]), fgGWithFills);

																			}
																			if (
																				(((int)nodes[Label(labels[pz][px + py * width])].type) == CORE)
																				&&
																				(((int)nodes[Label(labels[nzi][nxi + nyi * width])].type) == CORE)

																				) {
																				add_edge(Label(labels[pz][px + py * width]), Label(labels[nzi][nxi + nyi * width]), coreG);

																			}
																		}
																		if (
																			(((int)nodes[Label(labels[pz][px + py * width])].type) == N)
																			&&
																			(((int)nodes[Label(labels[nzi][nxi + nyi * width])].type) == N)

																			) {
																			add_edge(Label(labels[pz][px + py * width]), Label(labels[nzi][nxi + nyi * width]), nG);

																		}
																		
																	}
																	if (abs(nxi - px) + abs(nyi - py) + abs(nzi - pz) == 1) {
																		edgeWt[{(int)Label(labels[pz][px + (width*(py))]), (int)Label(labels[nzi][nxi + (width*(nyi))])}] = 1;
																		edgeWt[{(int)Label(labels[nzi][nxi + (width*(nyi))]), (int)Label(labels[pz][px + (width*(py))])}] = 1;
																		if (edgeWt[{(int)Label(labels[pz][px + (width * (py))]), (int)Label(labels[nz][nx + (width * (ny))])}] == 0) {
																			if (
																				(((int)nodes[Label(labels[pz][px + py * width])].type) == CORE || ((int)nodes[Label(labels[pz][px + py * width])].type) == CUT || ((int)nodes[Label(labels[pz][px + py * width])].type) == FILL)
																				&&
																				(((int)nodes[Label(labels[nzi][nxi + nyi * width])].type) == CORE || ((int)nodes[Label(labels[nzi][nxi + nyi * width])].type) == CUT || ((int)nodes[Label(labels[nzi][nxi + nyi * width])].type) == FILL)

																				) {
																				add_edge(Label(labels[pz][px + py * width]), Label(labels[nzi][nxi + nyi * width]), fgGWithFills);

																			}
																		}
																		edgeWt[{(int)Label(labels[pz][px + (width * (py))]), (int)Label(labels[nzi][nxi + (width * (nyi))])}] = 1;
																		edgeWt[{(int)Label(labels[nzi][nxi + (width * (nyi))]), (int)Label(labels[pz][px + (width * (py))])}] = 1;


																		
																	}
																	else {
																		if (edgeWt.find({ (int)Label(labels[pz][px + (width*(py))]), (int)Label(labels[nzi][nxi + (width*(nyi))]) }) == edgeWt.end()) {
																			edgeWt[{(int)Label(labels[pz][px + (width*(py))]), (int)Label(labels[nzi][nxi + (width*(nyi))])}] = 0;
																			edgeWt[{(int)Label(labels[nzi][nxi + (width*(nyi))]), (int)Label(labels[pz][px + (width*(py))])}] = 0;
																		}
																	}
																}
															}

														}

															if (((int)nodes[Label(labels[nzi][nxi + (width*(nyi))])].type) == N) {
																if (getParent(parentCompBg, nodes[Label(labels[pz][px + (width*(py))])].compIndex)
																	!= getParent(parentCompBg, nodes[Label(labels[nzi][nxi + (width*(nyi))])].compIndex)
																	) {
																	parentCompBg[getParent(parentCompBg, nodes[Label(labels[nzi][nxi + (width*(nyi))])].compIndex)] = getParent(parentCompBg, nodes[Label(labels[pz][px + (width*(py))])].compIndex);

																}
															}
													}

												}
												}
											}
										if (Label(labels[nz][nx + (width*(ny))]) == nodeIndex) {
											Coordinate np(nx, ny, nz);
											if (genVisited[np] == 0) {
												q.push(np);
												genVisited[np] = 1;
											}
										}
									}
								}
							}
						}
						else {
							genVisited[p] = 0;
						}
						
					}

				}
				else {
					//if h0, h1, h2 total does not decrease, add back to graph
					Coordinate wCoord(wc.x, wc.y, wc.z);
					miniFgValid[vtxToGNode[wCoord]] = true;
					for (int j = 0; j < fgMask.size(); j++) {
						nx = wc.x + fgMask[j][0]; ny = wc.y + fgMask[j][1]; nz = wc.z + fgMask[j][2];
						if (nx >= 0 && ny >= 0 && nz >= 0 && nx < width && ny < height && nz < numSlices) {
							if (Label(labels[nz][nx + (width*(ny))]) == nodeIndex) {
								if (!boost::edge(vtxToGNode[{nx, ny, nz}], vtxToGNode[{wc.x, wc.y, wc.z}], miniFgG).second) {
									add_edge(vtxToGNode[Coordinate(nx, ny, nz)], vtxToGNode[wCoord], miniFgG);
								}
							}
							else {
								if (((int)nodes[Label(labels[nz][nx + (width*(ny))])].type) == 0
									|| ((int)nodes[Label(labels[nz][nx + (width*(ny))])].type) == 2
									) {
									if (!boost::edge(vtxToGNode[{wc.x, wc.y, wc.z}], fgCompToMini[nodes[Label(labels[nz][nx + (width * (ny))])].overallCompIndexFg], miniFgG).second) {
										add_edge(vtxToGNode[wCoord], fgCompToMini[nodes[Label(labels[nz][nx + (width*(ny))])].overallCompIndexFg], miniFgG);
									}
								}

							}
						}
					}
				}

			}
		}
		
		if (voxelsDeleted >= P.size()) {
			nodes[nodeIndex].valid = false;
			auto neighbours = adjacent_vertices(nodeIndex, G);
			for (auto u : make_iterator_range(neighbours)) {
				edgeWt.erase({ nodeIndex, (int)u }); edgeWt.erase({ (int)u , nodeIndex });
			}
			clear_vertex(nodeIndex, G);
			for (int j = 0; j < P.size(); j++) {
				int px = P[j].x; int py = P[j].y; int pz = P[j].z;
				for (int k = 0; k < structCube.size(); k++) {
					int nx = px + structCube[k][0]; int ny = py + structCube[k][1]; int nz = pz + structCube[k][2];
					if (nx >= 0 && ny >= 0 && nz >= 0 && nx < width && ny < height && nz < numSlices) {
						if (Label(labels[pz][px + py * width]) != Label(labels[nz][nx + ny * width])) {
							if (((int)nodes[Label(labels[nz][nx + ny * width])].type) != CORE) {
								if (edgeWt.find({ (int)Label(labels[pz][px + py * width]) , (int)Label(labels[nz][nx + ny * width]) }) == edgeWt.end()) {
									if (ccNeighborFill6Conn(nodes, px, py, pz, nx, ny, nz, labels, width)) {
										add_edge(Label(labels[pz][px + py * width]), Label(labels[nz][nx + ny * width]), G);
										if (
											(((int)nodes[Label(labels[pz][px + py * width])].type) == N)
											&&
											(((int)nodes[Label(labels[nz][nx + ny * width])].type) == N)

											) {
											add_edge(Label(labels[pz][px + py * width]), Label(labels[nz][nx + ny * width]), nG);

										}
										
										if (abs(nx - px) + abs(ny - py) + abs(nz - pz) == 1) {
											edgeWt[{(int)Label(labels[pz][px + py * width]), (int)Label(labels[nz][nx + ny * width])}] = 1;
											edgeWt[{(int)Label(labels[nz][nx + ny * width]), (int)Label(labels[pz][px + py * width])}] = 1;
											if (
												(((int)nodes[Label(labels[pz][px + py * width])].type) == CORE || ((int)nodes[Label(labels[pz][px + py * width])].type) == CUT || ((int)nodes[Label(labels[pz][px + py * width])].type) == FILL)
												&&
												(((int)nodes[Label(labels[nz][nx + ny * width])].type) == CORE || ((int)nodes[Label(labels[nz][nx + ny * width])].type) == CUT || ((int)nodes[Label(labels[nz][nx + ny * width])].type) == FILL)

												) {
												add_edge(Label(labels[pz][px + py * width]), Label(labels[nz][nx + ny * width]), fgGWithFills);

											}
											if (
												(((int)nodes[Label(labels[pz][px + py * width])].type) == CORE)
												&&
												(((int)nodes[Label(labels[nz][nx + ny * width])].type) == CORE)

												) {
												add_edge(Label(labels[pz][px + py * width]), Label(labels[nz][nx + ny * width]), coreG);

											}
										}
										else {
											edgeWt[{(int)Label(labels[pz][px + py * width]), (int)Label(labels[nz][nx + ny * width])}] = 0;
											edgeWt[{(int)Label(labels[nz][nx + ny * width]), (int)Label(labels[pz][px + py * width])}] = 0;
										}
									}
								}
								else {
									if (abs(nx - px) + abs(ny - py) + abs(nz - pz) == 1) {
										edgeWt[{(int)Label(labels[pz][px + py * width]), (int)Label(labels[nz][nx + ny * width])}] = 1;
										edgeWt[{(int)Label(labels[nz][nx + ny * width]), (int)Label(labels[pz][px + py * width])}] = 1;
									}
								}
							}
						}
					}
				}


			}
		}

		else {
			if (voxelsDeleted > 0) {
				auto t1 = std::chrono::high_resolution_clock::now();
				
				map< std::vector<int>, int> edgeWtTemp = edgeWt;
				auto neighbours = adjacent_vertices(nodeIndex, G);
				for (auto u : make_iterator_range(neighbours)) {
					edgeWtTemp.erase({ nodeIndex, (int)u }); edgeWtTemp.erase({ (int)u , nodeIndex });
				}
				clear_vertex(nodeIndex, G);
				map<Coordinate, bool> visited;
				for (int j = 0; j < P.size(); j++) {
					visited[Coordinate(P[j].x, P[j].y, P[j].z)] = false;
				}
				int currentLabelIndex = nodeIndex;

				for (int j = 0; j < P.size(); j++) {
					Coordinate pNow(P[j].x, P[j].y, P[j].z);
					if (!visited[pNow]) {
						if (Label(labels[pNow.z][pNow.x + (width*(pNow.y))]) == nodeIndex) {
							node n; n.labelCost = 0; n.floatCost = 0.0;  n.type = 3; n.inFg = 0; n.index = currentLabelIndex;
							
							queue<Coordinate> q;
							q.push(pNow);
							if (currentLabelIndex != nodeIndex) {
								add_vertex(G);
								nodes.push_back(n);
							}
							else {
								nodes[currentLabelIndex] = n;
							}
							std::vector<float> diffs;
							while (!q.empty()) {
								Coordinate p = q.front();
								int px = p.x; int py = p.y; int pz = p.z;
								q.pop();
								visited[p] = true;
								if (g_Image3DF.size() > 0) {
									nodes[currentLabelIndex].floatCost += -1.0;
								}
								else {
									nodes[currentLabelIndex].floatCost += (-gradient(px, py, pz, g_Image3D, width, height, numSlices));
								}
								
								uint32_t label32 = currentLabelIndex;
								changeLabel(label32, labels[pz][px + py * width]);
								for (int k = 0; k < structCube.size(); k++) {
									nx = px + structCube[k][0]; ny = py + structCube[k][1]; nz = pz + structCube[k][2];
									if (nx >= 0 && ny >= 0 && nz >= 0 && nx < width && ny < height && nz < numSlices) {
										if (Label(labels[pz][px + py * width]) != Label(labels[nz][nx + ny * width])) {
											if (nodeIndex != Label(labels[nz][nx + ny * width])) {
												if (ccNeighborFill6Conn(nodes, px, py, pz, nx, ny, nz, labels, width)) { //, g_Image3D, S
													if (edgeWtTemp.find({ (int)Label(labels[pz][px + py * width]),(int)Label(labels[nz][nx + ny * width]) }) == edgeWtTemp.end()) {
														add_edge(Label(labels[pz][px + py * width]), Label(labels[nz][nx + ny * width]), G);
														if (
															(((int)nodes[Label(labels[pz][px + py * width])].type) == N || ((int)nodes[Label(labels[pz][px + py * width])].type) == CUT || ((int)nodes[Label(labels[pz][px + py * width])].type) == FILL)
															&&
															(((int)nodes[Label(labels[nz][nx + ny * width])].type) == N || ((int)nodes[Label(labels[nz][nx + ny * width])].type) == CUT || ((int)nodes[Label(labels[nz][nx + ny * width])].type) == FILL)

															) {
															add_edge(Label(labels[pz][px + py * width]), Label(labels[nz][nx + ny * width]), bgGWithCuts);

														}
														if (
															(((int)nodes[Label(labels[pz][px + py * width])].type) == N)
															&&
															(((int)nodes[Label(labels[nz][nx + ny * width])].type) == N)

															) {
															add_edge(Label(labels[pz][px + py * width]), Label(labels[nz][nx + ny * width]), nG);
														}
														if (abs(nx - px) + abs(ny - py) + abs(nz - pz) == 1) {
															edgeWtTemp[{(int)Label(labels[pz][px + py * width]), (int)Label(labels[nz][nx + ny * width])}] = 1;
															edgeWtTemp[{(int)Label(labels[nz][nx + ny * width]), (int)Label(labels[pz][px + py * width])}] = 1;
															if (
																(((int)nodes[Label(labels[pz][px + py * width])].type) == CORE || ((int)nodes[Label(labels[pz][px + py * width])].type) == CUT || ((int)nodes[Label(labels[pz][px + py * width])].type) == FILL)
																&&
																(((int)nodes[Label(labels[nz][nx + ny * width])].type) == CORE || ((int)nodes[Label(labels[nz][nx + ny * width])].type) == CUT || ((int)nodes[Label(labels[nz][nx + ny * width])].type) == FILL)

																) {
																add_edge(Label(labels[pz][px + py * width]), Label(labels[nz][nx + ny * width]), fgGWithFills);

															}

															if (
																(((int)nodes[Label(labels[pz][px + py * width])].type) == CORE)
																&&
																(((int)nodes[Label(labels[nz][nx + ny * width])].type) == CORE)

																) {
																add_edge(Label(labels[pz][px + py * width]), Label(labels[nz][nx + ny * width]), coreG);

															}
														}
														else {
															edgeWtTemp[{(int)Label(labels[pz][px + py * width]), (int)Label(labels[nz][nx + ny * width])}] = 0;
															edgeWtTemp[{(int)Label(labels[nz][nx + ny * width]), (int)Label(labels[pz][px + py * width])}] = 0;

														}
													}
													else {
														if (abs(nx - px) + abs(ny - py) + abs(nz - pz) == 1) {
															edgeWtTemp[{(int)Label(labels[pz][px + py * width]), (int)Label(labels[nz][nx + ny * width])}] = 1;
															edgeWtTemp[{(int)Label(labels[nz][nx + ny * width]), (int)Label(labels[pz][px + py * width])}] = 1;
														}
													}
												}
											}
										}
										if (Label(labels[nz][nx + ny * width]) == nodeIndex) {
											Coordinate np(nx, ny, nz);
											if (!visited[np]) {
												if (ccNeighborFill6Conn(nodes, px, py, pz, nx, ny, nz, labels, width)) {
													q.push(np);
													visited[np] = true;
												}
											}
										}
									}
								}
							}
							//If empty, do lexicographical ordering if necessary
							if (currentLabelIndex == nodeIndex) {
								currentLabelIndex = nodes.size();
							}
							else {
								currentLabelIndex += 1;
							}
						}
					}
				}
				edgeWt = edgeWtTemp;

				
				auto t2 = std::chrono::high_resolution_clock::now();
				std::chrono::duration<double> elapsed = t2 - t1;
				addETime += elapsed.count();
			}
		}
	}
	//node type is cut
	if (((int)nodes[nodeIndex].type) == 2) {
		int bgConn = 1 - fgConn;
		auto neighbours = adjacent_vertices(nodeIndex, G);
		std::vector<int> compIndices; int adjNodesBg = 0;
		for (auto u : make_iterator_range(neighbours)) {
			if (((int)nodes[u].inFg) == 0) {
				if (edgeWt[{nodeIndex, (int)u}] == 1 || (edgeWt[{nodeIndex, (int)u}] == 0 &&
					bgConn == 0
					)) {
					if (nodes[u].valid) {
						if (((int)nodes[u].type) == 1 || ((int)nodes[u].type) == 3) {
							if (nodes[u].overallCompIndexBg != -1) {
								compIndices.push_back(nodes[u].overallCompIndexBg);
								adjNodesBg += 1;
							}
						}
					}
				}
			}

		}

		


		//Delete duplicate connected components
		std::sort(compIndices.begin(), compIndices.end());
		compIndices.erase(unique(compIndices.begin(), compIndices.end()), compIndices.end());
		//Cut not connected to any bg comps
		int vx, vy, vz, nx, ny, nz;
		map<Coordinate, int> vtxToGNode;
		int  newVtxIndex = 1;
		map<int, int> bgCompToMini;
		if (compIndices.size() > 0) {
			for (int i = 0; i < compIndices.size(); i++) {
				bgCompToMini[compIndices[i]] = i;
			}
			newVtxIndex = compIndices.size();

		}
		grapht miniBgG = grapht();
		std::vector<weightedCoord> P;
		//Create vertex indices
		std::vector<bool> miniBgValid(newVtxIndex + nodeVoxels.size(), false);
		for (int i = 0; i < nodeVoxels.size(); i++) {
			vx = nodeVoxels[i].x; vy = nodeVoxels[i].y; vz = nodeVoxels[i].z;
			vtxToGNode[Coordinate(vx, vy, vz)] = newVtxIndex;
			miniBgValid[newVtxIndex] = true;
			newVtxIndex += 1;
			if (isFloat) {
				weightedCoord wc = { vx, vy, vz, abs((float)g_Image3DF[vz][vx + width * vy] - S) };
				P.push_back(wc);
			}
			else {
				weightedCoord wc = { vx, vy, vz, abs((float)g_Image3D[vz][vx + width * vy] - S) };
				P.push_back(wc);
			}
		}

		for (int i = 0; i < compIndices.size(); i++) {
			miniBgValid[bgCompToMini[compIndices[i]]] = true;
		}
		for (int i = 0; i < newVtxIndex; i++) {
			add_vertex(miniBgG);

		}
		//Construct internal and external BG minigraph edges
		for (int i = 0; i < nodeVoxels.size(); i++) {
			vx = nodeVoxels[i].x; vy = nodeVoxels[i].y; vz = nodeVoxels[i].z;
			Coordinate vC(vx, vy, vz);
			for (int j = 0; j < bgMask.size(); j++) {
				nx = vx + bgMask[j][0]; ny = vy + bgMask[j][1]; nz = vz + bgMask[j][2];
				Coordinate vN(nx, ny, nz);
				if (nx >= 0 && ny >= 0 && nz >= 0 && nx < width && ny < height && nz < numSlices) {
					if (Label(labels[nz][nx + (width*(ny))]) == Label(labels[vz][vx + (width*(vy))])) { //edge within voxel
						if (!boost::edge(vtxToGNode[vC], vtxToGNode[vN], miniBgG).second) {
							add_edge(vtxToGNode[vC], vtxToGNode[vN], miniBgG);
						}
					}
					else {
						if (((int)nodes[Label(labels[nz][nx + (width*(ny))])].type) == 1
							|| ((int)nodes[Label(labels[nz][nx + (width*(ny))])].type) == 3
							) {
							if (!boost::edge(vtxToGNode[vC], bgCompToMini[nodes[Label(labels[nz][nx + (width*(ny))])].overallCompIndexBg], miniBgG).second) {
								add_edge(vtxToGNode[vC], bgCompToMini[nodes[Label(labels[nz][nx + (width*(ny))])].overallCompIndexBg], miniBgG);
							}
						}
					}
				}
			}
		}
		int h2 = 1;
		int h0;
		map<int, int> nodeToComp;
		auto neighboursN = adjacent_vertices(nodeIndex, G);
		std::vector<int> uniqueComps;
		for (auto u : make_iterator_range(neighboursN)) {
			if (nodes[u].valid) {
				if (((int)nodes[u].type) == CORE) {
					uniqueComps.push_back(getParent(parentCompFg, nodes[u].compIndex));
				}
			}
		}
		std::sort(uniqueComps.begin(), uniqueComps.end());
		uniqueComps.erase(unique(uniqueComps.begin(), uniqueComps.end()), uniqueComps.end());
			
		h0 = uniqueComps.size();
		//Next step sort voxels by intensity and go thru them
		std::sort(P.begin(), P.end(), compareByIntensity);
		int voxelsDeleted = 0;
		auto t21 = std::chrono::high_resolution_clock::now();
		std::chrono::duration<double> elapsed1 = t21 - t1;
		otherTime += elapsed1.count();
		for (int i = 0; i < P.size(); i++) {
			weightedCoord wc = P[i];

			if (Label(labels[wc.z][wc.x + (width*(wc.y))]) == nodeIndex) {
				Coordinate wcC(wc.x, wc.y, wc.z);
				int changeH0;
				std::vector<int> nFgNodes;
					for (int j = 0; j < fgMask.size(); j++) {
						nx = wc.x + fgMask[j][0]; ny = wc.y + fgMask[j][1]; nz = wc.z + fgMask[j][2];
						if (nx >= 0 && ny >= 0 && nz >= 0 && nx < width && ny < height && nz < numSlices) {
							if (Label(labels[nz][nx + (width*(ny))]) != nodeIndex) { //edge within voxel
								if (((int)nodes[Label(labels[nz][nx + (width*(ny))])].type) == 0
									) {
									nFgNodes.push_back(getParent(parentCompFg, nodes[Label(labels[nz][nx + (width*(ny))])].compIndex  ));
									}
							}
						}
					}
					//Number of neighboring fg components
					std::sort(nFgNodes.begin(), nFgNodes.end());
					nFgNodes.erase(unique(nFgNodes.begin(), nFgNodes.end()), nFgNodes.end());

					changeH0 = 1 - nFgNodes.size();
				
					int h2N; int changeH2;
					
					clear_vertex(vtxToGNode[wcC], miniBgG);
					miniBgValid[vtxToGNode[wcC]] = false;
					auto t1 = std::chrono::high_resolution_clock::now();
						
					h2N = minigraphComps(miniBgG, miniBgValid, newVtxIndex);
					auto t2 = std::chrono::high_resolution_clock::now();
					std::chrono::duration<double> elapsed = t2 - t1;
					minigraphT += elapsed.count();
					changeH2 = h2N - h2;

				int dV = 1, dE = 0, dF = 0, dC = 0;
				for (int j = 0; j < fgMask.size(); j++) {
					nx = wc.x + fgMask[j][0]; ny = wc.y + fgMask[j][1]; nz = wc.z + fgMask[j][2];
					if (nx >= 0 && ny >= 0 && nz >= 0 && nx < width && ny < height && nz < numSlices) {
						if (((int)nodes[Label(labels[nz][nx + (width*(ny))])].type) == 0
							) {
							
							
							dE += 1;
						}
					}
				}
				for (int j = 0; j < adjFaces6Conn.size(); j++) {
					std::vector< std::vector<int> > adjFace = adjFaces6Conn[j];
					bool hasN = false;
					for (int k = 0; k < adjFace.size(); k++) {
						nx = wc.x + adjFace[k][0]; ny = wc.y + adjFace[k][1]; nz = wc.z + adjFace[k][2];
						if (nx >= 0 && ny >= 0 && nz >= 0 && nx < width && ny < height && nz < numSlices) {
							if (((int)nodes[Label(labels[nz][nx + (width*(ny))])].type) == 1 ||
								((int)nodes[Label(labels[nz][nx + (width*(ny))])].type) == 3 ||
								((int)nodes[Label(labels[nz][nx + (width*(ny))])].type) == 2 //is now core, cut has higher priority
								) {
								hasN = true;
							}
						}
					}
					if (!hasN) {
						dF += 1;
					}
				}
				for (int j = 0; j < adjCubes6Conn.size(); j++) {
					std::vector< std::vector<int> > adjCube = adjCubes6Conn[j];
					bool hasN = false;
					for (int k = 0; k < adjCube.size(); k++) {
						nx = wc.x + adjCube[k][0]; ny = wc.y + adjCube[k][1]; nz = wc.z + adjCube[k][2];
						if (nx >= 0 && ny >= 0 && nz >= 0 && nx < width && ny < height && nz < numSlices) {
							if (((int)nodes[Label(labels[nz][nx + (width*(ny))])].type) == 1 ||
								((int)nodes[Label(labels[nz][nx + (width*(ny))])].type) == 3 ||
								((int)nodes[Label(labels[nz][nx + (width*(ny))])].type) == 2
								) {
								hasN = true;
							}
						}
					}
					if (!hasN) {
						dC += 1;
					}
				}

				int dEuler = (dV - dE + dF - dC);
				int changeH1 = changeH0 + changeH2 - dEuler;
				if (changeH0 <= 0 && changeH2 <= 0 && changeH1 <= 0 && changeH0 + changeH2 + changeH1 <= 0) {
					generatorChanged = true;
					h0 = h0 + changeH0;
					bool relabeled = false;
					for (int j = 0; j < fgMask.size(); j++) {
						nx = wc.x + fgMask[j][0]; ny = wc.y + fgMask[j][1]; nz = wc.z + fgMask[j][2];
						if (nx >= 0 && ny >= 0 && nz >= 0 && nx < width && ny < height && nz < numSlices) {
							if (Label(labels[nz][nx + (width*(ny))]) != nodeIndex) { //edge within voxel
								if (((int)nodes[Label(labels[nz][nx + (width*(ny))])].type) == 0) {
									relabeled = true;
									labels[wc.z][wc.x + (width*(wc.y))] = labels[nz][nx + (width*(ny))];
								}

							}
						}
					}

					voxelsDeleted += 1;
					//voxel has now been relabeled to a fg component, create potential new edges for fg component
					for (int j = 0; j < structCross3D.size(); j++) {
						nx = wc.x + structCross3D[j][0]; ny = wc.y + structCross3D[j][1]; nz = wc.z + structCross3D[j][2];
						if (nx >= 0 && ny >= 0 && nz >= 0 && nx < width && ny < height && nz < numSlices) {
							if (Label(labels[wc.z][wc.x + (width*(wc.y))]) != Label(labels[nz][nx + (width*(ny))])) {
								if (((int)nodes[Label(labels[nz][nx + (width*(ny))])].type) == 0 ||
									((int)nodes[Label(labels[nz][nx + (width*(ny))])].type) == 2 || 
									((int)nodes[Label(labels[nz][nx + (width*(ny))])].type) == 3
									) {
									
									if (ccNeighbor(nodes, wc.x, wc.y, wc.z, nx, ny, nz, labels, width, height, numSlices, g_Image3D, g_Image3DF, S)) {
										if (edgeWt.find({ (int)Label(labels[wc.z][wc.x + (width*(wc.y))]), (int)Label(labels[nz][nx + (width*(ny))]) }) == edgeWt.end()) {
											add_edge(Label(labels[nz][nx + (width*(ny))]), Label(labels[wc.z][wc.x + (width*(wc.y))]), G);
											if (abs(nx - wc.x) + abs(ny - wc.y) + abs(nz - wc.z) == 1) {
												if (
													(((int)nodes[Label(labels[wc.z][wc.x + wc.y * width])].type) == CORE || ((int)nodes[Label(labels[wc.z][wc.x + wc.y * width])].type) == CUT || ((int)nodes[Label(labels[wc.z][wc.x + wc.y * width])].type) == FILL)
													&&
													(((int)nodes[Label(labels[nz][nx + ny * width])].type) == CORE || ((int)nodes[Label(labels[nz][nx + ny * width])].type) == CUT || ((int)nodes[Label(labels[nz][nx + ny * width])].type) == FILL)

													) {
													add_edge(Label(labels[wc.z][wc.x + wc.y * width]), Label(labels[nz][nx + ny * width]), fgGWithFills);

												}
												if (
													(((int)nodes[Label(labels[wc.z][wc.x + wc.y * width])].type) == CORE)
													&&
													(((int)nodes[Label(labels[nz][nx + ny * width])].type) == CORE)

													) {
													add_edge(Label(labels[wc.z][wc.x + wc.y * width]), Label(labels[nz][nx + ny * width]), coreG);

												}
											}
											if (
												(((int)nodes[Label(labels[wc.z][wc.x + wc.y * width])].type) == N || ((int)nodes[Label(labels[wc.z][wc.x + wc.y * width])].type) == CUT || ((int)nodes[Label(labels[wc.z][wc.x + wc.y * width])].type) == FILL)
												&&
												(((int)nodes[Label(labels[nz][nx + ny * width])].type) == N || ((int)nodes[Label(labels[nz][nx + ny * width])].type) == CUT || ((int)nodes[Label(labels[nz][nx + ny * width])].type) == FILL)

												) {
												add_edge(Label(labels[wc.z][wc.x + wc.y * width]), Label(labels[nz][nx + ny * width]), bgGWithCuts);

											}
											if (
												(((int)nodes[Label(labels[wc.z][wc.x + wc.y * width])].type) == N)
												&&
												(((int)nodes[Label(labels[nz][nx + ny * width])].type) == N)

												) {
												add_edge(Label(labels[wc.z][wc.x + wc.y * width]), Label(labels[nz][nx + ny * width]), nG);
											}
										}
									}

									if (abs(nx - wc.x) + abs(ny - wc.y) + abs(nz - wc.z) == 1) {
										edgeWt[{(int)Label(labels[wc.z][wc.x + (width*(wc.y))]), (int)Label(labels[nz][nx + (width*(ny))])}] = 1;
										edgeWt[{(int)Label(labels[nz][nx + (width*(ny))]), (int)Label(labels[wc.z][wc.x + (width*(wc.y))])}] = 1;
										if (edgeWt[{(int)Label(labels[wc.z][wc.x + (width * (wc.y))]), (int)Label(labels[nz][nx + (width * (ny))])}] == 0) {
											if (
												(((int)nodes[Label(labels[wc.z][wc.x + wc.y * width])].type) == CORE || ((int)nodes[Label(labels[wc.z][wc.x + wc.y * width])].type) == CUT || ((int)nodes[Label(labels[wc.z][wc.x + wc.y * width])].type) == FILL)
												&&
												(((int)nodes[Label(labels[nz][nx + ny * width])].type) == CORE || ((int)nodes[Label(labels[nz][nx + ny * width])].type) == CUT || ((int)nodes[Label(labels[nz][nx + ny * width])].type) == FILL)

												) {
												add_edge(Label(labels[wc.z][wc.x + wc.y * width]), Label(labels[nz][nx + ny * width]), fgGWithFills);

											}
										}
									}
									else {
										if (ccNeighbor(nodes, wc.x, wc.y, wc.z, nx, ny, nz, labels, width, height, numSlices, g_Image3D, g_Image3DF, S)) {
											if (edgeWt.find({ (int)Label(labels[wc.z][wc.x + (width*(wc.y))]), (int)Label(labels[nz][nx + (width*(ny))]) }) == edgeWt.end()) {
												edgeWt[{(int)Label(labels[wc.z][wc.x + (width*(wc.y))]), (int)Label(labels[nz][nx + (width*(ny))])}] = 0;
												edgeWt[{(int)Label(labels[nz][nx + (width*(ny))]), (int)Label(labels[wc.z][wc.x + (width*(wc.y))])}] = 0;
											}
										}
									}
								}

								if (abs(nx - wc.x) + abs(ny - wc.y) + abs(nz - wc.z) == 1) {
									if (((int)nodes[Label(labels[nz][nx + (width*(ny))])].type) == 0) {
										if (getParent(parentCompFg, nodes[Label(labels[wc.z][wc.x + (width*(wc.y))])].compIndex)
											!= getParent(parentCompFg, nodes[Label(labels[nz][nx + (width*(ny))])].compIndex)
											) {
											parentCompFg[getParent(parentCompFg, nodes[Label(labels[nz][nx + (width*(ny))])].compIndex)] = getParent(parentCompFg, nodes[Label(labels[wc.z][wc.x + (width*(wc.y))])].compIndex);
										}
									}
								}
							}

						}


					}

					queue<Coordinate> q;
					//Iteratively remove simple voxels
					for (int j = 0; j < structCube.size(); j++) {
						nx = wc.x + structCube[j][0]; ny = wc.y + structCube[j][1]; nz = wc.z + structCube[j][2];
						if (nx >= 0 && ny >= 0 && nz >= 0 && nx < width && ny < height && nz < numSlices) {
							if (Label(labels[nz][nx + (width*(ny))]) == nodeIndex) {
								q.push(Coordinate(nx, ny, nz));
							}
						}
					}
					map<Coordinate, int> genVisited;
					for (int j = 0; j < P.size(); j++) {

						if (Label(labels[P[j].z][P[j].x + (width*(P[j].y))]) == nodeIndex) {
							Coordinate pNow(P[j].x, P[j].y, P[j].z);
							genVisited[pNow] = 0;
						}
					}
					Coordinate wCoord(wc.x, wc.y, wc.z);
					genVisited[wCoord] = 1;
					while (!q.empty()) {
						Coordinate p = q.front();
						int px = p.x; int py = p.y; int pz = p.z;
						q.pop();
						genVisited[{px, py, pz}] = 1;

						if (simple3DLabelBg(labels, nodes, px, py, pz, numSlices, width,
							height, simpleDictionary3D, nodeIndex, true, bgConn)) {
							if (Label(labels[pz][px + (width*(py))]) == nodeIndex) {
								voxelsDeleted += 1;
								for (int j = 0; j < structCube.size(); j++) {
									nx = px + structCube[j][0]; ny = py + structCube[j][1]; nz = pz + structCube[j][2];
									if (nx >= 0 && ny >= 0 && nz >= 0 && nx < width && ny < height && nz < numSlices) {
										if (abs(nx - px) + abs(ny - py) + abs(nz - pz) == 1) {
											if (((int)nodes[Label(labels[nz][nx + (width*(ny))])].type) == 0) {
												labels[pz][px + (width*(py))] = labels[nz][nx + (width*(ny))];
												
												clear_vertex(vtxToGNode[p], miniBgG);
												miniBgValid[vtxToGNode[p]] = false;
												
												for (int a = 0; a < structCross3D.size(); a++) {
													int nxi = px + structCross3D[a][0];
													int nyi = py + structCross3D[a][1];
													int nzi = pz + structCross3D[a][2];
													if (nxi >= 0 && nyi >= 0 && nzi >= 0 && nxi < width && nyi < height && nzi < numSlices) {
														if (Label(labels[pz][px + (width*(py))]) != Label(labels[nzi][nxi + (width*(nyi))])) {
															if (((int)nodes[Label(labels[nzi][nxi + (width*(nyi))])].type) == 0 ||
																((int)nodes[Label(labels[nzi][nxi + (width*(nyi))])].type) == 2 ||
																((int)nodes[Label(labels[nzi][nxi + (width*(nyi))])].type) == 3
																) {
																if (ccNeighbor(nodes, px, py, pz, nxi, nyi, nzi, labels, width, height, numSlices, g_Image3D, g_Image3DF, S)) {
																	if (edgeWt.find({ (int)Label(labels[pz][px + (width*(py))]), (int)Label(labels[nzi][nxi + (width*(nyi))]) }) == edgeWt.end()) {
																		add_edge(Label(labels[nzi][nxi + (width*(nyi))]), Label(labels[pz][px + (width*(py))]), G);
																		if (abs(nxi - px) + abs(nyi - py) + abs(nzi - pz) == 1) {
																			if (
																				(((int)nodes[Label(labels[pz][px + py * width])].type) == CORE || ((int)nodes[Label(labels[pz][px + py * width])].type) == CUT || ((int)nodes[Label(labels[pz][px + py * width])].type) == FILL)
																				&&
																				(((int)nodes[Label(labels[nzi][nxi + nyi * width])].type) == CORE || ((int)nodes[Label(labels[nzi][nxi + nyi * width])].type) == CUT || ((int)nodes[Label(labels[nzi][nxi + nyi * width])].type) == FILL)

																				) {
																				add_edge(Label(labels[pz][px + py * width]), Label(labels[nzi][nxi + nyi * width]), fgGWithFills);

																			}
																			if (
																				(((int)nodes[Label(labels[pz][px + py * width])].type) == CORE)
																				&&
																				(((int)nodes[Label(labels[nzi][nxi + nyi * width])].type) == CORE)

																				) {
																				add_edge(Label(labels[pz][px + py * width]), Label(labels[nzi][nxi + nyi * width]), coreG);

																			}
																		}
																		if (
																			(((int)nodes[Label(labels[pz][px + py * width])].type) == N || ((int)nodes[Label(labels[pz][px + py * width])].type) == CUT || ((int)nodes[Label(labels[pz][px + py * width])].type) == FILL)
																			&&
																			(((int)nodes[Label(labels[nzi][nxi + nyi * width])].type) == N || ((int)nodes[Label(labels[nzi][nxi + nyi * width])].type) == CUT || ((int)nodes[Label(labels[nzi][nxi + nyi * width])].type) == FILL)

																			) {
																			add_edge(Label(labels[pz][px + py * width]), Label(labels[nzi][nxi + nyi * width]), bgGWithCuts);

																		}
																		if (
																			(((int)nodes[Label(labels[pz][px + py * width])].type) == N)
																			&&
																			(((int)nodes[Label(labels[nz][nx + ny * width])].type) == N)

																			) {
																			add_edge(Label(labels[pz][px + py * width]), Label(labels[nzi][nxi + nyi * width]), nG);

																		}

																		if (abs(nxi - px) + abs(nyi - py) + abs(nzi - pz) == 1) {
																			edgeWt[{(int)Label(labels[pz][px + (width*(py))]), (int)Label(labels[nzi][nxi + (width*(nyi))])}] = 1;
																			edgeWt[{(int)Label(labels[nzi][nxi + (width*(nyi))]), (int)Label(labels[pz][px + (width*(py))])}] = 1;
																		}
																		else {
																			if (edgeWt.find({ (int)Label(labels[pz][px + (width*(py))]), (int)Label(labels[nzi][nxi + (width*(nyi))]) }) == edgeWt.end()) {
																				edgeWt[{(int)Label(labels[pz][px + (width*(py))]), (int)Label(labels[nzi][nxi + (width*(nyi))])}] = 0;
																				edgeWt[{(int)Label(labels[nzi][nxi + (width*(nyi))]), (int)Label(labels[pz][px + (width*(py))])}] = 0;
																			}
																		}
																	}
																	else {
																		if (abs(nxi - px) + abs(nyi - py) + abs(nzi - pz) == 1) {
																			edgeWt[{(int)Label(labels[pz][px + (width*(py))]), (int)Label(labels[nzi][nxi + (width*(nyi))])}] = 1;
																			edgeWt[{(int)Label(labels[nzi][nxi + (width*(nyi))]), (int)Label(labels[pz][px + (width*(py))])}] = 1;
																			if (edgeWt[{(int)Label(labels[pz][px + (width * (py))]), (int)Label(labels[nz][nx + (width * (ny))])}] == 0) {
																				if (
																					(((int)nodes[Label(labels[pz][px + py * width])].type) == CORE || ((int)nodes[Label(labels[pz][px + py * width])].type) == CUT || ((int)nodes[Label(labels[pz][px + py * width])].type) == FILL)
																					&&
																					(((int)nodes[Label(labels[nz][nx + ny * width])].type) == CORE || ((int)nodes[Label(labels[nz][nx + ny * width])].type) == CUT || ((int)nodes[Label(labels[nz][nx + ny * width])].type) == FILL)

																					) {
																					add_edge(Label(labels[pz][px + py * width]), Label(labels[nz][nx + ny * width]), fgGWithFills);
																				}
																			}
																		}
																	}
																}


															}

															if (abs(nxi - px) + abs(nyi - py) + abs(nzi - pz) == 1) {
																if (((int)nodes[Label(labels[nzi][nxi + (width*(nyi))])].type) == 0) {
																	if (getParent(parentCompFg, nodes[Label(labels[pz][px + (width*(py))])].compIndex)
																		!= getParent(parentCompFg, nodes[Label(labels[nzi][nxi + (width*(nyi))])].compIndex)
																		) {
																		parentCompFg[getParent(parentCompFg, nodes[Label(labels[nzi][nxi + (width*(nyi))])].compIndex)] = getParent(parentCompFg, nodes[Label(labels[pz][px + (width*(py))])].compIndex);
																	}

																}
															}
														}
													}
												}

											}
										}
										if (Label(labels[nz][nx + (width*(ny))]) == nodeIndex) {
											Coordinate np(nx, ny, nz);
											if (genVisited[np] == 0) {
												q.push(np);
												genVisited[np] = 1;
											}
										}
									}
								}
							}
						}
						else {
							genVisited[p] = 0;
						}
						
					}
					
				}
				else {
					//if h0, h1, h2 total does not decrease, add back to graph
					Coordinate wCoord(wc.x, wc.y, wc.z);
					miniBgValid[vtxToGNode[wCoord]] = true;
					for (int j = 0; j < bgMask.size(); j++) {
						nx = wc.x + bgMask[j][0]; ny = wc.y + bgMask[j][1]; nz = wc.z + bgMask[j][2];
						if (nx >= 0 && ny >= 0 && nz >= 0 && nx < width && ny < height && nz < numSlices) {
							if (Label(labels[nz][nx + (width*(ny))]) == nodeIndex) {
								add_edge(vtxToGNode[Coordinate(nx, ny, nz)], vtxToGNode[wCoord], miniBgG);
							}
							else {
								if (((int)nodes[Label(labels[nz][nx + (width*(ny))])].type) == 1
									|| ((int)nodes[Label(labels[nz][nx + (width*(ny))])].type) == 3
									) {
									if (!boost::edge(vtxToGNode[wCoord], bgCompToMini[nodes[Label(labels[nz][nx + (width*(ny))])].overallCompIndexBg], miniBgG).second) {
										add_edge(vtxToGNode[wCoord], bgCompToMini[nodes[Label(labels[nz][nx + (width*(ny))])].overallCompIndexBg], miniBgG);
									}
								}

							}
						}
					}
				}
			}
		}

		if (voxelsDeleted >= P.size()) {
			nodes[nodeIndex].valid = false;
			auto neighbours = adjacent_vertices(nodeIndex, G);
			for (auto u : make_iterator_range(neighbours)) {
				edgeWt.erase({ nodeIndex, (int)u }); edgeWt.erase({ (int)u , nodeIndex });
			}
			clear_vertex(nodeIndex, G);
			for (int j = 0; j < P.size(); j++) {
				int px = P[j].x; int py = P[j].y; int pz = P[j].z;
				for (int k = 0; k < structCross3D.size(); k++) {
					int nx = px + structCross3D[k][0]; int ny = py + structCross3D[k][1]; int nz = pz + structCross3D[k][2];
					if (nx >= 0 && ny >= 0 && nz >= 0 && nx < width && ny < height && nz < numSlices) {
						if (Label(labels[pz][px + py * width]) != Label(labels[nz][nx + ny * width])) {
							if (((int)nodes[Label(labels[nz][nx + ny * width])].type) != N) {
								if (edgeWt.find({ (int)Label(labels[pz][px + py * width]) , (int)Label(labels[nz][nx + ny * width]) }) == edgeWt.end()) {
									add_edge(Label(labels[pz][px + py * width]), Label(labels[nz][nx + ny * width]), G);
									if (
										(((int)nodes[Label(labels[pz][px + py * width])].type) == CORE || ((int)nodes[Label(labels[pz][px + py * width])].type) == CUT || ((int)nodes[Label(labels[pz][px + py * width])].type) == FILL)
										&&
										(((int)nodes[Label(labels[nz][nx + ny * width])].type) == CORE || ((int)nodes[Label(labels[nz][nx + ny * width])].type) == CUT || ((int)nodes[Label(labels[nz][nx + ny * width])].type) == FILL)

										) {
										add_edge(Label(labels[pz][px + py * width]), Label(labels[nz][nx + ny * width]), fgGWithFills);

									}
									if (
										(((int)nodes[Label(labels[pz][px + py * width])].type) == CORE)
										&&
										(((int)nodes[Label(labels[nz][nx + ny * width])].type) == CORE)

										) {
										add_edge(Label(labels[pz][px + py * width]), Label(labels[nz][nx + ny * width]), coreG);

									}
									
									if (
										(((int)nodes[Label(labels[pz][px + py * width])].type) == N || ((int)nodes[Label(labels[pz][px + py * width])].type) == CUT || ((int)nodes[Label(labels[pz][px + py * width])].type) == FILL)
										&&
										(((int)nodes[Label(labels[nz][nx + ny * width])].type) == N || ((int)nodes[Label(labels[nz][nx + ny * width])].type) == CUT || ((int)nodes[Label(labels[nz][nx + ny * width])].type) == FILL)

										) {
										add_edge(Label(labels[pz][px + py * width]), Label(labels[nz][nx + ny * width]), bgGWithCuts);

									}
									if (
										(((int)nodes[Label(labels[pz][px + py * width])].type) == N)
										&&
										(((int)nodes[Label(labels[nz][nx + ny * width])].type) == N)

										) {
										add_edge(Label(labels[pz][px + py * width]), Label(labels[nz][nx + ny * width]), nG);

									}
								}
								edgeWt[{(int)Label(labels[pz][px + py * width]), (int)Label(labels[nz][nx + ny * width])}] = 1;
								edgeWt[{(int)Label(labels[nz][nx + ny * width]), (int)Label(labels[pz][px + py * width])}] = 1;
								if (edgeWt[{(int)Label(labels[pz][px + (width * (py))]), (int)Label(labels[nz][nx + (width * (ny))])}] == 0) {
									if (
										(((int)nodes[Label(labels[pz][px + py * width])].type) == CORE || ((int)nodes[Label(labels[pz][px + py * width])].type) == CUT || ((int)nodes[Label(labels[pz][px + py * width])].type) == FILL)
										&&
										(((int)nodes[Label(labels[nz][nx + ny * width])].type) == CORE || ((int)nodes[Label(labels[nz][nx + ny * width])].type) == CUT || ((int)nodes[Label(labels[nz][nx + ny * width])].type) == FILL)

										) {
										add_edge(Label(labels[pz][px + py * width]), Label(labels[nz][nx + ny * width]), fgGWithFills);
										if (
											(((int)nodes[Label(labels[pz][px + py * width])].type) == CORE)
											&&
											(((int)nodes[Label(labels[nz][nx + ny * width])].type) == CORE)

											) {
											add_edge(Label(labels[pz][px + py * width]), Label(labels[nz][nx + ny * width]), coreG);

										}
									}
								}
							}
						}
					}
				}
			}

		}

		else {

			if (voxelsDeleted > 0) {
				map< std::vector<int>, int> edgeWtTemp = edgeWt;
				auto neighbours = adjacent_vertices(nodeIndex, G);
				for (auto u : make_iterator_range(neighbours)) {
					edgeWtTemp.erase({ nodeIndex, (int)u }); edgeWtTemp.erase({ (int)u , nodeIndex });
				}
				clear_vertex(nodeIndex, G);
				map<Coordinate, bool> visited;
				for (int j = 0; j < P.size(); j++) {
					visited[Coordinate(P[j].x, P[j].y, P[j].z)] = false;
				}
				int currentLabelIndex = nodeIndex;
				auto t1 = std::chrono::high_resolution_clock::now();
				for (int j = 0; j < P.size(); j++) {
					Coordinate pNow(P[j].x, P[j].y, P[j].z);
					if (!visited[pNow]) {
						if (Label(labels[P[j].z][P[j].x + (width*(P[j].y))]) == nodeIndex) {
							node n; n.labelCost = 0; n.floatCost = 0.0;  n.type = 2; n.inFg = 1; n.index = currentLabelIndex; n.intensity = 0.0;
							queue<Coordinate> q;
							q.push(pNow);
							if (currentLabelIndex != nodeIndex) {
								add_vertex(G);// add_vertex(fgG);
								nodes.push_back(n);

							}
							else {
								nodes[currentLabelIndex] = n;
							}
							std::vector<float> diffs;
							while (!q.empty()) {
								Coordinate p = q.front();
								q.pop();
								int px = p.x; int py = p.y; int pz = p.z;
								visited[p] = true;
								if (g_Image3DF.size() > 0) {
									nodes[currentLabelIndex].floatCost += 1.0;
								}
								else {
									nodes[currentLabelIndex].floatCost += (gradient(px, py, pz, g_Image3D, width, height, numSlices));
								}

								uint32_t label32 = currentLabelIndex;
								changeLabel(label32, labels[pz][px + py * width]);
								for (int k = 0; k < structCube.size(); k++) {
									nx = px + structCube[k][0]; ny = py + structCube[k][1]; nz = pz + structCube[k][2];
									if (nx >= 0 && ny >= 0 && nz >= 0 && nx < width && ny < height && nz < numSlices) {
										if (Label(labels[pz][px + py * width]) != Label(labels[nz][nx + ny * width])) {
											if (nodeIndex != Label(labels[nz][nx + ny * width])) {
												if (abs(nx - px) + abs(ny - py) + abs(nz - pz) == 1) {

													if (((int)nodes[Label(labels[nz][nx + ny * width])].type) == CORE) {
														if (getParent(parentCompFg, nodes[Label(labels[pz][px + (width*(py))])].compIndex)
															!= getParent(parentCompFg, nodes[Label(labels[nz][nx + (width*(ny))])].compIndex)
															) {
															parentCompFg[getParent(parentCompFg, nodes[Label(labels[nz][nx + (width*(ny))])].compIndex)] = getParent(parentCompFg, nodes[Label(labels[pz][px + (width*(py))])].compIndex);


														}
													}

												}
												if (ccNeighbor(nodes, px, py, pz, nx, ny, nz, labels, width, height, numSlices, g_Image3D, g_Image3DF, S)) {
													if (edgeWtTemp.find({ (int)Label(labels[pz][px + py * width]),(int)Label(labels[nz][nx + ny * width]) }) == edgeWtTemp.end()) {
														add_edge(Label(labels[pz][px + py * width]), Label(labels[nz][nx + ny * width]), G);
														if (abs(nx - px) + abs(ny - py) + abs(nz - pz) == 1) {
															if (
																(((int)nodes[Label(labels[pz][px + py * width])].type) == CORE || ((int)nodes[Label(labels[pz][px + py * width])].type) == CUT || ((int)nodes[Label(labels[pz][px + py * width])].type) == FILL)
																&&
																(((int)nodes[Label(labels[nz][nx + ny * width])].type) == CORE || ((int)nodes[Label(labels[nz][nx + ny * width])].type) == CUT || ((int)nodes[Label(labels[nz][nx + ny * width])].type) == FILL)

																) {
																add_edge(Label(labels[pz][px + py * width]), Label(labels[nz][nx + ny * width]), fgGWithFills);

															}
															if (
																(((int)nodes[Label(labels[pz][px + py * width])].type) == CORE)
																&&
																(((int)nodes[Label(labels[nz][nx + ny * width])].type) == CORE)

																) {
																add_edge(Label(labels[pz][px + py * width]), Label(labels[nz][nx + ny * width]), coreG);

															}
														}
														
														if (
															(((int)nodes[Label(labels[pz][px + py * width])].type) == N || ((int)nodes[Label(labels[pz][px + py * width])].type) == CUT || ((int)nodes[Label(labels[pz][px + py * width])].type) == FILL)
															&&
															(((int)nodes[Label(labels[nz][nx + ny * width])].type) == N || ((int)nodes[Label(labels[nz][nx + ny * width])].type) == CUT || ((int)nodes[Label(labels[nz][nx + ny * width])].type) == FILL)

															) {
															add_edge(Label(labels[pz][px + py * width]), Label(labels[nz][nx + ny * width]), bgGWithCuts);

														}
														if (
															(((int)nodes[Label(labels[pz][px + py * width])].type) == N)
															&&
															(((int)nodes[Label(labels[nz][nx + ny * width])].type) == N)

															) {
															add_edge(Label(labels[pz][px + py * width]), Label(labels[nz][nx + ny * width]), nG);

														}
														//}
														if (abs(nx - px) + abs(ny - py) + abs(nz - pz) == 1) {
															edgeWtTemp[{(int)Label(labels[pz][px + py * width]), (int)Label(labels[nz][nx + ny * width])}] = 1;
															edgeWtTemp[{(int)Label(labels[nz][nx + ny * width]), (int)Label(labels[pz][px + py * width])}] = 1;
														}
														else {
															edgeWtTemp[{(int)Label(labels[pz][px + py * width]), (int)Label(labels[nz][nx + ny * width])}] = 0;
															edgeWtTemp[{(int)Label(labels[nz][nx + ny * width]), (int)Label(labels[pz][px + py * width])}] = 0;

														}
													}
													else {
														if (edgeWt[{(int)Label(labels[pz][px + (width * (py))]), (int)Label(labels[nz][nx + (width * (ny))])}] == 0) {
															if (
																(((int)nodes[Label(labels[pz][px + py * width])].type) == CORE || ((int)nodes[Label(labels[pz][px +py * width])].type) == CUT || ((int)nodes[Label(labels[pz][px + py * width])].type) == FILL)
																&&
																(((int)nodes[Label(labels[nz][nx + ny * width])].type) == CORE || ((int)nodes[Label(labels[nz][nx + ny * width])].type) == CUT || ((int)nodes[Label(labels[nz][nx + ny * width])].type) == FILL)

																) {
																add_edge(Label(labels[pz][px + py * width]), Label(labels[nz][nx + ny * width]), fgGWithFills);
																if (
																	(((int)nodes[Label(labels[pz][px + py * width])].type) == CORE)
																	&&
																	(((int)nodes[Label(labels[nz][nx + ny * width])].type) == CORE)

																	) {
																	add_edge(Label(labels[pz][px + py * width]), Label(labels[nz][nx + ny * width]), coreG);

																}
															}
														}

														if (abs(nx - px) + abs(ny - py) + abs(nz - pz) == 1) {
															edgeWtTemp[{(int)Label(labels[pz][px + py * width]), (int)Label(labels[nz][nx + ny * width])}] = 1;
															edgeWtTemp[{(int)Label(labels[nz][nx + ny * width]), (int)Label(labels[pz][px + py * width])}] = 1;
														}
													}
												}
												//}


											}
										}
										if (Label(labels[nz][nx + ny * width]) == nodeIndex) {
											Coordinate np(nx, ny, nz);
											if (!visited[np]) {
												if (ccNeighbor(nodes, px, py, pz, nx, ny, nz, labels, width, height, numSlices, g_Image3D, g_Image3DF, S)) {
													q.push(np);
													visited[np] = true;
												}
											}
										}
									}
								}
							}
							if (currentLabelIndex == nodeIndex) {
								currentLabelIndex = nodes.size();
							}
							else {
								currentLabelIndex += 1;
							}
						}
					}
				}
				edgeWt = edgeWtTemp;
				auto t2 = std::chrono::high_resolution_clock::now();
				std::chrono::duration<double> elapsed = t2 - t1;
				addETime += elapsed.count();

			}
		}

	}


}

int findComponents(Graph & G, std::vector<node> & nodes, int numNodes, bool inFg) {
	std::vector< std::vector<int> > components;
	std::vector<int> nodeToComp(numNodes);
	int n = (int)boost::connected_components(G, &nodeToComp[0]);
	int numComps = 0; std::vector<int> isCompIndex(n, -1);
	//If in fg, core. Else if in bg, neighborhood
	int type = 0;
	if (!inFg) {
		type = 1;
	}
	for (int i = 0; i < numNodes; i++) {
		if (nodes[i].valid) {
			if (((int)nodes[i].type) == type || ((int)nodes[i].type) == 2 || ((int)nodes[i].type) == 3) {
				if (isCompIndex[nodeToComp[i]] == -1) {

					isCompIndex[nodeToComp[i]] = numComps;
					std::vector<int> newComp = { i };
					components.push_back(newComp);
					numComps += 1;
				}
				else {
					components[isCompIndex[nodeToComp[i]]].push_back(i);
				}

			}
		}
	}
	int compsWithTerms = 0;
	for (int i = 0; i < components.size(); i++) {
		bool containsTerm = false;
		for (int j = 0; j < components[i].size(); j++) {
			if (((int)nodes[components[i][j]].type) == type) {
				containsTerm = true;
				break;
			}
		}
		if (containsTerm) {
			compsWithTerms += 1;
		}
	}
	return compsWithTerms;
}

int findComponentsVec(Graph & G, std::vector<node> & nodes, int numNodes, bool inFg, std::vector< std::vector<int> > & components) {
	std::vector<int> nodeToComp(numNodes);
	int n = (int)boost::connected_components(G, &nodeToComp[0]);
	int numComps = 0; std::vector<int> isCompIndex(n, -1);
	//If in fg, core. Else if in bg, neighborhood
	int type = 0;
	if (!inFg) {
		type = 1;
	}
	for (int i = 0; i < numNodes; i++) {
		if (nodes[i].valid) {
			if (((int)nodes[i].type) == type || ((int)nodes[i].type) == 2 || ((int)nodes[i].type) == 3) {
				if (isCompIndex[nodeToComp[i]] == -1) {

					isCompIndex[nodeToComp[i]] = numComps;
					std::vector<int> newComp = { i };
					components.push_back(newComp);
					numComps += 1;
				}
				else {
					components[isCompIndex[nodeToComp[i]]].push_back(i);
				}

			}
		}
	}
	int compsWithTerms = 0;
	for (int i = 0; i < components.size(); i++) {
		bool containsTerm = false;
		for (int j = 0; j < components[i].size(); j++) {
			if (((int)nodes[components[i][j]].type) == type) {
				containsTerm = true;
				break;
			}
		}
		if (containsTerm) {
			compsWithTerms += 1;
		}
	}
	return compsWithTerms;
}

struct genVoxels {
	std::vector<Coordinate> coords;
	node n;

};

struct CompareGen {
	bool operator()(genVoxels const& p1, genVoxels const& p2)
	{
		return abs(p1.n.labelCost) < abs(p2.n.labelCost);
	}
};

void simplifyGenerators(std::vector<uint32_t*> & labels, const std::vector<stbi_uc*>& g_Image3D, const std::vector< std::vector<float> >& g_Image3DF, int numSlices, int width, int height, float S,
	std::vector<node> & nodes, Graph & G, grapht & fgG, grapht & fgGWithFills, grapht & bgGWithCuts, grapht & coreG, grapht & bgG, grapht & nG, map< std::vector<int>, int> & edgeWt,
	int fgConn, const std::vector<unsigned char> & simpleDictionary3D, int genMode, bool rootMorpho) {
	auto before = std::chrono::high_resolution_clock::now();
	cout << "Begin DFS to find articulation nodes " << endl;

	map<int, map<int, int> > fgLocalArtConnectivity, bgLocalArtConnectivity;
	auto after = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double> elapsed = after - before;
	cout << "DFS time: " << elapsed.count() << endl;
	int n = nodes.size();
	int avgGenSize = 0;
	int numGens = 0;
	std::vector<bool> simplified(n, false); map<int, int> parentCompFg; map<int, int> parentCompBg;
	for (int i = 0; i < nodes.size(); i++) {
		if (nodes[i].type == CORE) {
			parentCompFg[nodes[i].compIndex] = nodes[i].compIndex;
			
		}
		if (nodes[i].type == N) {
			parentCompBg[nodes[i].compIndex] = nodes[i].compIndex;
		}
	}

	std::vector< genVoxels > generatorVoxels;
	priority_queue<genVoxels, std::vector<genVoxels>, CompareGen> genQ;
	for (int i = 0; i < width; i++) {
		for (int j = 0; j < height; j++) {
			for (int k = 0; k < numSlices; k++) {
				int l = Label(labels[k][i + j * width]);
				if (l < n) {
					if (((int)nodes[l].type) == 2 || ((int)nodes[l].type) == 3) {
						if (!simplified[l]) {
							std::vector<Coordinate> voxels;
							findGenVoxels(labels, voxels, Label(labels[k][i + j * width]), width, height, numSlices, i, j, k);
							genVoxels gen; gen.n = nodes[Label(labels[k][i + j * width])]; gen.coords = voxels;
							genQ.push(gen);

							generatorVoxels.push_back(gen);

							simplified[l] = true;
							voxels.clear();
						}
					}
				}
			}
		}
	}
	int numNodes = nodes.size();
	float timeT = 0.0;
	float minigraphT = 0.0; float addETime = 0.0; float otherTime = 0.0;
	std::cout << "Generators to simplify " << genQ.size() << std::endl;
	while (!genQ.empty()) {
		genVoxels gen = genQ.top();
		genQ.pop();
		if (nodes[gen.n.index].valid) {
			if (((int)nodes[gen.n.index].type) == 2 || ((int)nodes[gen.n.index].type) == 3) {
				std::vector<int> fNeighbors; std::vector<int> bNeighbors;
				auto neighbours = adjacent_vertices(gen.n.index, G);
				for (auto u : make_iterator_range(neighbours)) {
					if (nodes[u].valid) {
						if (((int)nodes[u].type) == 0 || ((int)nodes[u].type) == 2 || ((int)nodes[u].type) == 3) {
							if (edgeWt[{(int)gen.n.index, (int)u}] == 1) {
								fNeighbors.push_back((int)u);
							}
						}
						if (((int)nodes[u].type) == 1 || ((int)nodes[u].type) == 2 || ((int)nodes[u].type) == 3) {
							bNeighbors.push_back((int)u);
						}
					}
				}
					if (bNeighbors.size() == 0) {
						nodes[gen.n.index].valid = false;
						int nIndex = 0;
						for (int j = 0; j < fNeighbors.size(); j++) {
							if ((int)nodes[fNeighbors[j]].type == CORE) {
								nIndex = fNeighbors[j];
								edgeWt.erase({ gen.n.index, (int)fNeighbors[j] }); edgeWt.erase({ (int)fNeighbors[j] , gen.n.index });

							}
						}
						for (int p = 0; p < gen.coords.size(); p++) {
							labels[gen.coords[p].z][gen.coords[p].x + (width * (gen.coords[p].y))] = nIndex;
						}
						clear_vertex(gen.n.index, fgGWithFills); clear_vertex(gen.n.index, bgGWithCuts); clear_vertex(gen.n.index, G);

						continue;
					}
					if (fNeighbors.size() == 0) {
						nodes[gen.n.index].valid = false;
						int nIndex = 0;
						for (int j = 0; j < bNeighbors.size(); j++) {
							if ((int)nodes[bNeighbors[j]].type == N) {
								nIndex = bNeighbors[j];
								edgeWt.erase({ gen.n.index, (int)bNeighbors[j] }); edgeWt.erase({ (int)bNeighbors[j] , gen.n.index });

							}
						}
						for (int p = 0; p < gen.coords.size(); p++) {
							labels[gen.coords[p].z][gen.coords[p].x + (width * (gen.coords[p].y))] = nIndex;
						}
						clear_vertex(gen.n.index, fgGWithFills); clear_vertex(gen.n.index, bgGWithCuts); clear_vertex(gen.n.index, G);

						continue;
					}
					auto t1 = std::chrono::high_resolution_clock::now();
					if (((int)nodes[gen.n.index].type) == 2) {
						clear_vertex(gen.n.index, bgGWithCuts);
						for (int i = 0; i < nodes.size(); i++) {
							nodes[i].overallCompIndexBg = -1;
						}
						findOverallComponents(bgGWithCuts, nodes, 0);
					}
					else {
							clear_vertex(gen.n.index, fgGWithFills);
							for (int i = 0; i < nodes.size(); i++) {
								nodes[i].overallCompIndexFg = -1;
							}
							findOverallComponents(fgGWithFills, nodes, 1);
					}

				auto t2 = std::chrono::high_resolution_clock::now();
				
				
				
				std::chrono::duration<double> elapsed = t2 - t1;
				timeT += elapsed.count();
				
				for (int i = 0; i < nodes.size(); i++) {
					nodes[i].isArticulate = false;
					nodes[i].isNew = false;
					nodes[i].tin = 0;
					nodes[i].low = 0;
					nodes[i].compIndex = -1;
				}
				if (((int)nodes[gen.n.index].type) == CUT) {
					fgLocalArtConnectivity = findTermComponents(coreG, nodes, CORE);//fgG
				}
				else {
					bgLocalArtConnectivity = findTermComponents(nG, nodes, N); //bgG
				}
				for (int i = 0; i < nodes.size(); i++) {
					if (nodes[i].type == CORE) {
						parentCompFg[nodes[i].compIndex] = nodes[i].compIndex;

					}
					if (nodes[i].type == N) {
						parentCompBg[nodes[i].compIndex] = nodes[i].compIndex;
					}
				}
				int numNodesInit = nodes.size(); bool generatorChanged = false; //fgG, bgG, 
				if (rootMorpho) {
					simplifyGeneratorRoots(G, labels, g_Image3D, g_Image3DF, fgLocalArtConnectivity, bgLocalArtConnectivity, numSlices, width, height, fgConn, nodes, gen.n.index, edgeWt, S, simpleDictionary3D, gen.coords, genMode,
						parentCompFg, parentCompBg, generatorChanged, fgGWithFills, bgGWithCuts, coreG, nG, minigraphT, addETime, otherTime);
				}
				else {
					simplifyGenerator(G, labels, g_Image3D, g_Image3DF, fgLocalArtConnectivity, bgLocalArtConnectivity, numSlices, width, height, fgConn, nodes, gen.n.index, edgeWt, S, simpleDictionary3D, gen.coords, genMode,
						parentCompFg, parentCompBg, generatorChanged, fgGWithFills, bgGWithCuts, coreG, nG, minigraphT, addETime, otherTime);
				}
				map< std::vector<int>, int> newEdgeWt;
				if (nodes[gen.n.index].valid) {
					for (int i = 0; i < fNeighbors.size(); i++) {
						if (edgeWt.find({ gen.n.index, fNeighbors[i] }) != edgeWt.end()) {
							if (edgeWt[{ gen.n.index, fNeighbors[i] }] == 1) {
								add_edge(gen.n.index, fNeighbors[i], fgGWithFills);
							}
						}
					}
					for (int i = 0; i < bNeighbors.size(); i++) {
						if (edgeWt.find({ gen.n.index, bNeighbors[i] }) != edgeWt.end()) {
							add_edge(gen.n.index, bNeighbors[i], bgGWithCuts);
						}
					}
				}
				
			}
		}
	}
}

std::vector<int> getEulerNumbers(std::vector<node> & nodes, std::vector<uint32_t *> & labels, int width, int height, int numSlices) {
	int v = 0; int e = 0; int f = 0; int c = 0;
	for (int i = 0; i < width; i++) {
		for (int j = 0; j < height; j++) {
			for (int k = 0; k < numSlices; k++) {
				//add up vertices
				int l = Label(labels[k][i + (width*j)]);
				if ((int)nodes[l].type == 1) { continue; }
				switch ((int)nodes[l].type) {
				case 0:
					v += 1;
					break;
				case 2:
					v += 1;
					nodes[l].v += 1; //add to generator count
					break;
				case 3:
					//Fill, so dont add to fg v count
					nodes[l].v += 1; //add to generator count
					break;
				default:
					break;

				}

				//add up edges in x direction
				if (i + 1 < width) {
					//compare (i,j,k), (i+1, j, k)
					int l2 = Label(labels[k][(i + 1) + (width*j)]);
					//both vts have to not be in neighborhood
					if (((int)nodes[l].type) != 1 && ((int)nodes[l2].type) != 1) {
						//0 is core, 1 is cut, 2 is fill. Priority is fill because 6-conn, so should take max
						int maxNode = l;
						if (((int)nodes[l2].type) > ((int)nodes[l].type)) {
							maxNode = l2;
						}
						switch ((int)nodes[maxNode].type) {
						case 0:
							e += 1;
							break;
						case 2:
							e += 1;
							nodes[maxNode].e += 1; //add to generator count
							break;
						case 3:
							//Fill, so dont add to fg e count
							nodes[maxNode].e += 1; //add to generator count
							break;
						default:
							break;

						}

					}
				}

				//add up edges in y direction
				if (j + 1 < height) {
					//compare (i,j,k), (i, j+1, k)
					int l2 = Label(labels[k][i + (width*(j + 1))]);
					//both vts have to not be in neighborhood
					if (((int)nodes[l].type) != 1 && ((int)nodes[l2].type) != 1) {
						//0 is core, 1 is cut, 2 is fill. Priority is fill because 6-conn, so should take max
						int maxNode = l;
						if (((int)nodes[l2].type) > ((int)nodes[l].type)) {
							maxNode = l2;
						}
						switch ((int)nodes[maxNode].type) {
						case 0:
							e += 1;
							break;
						case 2:
							e += 1;
							nodes[maxNode].e += 1; //add to generator count
							break;
						case 3:
							//Fill, so dont add to fg e count
							nodes[maxNode].e += 1; //add to generator count
							break;
						default:
							break;

						}

					}
				}

				//add up edges in z direction
				if (k + 1 < numSlices) {
					//compare (i,j,k), (i, j+1, k)
					int l2 = Label(labels[k + 1][i + (width*j)]);
					//both vts have to not be in neighborhood
					if (((int)nodes[l].type) != 1 && ((int)nodes[l2].type) != 1) {
						//0 is core, 1 is cut, 2 is fill. Priority is fill because 6-conn, so should take max
						int maxNode = l;
						if (((int)nodes[l2].type) > ((int)nodes[l].type)) {
							maxNode = l2;
						}
						switch ((int)nodes[maxNode].type) {
						case 0:
							e += 1;
							break;
						case 2:
							e += 1;
							nodes[maxNode].e += 1; //add to generator count
							break;
						case 3:
							//Fill, so dont add to fg e count
							nodes[maxNode].e += 1; //add to generator count
							break;
						default:
							break;

						}

					}
				}

				//add up faces in xy plane
				if (i + 1 < width && j + 1 < height) {
					//compare (i,j,k), (i, j+1, k)
					int l2 = Label(labels[k][(i + 1) + (width*(j))]);
					int l3 = Label(labels[k][(i)+(width*(j + 1))]);
					int l4 = Label(labels[k][(i + 1) + (width*(j + 1))]);
					//both vts have to not be in neighborhood
					if (((int)nodes[l].type) != 1 && ((int)nodes[l2].type) != 1 && ((int)nodes[l3].type) != 1 && ((int)nodes[l4].type) != 1) {
						//0 is core, 1 is cut, 2 is fill. Priority is fill because 6-conn, so should take max
						int maxNode = l;
						if (((int)nodes[l2].type) > ((int)nodes[maxNode].type)) {
							maxNode = l2;
						}
						if (((int)nodes[l3].type) > ((int)nodes[maxNode].type)) {
							maxNode = l3;
						}
						if (((int)nodes[l4].type) > ((int)nodes[maxNode].type)) {
							maxNode = l4;
						}
						switch ((int)nodes[maxNode].type) {
						case 0:
							f += 1;
							break;
						case 2:
							f += 1;
							nodes[maxNode].f += 1; //add to generator count
							break;
						case 3:
							//Fill, so dont add to fg e count
							nodes[maxNode].f += 1; //add to generator count

							break;
						default:
							break;

						}

					}
				}

				//add up faces in xz plane
				if (i + 1 < width && k + 1 < numSlices) {
					//compare (i,j,k), (i, j+1, k)
					int l2 = Label(labels[k][(i + 1) + (width*(j))]);
					int l3 = Label(labels[k + 1][(i)+(width*(j))]);
					int l4 = Label(labels[k + 1][(i + 1) + (width*(j))]);
					//both vts have to not be in neighborhood
					if (((int)nodes[l].type) != 1 && ((int)nodes[l2].type) != 1 && ((int)nodes[l3].type) != 1 && ((int)nodes[l4].type) != 1) {						  //0 is core, 1 is cut, 2 is fill. Priority is fill because 6-conn, so should take max
						int maxNode = l;
						if (((int)nodes[l2].type) > ((int)nodes[maxNode].type)) {
							maxNode = l2;
						}
						if (((int)nodes[l3].type) > ((int)nodes[maxNode].type)) {
							maxNode = l3;
						}
						if (((int)nodes[l4].type) > ((int)nodes[maxNode].type)) {
							maxNode = l4;
						}
						switch ((int)nodes[maxNode].type) {
						case 0:
							f += 1;
							break;
						case 2:
							f += 1;
							nodes[maxNode].f += 1; //add to generator count
							break;
						case 3:

							//Fill, so dont add to fg e count
							nodes[maxNode].f += 1; //add to generator count
							break;
						default:
							break;

						}

					}
				}

				//add up faces in yz plane
				if (j + 1 < height && k + 1 < numSlices) {
					//compare (i,j,k), (i, j+1, k)
					int l2 = Label(labels[k][(i)+(width*(j + 1))]);
					int l3 = Label(labels[k + 1][(i)+(width*(j))]);
					int l4 = Label(labels[k + 1][(i)+(width*(j + 1))]);
					//both vts have to not be in neighborhood
					if (((int)nodes[l].type) != 1 && ((int)nodes[l2].type) != 1 && ((int)nodes[l3].type) != 1 && ((int)nodes[l4].type) != 1) {						  //0 is core, 1 is cut, 2 is fill. Priority is fill because 6-conn, so should take max
						int maxNode = l;
						if (((int)nodes[l2].type) > ((int)nodes[maxNode].type)) {
							maxNode = l2;
						}
						if (((int)nodes[l3].type) > ((int)nodes[maxNode].type)) {
							maxNode = l3;
						}
						if (((int)nodes[l4].type) > ((int)nodes[maxNode].type)) {
							maxNode = l4;
						}
						switch ((int)nodes[maxNode].type) {
						case 0:
							f += 1;
							break;
						case 2:
							f += 1;
							nodes[maxNode].f += 1; //add to generator count
							break;
						case 3:
							//Fill, so dont add to fg e count
							nodes[maxNode].f += 1; //add to generator count

							break;
						default:
							break;

						}

					}
				}

				//Add up cubes
				if (i + 1 < width && j + 1 < height && k + 1 < numSlices) {
					bool hasCube = true;
					int maxNode = l;
					for (int o = 0; o < cubeFrontMask.size(); o++) {
						int coord[3] = { i + cubeFrontMask[o][0], j + cubeFrontMask[o][1], k + cubeFrontMask[o][2] };
						if (((int)nodes[Label(labels[coord[2]][(coord[0]) + (width*(coord[1]))])].type) == 1) {

							hasCube = false;
							break;
						}
						if (((int)nodes[Label(labels[coord[2]][(coord[0]) + (width*(coord[1]))])].type) >
							((int)nodes[maxNode].type)
							) {
							maxNode = Label(labels[coord[2]][(coord[0]) + (width*(coord[1]))]);
						}
					}
					if (hasCube) {
						switch ((int)nodes[maxNode].type) {
						case 0:
							c += 1;
							break;
						case 2:
							c += 1;
							nodes[maxNode].c += 1; //add to generator count
							break;
						case 3:
							//Fill, so dont add to fg e count
							nodes[maxNode].c += 1; //add to generator count
							break;
						default:
							break;
						}
					}
				}

			}
		}
	}
	return { v,e,f,c };
}

std::vector< std::vector<int> > findGraphComponents(grapht & g, std::vector<node> & nodes, bool inFg) {

	int numNodes = nodes.size();
	std::vector<int> nodeToComp(numNodes);
	int n = (int)boost::connected_components(g, &nodeToComp[0]);
	std::vector< std::vector<int> > wholeComps(n);
	std::vector<bool> isCompIndex(n, false);
	for (int i = 0; i < nodeToComp.size(); i++) {
		if (nodes[i].valid) {
			wholeComps[nodeToComp[i]].push_back(i);
			if (!isCompIndex[nodeToComp[i]]) {
				isCompIndex[nodeToComp[i]] = true;
			}
		}
	}

	std::vector< std::vector<int> > comps;
	for (int i = 0; i < wholeComps.size(); i++) {
		std::vector<int> comp = wholeComps[i];
		if (comp.size() == 0) {
			continue;
		}
		bool addComp = true;
		for (int j = 0; j < comp.size(); j++) {
			if (nodes[comp[j]].valid) {
				if (((int)nodes[comp[j]].type) == 1 && inFg) {
					addComp = false;
				}
				if (((int)nodes[comp[j]].type) == 0 && !inFg) {
					addComp = false;
				}
			}
			else {
				addComp = false;
			}
		}
		if (addComp) {
			comps.push_back(comp);
		}
	}
	return comps;
}

void pruneIsolatedNonTerminalGroups(grapht & fgG, grapht & bgG, std::vector<node> & nodes, std::vector<bool> & isAffectedComp, std::vector<int> & affectedComponents, map<int, int> & nodeToComp, bool trackComponents) {
	std::vector< std::vector<int> > fgComps = findGraphComponents(fgG, nodes, true); std::vector< std::vector<int> > bgComps = findGraphComponents(bgG, nodes, false);
	for (int i = 0; i < fgComps.size(); i++) {
		bool hasCore = false;
		std::vector<int> comp = fgComps[i];
		for (int j = 0; j < comp.size(); j++) {
			if (((int)nodes[comp[j]].type) == 0) {
				hasCore = true;
				break;
			}
		}
		if (!hasCore) {
			for (int j = 0; j < comp.size(); j++) {
				nodes[comp[j]].type = 1;
				nodes[comp[j]].inFg = 0;
				if (trackComponents) {
					isAffectedComp[nodeToComp[comp[j]]] = true; affectedComponents.push_back(nodeToComp[comp[j]]);
				}
			}
		}
	}
	for (int i = 0; i < bgComps.size(); i++) {
		bool hasN = false;
		std::vector<int> comp = bgComps[i];
		for (int j = 0; j < comp.size(); j++) {
			if (((int)nodes[comp[j]].type) == 1) {
				hasN = true;
				break;
			}
		}
		if (!hasN) {
			for (int j = 0; j < comp.size(); j++) {
				nodes[comp[j]].type = 0;
				nodes[comp[j]].inFg = 1;
				if (trackComponents) {
					isAffectedComp[nodeToComp[comp[j]]] = true; affectedComponents.push_back(nodeToComp[comp[j]]);
				}
			}
		}
	}
}

void updateEdges(Graph & G, grapht & g, std::vector<node> & nodes, std::map< std::vector<int>, int > & edgeWtMap, bool inFg) {
	g = grapht();
	for (int i = 0; i < nodes.size(); i++) {
		add_vertex(g);
	}
	typedef boost::graph_traits<Graph>::edge_iterator edge_iter;
	edge_iter ei, ei_end;
	for (tie(ei, ei_end) = edges(G); ei != ei_end; ++ei) {
		int v1 = (int)ei->m_source; int v2 = (int)ei->m_target;
		if (inFg) {
			if ((int)nodes[v1].type == 1 || (int)nodes[v2].type == 1) {
				continue;
			}
			if (edgeWtMap[{v1, v2}] == 1) {
				add_edge(v1, v2, 0, g);
			}
		}
		else {
			if ((int)nodes[v1].type == 0 || (int)nodes[v2].type == 0) {
				continue;
			}
			add_edge(v1, v2, 0, g);
		}
	}
}

void removeCAndNEdges(Graph & g, std::vector<node> & nodes) {
	Graph gN;
	for (int i = 0; i < nodes.size(); i++) {
		add_vertex(gN);
	}
	typedef boost::graph_traits<Graph>::edge_iterator edge_iter;
	edge_iter ei, ei_end;
	for (tie(ei, ei_end) = edges(g); ei != ei_end; ++ei) {
		int v1 = (int)ei->m_source; int v2 = (int)ei->m_target;
		if ((int)nodes[v1].type == 0 && (int)nodes[v2].type == 1) {
			continue;
		}
		if ((int)nodes[v1].type == 1 && (int)nodes[v2].type == 0) {
			continue;
		}

		add_edge(v1, v2, gN);
	}
	g = gN;
}

std::vector<int> findCriticalArticulationNodes(grapht & g, std::vector<node> & nodes) {
	std::vector<vertex_t> art_points;
	boost::articulation_points(g, std::back_inserter(art_points));
	std::vector<int> isArticulation(nodes.size(), 0);
	for (int i = 0; i < art_points.size(); i++) {
		isArticulation[(int)art_points[i]] = 1;
	}
	
	std::vector<int> isArt(nodes.size(), 0);

	for (int i = 0; i < art_points.size(); i++) {
		int av = (int)art_points[i];
		std::vector<int> adjNodes;
		auto neighbours = adjacent_vertices(av, g);
		std::vector<bool> visited(nodes.size(), false);
		visited[av] = true;
		int termCt = 0;
		for (auto u : make_iterator_range(neighbours)) {
			//Each neighbour of articulation point forms potential new component
			if (!visited[u]) {
				bool hasTerm = false;
				queue<int> q;
				q.push(u); visited[u] = true;
				while (!q.empty()) {
					int p = q.front();
					q.pop();
					if (((int)nodes[p].type) == 0 || ((int)nodes[p].type) == 1) {
						hasTerm = true;
					}
					auto neighboursp = adjacent_vertices(p, g);
					for (auto up : make_iterator_range(neighboursp)) {
						if (!visited[up]) {
							q.push(up);
							visited[up] = true;
						}
					}
				}
				if (hasTerm) {
					termCt += 1;
				}
			}
		}
		if (termCt > 1) {
			isArt[av] = termCt;
		}
	}
	return isArt;
}

int assignArticulationNodes(grapht & fgA, grapht & bgA, std::vector<node> & nodes, std::vector<bool> & isAffectedComp, std::vector<int> & affectedComponents, map<int, int> & nodeToComp, bool trackComponents, Graph & G) {
	std::vector<int> isArtFg = findCriticalArticulationNodes(fgA, nodes); std::vector<int> isArtBg = findCriticalArticulationNodes(bgA, nodes);

	int artNodes = 0;
	int64_t maxIRange = 0;
	for (int i = 0; i < nodes.size(); i++) {
		if (nodes[i].valid) {
			if ((int)nodes[i].type == 2 || (int)nodes[i].type == 3) {
				maxIRange += abs(nodes[i].labelCost);
			}
		}
	}
	
	maxIRange *= 2.0;
	maxIRange = max((int64_t)1, maxIRange);
	for (int i = 0; i < nodes.size(); i++) {
		if (nodes[i].valid) {
			if ((int)nodes[i].type == 2 || (int)nodes[i].type == 3) {
				if (isArtFg[i] > 1 || isArtBg[i] > 1) {
					artNodes += 1;
					if (isArtFg[i] > 1 && isArtBg[i] == 0) {
						if ((int)nodes[i].type == 3) {
							//If fill, assign all neighbouring cuts to be fg
							auto neighbours = adjacent_vertices(i, G);
							for (auto vd : make_iterator_range(neighbours)) {
								if ((int)nodes[vd].type == 2) {
									nodes[vd].type = 0;
									nodes[vd].inFg = 1;
									if (trackComponents) {
										isAffectedComp[nodeToComp[vd]] = true; affectedComponents.push_back(nodeToComp[vd]);
									}
								}
							}
						}

						//assign as fg terminal
						nodes[i].type = 0;
						nodes[i].inFg = 1;
						if (trackComponents) {
							isAffectedComp[nodeToComp[i]] = true; affectedComponents.push_back(nodeToComp[i]);
						}
					}
					if (isArtFg[i] == 0 && isArtBg[i] > 1) {
						if ((int)nodes[i].type == 2) {
							auto neighbours = adjacent_vertices(i, G);
							for (auto vd : make_iterator_range(neighbours)) {
								if ((int)nodes[vd].type == 3) {
									nodes[vd].type = 1;
									nodes[vd].inFg = 0;
									if (trackComponents) {
										isAffectedComp[nodeToComp[vd]] = true; affectedComponents.push_back(nodeToComp[vd]);
									}
								}
							}
						}
						nodes[i].type = 1;
						nodes[i].inFg = 0;
						//If cut, assign all neighbouring fills to be bg
						if (trackComponents) {
							isAffectedComp[nodeToComp[i]] = true; affectedComponents.push_back(nodeToComp[i]);
						}
					}
					if (isArtFg[i] > 1 && isArtBg[i] > 1) {
						int64_t lexCost = (maxIRange * (isArtFg[i] - isArtBg[i])) + nodes[i].labelCost;
						if (lexCost > 0) { //separates more fg than bg comps
							if ((int)nodes[i].type == 3) {
								//If fill, assign all neighbouring cuts to be fg
								auto neighbours = adjacent_vertices(i, G);
								for (auto vd : make_iterator_range(neighbours)) {
									if ((int)nodes[vd].type == 2) {
										nodes[vd].type = 0;
										nodes[vd].inFg = 1;
										if (trackComponents) {
											isAffectedComp[nodeToComp[vd]] = true; affectedComponents.push_back(nodeToComp[vd]);
										}
									}
								}
							}
							nodes[i].type = 0;
							nodes[i].inFg = 1;
							//If fill, assign all neighbouring cuts to be fg
							if (trackComponents) {
								isAffectedComp[nodeToComp[i]] = true; affectedComponents.push_back(nodeToComp[i]);
							}
						}
						else {
							if ((int)nodes[i].type == 2) {
								auto neighbours = adjacent_vertices(i, G);
								for (auto vd : make_iterator_range(neighbours)) {
									if ((int)nodes[vd].type == 3) {
										nodes[vd].type = 1;
										nodes[vd].inFg = 0;
										if (trackComponents) {
											isAffectedComp[nodeToComp[vd]] = true; affectedComponents.push_back(nodeToComp[vd]);
										}
									}
								}
							}
							nodes[i].type = 1;
							nodes[i].inFg = 0;
							//If cut, assign all neighbouring fills to be bg
							if (trackComponents) {
								isAffectedComp[nodeToComp[i]] = true; affectedComponents.push_back(nodeToComp[i]);
							}
						}
					}
				}
			}
		}
	}
	return artNodes;
}

void preprocessGraph(Graph & G, std::vector<node> & nodes, std::map< std::vector<int>, int> & edgeWtMap, std::vector<bool> & isAffectedComp, std::vector<int> & affectedComponents, map<int, int> & nodeToComp, bool trackComponents) {
	int numArt = 100000;
	grapht fgGA, bgGA;
	for (int i = 0; i < nodes.size(); i++) {
		add_vertex(fgGA); add_vertex(bgGA);
	}
	typedef boost::graph_traits<Graph>::edge_iterator edge_iter;
	edge_iter ei, ei_end;
	for (tie(ei, ei_end) = edges(G); ei != ei_end; ++ei) {
		int v1 = (int)ei->m_source; int v2 = (int)ei->m_target;
		if ((((int)nodes[v1].type) == 0 || ((int)nodes[v1].type) == 2 || ((int)nodes[v1].type) == 3) &&
			(((int)nodes[v2].type) == 0 || ((int)nodes[v2].type) == 2 || ((int)nodes[v2].type) == 3) &&
			(nodes[v1].valid) && (nodes[v2].valid) && edgeWtMap[{v1, v2}] == 1) {
			//If src vtx or tgt vtx is core, cut, or fill, add to fg graph

			add_edge(v1, v2, 0, fgGA);
		}

		if ((((int)nodes[v1].type) == 1 || ((int)nodes[v1].type) == 2 || ((int)nodes[v1].type) == 3) &&
			(((int)nodes[v2].type) == 1 || ((int)nodes[v2].type) == 2 || ((int)nodes[v2].type) == 3) &&
			(nodes[v1].valid) && (nodes[v2].valid)) {
			//If src vtx or tgt vtx is core, cut, or fill, add to fg graph 
				add_edge(v1, v2, 0, bgGA);
		}
	}

	while (numArt > 0) {
		pruneIsolatedNonTerminalGroups(fgGA, bgGA, nodes, isAffectedComp, affectedComponents, nodeToComp, trackComponents);

		updateEdges(G, fgGA, nodes, edgeWtMap, true); updateEdges(G, bgGA, nodes, edgeWtMap, false);

		numArt = assignArticulationNodes(fgGA, bgGA, nodes, isAffectedComp, affectedComponents, nodeToComp, trackComponents, G);

		updateEdges(G, fgGA, nodes, edgeWtMap, true); updateEdges(G, bgGA, nodes, edgeWtMap, false);
	}
	removeCAndNEdges(G, nodes);
}

void reindexGraph(std::vector<node> & nodes, Graph & G, map<int, int> & oldToNewIndex, map<int, int> & newToOldIndex, map< std::vector<int>, int> & edgeWt) {
	int index = 0; std::vector<node> newNodes; Graph newG; map< std::vector<int>, int> newEdgeWt;
	for (int i = 0; i < nodes.size(); i++) {
		if (nodes[i].valid) {
			node n = nodes[i];
			n.index = newNodes.size(); n.intensity = nodes[i].intensity;
			oldToNewIndex[i] = newNodes.size(); newToOldIndex[newNodes.size()] = i;
			newNodes.push_back(n);
			add_vertex(newG);
		}
	}

	typedef boost::graph_traits<Graph>::edge_iterator edge_iter;
	edge_iter ei, ei_end;
	for (tie(ei, ei_end) = edges(G); ei != ei_end; ++ei) {
		int v1 = (int)ei->m_source; int v2 = (int)ei->m_target;
		if (nodes[v1].valid && nodes[v2].valid) {
			add_edge(oldToNewIndex[v1], oldToNewIndex[v2], newG);
			newEdgeWt[{oldToNewIndex[v1], oldToNewIndex[v2]}] = edgeWt[{v1, v2}];
			newEdgeWt[{oldToNewIndex[v2], oldToNewIndex[v1]}] = newEdgeWt[{oldToNewIndex[v1], oldToNewIndex[v2]}];
		}
	}
	nodes = newNodes;
	G = newG;
	edgeWt = newEdgeWt;
}

std::vector< std::vector<int>> getTypeComponents(grapht & g, std::vector<node> & nodes, int nodesSize, int type) {
	std::vector< std::vector<int> > typeComponents;
	std::vector<int> nodeToComp(nodesSize);
	int n = (int)boost::connected_components(g, &nodeToComp[0]);
	int numComps = 0; std::vector<int> isCompIndex(n, -1);
	for (int i = 0; i < nodesSize; i++) {
		
		if (((int)nodes[i].type) == type) {
			if (isCompIndex[nodeToComp[i]] == -1) {
				
				isCompIndex[nodeToComp[i]] = numComps;
				std::vector<int> newComp = {i};
				typeComponents.push_back(newComp);
				numComps += 1;
			}
			else {
				typeComponents[isCompIndex[nodeToComp[i]]].push_back(i);
			}

		}
	}
	return typeComponents;
}

std::vector<std::vector<int> > mergeAdjacentTerminals(Graph & G, std::vector<node> & nodes, map< std::vector<int>, int> & edgeWt, map<int, int> & oldToNew) {
	std::vector<node> newNodes;
	grapht coreG; grapht neighborhoodG;
	int nodesSize = nodes.size();
	for (int i = 0; i < nodesSize; i++) {
		add_vertex(coreG); add_vertex(neighborhoodG);
	}
	typedef boost::graph_traits<Graph>::edge_iterator edge_iter;
	edge_iter ei, ei_end;
	for (tie(ei, ei_end) = edges(G); ei != ei_end; ++ei) {
		int v1 = (int)ei->m_source; int v2 = (int)ei->m_target;
		if (((int)nodes[v1].type == 0) && nodes[v1].valid && ((int)nodes[v2].type == 0) && nodes[v2].valid && edgeWt[{v1,v2}] == 1) {
			add_edge(v1, v2, 0, coreG);
		}
		if (((int)nodes[v1].type == 1) && nodes[v1].valid && ((int)nodes[v2].type == 1) && nodes[v2].valid) {
			add_edge(v1, v2, 0, neighborhoodG);
		}
	}
	std::vector< std::vector<int>> coreComponents = getTypeComponents(coreG, nodes, nodesSize, 0); std::vector< std::vector<int>> neighborhoodComponents = getTypeComponents(neighborhoodG, nodes, nodesSize, 1);
	std::vector<std::vector<int> > newToOldComp; 
	for (int i = 0; i < coreComponents.size(); i++) {
		newToOldComp.push_back(coreComponents[i]);
		node n; n.type = 0; n.index = newNodes.size(); n.inFg = 1; newNodes.push_back(n);
		for (int j = 0; j < coreComponents[i].size(); j++) {
			oldToNew[coreComponents[i][j]] = newNodes.size() - 1;
		}
	}
	for (int i = 0; i < neighborhoodComponents.size(); i++) {
		newToOldComp.push_back(neighborhoodComponents[i]);
		node n; n.type = 1; n.index = newNodes.size(); n.inFg = 0; newNodes.push_back(n);
		for (int j = 0; j < neighborhoodComponents[i].size(); j++) {
			oldToNew[neighborhoodComponents[i][j]] = newNodes.size() - 1;
		}
	}
	for (int i = 0; i < nodes.size(); i++) {
		if (((int)nodes[i].type) == 2 ) {
			newToOldComp.push_back({ i });
			node n =nodes[i]; n.index = newNodes.size();  n.type = 2; n.inFg = 1; n.labelCost = nodes[i].labelCost; n.intensity = nodes[i].intensity;
			newNodes.push_back(n);
			oldToNew[i] = newNodes.size() - 1;
		}
		else {
			if (((int)nodes[i].type) == 3) {
				newToOldComp.push_back({ i });
				//node n = nodes[i];
				node n = nodes[i]; n.index = newNodes.size();
				n.type = 3; n.inFg = 0; n.index = newNodes.size(); n.labelCost = nodes[i].labelCost; n.intensity = nodes[i].intensity;
				newNodes.push_back(n);
				oldToNew[i] = newNodes.size() - 1;
			}
		}
	}
	map< std::vector<int>, int > newEdgeWt;
	Graph newG;
	for (int i = 0; i < newNodes.size(); i++) {
		add_vertex(newG);
	}
	for (tie(ei, ei_end) = edges(G); ei != ei_end; ++ei) {
		int v1 = (int)ei->m_source; int v2 = (int)ei->m_target;
		if (oldToNew[v1] == oldToNew[v2]) { continue; }
		if (newEdgeWt.find({ oldToNew[v1], oldToNew[v2] }) == newEdgeWt.end()) {
			add_edge(oldToNew[v1], oldToNew[v2], newG); 
			newEdgeWt[{ oldToNew[v1], oldToNew[v2] }] = edgeWt[{v1,v2}]; newEdgeWt[{ oldToNew[v2], oldToNew[v1] }] = edgeWt[{v1, v2}];
		}
		
		else {
			if (newEdgeWt[{ oldToNew[v1], oldToNew[v2] }] == 0) {
				if (edgeWt[{v1, v2}] == 1) {
					newEdgeWt[{ oldToNew[v1], oldToNew[v2] }] = edgeWt[{v1, v2}]; newEdgeWt[{ oldToNew[v2], oldToNew[v1] }] = edgeWt[{v1, v2}];
				}
			}
		}
	}
	G = newG;
	edgeWt = newEdgeWt;
	nodes = newNodes;
	return newToOldComp;
}

std::vector< std::vector<int>> getCfComponents(Graph & g, std::vector<node> & nodes, int numNodes) {
	std::vector< std::vector<int> > typeComponents;
	std::vector<int> nodeToComp(numNodes);
	int n = (int)boost::connected_components(g, &nodeToComp[0]);
	int numComps = 0; std::vector<int> isCompIndex(n, -1);
	for (int i = 0; i < numNodes; i++) {

		if (((int)nodes[i].type) == 2 || ((int)nodes[i].type) == 3) {
			if (isCompIndex[nodeToComp[i]] == -1) {

				isCompIndex[nodeToComp[i]] = numComps;
				std::vector<int> newComp = { i };
				typeComponents.push_back(newComp);
				numComps += 1;
			}
			else {
				typeComponents[isCompIndex[nodeToComp[i]]].push_back(i);
			}

		}
	}
	return typeComponents;
}

void updateGraphs(Graph & G, Graph & fgG, Graph & bgG, std::vector<node> & nodes, map<std::vector<int>, int> & edgeWts) {
	typedef boost::graph_traits<Graph>::edge_iterator edge_iter;
	edge_iter ei, ei_end;
	for (tie(ei, ei_end) = edges(G); ei != ei_end; ++ei) {
		int v1 = (int)ei->m_source; int v2 = (int)ei->m_target;
		if (((nodes[v1].type == 0) || (nodes[v1].type == 2) || (nodes[v1].type == 3)) && ((nodes[v2].type == 0) || (nodes[v2].type == 2) || (nodes[v2].type == 3)) && edgeWts[{v1,v2}] == 1) {
			add_edge(v1, v2, fgG);
		}
		if (((nodes[v1].type == 1) || (nodes[v1].type == 2) || (nodes[v1].type == 3)) && ((nodes[v2].type == 1) || (nodes[v2].type == 2) || (nodes[v2].type == 3))) {
			add_edge(v1, v2, bgG);
		}
	}
}

void getLocalGraphClusters(Graph & G, std::vector<node> & nodes, int numNodes, Graph & fgG, Graph & bgG, std::vector< std::vector< std::vector<int> > > & localGraphEdges, std::vector< std::vector<int> > & localNodesGlobalIndex, map< std::vector<int>, int> & edgeWt) {
	Graph cutFillGraph;
	for (int i = 0; i < numNodes; i++) {
		add_vertex(cutFillGraph); add_vertex(fgG); add_vertex(bgG);
	}
	typedef boost::graph_traits<Graph>::edge_iterator edge_iter;
	edge_iter ei, ei_end;
	for (tie(ei, ei_end) = edges(G); ei != ei_end; ++ei) {
		int v1 = (int)ei->m_source; int v2 = (int)ei->m_target;
		if ( ((nodes[v1].type == 2) || (nodes[v1].type == 3)) && ((nodes[v2].type == 2) || (nodes[v2].type == 3))) {
			add_edge(v1, v2, cutFillGraph);
		}
		if (((nodes[v1].type == 0) || (nodes[v1].type == 2) || (nodes[v1].type == 3)) && ((nodes[v2].type == 0) || (nodes[v2].type == 2) || (nodes[v2].type == 3)) && edgeWt[{v1,v2}] == 1) {
			add_edge(v1, v2, fgG);
		}
		if (((nodes[v1].type == 1) || (nodes[v1].type == 2) || (nodes[v1].type == 3)) && ((nodes[v2].type == 1) || (nodes[v2].type == 2) || (nodes[v2].type == 3))) {
			add_edge(v1, v2, bgG);
		}

	}
	std::vector< std::vector<int>> cfComponents = getCfComponents(cutFillGraph, nodes, numNodes);
	std::vector< std::vector<int> > globalToLocalNodes(numNodes, std::vector<int>(0));

	for (int i = 0; i < cfComponents.size(); i++) {
		std::vector<int> cfWithTerminals = cfComponents[i];
		map<int, bool> termAdded;
		for (int j = 0; j < cfComponents[i].size(); j++) {
			globalToLocalNodes[cfComponents[i][j]].push_back(i); //Map each node to the cluster which contains it

			auto neighbours = adjacent_vertices(cfComponents[i][j], G);
			for (auto vd : make_iterator_range(neighbours)) {
				if (((int)nodes[vd].type) == 0 || ((int)nodes[vd].type) == 1) {
					if (termAdded.find(vd) == termAdded.end()) {
						cfWithTerminals.push_back(vd);
						termAdded[vd] = true;
					}
				}
			}
		}
		localNodesGlobalIndex.push_back(cfWithTerminals);
		std::vector< std::vector<int> > localEdges; localGraphEdges.push_back(localEdges);
	}

	//Iterate through edges, find subgraph for each local cut-fill cluster in one sweep
	for (tie(ei, ei_end) = edges(G); ei != ei_end; ++ei) {
		int v1 = (int)ei->m_source; int v2 = (int)ei->m_target;
		if (((int)nodes[v1].type == 2) || ((int)nodes[v1].type == 3)) {
			localGraphEdges[globalToLocalNodes[v1][0]].push_back({ v1, v2 });
			continue;
		}
		else {
			if (((int)nodes[v2].type == 2) || ((int)nodes[v2].type == 3)) {
				localGraphEdges[globalToLocalNodes[v2][0]].push_back({ v1, v2 });
				continue;
			}

		}
	}
}

void getLocalGraphClustersMapping(Graph & G, std::vector<node> & nodes, int numNodes, std::vector< std::vector< std::vector<int> > > & localGraphEdges, std::vector< std::vector<int> > & localNodesGlobalIndex, map<int, int> & nodeToComp) {
	Graph cutFillGraph;
	for (int i = 0; i < numNodes; i++) {
		add_vertex(cutFillGraph);
	}
	typedef boost::graph_traits<Graph>::edge_iterator edge_iter;
	edge_iter ei, ei_end;
	for (tie(ei, ei_end) = edges(G); ei != ei_end; ++ei) {
		int v1 = (int)ei->m_source; int v2 = (int)ei->m_target;
		if (((nodes[v1].type == 2) || (nodes[v1].type == 3)) && ((nodes[v2].type == 2) || (nodes[v2].type == 3))) {
			add_edge(v1, v2, cutFillGraph);
		}

	}
	std::vector< std::vector<int>> cfComponents = getCfComponents(cutFillGraph, nodes, numNodes);
	std::vector< std::vector<int> > globalToLocalNodes(numNodes, std::vector<int>(0));

	for (int i = 0; i < cfComponents.size(); i++) {
		std::vector<int> cfWithTerminals = cfComponents[i];
		map<int, bool> termAdded;
		for (int j = 0; j < cfComponents[i].size(); j++) {
			nodeToComp[cfComponents[i][j]] = i;
			globalToLocalNodes[cfComponents[i][j]].push_back(i); //Map each node to the cluster which contains it

			auto neighbours = adjacent_vertices(cfComponents[i][j], G);
			for (auto vd : make_iterator_range(neighbours)) {
				if (((int)nodes[vd].type) == 0 || ((int)nodes[vd].type) == 1) {
					if (termAdded.find(vd) == termAdded.end()) {
						cfWithTerminals.push_back(vd);
						termAdded[vd] = true;
					}
				}
			}
		}
		localNodesGlobalIndex.push_back(cfWithTerminals);
		std::vector< std::vector<int> > localEdges; localGraphEdges.push_back(localEdges);
	}

	//Iterate through edges, find subgraph for each local cut-fill cluster in one sweep
	for (tie(ei, ei_end) = edges(G); ei != ei_end; ++ei) {
		int v1 = (int)ei->m_source; int v2 = (int)ei->m_target;
		if (((int)nodes[v1].type == 2) || ((int)nodes[v1].type == 3)) {
			localGraphEdges[globalToLocalNodes[v1][0]].push_back({ v1, v2 });
			continue;
		}
		else {
			if (((int)nodes[v2].type == 2) || ((int)nodes[v2].type == 3)) {
				localGraphEdges[globalToLocalNodes[v2][0]].push_back({ v1, v2 });
				continue;
			}

		}
	}
}

void getFgBgGraphs(Graph & G, std::vector<node> & nodes, int numNodes, Graph & fgG, Graph & bgG, map< std::vector<int>, int> & edgeWts) {
	Graph cutFillGraph;
	for (int i = 0; i < numNodes; i++) {
		add_vertex(fgG); add_vertex(bgG);
	}
	typedef boost::graph_traits<Graph>::edge_iterator edge_iter;
	edge_iter ei, ei_end;
	for (tie(ei, ei_end) = edges(G); ei != ei_end; ++ei) {
		int v1 = (int)ei->m_source; int v2 = (int)ei->m_target;
		if (((nodes[v1].type == 0) || (nodes[v1].type == 2) || (nodes[v1].type == 3)) && ((nodes[v2].type == 0) || (nodes[v2].type == 2) || (nodes[v2].type == 3)) && edgeWts[{v1,v2}] == 1) {
			add_edge(v1, v2, fgG);
		}
		if (((nodes[v1].type == 1) || (nodes[v1].type == 2) || (nodes[v1].type == 3)) && ((nodes[v2].type == 1) || (nodes[v2].type == 2) || (nodes[v2].type == 3))) {
			add_edge(v1, v2, bgG);
		}

	}
}

std::vector< std::vector<int> > pruneLocalImpossibleComplexEdges(std::vector< std::vector<int> > & clusterLocalEdgesGlobalIndices, std::vector<node> & nodes, int numNodes,
	std::vector<int> & clusterLocalNodesGlobalIndices, Graph & fgG, Graph & bgG, int fgComps, int bgComps) {
	std::vector<node> nodesTemp = nodes; std::vector< std::vector<int> > newEdges;
	for (int i = 0; i < clusterLocalEdgesGlobalIndices.size(); i++) {
		int v1 = clusterLocalEdgesGlobalIndices[i][0]; int v2 = clusterLocalEdgesGlobalIndices[i][1];
		if (((int)nodes[v1].type) == 2 || ((int)nodes[v1].type) == 3) {
			if (((int)nodes[v2].type) == 2 || ((int)nodes[v2].type) == 3) {
				bool removable = false;
				
				int cNode = v1; int fNode = v2;
				if (((int)nodes[v1].type) == 3 && ((int)nodes[v2].type) == 2) {
					cNode = v2; fNode = v1;
				}

				std::vector<int> connBgTerminals; //Find neighborhood nodes connected to cut in background graph, and all fill nodes connected to this cut node
				auto neighbours = adjacent_vertices(cNode, bgG); std::vector<int> fillNeighbors;
				for (auto vd : make_iterator_range(neighbours)) {
					if (((int)nodes[vd].type) == 1) {
						connBgTerminals.push_back(vd);
					}
					else {
						if (((int)nodes[vd].type) == 3) {
							fillNeighbors.push_back(vd);
						}
					}
				}
				if (connBgTerminals.size() == 1) { //If only connected to one BG terminal
					//Nodes connected to the lone BG terminal
					auto neighboursT = adjacent_vertices(connBgTerminals[0], bgG);
					bool allConnToTerm = true; int numNeighbors = 0; std::vector<int> bgNeighbors;
					for (auto vd : make_iterator_range(neighboursT)) {
						numNeighbors += 1;
					}
					if (numNeighbors == 0) { allConnToTerm = false; }
					//If one of the fills connected to the cut is not a neighbor to the neighborhood terminal
					for (int j = 0; j < fillNeighbors.size(); j++) {
						auto neighboursF = adjacent_vertices(fillNeighbors[j], bgG); bool connToTerm = false;
						for (auto vd : make_iterator_range(neighboursF)) {
							if (vd == connBgTerminals[0]) { connToTerm = true; }
						}
						if (!connToTerm) {
							allConnToTerm = false;
							break;
						}
					}
					if (allConnToTerm) {
						//Simulate sending cut to background
						Graph fgGS = fgG; clear_vertex(cNode, fgGS);
						nodesTemp[cNode].type = 1; nodesTemp[cNode].inFg = 0;
						int fgCompsS = findComponents(fgGS, nodesTemp, numNodes, true);
						nodesTemp[cNode].type = 2; nodesTemp[cNode].inFg = 1;
						if (fgCompsS > fgComps) {
							removable = true; 
						}
						else {
							if (fgCompsS == fgComps) {
								if (nodes[cNode].labelCost > 0) {
									removable = true;
								}
							}
						}
					}
				}
				if (!removable) {
					
					std::vector<int> connFgTerminals;
					auto neighboursFg = adjacent_vertices(fNode, fgG); std::vector<int> cutNeighbors;
					for (auto vd : make_iterator_range(neighboursFg)) {
						if (((int)nodes[vd].type) == 0) {
							connFgTerminals.push_back(vd);
						}
						else {
							if (((int)nodes[vd].type) == 2) {
								cutNeighbors.push_back(vd);
							}
						}
					}
				if (connFgTerminals.size() == 1) { //If only connected to one FG terminal

					//Nodes connected to the lone FG terminal
					auto neighboursT = adjacent_vertices(connFgTerminals[0], fgG);
					bool allConnToTerm = true; int numNeighbors = 0; std::vector<int> fgNeighbors;
					for (auto vd : make_iterator_range(neighboursT)) {
						numNeighbors += 1;
					}
					if (numNeighbors == 0) { allConnToTerm = false; }
					//If one of the cuts connected to the fill is not a neighbor to the core terminal
					for (int j = 0; j < cutNeighbors.size(); j++) {
						auto neighboursC = adjacent_vertices(cutNeighbors[j], fgG); bool connToTerm = false;
						for (auto vd : make_iterator_range(neighboursC)) {
							if (vd == connFgTerminals[0]) { connToTerm = true; }
						}
						if (!connToTerm) {
							allConnToTerm = false;
							break;
						}
					}
					if (allConnToTerm) {

						//Simulate sending fill to foreground
						Graph bgGS = bgG; clear_vertex(fNode, bgGS);
						nodesTemp[fNode].type = 0; nodesTemp[fNode].inFg = 1;
						int bgCompsS = findComponents(bgGS, nodesTemp, numNodes, false);
						nodesTemp[fNode].type = 3; nodesTemp[fNode].inFg = 0;
						if (bgCompsS > bgComps) {
							removable = true;
									}
									else {
										if (bgCompsS == bgComps) {
											if (nodes[fNode].labelCost < 0) {
												removable = true;
											}
										}
									}
								}
							}
					}
					if (!removable) {
						newEdges.push_back({ v1,v2 });
					}
				}
				else {
				newEdges.push_back({ v1, v2 });
				}
			}
			else {
			newEdges.push_back({ v1, v2 });
			}
	}
	return newEdges;
}

void getCompToLocalIndex(std::vector<int> & clusterLocalNodesGlobalIndices, map<int, int> & localToGlobal, map<int, int> & globalToLocal, map< std::vector<int>, int> & edgeWts) {
	for (int i = 0; i < clusterLocalNodesGlobalIndices.size(); i++) {
		globalToLocal[clusterLocalNodesGlobalIndices[i]] = i; localToGlobal[i] = clusterLocalNodesGlobalIndices[i];
	}
}



class doubleNode {
public:
	doubleNode(node n, int specialType, int side);
	int getType(); int getSide(); 
	std::vector<int> hypernodes; int localIndex; node origNode;
private:
	int type;
	int side;
};

doubleNode::doubleNode(node n, int nodeType, int nodeSide) {
	type = nodeType;
	side = nodeSide;
	origNode = n;
}

int doubleNode::getType() {
	return type;
}

int doubleNode::getSide() {
	return side;
}

class componentGraph {
public:
	componentGraph() {
		numNodes = 0;
		nodeAssignment = 0;
	};
	componentGraph(int graphSize) {
		numNodes = graphSize;
		for (int i = 0; i < graphSize; i++) {
			add_vertex(g);
		}
		nodeAssignment = 0;
	};
	void clearLocalVertex(int localIndex);
	std::vector<std::vector<int> > getGlobalComponentsExcluding(std::vector<int> & excludeIndices);
	Graph g;
	int numNodes; int nodeAssignment;
	std::vector<int> globalNodes;
};


void componentGraph::clearLocalVertex(int localIndex) {
	clear_vertex(localIndex, g);
}

std::vector<std::vector<int> > componentGraph::getGlobalComponentsExcluding(std::vector<int> & excludeIndices) {
	std::vector< std::vector<int> > components;
	std::vector<int> nodeToComp(numNodes);
	
	if (numNodes == 1) {
		return { {globalNodes[0]}  };
	}
	std::vector<bool> isExcluded(numNodes, false);
	for (int i = 0; i < excludeIndices.size(); i++) {
		isExcluded[excludeIndices[i]] = true;
	}
	if (num_edges(g) == 0) {
		for (int i = 0; i < numNodes; i++) {
			if (!isExcluded[i]) {
				components.push_back({ globalNodes[i] });
			}
		}
		return components;
	}
	
	int n = (int)boost::connected_components(g, &nodeToComp[0]);
	
	int numComps = 0; std::vector<int> isCompIndex(n, -1); 
	
	for (int i = 0; i < numNodes; i++) {
		if (!isExcluded[i]) {
			if (isCompIndex[nodeToComp[i]] == -1) {

				isCompIndex[nodeToComp[i]] = numComps;
				std::vector<int> newComp = { globalNodes[i] };
				components.push_back(newComp);
				numComps += 1;
			}
			else {
				components[isCompIndex[nodeToComp[i]]].push_back(globalNodes[i]);
			}
		}
	}
	return components;
}

class doubleGraph {
public:
	doubleGraph(std::vector<node> & nodes, std::vector<int> & clusterLocalNodesGlobalIndices, std::vector< std::vector<int> > & clusterLocalEdgesGlobalIndices, std::vector< std::vector<int> > & complexEdges,
		map<int, int> & globalToLocal, map< std::vector<int>, int> & edgeWts);
	doubleGraph(std::vector<node> & nodes, int numNodes, Graph & g, map< std::vector<int>, int> & edgeWts);
	doubleGraph() {};
	std::vector<std::vector<int>> findTerminalComponents(bool inFg); void findNTComponentsConstraintEdges(bool inFg);
	Graph getBoostGraph();
	std::vector< std::vector<int> > getFgCompsComplex(); std::vector< std::vector<int> > getBgCompsComplex(); void assignEdgesToFgAndBgComps(); int getSize();
	std::vector<int> cores; std::vector<int> ns; std::vector<int> midTs;
	std::vector< std::vector<int> > findRemainingCFComponents(bool inFg, std::vector<int> & excludeIndices);
	int getPiIndex(); std::vector<doubleNode> doubleNodes; Graph complexG; Graph fgG; Graph bgG;
	map<int, int> doubleNodeFgCompMapping; map<int, int> doubleNodeBgCompMapping; std::vector<componentGraph> fgCompGraphs; std::vector<componentGraph> bgCompGraphs;
	std::vector< std::vector<int> > doubleEdges;
	map<int, std::vector<int> > nodeToDoubleMapping;
private:
	
	std::vector< std::vector<int> > doubleEdgesComplex; //Graph doubleG;
	int numDoubleNodes; int piIndex; std::vector< std::vector<int> > fgCompsComplex; std::vector< std::vector<int> > bgCompsComplex; 
	
};

std::vector< std::vector<int> > doubleGraph::getFgCompsComplex() {
	return fgCompsComplex;
}

std::vector< std::vector<int> > doubleGraph::getBgCompsComplex() {
	return bgCompsComplex;
}

int doubleGraph::getPiIndex() {
	return piIndex;
}

int doubleGraph::getSize() {
	return numDoubleNodes;
}

std::vector<std::vector<int>>  doubleGraph::findTerminalComponents(bool inFg) {
	if (inFg) {
		Graph fgG;
		for (int i = 0; i < doubleNodes.size(); i++) {
			add_vertex(fgG);
		}
		for (int i = 0; i < doubleEdges.size(); i++) {
			
			if (doubleNodes[doubleEdges[i][0]].getSide() == 1 && doubleNodes[doubleEdges[i][1]].getSide() == 1) {
				add_edge(doubleEdges[i][0], doubleEdges[i][1], fgG);
			}
		}
		std::vector< std::vector<int> > components;
		std::vector<int> nodeToComp(numDoubleNodes);
		int n = (int)boost::connected_components(fgG, &nodeToComp[0]);
		int numComps = 0; std::vector<int> isCompIndex(n, -1);
		for (int i = 0; i < numDoubleNodes; i++) {
			if (((int)doubleNodes[i].getSide()) == 1) {
				if (isCompIndex[nodeToComp[i]] == -1) {

					isCompIndex[nodeToComp[i]] = numComps;
					std::vector<int> newComp = { i };
					components.push_back(newComp);
					numComps += 1; 
				}
				else {
					components[isCompIndex[nodeToComp[i]]].push_back(i);
				}

			}
		}
		
		std::vector<std::vector<int>> terminalComponents;
		for (int i = 0; i < components.size(); i++) {
			for (int j = 0; j < components[i].size(); j++) {
				if (((int)doubleNodes[components[i][j]].getType()) == CORE) {
					terminalComponents.push_back(components[i]);
					break;
				}
			}
		}
		return terminalComponents;
	}
	else {
		Graph bgG;
		for (int i = 0; i < doubleNodes.size(); i++) {
			add_vertex(bgG);
		}
		for (int i = 0; i < doubleEdges.size(); i++) {
			if (doubleNodes[doubleEdges[i][0]].getSide() == -1 && doubleNodes[doubleEdges[i][1]].getSide() == -1) {
				add_edge(doubleEdges[i][0], doubleEdges[i][1], bgG);
			}
		}
		std::vector< std::vector<int> > components;
		std::vector<int> nodeToComp(numDoubleNodes);
		int n = (int)boost::connected_components(bgG, &nodeToComp[0]);
		int numComps = 0; std::vector<int> isCompIndex(n, -1);
		for (int i = 0; i < numDoubleNodes; i++) {
			if (((int)doubleNodes[i].getSide()) == -1) {
				if (isCompIndex[nodeToComp[i]] == -1) {
					isCompIndex[nodeToComp[i]] = numComps;
					std::vector<int> newComp = { i };
					components.push_back(newComp);
					numComps += 1;
				}
				else {
					components[isCompIndex[nodeToComp[i]]].push_back(i);
				}
			}
		}
		std::vector<std::vector<int>> terminalComponents;
		for (int i = 0; i < components.size(); i++) {
			for (int j = 0; j < components[i].size(); j++) {
				if (((int)doubleNodes[components[i][j]].getType()) == N) {
					terminalComponents.push_back(components[i]);
					break;
				}
			}
		}
		return terminalComponents;
	}
}

void doubleGraph::findNTComponentsConstraintEdges(bool inFg) {
	if (inFg) {
		for (int i = 0; i < doubleNodes.size(); i++) {
			add_vertex(fgG);
		}
		for (int i = 0; i < doubleEdgesComplex.size(); i++) {

			if ((doubleNodes[doubleEdgesComplex[i][0]].getSide() == 1 && doubleNodes[doubleEdgesComplex[i][0]].getType() != CORE) &&
				( doubleNodes[doubleEdgesComplex[i][1]].getSide() == 1 && doubleNodes[doubleEdgesComplex[i][1]].getType() != CORE) ) {
				add_edge(doubleEdgesComplex[i][0], doubleEdgesComplex[i][1], fgG);
			}
		}
		std::vector< std::vector<int> > components;
		std::vector<int> nodeToComp(numDoubleNodes);
		int n = (int)boost::connected_components(fgG, &nodeToComp[0]);
		int numComps = 0; std::vector<int> isCompIndex(n, -1);
		for (int i = 0; i < numDoubleNodes; i++) {
			if (((int)doubleNodes[i].getSide()) == 1 && doubleNodes[i].getType() != CORE) {
				if (isCompIndex[nodeToComp[i]] == -1) {

					isCompIndex[nodeToComp[i]] = numComps;
					std::vector<int> newComp = { i };
					components.push_back(newComp);
					numComps += 1; doubleNodeFgCompMapping[i] = numComps - 1;
				}
				else {
					components[isCompIndex[nodeToComp[i]]].push_back(i); doubleNodeFgCompMapping[i] = isCompIndex[nodeToComp[i]];
				}
			}
		}
		for (int i = 0; i < components.size(); i++) {
			for (int j = 0; j < components[i].size(); j++) {
				if (((int)doubleNodes[components[i][j]].getSide()) == 1 && doubleNodes[components[i][j]].getType() != CORE) {
					fgCompsComplex.push_back(components[i]); fgCompGraphs.push_back(componentGraph(components[i].size()));
					break;
				}
			}
		}
	}
	else {
		for (int i = 0; i < doubleNodes.size(); i++) {
			add_vertex(bgG);
		}
		for (int i = 0; i < doubleEdgesComplex.size(); i++) {
			if ( (doubleNodes[doubleEdgesComplex[i][0]].getSide() == -1 && doubleNodes[doubleEdgesComplex[i][0]].getType() != N) && 
				(doubleNodes[doubleEdgesComplex[i][1]].getSide() == -1 && doubleNodes[doubleEdgesComplex[i][1]].getType() != N) ) {
				add_edge(doubleEdgesComplex[i][0], doubleEdgesComplex[i][1], bgG);
			}
		}
		std::vector< std::vector<int> > components;
		std::vector<int> nodeToComp(numDoubleNodes);
		int n = (int)boost::connected_components(bgG, &nodeToComp[0]);
		int numComps = 0; std::vector<int> isCompIndex(n, -1);
		for (int i = 0; i < numDoubleNodes; i++) {
			if (((int)doubleNodes[i].getSide()) == -1 && doubleNodes[i].getType() != N) {
				if (isCompIndex[nodeToComp[i]] == -1) {
					isCompIndex[nodeToComp[i]] = numComps;
					std::vector<int> newComp = { i };
					components.push_back(newComp);
					numComps += 1; doubleNodeBgCompMapping[i] = numComps - 1;
				}
				else {
					components[isCompIndex[nodeToComp[i]]].push_back(i); doubleNodeBgCompMapping[i] = isCompIndex[nodeToComp[i]];
				}
			}
		}
		for (int i = 0; i < components.size(); i++) {
			for (int j = 0; j < components[i].size(); j++) {
				if (((int)doubleNodes[components[i][j]].getSide()) == -1 && doubleNodes[components[i][j]].getType() != N) {
					bgCompsComplex.push_back(components[i]); bgCompGraphs.push_back(componentGraph(components[i].size()));
					break;
				}
			}
		}
	}
}

std::vector< std::vector<int> > doubleGraph::findRemainingCFComponents(bool inFg, std::vector<int> & excludeIndices) {
	if (inFg) {
		Graph fgRemaining = fgG; std::vector<bool> exclude(numDoubleNodes, false);
		for (int i = 0; i < excludeIndices.size(); i++) {
			clear_vertex(excludeIndices[i], fgRemaining); exclude[excludeIndices[i]] = true;
		}
		
		std::vector< std::vector<int> > components;
		std::vector<int> nodeToComp(numDoubleNodes); std::vector< std::vector<int> > fgCompsComplexR;
		int n = (int)boost::connected_components(fgRemaining, &nodeToComp[0]);
		int numComps = 0; std::vector<int> isCompIndex(n, -1);
		for (int i = 0; i < numDoubleNodes; i++) {
			if (((int)doubleNodes[i].getSide()) == 1 && doubleNodes[i].getType() != CORE && !exclude[i]) {
				if (isCompIndex[nodeToComp[i]] == -1) {

					isCompIndex[nodeToComp[i]] = numComps;
					std::vector<int> newComp = { i };
					components.push_back(newComp);
					numComps += 1; 
				}
				else {
					components[isCompIndex[nodeToComp[i]]].push_back(i);
				}
			}
		}
		for (int i = 0; i < components.size(); i++) {
			for (int j = 0; j < components[i].size(); j++) {
				if (((int)doubleNodes[components[i][j]].getSide()) == 1 && doubleNodes[components[i][j]].getType() != CORE && !exclude[components[i][j]]) {
					fgCompsComplexR.push_back(components[i]); 
					break;
				}
			}
		}
		return fgCompsComplexR;
	}
	else {
		Graph bgGRemaining = bgG; std::vector<bool> exclude(numDoubleNodes, false);
		for (int i = 0; i < excludeIndices.size(); i++) {
			clear_vertex(excludeIndices[i], bgGRemaining); exclude[excludeIndices[i]] = true;
		}
		std::vector< std::vector<int> > components;
		std::vector<int> nodeToComp(numDoubleNodes); std::vector< std::vector<int> > bgCompsComplexR;
		int n = (int)boost::connected_components(bgGRemaining, &nodeToComp[0]);
		int numComps = 0; std::vector<int> isCompIndex(n, -1);
		for (int i = 0; i < numDoubleNodes; i++) {
			if (((int)doubleNodes[i].getSide()) == -1 && doubleNodes[i].getType() != N && !exclude[i]) {
				if (isCompIndex[nodeToComp[i]] == -1) {
					isCompIndex[nodeToComp[i]] = numComps;
					std::vector<int> newComp = { i };
					components.push_back(newComp);
					numComps += 1; 
				}
				else {
					components[isCompIndex[nodeToComp[i]]].push_back(i);
				}
			}
		}
		for (int i = 0; i < components.size(); i++) {
			for (int j = 0; j < components[i].size(); j++) {
				if (((int)doubleNodes[components[i][j]].getSide()) == -1 && doubleNodes[components[i][j]].getType() != N && !exclude[components[i][j]]) {
					bgCompsComplexR.push_back(components[i]);
					break;
				}
			}
		}

		return bgCompsComplexR;
	}

}

void doubleGraph::assignEdgesToFgAndBgComps() {
	//Assign local node index to double node
	//Within local graph object, create vector of global ints
	for (int i = 0; i < doubleNodes.size(); i++) {
		if (doubleNodes[i].getSide() == 1 && doubleNodes[i].getType() != CORE) {
			doubleNodes[i].localIndex = fgCompGraphs[doubleNodeFgCompMapping[i]].nodeAssignment;
			fgCompGraphs[doubleNodeFgCompMapping[i]].nodeAssignment = fgCompGraphs[doubleNodeFgCompMapping[i]].nodeAssignment + 1;
			fgCompGraphs[doubleNodeFgCompMapping[i]].globalNodes.push_back(i);
		}
		else {
			if (doubleNodes[i].getSide() == -1 && doubleNodes[i].getType() != N) {
				doubleNodes[i].localIndex = bgCompGraphs[doubleNodeBgCompMapping[i]].nodeAssignment;
				bgCompGraphs[doubleNodeBgCompMapping[i]].nodeAssignment = bgCompGraphs[doubleNodeBgCompMapping[i]].nodeAssignment + 1;
				bgCompGraphs[doubleNodeBgCompMapping[i]].globalNodes.push_back(i);
			}
		}
	}
	
	for (int i = 0; i < doubleEdgesComplex.size(); i++) {
		if (doubleNodes[doubleEdgesComplex[i][0]].getSide() == 1 && doubleNodes[doubleEdgesComplex[i][1]].getSide() == 1) {
			if (doubleNodes[doubleEdgesComplex[i][0]].getType() != CORE && doubleNodes[doubleEdgesComplex[i][1]].getType() != CORE) { //Must be cut or fill
				add_edge(doubleNodes[doubleEdgesComplex[i][0]].localIndex, doubleNodes[doubleEdgesComplex[i][1]].localIndex, fgCompGraphs[doubleNodeFgCompMapping[i]].g);
			}
		}
		else {
			if (doubleNodes[doubleEdgesComplex[i][0]].getSide() == -1 && doubleNodes[doubleEdgesComplex[i][1]].getSide() == -1) {
				if (doubleNodes[doubleEdgesComplex[i][0]].getType() != N && doubleNodes[doubleEdgesComplex[i][1]].getType() != N) { //Must be cut or fill
					add_edge(doubleNodes[doubleEdgesComplex[i][0]].localIndex, doubleNodes[doubleEdgesComplex[i][1]].localIndex, bgCompGraphs[doubleNodeBgCompMapping[i]].g);
				}
			}
		}
	}
}

doubleGraph::doubleGraph(std::vector<node> & nodes, std::vector<int> & clusterLocalNodesGlobalIndices, std::vector< std::vector<int> > & clusterLocalEdgesGlobalIndices, 
	std::vector< std::vector<int> > & complexEdges, map<int, int> & globalToLocal, map< std::vector<int>, int> & edgeWts) {
	//Add nodes to double graph
	//Construct boost graph object to find components: could be useful later on as well
	for (int i = 0; i < clusterLocalNodesGlobalIndices.size(); i++) {
		if (((int)nodes[clusterLocalNodesGlobalIndices[i]].type) == CORE) {
			doubleNodes.push_back(doubleNode(nodes[clusterLocalNodesGlobalIndices[i]], CORE, 1));
			add_vertex(complexG); cores.push_back(((int)doubleNodes.size()) - 1);
			nodeToDoubleMapping[globalToLocal[clusterLocalNodesGlobalIndices[i]]] = { ((int)doubleNodes.size()) - 1 };
		}
		else {
			if (((int)nodes[clusterLocalNodesGlobalIndices[i]].type) == N) {
				doubleNodes.push_back(doubleNode(nodes[clusterLocalNodesGlobalIndices[i]], N, -1));
				add_vertex(complexG); ns.push_back(((int)doubleNodes.size()) - 1);
				nodeToDoubleMapping[globalToLocal[clusterLocalNodesGlobalIndices[i]]] = { ((int)doubleNodes.size()) - 1 };
			} 
			else {
				if (((int)nodes[clusterLocalNodesGlobalIndices[i]].type) == CUT) {
					doubleNodes.push_back(doubleNode(nodes[clusterLocalNodesGlobalIndices[i]], CUT, 1));
					add_vertex(complexG); 
					doubleNodes.push_back(doubleNode(nodes[clusterLocalNodesGlobalIndices[i]], MIDT, 0));
					add_vertex(complexG); midTs.push_back(doubleNodes.size() - 1); 
					doubleNodes.push_back(doubleNode(nodes[clusterLocalNodesGlobalIndices[i]], CUT, -1));
					add_vertex(complexG);
					nodeToDoubleMapping[globalToLocal[clusterLocalNodesGlobalIndices[i]]] = {((int)doubleNodes.size())-3, ((int)doubleNodes.size()) -2, ((int)doubleNodes.size()) -1};
					//Add edges between node on fg side, pi node, and node on bg side
					doubleEdges.push_back({ ((int)doubleNodes.size()) - 3, ((int)doubleNodes.size()) - 2 });
					doubleEdges.push_back({ ((int)doubleNodes.size()) - 2, ((int)doubleNodes.size()) - 1 });
					//Do same for complex edges
					doubleEdgesComplex.push_back({ ((int)doubleNodes.size()) - 3, ((int)doubleNodes.size()) - 2 }); doubleEdgesComplex.push_back({ ((int)doubleNodes.size()) - 2, ((int)doubleNodes.size()) - 1 });
					add_edge(((int)doubleNodes.size()) - 3, ((int)doubleNodes.size()) - 2, complexG); add_edge(((int)doubleNodes.size()) - 2, ((int)doubleNodes.size()) - 1, complexG);
				}
				else {
					if (((int)nodes[clusterLocalNodesGlobalIndices[i]].type) == FILL) {
						doubleNodes.push_back(doubleNode(nodes[clusterLocalNodesGlobalIndices[i]], FILL, 1)); 
						add_vertex(complexG); 
						doubleNodes.push_back(doubleNode(nodes[clusterLocalNodesGlobalIndices[i]], MIDT, 0)); 
						add_vertex(complexG); midTs.push_back(doubleNodes.size() - 1);
						doubleNodes.push_back(doubleNode(nodes[clusterLocalNodesGlobalIndices[i]], FILL, -1));
						add_vertex(complexG); 
						nodeToDoubleMapping[globalToLocal[clusterLocalNodesGlobalIndices[i]]] = { ((int)doubleNodes.size()) - 3, ((int)doubleNodes.size()) - 2, ((int)doubleNodes.size()) - 1 };
						//Add edges between node on fg side, pi node, and node on bg side
						doubleEdges.push_back({ ((int)doubleNodes.size()) - 3, ((int)doubleNodes.size()) - 2 });
						doubleEdges.push_back({ ((int)doubleNodes.size()) - 2, ((int)doubleNodes.size()) - 1 }); 
						//Do same for complex edges
						doubleEdgesComplex.push_back({ ((int)doubleNodes.size()) - 3, ((int)doubleNodes.size()) - 2 });
						doubleEdgesComplex.push_back({ ((int)doubleNodes.size()) - 2, ((int)doubleNodes.size()) - 1 });
						add_edge(((int)doubleNodes.size()) - 3, ((int)doubleNodes.size()) - 2, complexG); 
						add_edge(((int)doubleNodes.size()) - 2, ((int)doubleNodes.size()) - 1, complexG);
					}
				}
			}
		}
	}
	node n; n.type = PINODE; doubleNodes.push_back(doubleNode(n, PINODE, 0)); piIndex = doubleNodes.size() - 1; numDoubleNodes = doubleNodes.size(); add_vertex(complexG); //add_vertex(doubleG); 

	//Construct double graph edges for all edges from original graph
	for (int i = 0; i < clusterLocalEdgesGlobalIndices.size(); i++) {
		int v1 = clusterLocalEdgesGlobalIndices[i][0]; int v2 = clusterLocalEdgesGlobalIndices[i][1];

		int local1 = globalToLocal[v1]; int local2 = globalToLocal[v2];
		
		if (nodeToDoubleMapping[local1].size() == 1 && nodeToDoubleMapping[local2].size() > 1) {
			
			if (((int)nodes[v1].type) == 0) {
				if (edgeWts[{v1, v2}] == 1) {
					doubleEdges.push_back({ nodeToDoubleMapping[local1][0], nodeToDoubleMapping[local2][0] }); 
				}
			}
			else {
				if (((int)nodes[v1].type) == 1) {
					doubleEdges.push_back({ nodeToDoubleMapping[local1][0], nodeToDoubleMapping[local2][2] }); 
				}
			}
		}
		if (nodeToDoubleMapping[local2].size() == 1 && nodeToDoubleMapping[local1].size() > 1) {
			
			if (((int)nodes[v2].type) == CORE) {
				if (edgeWts[{v1, v2}] == 1) {
					doubleEdges.push_back({ nodeToDoubleMapping[local2][0], nodeToDoubleMapping[local1][0] });
				}
			}
			else {
				if (((int)nodes[v2].type) == 1) {
					doubleEdges.push_back({ nodeToDoubleMapping[local2][0], nodeToDoubleMapping[local1][2] });
				}
			}
		}
		if (nodeToDoubleMapping[local1].size() > 1 && nodeToDoubleMapping[local2].size() > 1) {
			
			if (edgeWts[{v1, v2}] == 1) {
				doubleEdges.push_back({ nodeToDoubleMapping[local1][0], nodeToDoubleMapping[local2][0] });
			}
			doubleEdges.push_back({ nodeToDoubleMapping[local1][2], nodeToDoubleMapping[local2][2] });
		}
		if (nodeToDoubleMapping[local1].size() == 1 && nodeToDoubleMapping[local2].size() == 1) {
			if (((int)nodes[v1].type) == CORE && ((int)nodes[v2].type) == CORE) {
				if (edgeWts[{v1, v2}] == 1) {
					doubleEdges.push_back({ nodeToDoubleMapping[local1][0], nodeToDoubleMapping[local2][0] });
				}
			}
			else {
				if (((int)nodes[v1].type) == N && ((int)nodes[v2].type) == N) {
					doubleEdges.push_back({ nodeToDoubleMapping[local1][0], nodeToDoubleMapping[local2][0] });
				}
			}
		}
	}
	//Construct complex double graph edges for all edges from original graph
	for (int i = 0; i < complexEdges.size(); i++) {

		int v1 = complexEdges[i][0]; int v2 = complexEdges[i][1];
		int local1 = globalToLocal[v1]; int local2 = globalToLocal[v2];
		if (nodeToDoubleMapping[local1].size() == 1 && nodeToDoubleMapping[local2].size() > 1) {
			if (((int)nodes[v1].type) == 0) {
				if (edgeWts[{v1, v2}] == 1) {
					doubleEdgesComplex.push_back({ nodeToDoubleMapping[local1][0], nodeToDoubleMapping[local2][0] });
					add_edge(nodeToDoubleMapping[local1][0], nodeToDoubleMapping[local2][0], complexG);
				}
			}
			else {
				if (((int)nodes[v1].type) == 1) {
					doubleEdgesComplex.push_back({ nodeToDoubleMapping[local1][0], nodeToDoubleMapping[local2][2] });
					add_edge(nodeToDoubleMapping[local1][0], nodeToDoubleMapping[local2][2], complexG);
				}
			}
		}
		if (nodeToDoubleMapping[local2].size() == 1 && nodeToDoubleMapping[local1].size() > 1) {
			if (((int)nodes[v2].type) == 0) {
				if (edgeWts[{v1, v2}] == 1) {
					doubleEdgesComplex.push_back({ nodeToDoubleMapping[local2][0], nodeToDoubleMapping[local1][0] }); 
					add_edge(nodeToDoubleMapping[local2][0], nodeToDoubleMapping[local1][0], complexG);
				}
			}
			else {
				if (((int)nodes[v2].type) == 1) {
					doubleEdgesComplex.push_back({ nodeToDoubleMapping[local2][0], nodeToDoubleMapping[local1][2] });
					add_edge(nodeToDoubleMapping[local2][0], nodeToDoubleMapping[local1][2], complexG);
				}
			}
		}
		if (nodeToDoubleMapping[local1].size() > 1 && nodeToDoubleMapping[local2].size() > 1) {
			doubleEdgesComplex.push_back({ nodeToDoubleMapping[local1][0], nodeToDoubleMapping[local2][0] });
			add_edge(nodeToDoubleMapping[local1][0], nodeToDoubleMapping[local2][0], complexG);
			doubleEdgesComplex.push_back({ nodeToDoubleMapping[local1][2], nodeToDoubleMapping[local2][2] });
			add_edge(nodeToDoubleMapping[local1][2], nodeToDoubleMapping[local2][2], complexG);
		}
		
		if (nodeToDoubleMapping[local1].size() == 1 && nodeToDoubleMapping[local2].size() == 1) {
			if (((int)nodes[v1].type) == CORE && ((int)nodes[v2].type) == CORE) {
				if (edgeWts[{v1, v2}] == 1) {
					doubleEdgesComplex.push_back({ nodeToDoubleMapping[local1][0], nodeToDoubleMapping[local2][0] });
					add_edge(nodeToDoubleMapping[local1][0], nodeToDoubleMapping[local2][0], complexG);
				}
			}
			else {
				if (((int)nodes[v1].type) == N && ((int)nodes[v2].type) == N) {
					doubleEdgesComplex.push_back({ nodeToDoubleMapping[local1][0], nodeToDoubleMapping[local2][0] }); 
					add_edge(nodeToDoubleMapping[local1][0], nodeToDoubleMapping[local2][0], complexG);
				}
			}
		}

	}
	int coreCompEs = 0;
	//Find terminal components
	std::vector<std::vector<int> > coreComps = findTerminalComponents(true); 
	for (int i = 0; i < coreComps.size(); i++) {
		for (int j = 0; j < coreComps[i].size(); j++) {
			if (doubleNodes[coreComps[i][j]].getType() == CORE) {
				doubleEdges.push_back({ coreComps[i][j], piIndex }); //add_edge(coreComps[i][j], piIndex, doubleG);
				coreCompEs += 1;
				break;
			}
		}
	}
	int nCompEs = 0;
	std::vector<std::vector<int> > nComps = findTerminalComponents(false);
	for (int i = 0; i < nComps.size(); i++) {
		for (int j = 0; j < nComps[i].size(); j++) {
			if (doubleNodes[nComps[i][j]].getType() == N) {
				doubleEdges.push_back({ nComps[i][j], piIndex }); //add_edge(nComps[i][j], piIndex, doubleG);
				nCompEs += 1;
				break;
			}
		}
	}
	findNTComponentsConstraintEdges(true); findNTComponentsConstraintEdges(false);

	assignEdgesToFgAndBgComps(); //Will make it faster to build up hypernodes of connected components excluding particular nodes
}

doubleGraph::doubleGraph(std::vector<node> & nodes, int numNodes, Graph & g, map< std::vector<int>, int> & edgeWts) {
	//Add nodes to double graph
	//Construct boost graph object to find components: could be useful later on as well
	int numCores = 0;
	for (int i = 0; i < numNodes; i++) {
		if (((int)nodes[i].type) == CORE) {
			doubleNodes.push_back(doubleNode(nodes[i], CORE, 1));  
			cores.push_back(((int)doubleNodes.size()) - 1);
			nodeToDoubleMapping[i] = { ((int)doubleNodes.size()) - 1 }; 
			numCores += 1;
		}
		else {
			if (((int)nodes[i].type) == N) {
				doubleNodes.push_back(doubleNode(nodes[i], N, -1)); 
				ns.push_back(((int)doubleNodes.size()) - 1);
				nodeToDoubleMapping[i] = { ((int)doubleNodes.size()) - 1 };
			}
			else {
				if (((int)nodes[i].type) == CUT) {
					doubleNodes.push_back(doubleNode(nodes[i], CUT, 1));
					doubleNodes.push_back(doubleNode(nodes[i], MIDT, 0));
					midTs.push_back(doubleNodes.size() - 1);
					doubleNodes.push_back(doubleNode(nodes[i], CUT, -1)); //add_vertex(doubleG); 
					nodeToDoubleMapping[i] = { ((int)doubleNodes.size()) - 3, ((int)doubleNodes.size()) - 2, ((int)doubleNodes.size()) - 1 };
					//Add edges between node on fg side, pi node, and node on bg side
					doubleEdges.push_back({ ((int)doubleNodes.size()) - 3, ((int)doubleNodes.size()) - 2 });
					doubleEdges.push_back({ ((int)doubleNodes.size()) - 2, ((int)doubleNodes.size()) - 1 });
				}
				else {
					if (((int)nodes[i].type) == FILL) {
						doubleNodes.push_back(doubleNode(nodes[i], FILL, 1)); 
						doubleNodes.push_back(doubleNode(nodes[i], MIDT, 0)); midTs.push_back(doubleNodes.size() - 1);
						doubleNodes.push_back(doubleNode(nodes[i], FILL, -1));
						nodeToDoubleMapping[i] = { ((int)doubleNodes.size()) - 3, ((int)doubleNodes.size()) - 2, ((int)doubleNodes.size()) - 1 };
						//Add edges between node on fg side, pi node, and node on bg side
						doubleEdges.push_back({ ((int)doubleNodes.size()) - 3, ((int)doubleNodes.size()) - 2 }); 
						doubleEdges.push_back({ ((int)doubleNodes.size()) - 2, ((int)doubleNodes.size()) - 1 }); 
					}
				}
			}
		}
	}
	node n; n.type = PINODE; doubleNodes.push_back(doubleNode(n, PINODE, 0)); piIndex = doubleNodes.size() - 1; numDoubleNodes = doubleNodes.size();
	

	//Construct double graph edges for all edges from original graph
	typedef boost::graph_traits<Graph>::edge_iterator edge_iter;
	edge_iter ei, ei_end;
	for (tie(ei, ei_end) = edges(g); ei != ei_end; ++ei) {
		int v1 = (int)ei->m_source; int v2 = (int)ei->m_target;
		if (nodeToDoubleMapping[v1].size() == 1 && nodeToDoubleMapping[v2].size() > 1) {

			if (((int)nodes[v1].type) == CORE) {
				if (edgeWts[{v1, v2}] == 1) {
					doubleEdges.push_back({ nodeToDoubleMapping[v1][0], nodeToDoubleMapping[v2][0] });
				}
			} 
			else {
				if (((int)nodes[v1].type) == N) {
					doubleEdges.push_back({ nodeToDoubleMapping[v1][0], nodeToDoubleMapping[v2][2] });
				}
			}
		}
		if (nodeToDoubleMapping[v2].size() == 1 && nodeToDoubleMapping[v1].size() > 1) {

			if (((int)nodes[v2].type) == CORE) {
				if (edgeWts[{v1, v2}] == 1) {
					doubleEdges.push_back({ nodeToDoubleMapping[v2][0], nodeToDoubleMapping[v1][0] }); 
				}
			}
			else {
				if (((int)nodes[v2].type) == N) {
					doubleEdges.push_back({ nodeToDoubleMapping[v2][0], nodeToDoubleMapping[v1][2] });
				}
			}
		}
		if (nodeToDoubleMapping[v1].size() > 1 && nodeToDoubleMapping[v2].size() > 1) {
			if (edgeWts[{v1, v2}] == 1) {
				doubleEdges.push_back({ nodeToDoubleMapping[v1][0], nodeToDoubleMapping[v2][0] });
			}
			doubleEdges.push_back({ nodeToDoubleMapping[v1][2], nodeToDoubleMapping[v2][2] });
		}
		if (nodeToDoubleMapping[v1].size() == 1 && nodeToDoubleMapping[v2].size() == 1) {
			if (((int)nodes[v1].type) == CORE && ((int)nodes[v2].type) == CORE) {
				if (edgeWts[{v1, v2}] == 1) {
					doubleEdges.push_back({ nodeToDoubleMapping[v1][0], nodeToDoubleMapping[v2][0] });
				}
			}
			else {
				if (((int)nodes[v1].type) == N && ((int)nodes[v2].type) == N) {
					doubleEdges.push_back({ nodeToDoubleMapping[v1][0], nodeToDoubleMapping[v2][0] });
				}
			}
		}
	}
	
	int coreCompEs = 0;
	//Find terminal components
	std::vector<std::vector<int> > coreComps = findTerminalComponents(true);
	for (int i = 0; i < coreComps.size(); i++) {
		for (int j = 0; j < coreComps[i].size(); j++) {
			if (doubleNodes[coreComps[i][j]].getType() == CORE) {
				doubleEdges.push_back({ coreComps[i][j], piIndex });
				coreCompEs += 1;
				break;
			}
		}
	
	}
	int nCompEs = 0;
	std::vector<std::vector<int> > nComps = findTerminalComponents(false);
	for (int i = 0; i < nComps.size(); i++) {
		for (int j = 0; j < nComps[i].size(); j++) {
			if (doubleNodes[nComps[i][j]].getType() == N) {
				doubleEdges.push_back({ nComps[i][j], piIndex });
				
				nCompEs += 1;
				break;
			}
		}
	}
	
}

class hyperNode {
public:
	hyperNode(std::vector<int> & subnodes, int specialType, int fgSide);
	int getType(); void setWeight(int64_t wt);
	int getSide(); void assignHyperNodeWt(std::vector<doubleNode> & doubleNodes, int64_t wtSum);
	std::vector<int> doubleSubnodes; double getWeight(); int decIndex;
private:
	int type; int side; int64_t weight;
};

hyperNode::hyperNode(std::vector<int> & subnodes, int specialType, int fgSide) {
	doubleSubnodes = subnodes; type = specialType; side = fgSide; decIndex = -1;
}

int hyperNode::getType() {
	return type;
}

int hyperNode::getSide() {
	return side;
}

double hyperNode::getWeight() {
	return weight;
}

void hyperNode::setWeight(int64_t wt) {
	weight = wt;
}

void hyperNode::assignHyperNodeWt(std::vector<doubleNode> & doubleNodes, int64_t wtSum) {

	if ((type == CORE) || (type == N) || (type == MIDT) || (type == PINODE) ) {
		weight = INFINITY;
	}
	else {
		if (side == 1) {
			weight = 0;
			for (int i = 0; i < doubleSubnodes.size(); i++) {
				weight += (- (int64_t)doubleNodes[doubleSubnodes[i]].origNode.labelCost + wtSum);
			}
		}
		else {//Side must be  -1 (AKA background)
			weight = 0;
			for (int i = 0; i < doubleSubnodes.size(); i++) {
				weight += (((int64_t)doubleNodes[doubleSubnodes[i]].origNode.labelCost) + wtSum);
			}
		}
	}
	
}

class hyperGraph {
	public:
		hyperGraph(doubleGraph & doubleGIn, int nodeSize, int64_t wtSum); //Constructor local stage
		hyperGraph(std::vector<node> & nodes, doubleGraph & doubleGIn, map< std::vector<int>, int> & edgeWts, Graph & G, tbb::concurrent_vector< hyperNode > & globalHypernodes, int64_t wtSum); //Constructor for global stage
		void constructHyperEdges();
		std::vector<hyperNode> hyperNodes;
		std::vector<std::vector<int> > hyperEdges;		
		doubleGraph doubleG; int numHypernodes; int numTerminals;
		std::vector<int> coreIndices; std::vector<int> nIndices; int piIndex;
};


void hyperGraph::constructHyperEdges() {
	map<std::vector<int>, bool> edgeExists; Graph G;
	for (int i = 0; i < hyperNodes.size(); i++) {
		add_vertex(G);
	}
	for (int i = 0; i < doubleG.doubleEdges.size(); i++) {
		int v1 = doubleG.doubleEdges[i][0]; int v2 = doubleG.doubleEdges[i][1];
		std::vector<int> v1Hypernodes = doubleG.doubleNodes[v1].hypernodes; std::vector<int> v2Hypernodes = doubleG.doubleNodes[v2].hypernodes;

		for (int j = 0; j < v1Hypernodes.size(); j++) {
			for (int k = 0; k < v2Hypernodes.size(); k++) {
				if (v1Hypernodes[j] != v2Hypernodes[k]) {
					if (hyperNodes[v1Hypernodes[j]].getType() == HYPERNODE && hyperNodes[v2Hypernodes[k]].getType() == HYPERNODE) {
						continue;
					}
 
					std::vector<int> hEdge = { v1Hypernodes[j], v2Hypernodes[k] }; sort(hEdge.begin(), hEdge.end());
						if (edgeExists.find(hEdge) == edgeExists.end()) {

							hyperEdges.push_back(hEdge);
							edgeExists[hEdge] = true; edgeExists[{hEdge[1], hEdge[0]}] = true;
							add_edge(hEdge[0], hEdge[1], G);

						}
					
				}
			}
		}
	}

	edgeExists.clear();
	return;
}

hyperGraph::hyperGraph(doubleGraph & doubleGIn, int nodeSize, int64_t wtSum) {
	doubleG = doubleGIn; //Initialize hypergraph to have its first nodes be terminal nodes: cores, neighborhoods, middle terminals, pi node
	numHypernodes = 0; numTerminals = 0;
	for (int i = 0; i < doubleG.cores.size(); i++) {
		std::vector<int> coreV = { doubleG.cores[i] }; 
		hyperNodes.push_back(hyperNode(coreV, CORE, 1)); 
		numHypernodes += 1; 
		numTerminals += 1; 
		hyperNodes[numHypernodes - 1].assignHyperNodeWt(doubleG.doubleNodes, wtSum); 
		coreIndices.push_back(numHypernodes-1); 
		doubleG.doubleNodes[doubleG.cores[i]].hypernodes.push_back(hyperNodes.size() - 1);
	}
	for (int i = 0; i < doubleG.ns.size(); i++) {
		std::vector<int> nV = { doubleG.ns[i] }; 
		hyperNodes.push_back(hyperNode(nV, N, -1)); 
		numHypernodes += 1; numTerminals += 1; 
		hyperNodes[numHypernodes - 1].assignHyperNodeWt(doubleG.doubleNodes, wtSum); 
		nIndices.push_back(numHypernodes - 1); 
		doubleG.doubleNodes[doubleG.ns[i]].hypernodes.push_back(hyperNodes.size() - 1);

	}
	for (int i = 0; i < doubleG.midTs.size(); i++) {
		std::vector<int> midTV = {doubleG.midTs[i]}; 
		hyperNodes.push_back(hyperNode(midTV, MIDT, 0)); 
		numHypernodes += 1; numTerminals += 1; 
		hyperNodes[numHypernodes - 1].assignHyperNodeWt(doubleG.doubleNodes, wtSum); 
		doubleG.doubleNodes[doubleG.midTs[i]].hypernodes.push_back(hyperNodes.size() - 1);
	}
	std::vector<int> piV = { doubleG.getPiIndex() }; 
	hyperNodes.push_back(hyperNode(piV, PINODE, 0)); 
	numHypernodes += 1; 
	numTerminals += 1; 
	hyperNodes[numHypernodes - 1].assignHyperNodeWt(doubleG.doubleNodes, wtSum); 
	piIndex = numHypernodes - 1; 
	doubleG.doubleNodes[doubleG.getPiIndex()].hypernodes.push_back(hyperNodes.size() - 1);
	//Add individual cuts on fg side and individual fills on bg side
	for (int i = 0; i < doubleG.doubleNodes.size(); i++) {
		if (doubleG.doubleNodes[i].getType() == CUT) {
			if (doubleG.doubleNodes[i].getSide() == 1) {
				std::vector<int> cutV = { i }; 
				hyperNodes.push_back(hyperNode(cutV, HYPERNODE, 1)); 
				numHypernodes += 1; hyperNodes[numHypernodes - 1].assignHyperNodeWt(doubleG.doubleNodes, wtSum); 
				doubleG.doubleNodes[i].hypernodes.push_back(hyperNodes.size() - 1);
			}
		}

		if (doubleG.doubleNodes[i].getType() == FILL) {
			if (doubleG.doubleNodes[i].getSide() == -1) {
				std::vector<int> fillV = {i};
				hyperNodes.push_back(hyperNode(fillV, HYPERNODE, -1));
				numHypernodes += 1; 
				hyperNodes[numHypernodes - 1].assignHyperNodeWt(doubleG.doubleNodes, wtSum); 
				doubleG.doubleNodes[i].hypernodes.push_back(hyperNodes.size() - 1);
			}
		}
	}
	int hs0 = hyperNodes.size();
	map< std::vector<int>, bool> subsetExists;
	std::vector< std::vector<int> > fgComplexEdges = doubleG.getFgCompsComplex();
	std::vector< std::vector<int> > bgComplexEdges = doubleG.getBgCompsComplex(); 
	for (int i = 0; i < fgComplexEdges.size(); i++) { 
		std::vector<int> fgComplexComp = fgComplexEdges[i];
		if (fgComplexComp.size() == 1 && doubleG.doubleNodes[fgComplexComp[0]].getType() == CUT) {
			continue;
		}
		else {
			std::vector<int> setFills;
			for (int j = 0; j < fgComplexComp.size(); j++) {
				if (doubleG.doubleNodes[fgComplexComp[j]].getType() == FILL) { setFills.push_back(fgComplexComp[j]); }
			}
			sort(setFills.begin(), setFills.end()); subsetExists[setFills] = true; //Record that this subset of fills exists
			std::vector<int> allCutsAndFills = setFills;
			for (int j = 0; j < setFills.size(); j++) {
				auto neighboursFg = adjacent_vertices(setFills[j], doubleG.complexG);
				for (auto vd : make_iterator_range(neighboursFg)) {
					if (doubleG.doubleNodes[vd].getType() == CUT) {
						allCutsAndFills.push_back(vd);
					}
				}
			}
			sort(allCutsAndFills.begin(), allCutsAndFills.end());
			allCutsAndFills.erase(unique(allCutsAndFills.begin(), allCutsAndFills.end()), allCutsAndFills.end()); //Sort and delete duplicates
			hyperNodes.push_back(hyperNode(allCutsAndFills, HYPERNODE, 1)); 
			numHypernodes += 1; 
			hyperNodes[numHypernodes - 1].assignHyperNodeWt(doubleG.doubleNodes, wtSum);
			for (int j = 0; j < allCutsAndFills.size(); j++) {

				doubleG.doubleNodes[allCutsAndFills[j]].hypernodes.push_back(hyperNodes.size()-1);
			}
		}
	}

	for (int i = 0; i < bgComplexEdges.size(); i++) { //k = -1 (all components on fg side)
		std::vector<int> bgComplexComp = bgComplexEdges[i];
		if (bgComplexComp.size() == 1 && doubleG.doubleNodes[bgComplexComp[0]].getType() == FILL) {
			continue;
		}
		else {
			std::vector<int> setCuts;
			for (int j = 0; j < bgComplexComp.size(); j++) {
				if (doubleG.doubleNodes[bgComplexComp[j]].getType() == CUT) { 
					setCuts.push_back(bgComplexComp[j]);
				}
			}
			sort(setCuts.begin(), setCuts.end()); subsetExists[setCuts] = true; //Record that this subset of cuts exists
			std::vector<int> allCutsAndFills = setCuts;
			for (int j = 0; j < setCuts.size(); j++) {
				auto neighboursBg = adjacent_vertices(setCuts[j], doubleG.complexG);
				for (auto vd : make_iterator_range(neighboursBg)) {
					if (doubleG.doubleNodes[vd].getType() == FILL) {
						allCutsAndFills.push_back(vd);
					}
				}
			}
			sort(allCutsAndFills.begin(), allCutsAndFills.end());
			allCutsAndFills.erase(unique(allCutsAndFills.begin(), allCutsAndFills.end()), allCutsAndFills.end()); //Sort and delete duplicates
			hyperNodes.push_back(hyperNode(allCutsAndFills, HYPERNODE, -1)); 
			numHypernodes += 1; 
			hyperNodes[numHypernodes - 1].assignHyperNodeWt(doubleG.doubleNodes, wtSum);
			for (int j = 0; j < allCutsAndFills.size(); j++) {

				doubleG.doubleNodes[allCutsAndFills[j]].hypernodes.push_back(hyperNodes.size()-1);
			}
		}
	}
	int hs1 = hyperNodes.size() - hs0;
	//Construct size 0 nodes
	for (int i = 0; i < doubleG.doubleNodes.size(); i++) {
		if (doubleG.doubleNodes[i].getSide() == 1) {
			if (doubleG.doubleNodes[i].getType() == CUT) {
				//Find bg complement of individual cut
				int excludeIndex = i + 2;

				std::vector<int> excludeIndices = {excludeIndex};
				std::vector< std::vector<int> >  bgComplementComps = doubleG.findRemainingCFComponents(false, excludeIndices);

				for (int q = 0; q < bgComplementComps.size(); q++) {
					std::vector<int> currentBgComp = bgComplementComps[q];
					if (currentBgComp.size() == 0) { continue; }
					if (currentBgComp.size() == 1 && doubleG.doubleNodes[currentBgComp[0]].getType() == FILL) { continue; }
					else {
						std::vector<int> setCuts;
						for (int k = 0; k < currentBgComp.size(); k++) {
							if (doubleG.doubleNodes[currentBgComp[k]].getType() == CUT) {
								setCuts.push_back(currentBgComp[k]);
							}
						}
						if (setCuts.size() == 0) { continue; }
						sort(setCuts.begin(), setCuts.end());
						if (subsetExists.find(setCuts) == subsetExists.end()) { //Add node if the set of e-linked cuts in the component does not exist yet

							subsetExists[setCuts] = true;
							std::vector<int> allCutsAndFillsC = setCuts;
							for (int l = 0; l < setCuts.size(); l++) {
								auto neighboursBg = adjacent_vertices(setCuts[l], doubleG.complexG);
								for (auto vd : make_iterator_range(neighboursBg)) {
									if (doubleG.doubleNodes[vd].getType() == FILL) {
										allCutsAndFillsC.push_back(vd);
									}
								}
							}
							sort(allCutsAndFillsC.begin(), allCutsAndFillsC.end());
							allCutsAndFillsC.erase(unique(allCutsAndFillsC.begin(), allCutsAndFillsC.end()), allCutsAndFillsC.end()); //Sort and delete duplicates
							hyperNodes.push_back(hyperNode(allCutsAndFillsC, HYPERNODE, -1));
							numHypernodes += 1; 
							hyperNodes[numHypernodes - 1].assignHyperNodeWt(doubleG.doubleNodes, wtSum);
							for (int l = 0; l < allCutsAndFillsC.size(); l++) {
								doubleG.doubleNodes[allCutsAndFillsC[l]].hypernodes.push_back(hyperNodes.size() - 1);
							}
						}
					}
				}
			}
		}
		else {
			if (doubleG.doubleNodes[i].getSide() == -1) {
				if (doubleG.doubleNodes[i].getType() == FILL) {
					//Find fg complement of individual fill
					int excludeIndex = i - 2;
					
					std::vector<int> excludeIndices = {excludeIndex};
					std::vector< std::vector<int> > fgComplementComps = doubleG.findRemainingCFComponents(true, excludeIndices);
					for (int q = 0; q < fgComplementComps.size(); q++) {
						std::vector<int> currentFgComp = fgComplementComps[q];
						if (currentFgComp.size() == 0) { continue; }
						if (currentFgComp.size() == 1 && doubleG.doubleNodes[currentFgComp[0]].getType() == CUT) { continue; }
						else {
							std::vector<int> setFills;
							for (int k = 0; k < currentFgComp.size(); k++) {
								if (doubleG.doubleNodes[currentFgComp[k]].getType() == FILL) {
									setFills.push_back(currentFgComp[k]);
								}
							}
							if (setFills.size() == 0) { continue; }
							sort(setFills.begin(), setFills.end());
							if (subsetExists.find(setFills) == subsetExists.end()) { //Add node if the set of e-linked fills in the component does not exist yet
								subsetExists[setFills] = true;
								std::vector<int> allCutsAndFillsC = setFills;
								for (int l = 0; l < setFills.size(); l++) {
									auto neighboursFg = adjacent_vertices(setFills[l], doubleG.complexG);
									for (auto vd : make_iterator_range(neighboursFg)) {
										if (doubleG.doubleNodes[vd].getType() == CUT) {
											allCutsAndFillsC.push_back(vd);
										}
									}
								}
								sort(allCutsAndFillsC.begin(), allCutsAndFillsC.end());
								allCutsAndFillsC.erase(unique(allCutsAndFillsC.begin(), allCutsAndFillsC.end()), allCutsAndFillsC.end()); //Sort and delete duplicates
								hyperNodes.push_back(hyperNode(allCutsAndFillsC, HYPERNODE, 1)); 
								numHypernodes += 1; 
								hyperNodes[numHypernodes - 1].assignHyperNodeWt(doubleG.doubleNodes, wtSum);
								for (int l = 0; l < allCutsAndFillsC.size(); l++) {
									doubleG.doubleNodes[allCutsAndFillsC[l]].hypernodes.push_back(hyperNodes.size() - 1);
								}
							}
						}
					}

				}
			}
		}
	}
	
	//Recursively build up hypernodes up to the maximum hypernode size
	std::vector< std::vector<int> > nodesLastRoundFg; std::vector< std::vector<int> > nodesLastRoundBg; int fgCt = 0; int bgCt = 0;
	for (int i = 0; i < doubleG.getSize(); i++) {
		if (doubleG.doubleNodes[i].getSide() == 1) {
			fgCt += 1;
			if (doubleG.doubleNodes[i].getType() == FILL) {
				nodesLastRoundFg.push_back({ i });
			}
		}
		else {
			if (doubleG.doubleNodes[i].getSide() == -1) {
				bgCt += 1;
				if (doubleG.doubleNodes[i].getType() == CUT) {
					nodesLastRoundBg.push_back({ i });
				}
			}
		}
	}
	int maxNodeSize = min(nodeSize, max(fgCt, bgCt)); //If user accidentally specifies an infinitely large node size, this limits the amount of node building iterations
	//Construct nodes up to user-specified maximum size
	for (int i = 0; i < maxNodeSize; i++) {
		//Construct FG nodes and its BG complement
		std::vector< std::vector<int> > currentHyperNodesFg = nodesLastRoundFg; 
		std::vector< std::vector<int> > newNodesFg; 
		map< std::vector<int>, bool> newNodeExistsFg;
		for (int j = 0; j < currentHyperNodesFg.size(); j++) {
			std::vector<int> eLinkedFills = currentHyperNodesFg[j]; 
			std::vector<int> allCutsAndFills = eLinkedFills;
			std::sort(eLinkedFills.begin(), eLinkedFills.end());
			if (subsetExists.find(eLinkedFills) == subsetExists.end()) {
				for (int k = 0; k < eLinkedFills.size(); k++) {
					auto neighbourCuts = adjacent_vertices(eLinkedFills[k], doubleG.complexG);
					for (auto vd : make_iterator_range(neighbourCuts)) {
						if (doubleG.doubleNodes[vd].getType() == CUT) {
							allCutsAndFills.push_back(vd);
							auto neighbourFills = adjacent_vertices(vd, doubleG.complexG);
							for (auto vf : make_iterator_range(neighbourFills)) {
								if (doubleG.doubleNodes[vf].getType() == FILL) {
									if (std::find(eLinkedFills.begin(), eLinkedFills.end(), vf) == eLinkedFills.end()) {
										std::vector<int> newNode = eLinkedFills; 
										newNode.push_back(vf);
										std::sort(newNode.begin(), newNode.end());
										if (newNodeExistsFg.find(newNode) == newNodeExistsFg.end()) {
											newNodesFg.push_back(newNode);
											newNodeExistsFg[newNode] = true;
										}
									}
								}
							}
						}
					}
				}
				std::sort(allCutsAndFills.begin(), allCutsAndFills.end());
				allCutsAndFills.erase(unique(allCutsAndFills.begin(), allCutsAndFills.end()), allCutsAndFills.end());
				hyperNodes.push_back(hyperNode(allCutsAndFills, HYPERNODE, 1)); 
				numHypernodes += 1; 
				hyperNodes[numHypernodes - 1].assignHyperNodeWt(doubleG.doubleNodes, wtSum); 
				subsetExists[eLinkedFills] = true;
				std::vector<int> excludeIndices;
				for (int k = 0; k < allCutsAndFills.size(); k++) {
					doubleG.doubleNodes[allCutsAndFills[k]].hypernodes.push_back(hyperNodes.size() - 1);
					//for each cut and fill simulate removing from fg graph, find complement of component
					int excludeIndex = allCutsAndFills[k] + 2;
					excludeIndices.push_back(excludeIndex);

				}
				if (excludeIndices.size() > 0) {
					std::vector< std::vector<int> > bgComplementComps = doubleG.findRemainingCFComponents(false, excludeIndices);

					for (int q = 0; q < bgComplementComps.size(); q++) {
						std::vector<int> currentBgComp = bgComplementComps[q];
						if (currentBgComp.size() == 0) { continue; }
						if (currentBgComp.size() == 1 && doubleG.doubleNodes[currentBgComp[0]].getType() == FILL) { continue; }
						else {
							std::vector<int> setCuts;
							for (int k = 0; k < currentBgComp.size(); k++) {
								if (doubleG.doubleNodes[currentBgComp[k]].getType() == CUT) {
									setCuts.push_back(currentBgComp[k]);
								}
							}
							if (setCuts.size() == 0) { continue; }
							sort(setCuts.begin(), setCuts.end());
							if (subsetExists.find(setCuts) == subsetExists.end()) { //Add node if the set of e-linked cuts in the component does not exist yet

								subsetExists[setCuts] = true;
								std::vector<int> allCutsAndFillsC = setCuts;
								for (int l = 0; l < setCuts.size(); l++) {
									auto neighboursBg = adjacent_vertices(setCuts[l], doubleG.complexG);
									for (auto vd : make_iterator_range(neighboursBg)) {
										if (doubleG.doubleNodes[vd].getType() == FILL) {
											allCutsAndFillsC.push_back(vd);
										}
									}
								}
								sort(allCutsAndFillsC.begin(), allCutsAndFillsC.end());
								allCutsAndFillsC.erase(unique(allCutsAndFillsC.begin(), allCutsAndFillsC.end()), allCutsAndFillsC.end()); //Sort and delete duplicates
								hyperNodes.push_back(hyperNode(allCutsAndFillsC, HYPERNODE, -1)); 
								numHypernodes += 1; 
								hyperNodes[numHypernodes - 1].assignHyperNodeWt(doubleG.doubleNodes, wtSum);
								for (int l = 0; l < allCutsAndFillsC.size(); l++) {
									doubleG.doubleNodes[allCutsAndFillsC[l]].hypernodes.push_back(hyperNodes.size() - 1);
								}
							}
						}
					}
				}
			}
		}
		nodesLastRoundFg = newNodesFg;

		//Construct BG nodes and its FG complement
		std::vector< std::vector<int> > currentHyperNodesBg = nodesLastRoundBg; 
		std::vector< std::vector<int> > newNodesBg;
		map< std::vector<int>, bool> newNodeExistsBg;
		for (int j = 0; j < currentHyperNodesBg.size(); j++) {
			std::vector<int> eLinkedCuts = currentHyperNodesBg[j];
			std::vector<int> allCutsAndFills = eLinkedCuts;
			std::sort(eLinkedCuts.begin(), eLinkedCuts.end());
			if (subsetExists.find(eLinkedCuts) == subsetExists.end()) {
				for (int k = 0; k < eLinkedCuts.size(); k++) {
					auto neighbourFills = adjacent_vertices(eLinkedCuts[k], doubleG.complexG);
					for (auto vd : make_iterator_range(neighbourFills)) {
						if (doubleG.doubleNodes[vd].getType() == FILL) {
							allCutsAndFills.push_back(vd);
							auto neighbourCuts = adjacent_vertices(vd, doubleG.complexG);
							for (auto vf : make_iterator_range(neighbourCuts)) {
								if (doubleG.doubleNodes[vf].getType() == CUT) {
									if (std::find(eLinkedCuts.begin(), eLinkedCuts.end(), vf) == eLinkedCuts.end()) {
										std::vector<int> newNode = eLinkedCuts; newNode.push_back(vf);
										std::sort(newNode.begin(), newNode.end());
										if (newNodeExistsBg.find(newNode) == newNodeExistsBg.end()) {
											newNodesBg.push_back(newNode);
											newNodeExistsBg[newNode] = true;
										}
									}
								}
							}
						}
					}
				}

				subsetExists[eLinkedCuts] = true;
				std::sort(allCutsAndFills.begin(), allCutsAndFills.end());
				allCutsAndFills.erase(unique(allCutsAndFills.begin(), allCutsAndFills.end()), allCutsAndFills.end());
				hyperNodes.push_back(hyperNode(allCutsAndFills, HYPERNODE, -1)); 
				numHypernodes += 1; 
				hyperNodes[numHypernodes - 1].assignHyperNodeWt(doubleG.doubleNodes, wtSum);
				
				std::vector<int> excludeIndices;
				for (int k = 0; k < allCutsAndFills.size(); k++) {
					
					doubleG.doubleNodes[allCutsAndFills[k]].hypernodes.push_back(hyperNodes.size() - 1);
					//for each cut and fill simulate removing from bg graph, find complement of component
					int excludeIndex = allCutsAndFills[k] - 2;
					excludeIndices.push_back(excludeIndex);
				}

				if (excludeIndices.size() > 0) {
					std::vector< std::vector<int> > fgComplementComps = doubleG.findRemainingCFComponents(true, excludeIndices);
					for (int q = 0; q < fgComplementComps.size(); q++) {
						std::vector<int> currentFgComp = fgComplementComps[q];
						if (currentFgComp.size() == 0) { continue; }
						if (currentFgComp.size() == 1 && doubleG.doubleNodes[currentFgComp[0]].getType() == CUT) { continue; }
						else {
							std::vector<int> setFills;
							for (int k = 0; k < currentFgComp.size(); k++) {
								if (doubleG.doubleNodes[currentFgComp[k]].getType() == FILL) {
									setFills.push_back(currentFgComp[k]);
								}
							}
							if (setFills.size() == 0) { continue; }
							sort(setFills.begin(), setFills.end());
							if (subsetExists.find(setFills) == subsetExists.end()) { //Add node if the set of e-linked fills in the component does not exist yet
								subsetExists[setFills] = true;
								std::vector<int> allCutsAndFillsC = setFills;
								for (int l = 0; l < setFills.size(); l++) {
									auto neighboursFg = adjacent_vertices(setFills[l], doubleG.complexG);
									for (auto vd : make_iterator_range(neighboursFg)) {
										if (doubleG.doubleNodes[vd].getType() == CUT) {
											allCutsAndFillsC.push_back(vd);
										}
									}
								}
								sort(allCutsAndFillsC.begin(), allCutsAndFillsC.end());
								allCutsAndFillsC.erase(unique(allCutsAndFillsC.begin(), allCutsAndFillsC.end()), allCutsAndFillsC.end()); //Sort and delete duplicates
								hyperNodes.push_back(hyperNode(allCutsAndFillsC, HYPERNODE, 1)); 
								numHypernodes += 1; hyperNodes[numHypernodes - 1].assignHyperNodeWt(doubleG.doubleNodes, wtSum);
								for (int l = 0; l < allCutsAndFillsC.size(); l++) {
									doubleG.doubleNodes[allCutsAndFillsC[l]].hypernodes.push_back(hyperNodes.size() - 1);
								}
							}
						}
					}
				}
			}
		}
		nodesLastRoundBg = newNodesBg;
	}
	
	//Construct edges of hypergraph
	constructHyperEdges();
	
}

hyperGraph::hyperGraph(std::vector<node> & nodes, doubleGraph & doubleGIn, map< std::vector<int>, int> & edgeWts, Graph & G, tbb::concurrent_vector< hyperNode > & globalHypernodes,
	 int64_t wtSum) {
	doubleG = doubleGIn; //Initialize hypergraph to have its first nodes be terminal nodes: cores, neighborhoods, middle terminals, pi node
	numHypernodes = 0; numTerminals = 0; 
	for (int i = 0; i < doubleG.cores.size(); i++) {
		std::vector<int> coreV = { doubleG.cores[i] }; 
		hyperNodes.push_back(hyperNode(coreV, CORE, 1));
		numHypernodes += 1; 
		numTerminals += 1; 
		hyperNodes[numHypernodes - 1].assignHyperNodeWt(doubleG.doubleNodes, wtSum);
		coreIndices.push_back(numHypernodes - 1);
		doubleG.doubleNodes[doubleG.cores[i]].hypernodes.push_back(hyperNodes.size() - 1);
	}
	for (int i = 0; i < doubleG.ns.size(); i++) {
		std::vector<int> nV = { doubleG.ns[i] }; 
		hyperNodes.push_back(hyperNode(nV, N, -1)); 
		numHypernodes += 1; numTerminals += 1; 
		hyperNodes[numHypernodes - 1].assignHyperNodeWt(doubleG.doubleNodes, wtSum); 
		nIndices.push_back(numHypernodes - 1); 
		doubleG.doubleNodes[doubleG.ns[i]].hypernodes.push_back(hyperNodes.size() - 1);
	}
	for (int i = 0; i < doubleG.midTs.size(); i++) {
		std::vector<int> midTV = { doubleG.midTs[i] }; 
		hyperNodes.push_back(hyperNode(midTV, MIDT, 0)); 
		numHypernodes += 1; numTerminals += 1; 
		hyperNodes[numHypernodes - 1].assignHyperNodeWt(doubleG.doubleNodes, wtSum); 
		doubleG.doubleNodes[doubleG.midTs[i]].hypernodes.push_back(hyperNodes.size() - 1);
	}
	std::vector<int> piV = { doubleG.getPiIndex() }; 
	hyperNodes.push_back(hyperNode(piV, PINODE, 0));
	numHypernodes += 1; 
	numTerminals += 1; 
	hyperNodes[numHypernodes - 1].assignHyperNodeWt(doubleG.doubleNodes, wtSum); 
	piIndex = numHypernodes - 1; 
	doubleG.doubleNodes[doubleG.getPiIndex()].hypernodes.push_back(hyperNodes.size() - 1);
	//Create hypernodes 
	for (int i = 0; i < globalHypernodes.size(); i++) {
		std::vector<int> subnodes = globalHypernodes[i].doubleSubnodes; 
		std::vector<int> doubleSubnodes; 
		int side = globalHypernodes[i].getSide(); //All subnodes must be cuts or fills
		if (side == 1) {
			for (int j = 0; j < subnodes.size(); j++) {
				doubleSubnodes.push_back(doubleG.nodeToDoubleMapping[subnodes[j]][0]);
				doubleG.doubleNodes[doubleG.nodeToDoubleMapping[subnodes[j]][0]].hypernodes.push_back(hyperNodes.size());
			}
			hyperNodes.push_back(hyperNode(doubleSubnodes, HYPERNODE, 1));
			numHypernodes += 1; 
			hyperNodes[numHypernodes - 1].assignHyperNodeWt(doubleG.doubleNodes, wtSum);
		}
		else { // side must be -1
			for (int j = 0; j < subnodes.size(); j++) {
				doubleSubnodes.push_back(doubleG.nodeToDoubleMapping[subnodes[j]][2]);
				doubleG.doubleNodes[doubleG.nodeToDoubleMapping[subnodes[j]][2]].hypernodes.push_back(hyperNodes.size());
			}
			hyperNodes.push_back(hyperNode(doubleSubnodes, HYPERNODE, -1)); 
			numHypernodes += 1;
			hyperNodes[numHypernodes - 1].assignHyperNodeWt(doubleG.doubleNodes, wtSum);
		}
	}

	constructHyperEdges();
}

//Find 2 * sum of node weights, to ensure hypergraph nodes always have positive weights
int64_t getWtSum(std::vector<node> & nodes) {
	int64_t nodeSum = 0;
	for (int i = 0; i < nodes.size(); i++) {
		if (nodes[i].valid) {
			if (((int)nodes[i].type) == 2 || ((int)nodes[i].type) == 3) {
				nodeSum += (2 * abs((int64_t)nodes[i].labelCost));
			}
		}
	}
	cout.precision(14);
	nodeSum = std::min(nodeSum, (int64_t)(WMAX / (nodes.size() * 2)));
	return nodeSum;
}

Inst loadNWSTP(hyperGraph & hyperG) {
	Inst inst;
	inst.offset = 0; //double w, prize;
	inst.r = -1;
	inst.isInt = true;
	inst.resizeNodes(hyperG.numHypernodes); inst.resizeEdges(2*hyperG.hyperEdges.size()); inst.t = hyperG.numTerminals; std::vector<weight_t> nw(hyperG.numHypernodes, 0.0);
	int ij = 0;
	for (int i = 0; i < hyperG.hyperEdges.size(); i++) { //Add edges
		inst.newArc(hyperG.hyperEdges[i][0], hyperG.hyperEdges[i][1], ij, ij + 1, (weight_t)0); ij++; 
		inst.newArc(hyperG.hyperEdges[i][1], hyperG.hyperEdges[i][0], ij, ij - 1, (weight_t)0); ij++;
	}
	int assignedT = 0;
	for (int i = 0; i < hyperG.numHypernodes; i++) {
		if (hyperG.hyperNodes[i].getType() == CORE || hyperG.hyperNodes[i].getType() == N || hyperG.hyperNodes[i].getType() == MIDT || hyperG.hyperNodes[i].getType() == PINODE) {
			inst.T[i] = true; 
			inst.f1[i] = 1; 
			inst.p[i] = WMAX;
		}
		
	}
	
	for (int i = 0; i < inst.n; i++) {
		nw[i] = (weight_t)hyperG.hyperNodes[i].getWeight();
		if (nw[i] == INFINITY) {
			nw[i] = 1;
		}
		for (int ij : inst.din[i]) {
			inst.c[ij] = inst.c[ij] + nw[i];
		}
	}

	for (int i = 0; i < inst.n; i++) {
		if (inst.T[i]) {
			inst.r = i;
			break;
		}
	}

	inst.offset += nw[inst.r];

	for (int i = 0; i < inst.n; i++) {
		inst.din[i].shrink_to_fit();
		inst.dout[i].shrink_to_fit();
	}

	inst.t = 0;
	for (int i = 0; i < inst.n; i++) {
		if (inst.p[i] > 0 || inst.f1[i]) inst.t++;
		inst.T[i] = (inst.p[i] != 0);
		
	}
	stats.initial = inst.countInstSize();

	// associates the anti-parallel arc to each arc if it exists (-1 otherwise)
	int antiparallelArcs = 0;
	if (inst.isAsym) {
		for (int ij = 0; ij < inst.m; ij++)
			inst.opposite[ij] = -1;
		for (int ij = 0; ij < inst.m; ij++) {
			if (inst.opposite[ij] != -1) continue;
			const int i = inst.tail[ij];
			const int j = inst.head[ij];
			for (int jk : inst.dout[j]) {
				const int k = inst.head[jk];
				if (k == i) {
					inst.opposite[ij] = jk;
					inst.opposite[jk] = ij;
					break;
				}
			}
		}
		for (int ij = 0; ij < inst.m; ij++) {
			if (inst.opposite[ij] == -1) continue;
			antiparallelArcs++;
		}
	}
	else {
		antiparallelArcs = inst.m;
	}

	// compute the ratio of bidirected edges / arcs that have an antiparallel counterpart
	stats.bidirect = (double)antiparallelArcs / (2 * inst.m - antiparallelArcs);
	if (params.bigM) {
		if (inst.r == -1) {
			inst = inst.createRootedBigMCopy();
		}
		else {
			params.bigM = false;
		}
	}
	return inst;
}

void setSteinerParams(int timelimit, int bbtime) {
	params.timelimit = timelimit;
	params.memlimit = 10000000000;
	params.cutoff = -1;
	params.cutoffopt = false;
	params.absgap = 0;
	params.bbinfofreq = 100;
	params.branchtype = 0;
	params.nodeselect = 0;
	params.daiterations = 10;
	params.perturbedheur = true;
	params.nodelimit = -1;
	params.daeager = 1.25;
	params.dasat = -1;
	params.daguide = true;
	params.seed = 0;

	params.rootonly = false;
	params.heuronly = false;
	params.initheur = true;
	params.initprep = true;
	params.redrootonly = false;
	params.bigM = false;
	params.semiBigM = false;
	params.eagerroot = false;

	params.heureps = -1;
	params.heurroots = 1;
	params.heurbb = true;
	params.heurbbtime = timelimit;
	params.heursupportG = true;

	// reduction tests
	params.d1 = true;
	params.d2 = true;
	params.ma = true;
	params.ms = true;
	params.ss = true;
	params.lc = true;
	params.nr = true;
	params.boundbased = true;

	int p = 10;
	params.precision = 1;

	for (int i = 0; i < p; i++) {
		params.precision *= 10;
	}
	params.type = "nwstp";
}

void solveSteinerTree(hyperGraph & hyperG, std::vector<node> & nodes, std::vector< std::vector<int> > & slnEdges, int steinerTime, int bbTime) {
	setSteinerParams(steinerTime, bbTime);
	double bestKnown = -1; 
	ProcStatus::setMemLimit(params.memlimit);
	params.timelimit = steinerTime;
	srand(params.seed);
	Timer tLoad(true);
	Inst nwstp = loadNWSTP(hyperG);
	if (params.bigM) {
		if (nwstp.r == -1) {
			nwstp = nwstp.createRootedBigMCopy();
		}
		else {
			params.bigM = false;
		}
	}
	BBTree bbtree(nwstp);
	if (params.initprep) {
		Timer tPrep(true);
		
		bbtree.initPrep();
		stats.prep = nwstp.countInstSize();
		stats.preptime = tPrep.elapsed().getSeconds();
	}
	if (params.cutoff > 0.0)
		bbtree.setCutUp(params.cutoff);
	if (params.cutoffopt)
		bbtree.setCutUp(bestKnown);

	bbtree.setBestKnown(bestKnown);
	// solve
	if (params.heuronly) {
		bbtree.initHeur();
	}
	else if (params.rootonly) {
		
		bbtree.initHeur();

		if (params.timelimit >= 0)
			bbtree.setTimeLim(max(0.0, params.timelimit));
		if (params.nodelimit >= 0)
			bbtree.setNodeLim(params.nodelimit);
		bbtree.processRoots();
	}

	else {
		bbtree.initHeur();
		if (params.timelimit >= 0) {
			bbtree.setTimeLim(max(0.0, params.timelimit));
		}
		if (params.nodelimit >= 0)
			bbtree.setNodeLim(params.nodelimit);
		bbtree.solve();
	}
	stats.bbnodes = bbtree.getNnodes();
	stats.isInt = nwstp.isInt;
	stats.isAsym = nwstp.isAsym;

	// timing
	stats.bbtime = bbtree.getTime();
	stats.timeBest = bbtree.getTimeBest();
	stats.roottime = bbtree.getRootTime();
	stats.heurtime = bbtree.getHeurTime();
	stats.heurbbtime = bbtree.getHeurBBTime();

	// bounds
	stats.ub = format(bbtree.getUB(), nwstp);
	stats.lb = format(bbtree.getLB(), nwstp);
	stats.gap = gapP(stats.lb, stats.ub);
	
	// root
	stats.rootub = format(bbtree.getRootUB(), nwstp);
	stats.rootlb = format(bbtree.getRootLB(), nwstp);
	stats.rootgap = gapP(stats.rootlb, stats.rootub);
	stats.roots = bbtree.getNroots();
	stats.oroots = bbtree.getNrootsOpen();
	stats.proots = bbtree.getNrootsProcessed();
	
	if (bbtree.getState() == BBTree::State::BB_MEMLIMIT)
		stats.memout = 1;

	// get running time and backmapped solution
	stats.time = Timer::total.elapsed().getSeconds();
	auto S = bbtree.getInc1();
	weight_t ub = S.obj;
	weight_t lb = bbtree.getLB();

	// check if value from bound file matches computed optimum value
	const bool match = (abs(format(ub, nwstp) - bestKnown) < 1e-5);
	// validates solution
	stats.valid = S.validate();
	auto inst = bbtree.getInst1(); auto sln = bbtree.getInc1();
	int slnSize = 0;
	for (int ij = 0; ij < inst.m; ij++) {
		if (sln.arcs[ij]) {
			slnSize += 1;
			slnEdges.push_back({ hyperG.hyperEdges[ij / 2][0] , hyperG.hyperEdges[ij / 2][1] });
		}
	}
	
}

void getCoreG(Graph & G, std::vector<node> & nodes, int numNodes, Graph & coreG, map<std::vector<int>, int> & edgeWts) {
	for (int i = 0; i < numNodes; i++) {
		add_vertex(coreG);
	}
	typedef boost::graph_traits<Graph>::edge_iterator edge_iter;
	edge_iter ei, ei_end;
	for (tie(ei, ei_end) = edges(G); ei != ei_end; ++ei) {
		int v1 = (int)ei->m_source; int v2 = (int)ei->m_target;
		if ((int)nodes[v1].type == CORE && (int)nodes[v2].type == CORE) {
			if (edgeWts[{v1, v2}] == 1) {
				add_edge(v1, v2, coreG);
			}
		}
	}
}

void getNG(Graph & G, std::vector<node> & nodes, int numNodes, Graph & nG) {
	for (int i = 0; i < numNodes; i++) {
		add_vertex(nG);
	}
	typedef boost::graph_traits<Graph>::edge_iterator edge_iter;
	edge_iter ei, ei_end;
	for (tie(ei, ei_end) = edges(G); ei != ei_end; ++ei) {
		int v1 = (int)ei->m_source; int v2 = (int)ei->m_target;
		if ((int)nodes[v1].type == N && (int)nodes[v2].type == N) {
			add_edge(v1, v2, nG);
		}
	}
}

//Check if terminals connected outside this cluster
std::vector< std::vector<int> > getTermsConnectedOutsideCluster(Graph & inG, int type, std::vector<node> & nodes, int numNodes, std::vector<int> & clusterGlobalIndices, hyperGraph & hyp) {
	std::vector<bool> inCluster(numNodes, false); Graph g = inG; std::vector<bool> termInComp(numNodes, false); map<int, int> nodeToHyper;
	if (type == CORE) {
		for (int i = 0; i < hyp.coreIndices.size(); i++) {
			//For each term index in hypergraph, check if in same component in original graph
			termInComp[hyp.doubleG.doubleNodes[hyp.hyperNodes[hyp.coreIndices[i]].doubleSubnodes[0]].origNode.index] = true;
			nodeToHyper[hyp.doubleG.doubleNodes[hyp.hyperNodes[hyp.coreIndices[i]].doubleSubnodes[0]].origNode.index] = hyp.coreIndices[i];
		}
		typedef boost::graph_traits<Graph>::edge_iterator edge_iter;
		edge_iter ei, ei_end;
		for (tie(ei, ei_end) = edges(inG); ei != ei_end; ++ei) {
			int v1 = (int)ei->m_source; int v2 = (int)ei->m_target;
			if ((int)nodes[v1].type == CORE && (int)nodes[v2].type == CORE) {
				remove_edge(v1, v2, g);
			}
		}

	}
	else { //type must be neighborhood
		for (int i = 0; i < hyp.nIndices.size(); i++) {
			//For each term index in hypergraph, check if in same component in original graph
			termInComp[hyp.doubleG.doubleNodes[hyp.hyperNodes[hyp.nIndices[i]].doubleSubnodes[0]].origNode.index] = true; 
			nodeToHyper[hyp.doubleG.doubleNodes[hyp.hyperNodes[hyp.nIndices[i]].doubleSubnodes[0]].origNode.index] = hyp.nIndices[i];
		}
		typedef boost::graph_traits<Graph>::edge_iterator edge_iter;
		edge_iter ei, ei_end;
		for (tie(ei, ei_end) = edges(inG); ei != ei_end; ++ei) {
			int v1 = (int)ei->m_source; int v2 = (int)ei->m_target;
			if ((int)nodes[v1].type == N && (int)nodes[v2].type == N) {
				remove_edge(v1, v2, g);
			}
		}
	}

	for (int i = 0; i < clusterGlobalIndices.size(); i++) {
		if (nodes[clusterGlobalIndices[i]].type == CORE || nodes[clusterGlobalIndices[i]].type == N) {
			continue;
		}
		inCluster[clusterGlobalIndices[i]] = true;
		clear_vertex(clusterGlobalIndices[i], g);
	}
	
	std::vector< std::vector<int> > termComponents;
	std::vector<int> nodeToComp(numNodes);
	int n = (int)boost::connected_components(g, &nodeToComp[0]);
	int numComps = 0; std::vector<int> isCompIndex(n, -1);
	for (int i = 0; i < numNodes; i++) {
		if (termInComp[i]) {
			if (isCompIndex[nodeToComp[i]] == -1) {

				isCompIndex[nodeToComp[i]] = numComps;
				std::vector<int> newComp = { nodeToHyper[i] };
				termComponents.push_back(newComp);
				numComps += 1;
			}
			else {
				termComponents[isCompIndex[nodeToComp[i]]].push_back(nodeToHyper[i]);
			}

		}
	}
	return termComponents;
}



//Check if terminals connected outside this cluster
std::vector< std::vector<int> > getTermsConnectedOutsideCluster(Graph & inG, int type, std::vector<node> & nodes, int numNodes, std::vector<int> & clusterGlobalIndices,
	hyperGraph & hyp, map<int, int> & nodeToCompIn) {
	std::vector<bool> inCluster(numNodes, false); Graph g = inG; std::vector<bool> termInComp(numNodes, false); map<int, int> nodeToHyper;
	

	for (int i = 0; i < clusterGlobalIndices.size(); i++) {
		if (nodes[clusterGlobalIndices[i]].type == CORE || nodes[clusterGlobalIndices[i]].type == N) {
			if (nodeToCompIn.find(clusterGlobalIndices[i]) == nodeToCompIn.end()) {
				continue;
			}
		}
		
		inCluster[clusterGlobalIndices[i]] = true;
		clear_vertex(clusterGlobalIndices[i], g);
	}
	
	for (int i = 0; i < hyp.hyperEdges.size(); i++) {
		if ((hyp.hyperNodes[hyp.hyperEdges[i][0]].getType() == CORE && hyp.hyperNodes[hyp.hyperEdges[i][1]].getType() == CORE)
			||
			(hyp.hyperNodes[hyp.hyperEdges[i][0]].getType() == N && hyp.hyperNodes[hyp.hyperEdges[i][1]].getType() == N)
			) {
			int v1 = hyp.doubleG.doubleNodes[hyp.hyperNodes[hyp.hyperEdges[i][0]].doubleSubnodes[0]].origNode.index;
			int v2 = hyp.doubleG.doubleNodes[hyp.hyperNodes[hyp.hyperEdges[i][1]].doubleSubnodes[0]].origNode.index;
		}
	}


	std::vector< std::vector<int> > termComponents;
	std::vector<int> nodeToComp(numNodes);
	int n = (int)boost::connected_components(g, &nodeToComp[0]);
	int numComps = 0; std::vector<int> isCompIndex(n, -1);
	for (int i = 0; i < numNodes; i++) {
		if (termInComp[i]) {
			if (!inCluster[i]) {
				if (isCompIndex[nodeToComp[i]] == -1) {

					isCompIndex[nodeToComp[i]] = numComps;
					std::vector<int> newComp = { nodeToHyper[i] };
					termComponents.push_back(newComp);
					numComps += 1;
				}
				else {
					termComponents[isCompIndex[nodeToComp[i]]].push_back(nodeToHyper[i]);
				}
			}
		}
	}
	return termComponents;
}


std::vector<std::vector<std::vector<int>>> getPartitions(const std::vector<int>& elements) {
	std::vector<std::vector<std::vector<int>>> fList;

	std::vector<std::vector<int>> lists;
	std::vector<int> indexes(elements.size(), 0);
	lists.emplace_back(std::vector<int>());
	lists[0].insert(lists[0].end(), elements.begin(), elements.end());

	int counter = -1;

	for (;;) {
		counter += 1;
		fList.emplace_back(lists);

		int i, index;
		bool obreak = false;
		for (i = indexes.size() - 1;; --i) {
			if (i <= 0) {
				obreak = true;
				break;
			}
			index = indexes[i];
			lists[index].erase(lists[index].begin() + lists[index].size() - 1);
			if (lists[index].size() > 0)
				break;
			lists.erase(lists.begin() + index);
		}
		if (obreak) break;

		++index;
		if (index >= lists.size())
			lists.emplace_back(std::vector<int>());
		for (; i < indexes.size(); ++i) {
			indexes[i] = index;
			lists[index].emplace_back(elements[i]);
			index = 0;
		}

	}
	return fList;
}

void getAllCombinations(const std::vector< std::vector<int> > & comboVecs, int index, std::vector<int> combo, std::vector< std::vector<int> > & combinations) {
	if (index >= comboVecs.size()) {
		combinations.push_back(combo);
		return;
	}
	for (int i = 0; i < comboVecs[index].size(); i++) {
		std::vector<int> newCombo = combo;
		newCombo.push_back(comboVecs[index][i]);
		getAllCombinations(comboVecs, index + 1, newCombo, combinations);
		
	}
}

int findComponentsFGNT(Graph & G, std::vector<node> & nodes, int numNodes, map<std::vector<int>, int> & edgeWt) {
	
	//If in fg, core. Else if in bg, neighborhood
	Graph fgG;
	for (int i = 0; i < numNodes; i++) {
		add_vertex(fgG);
	}
	typedef boost::graph_traits<Graph>::edge_iterator edge_iter;
	edge_iter ei, ei_end;
	for (tie(ei, ei_end) = edges(G); ei != ei_end; ++ei) {
		int v1 = (int)ei->m_source; int v2 = (int)ei->m_target;
		if ((nodes[v1].type == CORE || nodes[v1].type == CUT) && (nodes[v2].type == CORE || nodes[v2].type == CUT) && edgeWt[{v1,v2}] == 1) {
			add_edge(v1, v2, fgG);
		}
	}
	std::vector< std::vector<int> > components;
	std::vector<int> nodeToComp(numNodes);
	int n = (int)boost::connected_components(fgG, &nodeToComp[0]);
	int numComps = 0; std::vector<int> isCompIndex(n, -1);

	for (int i = 0; i < numNodes; i++) {
		if (nodes[i].valid) {
			if (((int)nodes[i].type) == CORE || ((int)nodes[i].type) == CUT) {
				if (isCompIndex[nodeToComp[i]] == -1) {

					isCompIndex[nodeToComp[i]] = numComps;
					std::vector<int> newComp = { i };
					components.push_back(newComp);
					numComps += 1;
				}
				else {
					components[isCompIndex[nodeToComp[i]]].push_back(i);
				}

			}
		}
	}
	return components.size();
}

int findComponentsBGNT(Graph & G, std::vector<node> & nodes, int numNodes) {
	
	//If in fg, core. Else if in bg, neighborhood
	Graph bgG;
	for (int i = 0; i < numNodes; i++) {
		add_vertex(bgG);
	}
	typedef boost::graph_traits<Graph>::edge_iterator edge_iter;
	edge_iter ei, ei_end;
	for (tie(ei, ei_end) = edges(G); ei != ei_end; ++ei) {
		int v1 = (int)ei->m_source; int v2 = (int)ei->m_target;
		if ((nodes[v1].type == N || nodes[v1].type == FILL) && (nodes[v2].type == N || nodes[v2].type == FILL)) {
			add_edge(v1, v2, bgG);
		}
	}
	std::vector< std::vector<int> > components;
	std::vector<int> nodeToComp(numNodes);
	int n = (int)boost::connected_components(bgG, &nodeToComp[0]);
	int numComps = 0; std::vector<int> isCompIndex(n, -1);

	for (int i = 0; i < numNodes; i++) {
		if (nodes[i].valid) {
			if (((int)nodes[i].type) == N || ((int)nodes[i].type) == FILL) {
				if (isCompIndex[nodeToComp[i]] == -1) {

					isCompIndex[nodeToComp[i]] = numComps;
					std::vector<int> newComp = { i };
					components.push_back(newComp);
					numComps += 1;
				}
				else {
					components[isCompIndex[nodeToComp[i]]].push_back(i);
				}

			}
		}
	}
	return components.size();
}

int64_t findNodeCost(int index, int side, std::vector<node> & nodes, int numNodes, Graph & G, Graph & fgG, Graph & bgG, int h0, int h2, map<std::vector<int>, int> & edgeWt) {
	std::vector<node> nodesTemp = nodes; int64_t c = 1000000000;
	Graph fgGTemp = fgG; Graph bgGTemp = bgG;
	if (side == 1) {

		
		nodesTemp[index].type = CORE; nodesTemp[index].inFg = 1; 
		if (nodes[index].type == FILL) {
			auto neighboursT = adjacent_vertices(index, G);
			for (auto u : make_iterator_range(neighboursT)) {
				if (nodes[u].type == CUT) {
					nodesTemp[u].type = CORE;
					clear_vertex(u, bgGTemp);
				}
			}
		}
		int newH0 = findComponents(fgGTemp, nodesTemp, numNodes, true);
		clear_vertex(index, bgGTemp);
		int newH2 = findComponents(bgGTemp, nodesTemp, numNodes, false);
		int64_t labelCost = -nodes[index].labelCost;
		int deltaE = nodes[index].v - nodes[index].e + nodes[index].f - nodes[index].c;
		if (nodes[index].type == CUT) {
			labelCost = 0;
		}
		int64_t cost = (int64_t)(((newH0 - h0) + (newH2 - h2)) * c +  labelCost);
		return cost;
	}
	else {
		nodesTemp[index].type = N;
		nodesTemp[index].inFg = 0;
		if (nodes[index].type == CUT) {
			auto neighboursT = adjacent_vertices(index, G);
			for (auto u : make_iterator_range(neighboursT)) {
				if (nodes[u].type == FILL) {
					nodesTemp[u].type =  N;
					clear_vertex(u, fgGTemp);
				}
			}
		}
		clear_vertex(index, fgGTemp);
		int newH0 = findComponents(fgGTemp, nodesTemp, numNodes, true);
		int newH2 = findComponents(bgGTemp, nodesTemp, numNodes, false);
		int64_t labelCost = nodes[index].labelCost;
		int deltaE = nodes[index].v - nodes[index].e + nodes[index].f - nodes[index].c;
		if (nodes[index].type == FILL) {
			labelCost = 0;
		}
		int64_t cost = (int64_t)(((newH0 - h0) + (newH2 - h2)) * c + labelCost);
		return cost;

	}
}

int findLocalNodeToFix(std::vector<node> & nodes, int numNodes, map<int, int> & globalToLocal, std::vector<bool> & inFg, std::vector<bool> & inBg, hyperGraph & hypG,
	std::vector< std::vector<int> > & slnEdges, Graph & G, Graph & fgG, Graph & bgG, int fgComps, int bgComps, int & termSide,
	 map<std::vector<int>, int> & edgeWt, std::vector<int> & clusterLocalNodesGlobalIndices
	) {
	int violatingNode = -1; 
	int64_t lowestCost = WMAX;
	for (int i = 0; i < slnEdges.size(); i++) {
		int v1 = slnEdges[i][0];
		switch (hypG.hyperNodes[v1].getType()) {
			case CORE:
				inFg[globalToLocal[hypG.doubleG.doubleNodes[hypG.hyperNodes[v1].doubleSubnodes[0]].origNode.index]] = true;
				break;
			case N:
				inBg[globalToLocal[hypG.doubleG.doubleNodes[hypG.hyperNodes[v1].doubleSubnodes[0]].origNode.index]] = true;
				break;
			case HYPERNODE:
				if (hypG.hyperNodes[v1].getSide() == 1) {
					for (int j = 0; j < hypG.hyperNodes[v1].doubleSubnodes.size(); j++) {
						int globalI = hypG.doubleG.doubleNodes[hypG.hyperNodes[v1].doubleSubnodes[j]].origNode.index;
						inFg[globalToLocal[globalI]] = true;
						if (inBg[globalToLocal[globalI]]) {
							int64_t cost1 = findNodeCost(globalI, 1, nodes, numNodes, G, fgG, bgG, fgComps, bgComps, edgeWt);
							if (cost1 < lowestCost) {
								violatingNode = globalToLocal[globalI]; 
								lowestCost = cost1; 
								termSide = 1;
							}
							int64_t cost0 = findNodeCost(globalI, 0, nodes, numNodes, G, fgG, bgG, fgComps, bgComps, edgeWt);
							if (cost0 < lowestCost) {
								violatingNode = globalToLocal[globalI];
								lowestCost = cost0;
								termSide = 0;
							}
						}
					}
				}
				else { //Must be background side hypernode
					for (int j = 0; j < hypG.hyperNodes[v1].doubleSubnodes.size(); j++) {

						int globalI = hypG.doubleG.doubleNodes[hypG.hyperNodes[v1].doubleSubnodes[j]].origNode.index;
						inBg[globalToLocal[globalI]] = true;
						if (inFg[globalToLocal[globalI]]) { // fgCompNT, bgCompNT,
							int64_t cost1 = findNodeCost(globalI, 1, nodes, numNodes, G, fgG, bgG, fgComps, bgComps,  edgeWt);
							if (cost1 < lowestCost) {
								violatingNode = globalToLocal[globalI];
								lowestCost = cost1;
								termSide = 1;
							}
							int64_t cost0 = findNodeCost(globalI, 0, nodes, numNodes, G, fgG, bgG, fgComps, bgComps, edgeWt);
							if (cost0 < lowestCost) {
								violatingNode = globalToLocal[globalI];
								lowestCost = cost0;
								termSide = 0;
							}
						}
					}
				}
				break;
			default:
				break;
		}
		int v2 = slnEdges[i][1];
		switch (hypG.hyperNodes[v2].getType()) {
			case CORE:
				inFg[globalToLocal[hypG.doubleG.doubleNodes[hypG.hyperNodes[v2].doubleSubnodes[0]].origNode.index]] = true;
				break;
			case N:
				inBg[globalToLocal[hypG.doubleG.doubleNodes[hypG.hyperNodes[v2].doubleSubnodes[0]].origNode.index]] = true;
				break;
			case HYPERNODE:
				if (hypG.hyperNodes[v2].getSide() == 1) {
					for (int j = 0; j < hypG.hyperNodes[v2].doubleSubnodes.size(); j++) {
						int globalI = hypG.doubleG.doubleNodes[hypG.hyperNodes[v2].doubleSubnodes[j]].origNode.index;
						inFg[globalToLocal[globalI]] = true;
						if (inBg[globalToLocal[globalI]]) {
							int64_t cost1 = findNodeCost(globalI, 1, nodes, numNodes, G, fgG, bgG, fgComps, bgComps, edgeWt);
							if (cost1 < lowestCost) {
								violatingNode = globalToLocal[globalI];
								lowestCost = cost1;
								termSide = 1;
							}
							int64_t cost0 = findNodeCost(globalI, 0, nodes, numNodes, G, fgG, bgG, fgComps, bgComps,  edgeWt);
							if (cost0 < lowestCost) {
								violatingNode = globalToLocal[globalI];
								lowestCost = cost0;
								termSide = 0;
							}
						}
					}
				}
				else { //Must be background side hypernode
					for (int j = 0; j < hypG.hyperNodes[v2].doubleSubnodes.size(); j++) {
						int globalI = hypG.doubleG.doubleNodes[hypG.hyperNodes[v2].doubleSubnodes[j]].origNode.index;
						inBg[globalToLocal[globalI]] = true;
						if (inFg[globalToLocal[globalI]]) {
							int64_t cost1 = findNodeCost(globalI, 1, nodes, numNodes, G, fgG, bgG, fgComps, bgComps, edgeWt);
							if (cost1 < lowestCost) {
								violatingNode = globalToLocal[globalI];
								lowestCost = cost1;
								termSide = 1;
							}
							int64_t cost0 = findNodeCost(globalI, 0, nodes, numNodes, G, fgG, bgG, fgComps, bgComps,  edgeWt);
							if (cost0 < lowestCost) {
								violatingNode = globalToLocal[globalI]; 
								lowestCost = cost0; 
								termSide = 0;
							}
						}
					}
				}
				break;
			default:
				break;
			}
	}
	return violatingNode;
}

int findGlobalNodeToFix(std::vector<node> & nodes, int numNodes, std::vector<bool> & inFg, std::vector<bool> & inBg, hyperGraph & hypG, 
	std::vector< std::vector<int> > & slnEdges, Graph & G, Graph & fgG, Graph & bgG, int fgComps, int bgComps, int & termSide,
	map<std::vector<int>, int> & edgeWt, std::vector<bool> & nodeToFix, int64_t & cost
	) {
	int violatingNode = -1; int64_t lowestCost = WMAX;
	for (int i = 0; i < slnEdges.size(); i++) {
		int v1 = slnEdges[i][0];
		switch (hypG.hyperNodes[v1].getType()) {
			case CORE:
				inFg[hypG.doubleG.doubleNodes[hypG.hyperNodes[v1].doubleSubnodes[0]].origNode.index] = true;
				break;
			case N:
				inBg[hypG.doubleG.doubleNodes[hypG.hyperNodes[v1].doubleSubnodes[0]].origNode.index] = true;
				break;
			case HYPERNODE:
				if (hypG.hyperNodes[v1].getSide() == 1) {
					for (int j = 0; j < hypG.hyperNodes[v1].doubleSubnodes.size(); j++) {
						int globalI = hypG.doubleG.doubleNodes[hypG.hyperNodes[v1].doubleSubnodes[j]].origNode.index;
						inFg[globalI] = true;
						if (inBg[globalI]) { 
							if (nodeToFix[globalI]) { continue; } 
							double cost1 = findNodeCost(globalI, 1, nodes, numNodes, G, fgG, bgG, fgComps, bgComps, edgeWt);
							if (cost1 < lowestCost) {
								violatingNode = globalI; lowestCost = cost1; termSide = 1;
							}
							double cost0 = findNodeCost(globalI, 0, nodes, numNodes, G, fgG, bgG, fgComps, bgComps,  edgeWt);
							if (cost0 < lowestCost) {
								violatingNode = globalI; lowestCost = cost0; termSide = 0;
							}
						}
					}
				}
				else { //Must be background side hypernode
					for (int j = 0; j < hypG.hyperNodes[v1].doubleSubnodes.size(); j++) {

						int globalI = hypG.doubleG.doubleNodes[hypG.hyperNodes[v1].doubleSubnodes[j]].origNode.index;
						inBg[globalI] = true;
						if (inFg[globalI]) {
							if (nodeToFix[globalI]) { continue; } 
							double cost1 = findNodeCost(globalI, 1, nodes, numNodes, G, fgG, bgG, fgComps, bgComps,  edgeWt);
							if (cost1 < lowestCost) {
								violatingNode = globalI; lowestCost = cost1; termSide = 1;
							}
							double cost0 = findNodeCost(globalI, 0, nodes, numNodes, G, fgG, bgG, fgComps, bgComps,  edgeWt);
							if (cost0 < lowestCost) {
								violatingNode = globalI; lowestCost = cost0; termSide = 0;
							}
						}
					}
				}
				break;
			default:
				break;
		}
		int v2 = slnEdges[i][1];
		switch (hypG.hyperNodes[v2].getType()) {
			case CORE:
				inFg[hypG.doubleG.doubleNodes[hypG.hyperNodes[v2].doubleSubnodes[0]].origNode.index] = true;
				break;
			case N:
				inBg[hypG.doubleG.doubleNodes[hypG.hyperNodes[v2].doubleSubnodes[0]].origNode.index] = true;
				break;
			case HYPERNODE:
				if (hypG.hyperNodes[v2].getSide() == 1) {
					for (int j = 0; j < hypG.hyperNodes[v2].doubleSubnodes.size(); j++) {
						int globalI = hypG.doubleG.doubleNodes[hypG.hyperNodes[v2].doubleSubnodes[j]].origNode.index;
						inFg[globalI] = true;
						if (inBg[globalI]) {
							if (nodeToFix[globalI]) { continue; }
							double cost1 = findNodeCost(globalI, 1, nodes, numNodes, G, fgG, bgG, fgComps, bgComps, edgeWt);
							if (cost1 < lowestCost) {
								violatingNode = globalI; lowestCost = cost1; termSide = 1;
							}
							double cost0 = findNodeCost(globalI, 0, nodes, numNodes, G, fgG, bgG, fgComps, bgComps, edgeWt);
							if (cost0 < lowestCost) {
								violatingNode = globalI; lowestCost = cost0; termSide = 0;
							}
						}
					}
				}
				else { //Must be background side hypernode
					for (int j = 0; j < hypG.hyperNodes[v2].doubleSubnodes.size(); j++) {
						int globalI = hypG.doubleG.doubleNodes[hypG.hyperNodes[v2].doubleSubnodes[j]].origNode.index;
						inBg[globalI] = true; 
						if (inFg[globalI]) {
							if (nodeToFix[globalI]) { continue; }
							double cost1 = findNodeCost(globalI, 1, nodes, numNodes, G, fgG, bgG, fgComps, bgComps,  edgeWt);
							if (cost1 < lowestCost) {
								violatingNode = globalI; lowestCost = cost1; termSide = 1;
							}
							double cost0 = findNodeCost(globalI, 0, nodes, numNodes, G, fgG, bgG, fgComps, bgComps,edgeWt);
							if (cost0 < lowestCost) {
								violatingNode = globalI; lowestCost = cost0; termSide = 0;
							}
						}
					}
				}
				break;
			default:
				break;
		}
	}
	cost = lowestCost;
	return violatingNode;
}

void getGlobalToLocalCFGraphs(Graph & localG, std::vector< std::vector<int> > & clusterLocalEdgesGlobalIndices, std::vector<int> & clusterLocalNodesGlobalIndices, map<int, int> & globalToLocal, std::vector<node> & nodes) {
	for (int i = 0; i < clusterLocalNodesGlobalIndices.size(); i++) {
		add_vertex(localG);
	}
	
	for (int i = 0; i < clusterLocalEdgesGlobalIndices.size(); i++) {
		int v1 = clusterLocalEdgesGlobalIndices[i][0]; int v2 = clusterLocalEdgesGlobalIndices[i][1];
		if ((nodes[v1].type == CUT || nodes[v1].type == FILL) && (nodes[v2].type == CUT || nodes[v2].type == FILL)) {
			add_edge(globalToLocal[v1], globalToLocal[v2], localG);
		}
	}
}

void getSteinerGroupingsForDecision(std::vector<bool> & steinerDecision, Graph & localG, std::vector<int> & clusterLocalNodesGlobalIndices, std::vector<node> & nodes, 
	std::vector< std::vector<int> > & fgComponents, std::vector< std::vector<int> > & bgComponents) {
	std::vector<bool> cfInFg(clusterLocalNodesGlobalIndices.size(), false); std::vector<bool> cfInBg(clusterLocalNodesGlobalIndices.size(), false);
	Graph fgClusterGraph = localG; Graph bgClusterGraph = localG;
	for (int d = 0; d < steinerDecision.size(); d++) {
		if (nodes[clusterLocalNodesGlobalIndices[d]].type == CORE || nodes[clusterLocalNodesGlobalIndices[d]].type == N) {
			clear_vertex(d, fgClusterGraph);
			clear_vertex(d, bgClusterGraph);
			continue;
		}
		else {
			if (steinerDecision[d]) {
				cfInFg[d] = true; clear_vertex(d, bgClusterGraph);
			}
			else {
				cfInBg[d] = true; clear_vertex(d, fgClusterGraph);
			}
		}
	}

	
	std::vector<int> nodeToCompFg(clusterLocalNodesGlobalIndices.size());
	int nFg = (int)boost::connected_components(fgClusterGraph, &nodeToCompFg[0]);
	int numCompsFg = 0;
	std::vector<int> isCompIndexFg(nFg, -1);
	for (int i = 0; i < clusterLocalNodesGlobalIndices.size(); i++) {
		if (cfInFg[i]) {
			if (isCompIndexFg[nodeToCompFg[i]] == -1) {
				isCompIndexFg[nodeToCompFg[i]] = numCompsFg;
				std::vector<int> newComp = { i };
				fgComponents.push_back(newComp);
				numCompsFg += 1;
			}
			else {
				fgComponents[isCompIndexFg[nodeToCompFg[i]]].push_back(i);
			}

		}
	}

	std::vector<int> nodeToCompBg(clusterLocalNodesGlobalIndices.size());
	int nBg = (int)boost::connected_components(bgClusterGraph, &nodeToCompBg[0]);
	int numCompsBg = 0;
	std::vector<int> isCompIndexBg(nBg, -1);
	for (int i = 0; i < clusterLocalNodesGlobalIndices.size(); i++) {
		if (cfInBg[i]) {
			if (isCompIndexBg[nodeToCompBg[i]] == -1) {
				isCompIndexBg[nodeToCompBg[i]] = numCompsBg;
				std::vector<int> newComp = { i };
				bgComponents.push_back(newComp);
				numCompsBg += 1;
			}
			else {
				bgComponents[isCompIndexBg[nodeToCompBg[i]]].push_back(i);
			}

		}
	}
}

void connectTerminalsExternally(hyperGraph & hypG, Graph & coreGIn, Graph & nGIn, int numNodes, std::vector<int> & clusterLocalNodesGlobalIndices, std::vector< std::vector<int> > & clusterLocalEdgesGlobalIndices, map<std::vector<int>, bool> & edgeExists) {
	 Graph coreG = coreGIn;
	 Graph nG = nGIn;
	for (int i = 0; i < clusterLocalEdgesGlobalIndices.size(); i++) {
		remove_edge(clusterLocalEdgesGlobalIndices[i][0], clusterLocalEdgesGlobalIndices[i][1], coreG);
		remove_edge(clusterLocalEdgesGlobalIndices[i][0], clusterLocalEdgesGlobalIndices[i][1], nG);
	}
	map<int, int> nodeToHyper;
	for (int i = 0; i < hypG.coreIndices.size(); i++) {
		nodeToHyper[hypG.doubleG.doubleNodes[hypG.hyperNodes[hypG.coreIndices[i]].doubleSubnodes[0]].origNode.index] = hypG.coreIndices[i];
	}
	for (int i = 0; i < hypG.nIndices.size(); i++) {
		nodeToHyper[hypG.doubleG.doubleNodes[hypG.hyperNodes[hypG.nIndices[i]].doubleSubnodes[0]].origNode.index] = hypG.nIndices[i];
	}

	std::vector<int> nodeToCompFg(numNodes);
	int nFg = (int)boost::connected_components(coreG, &nodeToCompFg[0]);
	int numCompsFg = 0; std::vector<int> isCompIndexFg(nFg, -1);
	std::vector< std::vector<int> > coreComponents;
	for (int i = 0; i < hypG.coreIndices.size(); i++) {
		int coreNIndex = hypG.doubleG.doubleNodes[hypG.hyperNodes[hypG.coreIndices[i]].doubleSubnodes[0]].origNode.index;
			if (isCompIndexFg[nodeToCompFg[coreNIndex]] == -1) {
				isCompIndexFg[nodeToCompFg[coreNIndex]] = numCompsFg;
				std::vector<int> newComp = { nodeToHyper[coreNIndex] };
				coreComponents.push_back(newComp);
				numCompsFg += 1;
			}
			else {
				coreComponents[isCompIndexFg[nodeToCompFg[coreNIndex]]].push_back(nodeToHyper[coreNIndex]);
			}
	}

	std::vector<int> nodeToCompBg(numNodes);
	int nBg = (int)boost::connected_components(nG, &nodeToCompBg[0]);
	int numCompsBg = 0; 
	std::vector<int> isCompIndexBg(nBg, -1);
	std::vector< std::vector<int> > nComponents;
	for (int i = 0; i < hypG.nIndices.size(); i++) {
		int nNIndex = hypG.doubleG.doubleNodes[hypG.hyperNodes[hypG.nIndices[i]].doubleSubnodes[0]].origNode.index;
		if (isCompIndexBg[nodeToCompBg[nNIndex]] == -1) {
			isCompIndexBg[nodeToCompBg[nNIndex]] = numCompsBg;
			std::vector<int> newComp = { nodeToHyper[nNIndex] } ;
			nComponents.push_back(newComp);
			numCompsBg += 1;
		}
		else {
			nComponents[isCompIndexBg[nodeToCompBg[nNIndex]]].push_back(nodeToHyper[nNIndex]);
		}
	}

	

	for (int i = 0; i < coreComponents.size(); i++) {
		for (int j = 0; j < coreComponents[i].size()-1; j++) {
			std::vector<int> e = {coreComponents[i][j], coreComponents[i][j+1] };
			if (edgeExists.find(e) == edgeExists.end()) {
				hypG.hyperEdges.push_back(e);
				edgeExists[e] = true; edgeExists[{e[1], e[0]}] = true;
			}
		}
	}

	for (int i = 0; i < nComponents.size(); i++) {
		for (int j = 0; j < nComponents[i].size() - 1; j++) {
			std::vector<int> e = { nComponents[i][j], nComponents[i][j + 1] };
			if (edgeExists.find(e) == edgeExists.end()) {
				hypG.hyperEdges.push_back(e); 
				edgeExists[e] = true; edgeExists[{e[1], e[0]}] = true;
			}
		}
	}


}



void solveLocalGraphs(Graph & G, std::vector<node> & nodes, std::vector<node> & newNodes, int numNodes, map< std::vector<int>, int> & edgeWts, int hypernodeSize, int64_t wtSum, int productThresh, tbb::concurrent_vector<hyperNode> & globalHypernodes,
	int localSteinerTime, std::vector< std::vector<int> > & newToOldComp, std::vector<node> & nodesBeforeMerging, int bbTime, int & maxLocalGraphNodes, int & maxLocalGraphEdges) {
	
	Graph fgG, bgG;
	std::vector< std::vector< std::vector<int> > > localEdgesGlobalIndex;
	std::vector< std::vector<int> > localNodesGlobalIndex; newNodes = nodes;
	getLocalGraphClusters(G, nodes, numNodes, fgG, bgG, localEdgesGlobalIndex, localNodesGlobalIndex, edgeWts);
	int numClusters = localNodesGlobalIndex.size();

	//These two graphs used to connect terminals together in local graph that must be connected outside graph
	Graph coreG; 
	getCoreG(G, nodes, numNodes, coreG, edgeWts);
	Graph nG; 
	getNG(G, nodes, numNodes, nG);

	int decisionIndex = 0;
	//Find global components first, so don't need to re-find for each run of pruneLocalImpossibleComplexEdges
	int fgComps = findComponents(fgG, nodes, numNodes, true); int bgComps = findComponents(bgG, nodes, numNodes, false); std::vector<hyperGraph> localHyperGraphs;
	tbb::parallel_for(tbb::blocked_range<int>(0, localNodesGlobalIndex.size()),
		[&](tbb::blocked_range<int> r)
	{
		for (int i = r.begin(); i < r.end(); ++i)
		{
			
		std::vector<bool> steinerDecisions;
		std::vector< std::vector<int> > clusterLocalEdgesGlobalIndices = localEdgesGlobalIndex[i];
		std::vector<int> clusterLocalNodesGlobalIndices = localNodesGlobalIndex[i];

		std::vector< std::vector<int> > complexEdges = clusterLocalEdgesGlobalIndices; 
		map<int, int> localToGlobal; map<int, int> globalToLocal; 
		getCompToLocalIndex(clusterLocalNodesGlobalIndices, localToGlobal, globalToLocal, edgeWts);
		doubleGraph localDoubleG(nodes, clusterLocalNodesGlobalIndices, clusterLocalEdgesGlobalIndices, complexEdges, globalToLocal, edgeWts);
		hyperGraph localHyperG(localDoubleG, hypernodeSize, wtSum);
		maxLocalGraphNodes = max((int)localHyperG.numHypernodes, maxLocalGraphNodes);
		maxLocalGraphEdges = max((int)localHyperG.hyperEdges.size(), maxLocalGraphEdges);
		//If two nodes have to be connected outside cluster, create edge in hypergraph
		map< std::vector<int>, bool> edgeExists;
		for (int i = 0; i < localHyperG.hyperEdges.size(); i++) {
			edgeExists[{localHyperG.hyperEdges[i][0], localHyperG.hyperEdges[i][1]}] = true; edgeExists[{localHyperG.hyperEdges[i][1], localHyperG.hyperEdges[i][0]}] = true;
		}
		connectTerminalsExternally(localHyperG, coreG, nG, numNodes, clusterLocalNodesGlobalIndices, clusterLocalEdgesGlobalIndices, edgeExists);

		//Solve hypergraph for every combination of terminals
		std::vector< std::vector<int> > connectedCores = getTermsConnectedOutsideCluster(fgG, CORE, nodes, numNodes, clusterLocalNodesGlobalIndices, localHyperG); 
		std::vector< std::vector<int> > connectedNs = getTermsConnectedOutsideCluster(bgG, N, nodes, numNodes, clusterLocalNodesGlobalIndices, localHyperG);
		int numCombinations = 1;
		for (int j = 0; j < connectedCores.size(); j++) { numCombinations *= connectedCores[j].size(); }
		for (int j = 0; j < connectedNs.size(); j++) { numCombinations *= connectedNs[j].size(); }
		if (numCombinations > productThresh) {
			//Defer nodes to global graph
			for (int j = 0; j < localHyperG.hyperNodes.size(); j++) {
				if (localHyperG.hyperNodes[j].getType() == HYPERNODE) {
					std::vector<int> hyperNodeGlobalIndices;
					for (int s = 0; s < localHyperG.hyperNodes[j].doubleSubnodes.size(); s++) {
						hyperNodeGlobalIndices.push_back(localDoubleG.doubleNodes[localHyperG.hyperNodes[j].doubleSubnodes[s]].origNode.index);
					}
					std::sort(hyperNodeGlobalIndices.begin(), hyperNodeGlobalIndices.end());
					globalHypernodes.push_back(hyperNode(hyperNodeGlobalIndices, HYPERNODE, localHyperG.hyperNodes[j].getSide()));
				}
			}
		}
		else {
			std::vector< std::vector<std::vector<std::vector<int> > > > cCompPartitions;
			for (int j = 0; j < connectedCores.size(); j++) {
				std::vector<std::vector<std::vector<int> > > corePartitions = getPartitions(connectedCores[j]);
				cCompPartitions.push_back(corePartitions);
			}
			std::vector< std::vector<std::vector<std::vector<int> > > > nCompPartitions;
			for (int j = 0; j < connectedNs.size(); j++) {
				std::vector<std::vector<std::vector<int> > > nPartitions = getPartitions(connectedNs[j]);
				nCompPartitions.push_back(nPartitions);
			}
			std::vector< std::vector<int> > cCompIndices;
			for (int j = 0; j < cCompPartitions.size(); j++) {
				std::vector<int> compPartIndices;
				for (int k = 0; k < cCompPartitions[j].size(); k++) {
					compPartIndices.push_back(k);
				}
				cCompIndices.push_back(compPartIndices);
			}
			std::vector< std::vector<int> > nCompIndices;
			for (int j = 0; j < nCompPartitions.size(); j++) {
				std::vector<int> compPartIndices;
				for (int k = 0; k < nCompPartitions[j].size(); k++) {
					compPartIndices.push_back(k);
				}
				nCompIndices.push_back(compPartIndices);
			}
			
			std::vector<int> coreCombo; std::vector< std::vector<int> > coreCombinations;
			getAllCombinations(cCompIndices, 0, coreCombo, coreCombinations);
			std::vector<int> nCombo; std::vector< std::vector<int> > nCombinations;
			getAllCombinations(nCompIndices, 0, nCombo, nCombinations);
			int totalCombinations = coreCombinations.size() * nCombinations.size(); std::vector< std::vector<bool> > steinerDecisions;
			std::vector<int> combIndices; for (int index = 0; index < totalCombinations; index++) { combIndices.push_back(index); }
			tbb::parallel_for(tbb::blocked_range<int>(0, combIndices.size()),
				[&](tbb::blocked_range<int> range)
			{
				for (int itr = range.begin(); itr < range.end(); ++itr)
				{
					int c = itr / ((int)nCombinations.size());
					int n = itr % ((int)nCombinations.size());
							std::vector<int> cPartitionSelection = coreCombinations[c]; map< std::vector<int>, bool > edgeExistsTemp = edgeExists; //map< std::vector<int>, int > edgeWtsTemp = edgeWts;
							hyperGraph partitionHyperG = localHyperG; std::vector<node> nodesTemp = nodes;
							int fgCompsTemp = fgComps; 
							int bgCompsTemp = bgComps; 
							Graph tempG = G;  
							Graph fgGTemp = fgG; 
							Graph bgGTemp = bgG; 
							std::vector< std::vector<int> > termEdges; 
							std::vector<int> connTermIndices;
							for (int p = 0; p < cPartitionSelection.size(); p++) {
								std::vector<std::vector<int> > componentPartition = cCompPartitions[p][cPartitionSelection[p]];
								for (int g = 0; g < componentPartition.size(); g++) {
									std::vector<int> group = componentPartition[g];
									for (int gi = 0; gi < group.size() - 1; gi++) {
										if (edgeExistsTemp.find({ group[gi], group[gi + 1] }) == edgeExistsTemp.end()) {
											partitionHyperG.hyperEdges.push_back({ group[gi], group[gi + 1] }); termEdges.push_back({ group[gi], group[gi + 1] }); connTermIndices.push_back(group[gi]); connTermIndices.push_back(group[gi + 1]);
											edgeExistsTemp[{ group[gi + 1], group[gi] }] = true; edgeExistsTemp[{ group[gi], group[gi + 1] }] = true;
											
										}
									}
								}
							}
							std::vector<int> nPartitionSelection = nCombinations[n];
							for (int p = 0; p < nPartitionSelection.size(); p++) {
								std::vector<std::vector<int> > componentPartition = nCompPartitions[p][nPartitionSelection[p]];
								for (int g = 0; g < componentPartition.size(); g++) {
									std::vector<int> group = componentPartition[g];
									for (int gi = 0; gi < group.size() - 1; gi++) {
										if (edgeExistsTemp.find({ group[gi], group[gi + 1] }) == edgeExistsTemp.end()) {
											partitionHyperG.hyperEdges.push_back({ group[gi], group[gi + 1] }); termEdges.push_back({ group[gi], group[gi + 1] }); termEdges.push_back({ group[gi], group[gi + 1] }); connTermIndices.push_back(group[gi]); connTermIndices.push_back(group[gi + 1]);
											edgeExistsTemp[{ group[gi + 1], group[gi] }] = true; edgeExistsTemp[{ group[gi], group[gi + 1] }] = true;
										}
									}
								}
							}

							bool violating = true;
							while (violating) {

								std::vector< std::vector<int> > slnEdges; hyperGraph currentHyperG = partitionHyperG;

								solveSteinerTree(currentHyperG, nodesTemp, slnEdges, localSteinerTime, bbTime);
								violating = false; 
								std::vector<bool> inFg(clusterLocalNodesGlobalIndices.size(), false); 
								std::vector<bool> inBg(clusterLocalNodesGlobalIndices.size(), false);
								int termSide;
								int newTerminal = findLocalNodeToFix(nodesTemp, numNodes, globalToLocal, inFg, inBg, currentHyperG, slnEdges, G, fgGTemp, bgGTemp, fgCompsTemp, bgCompsTemp, termSide, edgeWts, clusterLocalNodesGlobalIndices);
								if (newTerminal != -1) {
									newTerminal = clusterLocalNodesGlobalIndices[newTerminal];
									violating = true;
									if (termSide == 1) { //Terminal set to the foreground
										if (nodesTemp[newTerminal].type == FILL) { //If type fill sent to FG, adjacent cuts must be sent to FG as well
											auto neighboursT = adjacent_vertices(newTerminal, G);
											for (auto u : make_iterator_range(neighboursT)) {
												if (nodesTemp[u].type == CUT) {

													nodesTemp[u].type = CORE;
												}
											}
										}
										nodesTemp[newTerminal].type = CORE; nodesTemp[newTerminal].inFg = 1;
									}
									else {
										if (nodesTemp[newTerminal].type == CUT) { //If type cut sent to BG, adjacent fills must be sent to BG as well
											auto neighboursT = adjacent_vertices(newTerminal, G);
											for (auto u : make_iterator_range(neighboursT)) {
												if (nodesTemp[u].type == FILL) {
													nodesTemp[u].type = N;
												}
											}
										}
										nodesTemp[newTerminal].type = N; nodesTemp[newTerminal].inFg = 0;
									}
									removeCAndNEdges(tempG, nodesTemp); 
									updateGraphs(tempG, fgGTemp, bgGTemp, nodesTemp, edgeWts); 
									fgCompsTemp = findComponents(fgGTemp, nodesTemp, numNodes, true); 
									int bgCompsTemp = findComponents(bgGTemp, nodesTemp, numNodes, false);
									doubleGraph localDoubleGTemp(nodesTemp, clusterLocalNodesGlobalIndices, clusterLocalEdgesGlobalIndices, complexEdges, globalToLocal, edgeWts);
									partitionHyperG = hyperGraph(localDoubleGTemp, hypernodeSize, wtSum);
									//Add edges between terminals using indices of updated partition graph
									std::sort(connTermIndices.begin(), connTermIndices.end());
									connTermIndices.erase(unique(connTermIndices.begin(), connTermIndices.end()), connTermIndices.end()); 
									map<int, int> connTermIndicesMapping;
									for (int j = 0; j < connTermIndices.size(); j++) {
										connTermIndicesMapping[localHyperG.doubleG.doubleNodes[localHyperG.hyperNodes[connTermIndices[j]].doubleSubnodes[0]].origNode.index] = -1;
									}
									for (int j = 0; j < partitionHyperG.coreIndices.size(); j++) {
										int localI = localDoubleGTemp.doubleNodes[partitionHyperG.hyperNodes[partitionHyperG.coreIndices[j]].doubleSubnodes[0]].origNode.index;
										if (connTermIndicesMapping.find(localI) != connTermIndicesMapping.end()) {
											connTermIndicesMapping[localI] = partitionHyperG.coreIndices[j];
										}
									}
									for (int j = 0; j < partitionHyperG.nIndices.size(); j++) {
										int localI = localDoubleGTemp.doubleNodes[partitionHyperG.hyperNodes[partitionHyperG.nIndices[j]].doubleSubnodes[0]].origNode.index;
										if (connTermIndicesMapping.find(localI) != connTermIndicesMapping.end()) {
											connTermIndicesMapping[localI] = partitionHyperG.nIndices[j];
										}
									}
									for (int j = 0; j < termEdges.size(); j++) {
										partitionHyperG.hyperEdges.push_back({
										connTermIndicesMapping[localHyperG.doubleG.doubleNodes[localHyperG.hyperNodes[termEdges[j][0]].doubleSubnodes[0]].origNode.index],
											connTermIndicesMapping[localHyperG.doubleG.doubleNodes[localHyperG.hyperNodes[termEdges[j][1]].doubleSubnodes[0]].origNode.index]
											});
									}

								}
								else {
									steinerDecisions.push_back(inFg);
								}
							}
				}

			});

			//if a node always makes the same decisions across all steiner trees, assign it to fg or bg
			int newTerms = 0; int aFg = 0; int aBg = 0; 
			for (int j = 0; j < clusterLocalNodesGlobalIndices.size(); j++) {

				if (nodes[clusterLocalNodesGlobalIndices[j]].type == CORE || nodes[clusterLocalNodesGlobalIndices[j]].type == N) { continue; };
				bool allFg = true; 
				bool allBg = true; 
				for (int k = 0; k < steinerDecisions.size(); k++) {
					if (steinerDecisions[k][j]) { 
						allBg = false; 
					}
					else { 
						allFg = false; 
					}
				}
				
				if (allFg && allBg) { cout << "all fg and all bg: bug! " << endl; }
				else {
					if (allFg) { //Assign to be core
						aFg += 1;
						if (nodes[clusterLocalNodesGlobalIndices[j]].type == FILL) { //If type fill sent to FG, adjacent cuts must be sent to FG as well
							auto neighboursT = adjacent_vertices(clusterLocalNodesGlobalIndices[j], G);
							for (auto u : make_iterator_range(neighboursT)) {
								if (newNodes[u].type == CUT) {
									newNodes[u].type = CORE; 
									newNodes[u].inFg = 1;
								}
							}
						}
						newNodes[clusterLocalNodesGlobalIndices[j]].type = CORE; 
						newNodes[clusterLocalNodesGlobalIndices[j]].inFg = 1;
					}
					else {
						if (allBg) { //Assign to be N
							aBg += 1;
							if (nodes[clusterLocalNodesGlobalIndices[j]].type == CUT) { //If type fill sent to FG, adjacent cuts must be sent to FG as well
								auto neighboursT = adjacent_vertices(clusterLocalNodesGlobalIndices[j], G);
								for (auto u : make_iterator_range(neighboursT)) {
									if (newNodes[u].type == FILL) {
										newNodes[u].type = N; 
										newNodes[u].inFg = 0;
									}
								}
							}
							newNodes[clusterLocalNodesGlobalIndices[j]].type = N; 
							newNodes[clusterLocalNodesGlobalIndices[j]].inFg = 0;
						}
					}
				}
			}
			Graph localG;
			getGlobalToLocalCFGraphs(localG, complexEdges, clusterLocalNodesGlobalIndices, globalToLocal, newNodes);
			//Cluster nodes which make same decisions together
			int pushed = 0; 
			map< std::vector<int>, bool> hypAdded;
			for (int j = 0; j < steinerDecisions.size(); j++) {
				decisionIndex += 1;
				std::vector<bool> hasPos(clusterLocalNodesGlobalIndices.size(), false); 
				std::vector<bool> hasNeg(clusterLocalNodesGlobalIndices.size(), false);
				
				std::vector< std::vector<int> > cfFgComps; 
				std::vector< std::vector<int> > cfBgComps;
				getSteinerGroupingsForDecision(steinerDecisions[j], localG, clusterLocalNodesGlobalIndices, newNodes, cfFgComps, cfBgComps);
				for (int cfComp = 0; cfComp < cfFgComps.size(); cfComp++) {
					std::vector<int> fgCFCompLocalIndices = cfFgComps[cfComp]; std::vector<int> fgCFComp;
					bool hasIndex = false;
					for (int k = 0; k < fgCFCompLocalIndices.size(); k++) {
						fgCFComp.push_back(clusterLocalNodesGlobalIndices[fgCFCompLocalIndices[k]]);
					}
					
					
					std::sort(fgCFComp.begin(), fgCFComp.end());
					fgCFComp.erase(unique(fgCFComp.begin(), fgCFComp.end()), fgCFComp.end());
					std::vector<int> nodeWithSign = fgCFComp; 
					nodeWithSign.push_back(1);
					for (int k = 0; k < fgCFComp.size(); k++) {
						hasPos[globalToLocal[fgCFComp[k]]] = true;
					}

					if (hypAdded.find(nodeWithSign) == hypAdded.end()) {
						globalHypernodes.push_back(hyperNode(fgCFComp,HYPERNODE, 1));
						pushed += 1;
						hypAdded[nodeWithSign] = true;
					}
				}
				for (int cfComp = 0; cfComp < cfBgComps.size(); cfComp++) {
					std::vector<int> bgCFCompLocalIndices = cfBgComps[cfComp]; 
					std::vector<int> bgCFComp;
					bool hasIndex = false;
					for (int k = 0; k < bgCFCompLocalIndices.size(); k++) {
						bgCFComp.push_back(clusterLocalNodesGlobalIndices[bgCFCompLocalIndices[k]]);
					}
					std::sort(bgCFComp.begin(), bgCFComp.end()); 
					bgCFComp.erase(unique(bgCFComp.begin(), bgCFComp.end()), bgCFComp.end());
					for (int k = 0; k < bgCFComp.size(); k++) {
						hasNeg[globalToLocal[bgCFComp[k]]] = true;
					}
					std::vector<int> nodeWithSign = bgCFComp; 
					nodeWithSign.push_back(-1);
					if (hypAdded.find(nodeWithSign) == hypAdded.end()) {
						globalHypernodes.push_back(hyperNode(bgCFComp, HYPERNODE, -1));
						hypAdded[nodeWithSign] = true;
						pushed += 1;
					}
				}
				for (int k = 0; k < clusterLocalNodesGlobalIndices.size(); k++) {
					if (newNodes[clusterLocalNodesGlobalIndices[k]].type == CUT || newNodes[clusterLocalNodesGlobalIndices[k]].type == FILL) {
						if (!hasPos[k] && !hasNeg[k]) {
							cout << "doesn't have positive or negative node " << k << " " << clusterLocalNodesGlobalIndices[k] << endl;
						}
						
					}
				}
			}
			
		}
		}
	});
	nodes = newNodes;
}

void solveComponentGraph(Graph & G, std::vector<node> & nodes, std::vector<node> & newNodes, int numNodes, map< std::vector<int>, int> & edgeWts, int hypernodeSize, int64_t wtSum, int productThresh, tbb::concurrent_vector< hyperNode > & globalHypernodes,
	 std::vector< std::vector<int> > & clusterLocalEdgesGlobalIndices, std::vector<int> & clusterLocalNodesGlobalIndices, int fgComps, int bgComps, int localSteinerTime, Graph & fgG, Graph & bgG, int compIndex, map<int, int> & nodeToComp
	, int bbTime) {
	newNodes = nodes;

	//These two graphs used to connect terminals together in local graph that must be connected outside graph
	Graph coreG; getCoreG(G, nodes, numNodes, coreG, edgeWts); 
	Graph nG; getNG(G, nodes, numNodes, nG); 
	
	std::vector<bool> steinerDecisions;
	std::vector< std::vector<int> > complexEdges = clusterLocalEdgesGlobalIndices; 
	map<int, int> localToGlobal; map<int, int> globalToLocal; 
	getCompToLocalIndex(clusterLocalNodesGlobalIndices, localToGlobal, globalToLocal, edgeWts);

	doubleGraph localDoubleG(nodes, clusterLocalNodesGlobalIndices, clusterLocalEdgesGlobalIndices, complexEdges, globalToLocal, edgeWts);
	
	hyperGraph localHyperG(localDoubleG, hypernodeSize, wtSum);
	map< std::vector<int>, bool> edgeExists;
	for (int i = 0; i < localHyperG.hyperEdges.size(); i++) {
		edgeExists[{localHyperG.hyperEdges[i][0], localHyperG.hyperEdges[i][1]}] = true; 
		edgeExists[{localHyperG.hyperEdges[i][1], localHyperG.hyperEdges[i][0]}] = true;
	}
	connectTerminalsExternally(localHyperG, coreG, nG, numNodes, clusterLocalNodesGlobalIndices, clusterLocalEdgesGlobalIndices, edgeExists);
	//Solve hypergraph for every combination of terminals

	std::vector< std::vector<int> > connectedCores = getTermsConnectedOutsideCluster(fgG, CORE, nodes, numNodes, clusterLocalNodesGlobalIndices, localHyperG, nodeToComp);
	std::vector< std::vector<int> > connectedNs = getTermsConnectedOutsideCluster(bgG, N, nodes, numNodes, clusterLocalNodesGlobalIndices, localHyperG, nodeToComp);
	int numCombinations = 1;
	for (int j = 0; j < connectedCores.size(); j++) { numCombinations *= connectedCores[j].size(); } for (int j = 0; j < connectedNs.size(); j++) { numCombinations *= connectedNs[j].size(); }
	if (numCombinations > productThresh) {
		for (int j = 0; j < localHyperG.hyperNodes.size(); j++) {
			if (localHyperG.hyperNodes[j].getType() == HYPERNODE) {
				std::vector<int> hyperNodeGlobalIndices;
				for (int s = 0; s < localHyperG.hyperNodes[j].doubleSubnodes.size(); s++) {
					hyperNodeGlobalIndices.push_back(localDoubleG.doubleNodes[localHyperG.hyperNodes[j].doubleSubnodes[s]].origNode.index);
					
				}
				globalHypernodes.push_back(hyperNode(hyperNodeGlobalIndices, HYPERNODE, localHyperG.hyperNodes[j].getSide()));
			}
		}
	}
	else {
		std::vector< std::vector<std::vector<std::vector<int> > > > cCompPartitions;
		for (int j = 0; j < connectedCores.size(); j++) {
			std::vector<std::vector<std::vector<int> > > corePartitions = getPartitions(connectedCores[j]);
			cCompPartitions.push_back(corePartitions);
		}
		std::vector< std::vector<std::vector<std::vector<int> > > > nCompPartitions;
		for (int j = 0; j < connectedNs.size(); j++) {
			std::vector<std::vector<std::vector<int> > > nPartitions = getPartitions(connectedNs[j]);
			nCompPartitions.push_back(nPartitions);
		}
		std::vector< std::vector<int> > cCompIndices;
		for (int j = 0; j < cCompPartitions.size(); j++) {
			std::vector<int> compPartIndices;
			for (int k = 0; k < cCompPartitions[j].size(); k++) {
				compPartIndices.push_back(k);
			}
			cCompIndices.push_back(compPartIndices);
		}
		std::vector< std::vector<int> > nCompIndices;
		for (int j = 0; j < nCompPartitions.size(); j++) {
			std::vector<int> compPartIndices;
			for (int k = 0; k < nCompPartitions[j].size(); k++) {
				compPartIndices.push_back(k);
			}
			nCompIndices.push_back(compPartIndices);
		}

		std::vector<int> coreCombo; std::vector< std::vector<int> > coreCombinations;
		getAllCombinations(cCompIndices, 0, coreCombo, coreCombinations);
		std::vector<int> nCombo; std::vector< std::vector<int> > nCombinations;
		getAllCombinations(nCompIndices, 0, nCombo, nCombinations);
		
		int totalCombinations = coreCombinations.size() * nCombinations.size(); std::vector< std::vector<bool> > steinerDecisions;
		std::vector<int> combIndices; for (int index = 0; index < totalCombinations; index++) { combIndices.push_back(index); }
		tbb::parallel_for(tbb::blocked_range<int>(0, combIndices.size()),
			[&](tbb::blocked_range<int> range)
		{
			for (int itr = range.begin(); itr < range.end(); ++itr)
			{
				int c = itr / ((int)nCombinations.size());
				int n = itr % ((int)nCombinations.size());
				std::vector<int> cPartitionSelection = coreCombinations[c];
				map< std::vector<int>, bool > edgeExistsTemp = edgeExists; 
				hyperGraph partitionHyperG = localHyperG; 
				std::vector<node> nodesTemp = nodes;
				int fgCompsTemp = fgComps;
				int bgCompsTemp = bgComps; 
				Graph tempG = G; 
				Graph fgGTemp = fgG; 
				Graph bgGTemp = bgG; 
				std::vector< std::vector<int> > termEdges; 
				std::vector<int> connTermIndices;
				for (int p = 0; p < cPartitionSelection.size(); p++) {
					std::vector<std::vector<int> > componentPartition = cCompPartitions[p][cPartitionSelection[p]];
					for (int g = 0; g < componentPartition.size(); g++) {
						std::vector<int> group = componentPartition[g];
						for (int gi = 0; gi < group.size() - 1; gi++) {
							if (edgeExistsTemp.find({ group[gi], group[gi + 1] }) == edgeExistsTemp.end()) {
								partitionHyperG.hyperEdges.push_back({ group[gi], group[gi + 1] }); termEdges.push_back({ group[gi], group[gi + 1] }); connTermIndices.push_back(group[gi]); connTermIndices.push_back(group[gi + 1]);
								edgeExistsTemp[{ group[gi + 1], group[gi] }] = true; edgeExistsTemp[{ group[gi], group[gi + 1] }] = true;
							}
						}
					}
				}
				std::vector<int> nPartitionSelection = nCombinations[n];
				for (int p = 0; p < nPartitionSelection.size(); p++) {
					std::vector<std::vector<int> > componentPartition = nCompPartitions[p][nPartitionSelection[p]];
					for (int g = 0; g < componentPartition.size(); g++) {
						std::vector<int> group = componentPartition[g];
						for (int gi = 0; gi < group.size() - 1; gi++) {
							if (edgeExistsTemp.find({ group[gi], group[gi + 1] }) == edgeExistsTemp.end()) {
								partitionHyperG.hyperEdges.push_back({ group[gi], group[gi + 1] }); termEdges.push_back({ group[gi], group[gi + 1] }); termEdges.push_back({ group[gi], group[gi + 1] }); connTermIndices.push_back(group[gi]); connTermIndices.push_back(group[gi + 1]);
								edgeExistsTemp[{ group[gi + 1], group[gi] }] = true; edgeExistsTemp[{ group[gi], group[gi + 1] }] = true;
							}
						}
					}
				}
				bool violating = true;
				while (violating) {
					std::vector< std::vector<int> > slnEdges; 
					hyperGraph currentHyperG = partitionHyperG;
					solveSteinerTree(currentHyperG, nodesTemp, slnEdges, localSteinerTime, bbTime);

					violating = false; std::vector<bool> inFg(clusterLocalNodesGlobalIndices.size(), false); 
					std::vector<bool> inBg(clusterLocalNodesGlobalIndices.size(), false); int termSide;
					int newTerminal = findLocalNodeToFix(nodesTemp, numNodes, globalToLocal, inFg, inBg, currentHyperG, slnEdges, G, fgGTemp, bgGTemp, fgCompsTemp, bgCompsTemp, termSide, edgeWts, clusterLocalNodesGlobalIndices);

					if (newTerminal != -1) {
						newTerminal = clusterLocalNodesGlobalIndices[newTerminal];
						violating = true;
						if (termSide == 1) { //Terminal set to the foreground

							if (nodesTemp[newTerminal].type == FILL) { //If type fill sent to FG, adjacent cuts must be sent to FG as well
								auto neighboursT = adjacent_vertices(newTerminal, G);
								for (auto u : make_iterator_range(neighboursT)) {
									if (nodesTemp[u].type == CUT) {
										nodesTemp[u].type = CORE;
									}
								}
							}
							nodesTemp[newTerminal].type = CORE; nodesTemp[newTerminal].inFg = 1;
						}
						else {

							if (nodesTemp[newTerminal].type == CUT) { //If type cut sent to BG, adjacent fills must be sent to BG as well
								auto neighboursT = adjacent_vertices(newTerminal, G);
								for (auto u : make_iterator_range(neighboursT)) {
									if (nodesTemp[u].type == FILL) {
										nodesTemp[u].type = N;
									}
								}
							}
							nodesTemp[newTerminal].type = N; nodesTemp[newTerminal].inFg = 0;
						}
						removeCAndNEdges(tempG, nodesTemp); updateGraphs(tempG, fgGTemp, bgGTemp, nodesTemp, edgeWts); 
						fgCompsTemp = findComponents(fgGTemp, nodesTemp, numNodes, true); 
						int bgCompsTemp = findComponents(bgGTemp, nodesTemp, numNodes, false);
						doubleGraph localDoubleGTemp(nodesTemp, clusterLocalNodesGlobalIndices, clusterLocalEdgesGlobalIndices, complexEdges, globalToLocal, edgeWts);

						partitionHyperG = hyperGraph(localDoubleGTemp, hypernodeSize, wtSum);
						//Add edges between terminals using indices of updated partition graph
						std::sort(connTermIndices.begin(), connTermIndices.end());
						connTermIndices.erase(unique(connTermIndices.begin(), connTermIndices.end()), connTermIndices.end()); map<int, int> connTermIndicesMapping;
						for (int j = 0; j < connTermIndices.size(); j++) {
							connTermIndicesMapping[localHyperG.doubleG.doubleNodes[localHyperG.hyperNodes[connTermIndices[j]].doubleSubnodes[0]].origNode.index] = -1;
						}
						for (int j = 0; j < partitionHyperG.coreIndices.size(); j++) {
							int localI = localDoubleGTemp.doubleNodes[partitionHyperG.hyperNodes[partitionHyperG.coreIndices[j]].doubleSubnodes[0]].origNode.index;
							if (connTermIndicesMapping.find(localI) != connTermIndicesMapping.end()) {
								connTermIndicesMapping[localI] = partitionHyperG.coreIndices[j];
							}
						}
						for (int j = 0; j < partitionHyperG.nIndices.size(); j++) {
							int localI = localDoubleGTemp.doubleNodes[partitionHyperG.hyperNodes[partitionHyperG.nIndices[j]].doubleSubnodes[0]].origNode.index;
							if (connTermIndicesMapping.find(localI) != connTermIndicesMapping.end()) {
								connTermIndicesMapping[localI] = partitionHyperG.nIndices[j];
							}
						}
						for (int j = 0; j < termEdges.size(); j++) {
							partitionHyperG.hyperEdges.push_back({
							connTermIndicesMapping[localHyperG.doubleG.doubleNodes[localHyperG.hyperNodes[termEdges[j][0]].doubleSubnodes[0]].origNode.index],
								connTermIndicesMapping[localHyperG.doubleG.doubleNodes[localHyperG.hyperNodes[termEdges[j][1]].doubleSubnodes[0]].origNode.index]
								});
						}
					}
					else {
						steinerDecisions.push_back(inFg);
					}
				}
			}
		});
		//if a node always makes the same decisions across all steiner trees, assign it to fg or bg
		int newTerms = 0; int aFg = 0; int aBg = 0;
		for (int j = 0; j < clusterLocalNodesGlobalIndices.size(); j++) {
			if (nodes[clusterLocalNodesGlobalIndices[j]].type == CORE || nodes[clusterLocalNodesGlobalIndices[j]].type == N) { continue; };
			bool allFg = true;
			bool allBg = true;
			for (int k = 0; k < steinerDecisions.size(); k++) {
				if (steinerDecisions[k][j]) { allBg = false; }
				else { allFg = false; }
			}

			if (allFg && allBg) { cout << "all fg and all bg: bug! " << endl; }
			if (allFg) { //Assign to be core
				aFg += 1;
				if (nodes[clusterLocalNodesGlobalIndices[j]].type == FILL) { //If type fill sent to FG, adjacent cuts must be sent to FG as well
					auto neighboursT = adjacent_vertices(clusterLocalNodesGlobalIndices[j], G);
					for (auto u : make_iterator_range(neighboursT)) {
						if (newNodes[u].type == CUT) {
							newNodes[u].type = CORE; newNodes[u].inFg = 1;
							newTerms += 1;
						}
					}
				}
				newTerms += 1;
				newNodes[clusterLocalNodesGlobalIndices[j]].type = CORE; newNodes[clusterLocalNodesGlobalIndices[j]].inFg = 1;
			}
			if (allBg) { //Assign to be N
				aBg += 1;
				if (nodes[clusterLocalNodesGlobalIndices[j]].type == CUT) { //If type fill sent to FG, adjacent cuts must be sent to FG as well
					auto neighboursT = adjacent_vertices(clusterLocalNodesGlobalIndices[j], G);
					for (auto u : make_iterator_range(neighboursT)) {
						if (newNodes[u].type == FILL) {
							newTerms += 1;
							newNodes[u].type = N; newNodes[u].inFg = 0;
						}
					}
				}
				newTerms += 1;
				newNodes[clusterLocalNodesGlobalIndices[j]].type = N; newNodes[clusterLocalNodesGlobalIndices[j]].inFg = 0;
			}
		}
		Graph localG;
		getGlobalToLocalCFGraphs(localG, complexEdges, clusterLocalNodesGlobalIndices, globalToLocal, newNodes);
		//Cluster nodes which make same decisions together
		

		map< std::vector<int>, bool> hypAdded;
		for (int j = 0; j < steinerDecisions.size(); j++) {
			std::vector< std::vector<int> > cfFgComps; std::vector< std::vector<int> > cfBgComps;
			getSteinerGroupingsForDecision(steinerDecisions[j], localG, clusterLocalNodesGlobalIndices, newNodes, cfFgComps, cfBgComps);
			for (int cfComp = 0; cfComp < cfFgComps.size(); cfComp++) {
				std::vector<int> fgCFCompLocalIndices = cfFgComps[cfComp]; std::vector<int> fgCFComp;
				for (int k = 0; k < fgCFCompLocalIndices.size(); k++) {
					fgCFComp.push_back(clusterLocalNodesGlobalIndices[fgCFCompLocalIndices[k]]);
					
				}

				std::sort(fgCFComp.begin(), fgCFComp.end());
				fgCFComp.erase(unique(fgCFComp.begin(), fgCFComp.end()), fgCFComp.end());
				std::vector<int> nodeWithSign = fgCFComp; nodeWithSign.push_back(1);
				if (hypAdded.find(nodeWithSign) == hypAdded.end()) {
					globalHypernodes.push_back(hyperNode(fgCFComp, HYPERNODE, 1));
					hypAdded[nodeWithSign] = true;
				}
			}
			for (int cfComp = 0; cfComp < cfBgComps.size(); cfComp++) {
				std::vector<int> bgCFCompLocalIndices = cfBgComps[cfComp];
				std::vector<int> bgCFComp;
				for (int k = 0; k < bgCFCompLocalIndices.size(); k++) {
					bgCFComp.push_back(clusterLocalNodesGlobalIndices[bgCFCompLocalIndices[k]]);
					
				}
				std::sort(bgCFComp.begin(), bgCFComp.end()); 
				bgCFComp.erase(unique(bgCFComp.begin(), bgCFComp.end()), bgCFComp.end());
				std::vector<int> nodeWithSign = bgCFComp; 
				nodeWithSign.push_back(-1);
				if (hypAdded.find(nodeWithSign) == hypAdded.end()) {
					globalHypernodes.push_back(hyperNode(bgCFComp, HYPERNODE, -1));
					hypAdded[nodeWithSign] = true;
				}
			}
		}
	}
	nodes = newNodes;
}



void getInFgComponents(std::vector<bool> & inFg, std::vector<node> & nodes, int numNodes, Graph & G, map<std::vector<int>, int> & edgeWts, std::vector< std::vector<int> > & slnEdges, hyperGraph & hypG) {
	Graph fgG = Graph();
	Graph bgG = Graph();
	for (int i = 0; i < numNodes; i++) {
		add_vertex(fgG); add_vertex(bgG);
	}
	typedef boost::graph_traits<Graph>::edge_iterator edge_iter;
	edge_iter ei, ei_end;
	for (tie(ei, ei_end) = edges(G); ei != ei_end; ++ei) {
		int v1 = (int)ei->m_source; int v2 = (int)ei->m_target;
		if ((inFg[v1]) && (inFg[v2]) && edgeWts[{v1,v2}] == 1) {
			add_edge(v1, v2, fgG);
		}
		else {
			if ((!inFg[v1]) && (!inFg[v2])) {
				add_edge(v1, v2, bgG);
			}
		}
	}
	std::vector< std::vector<int> > fgComponents;
	std::vector<int> nodeComp(numNodes);
	int n = (int)boost::connected_components(fgG, &nodeComp[0]);
	int numComps = 0; 
	std::vector<int> isCompIndex(n, -1);
	for (int i = 0; i < numNodes; i++) {
		if (inFg[i]) {
			if (isCompIndex[nodeComp[i]] == -1) {
				isCompIndex[nodeComp[i]] = numComps;
				numComps += 1;
				std::vector<int> newComp = { i };
				fgComponents.push_back(newComp);
			}
			else {
				fgComponents[isCompIndex[nodeComp[i]]].push_back(i);
			}
		}
	}

	std::vector<int> nodeCompB(numNodes);
	int nB = (int)boost::connected_components(bgG, &nodeCompB[0]);
	int numCompsB = 0; std::vector<int> isCompIndexB(nB, -1);
	for (int i = 0; i < numNodes; i++) {
		if (!inFg[i]) {
			if (isCompIndexB[nodeCompB[i]] == -1) {
				isCompIndexB[nodeCompB[i]] = numCompsB;
				numCompsB += 1;
			}
		}
	}
	cout << "fgcompsB " << numCompsB << endl;
}

int labelComponentsB(const std::vector<uint8_t *> & bImg, int conn, int width, int height, int numSlices) {
	int ct = 0;
	std::vector< std::vector<int > > mask;
	if (conn == 0) {
		mask = structCube;
	}
	else {
		mask = structCross3D;
	}

	std::vector< std::vector< std::vector<bool>>> visited(width, std::vector<std::vector<bool>>(height, std::vector<bool>(numSlices, false)));

	for (int i = 0; i < width; i++) {
		for (int j = 0; j < height; j++) {
			for (int k = 0; k < numSlices; k++) {
				//Unvisited foreground voxels
				if (((int)bImg[k][i + j * width]) == 1 && visited[i][j][k] == false) {
					ct += 1;
					std::queue<Coordinate> q;
					q.push(Coordinate(i, j, k));
					visited[i][j][k] = true;
					while (!q.empty()) {
						Coordinate qp = q.front();
						q.pop();
						for (int s = 0; s < mask.size(); s++) {
							Coordinate np(qp.x + mask[s][0], qp.y + mask[s][1], qp.z + mask[s][2]);
							if (np.x >= 0 && np.x < width && np.y >= 0 && np.y < height && np.z >= 0 && np.z < numSlices) {
								if (((int)bImg[np.z][np.x + (np.y*width)]) == 1 && visited[np.x][np.y][np.z] == false) {
									visited[np.x][np.y][np.z] = true;
									q.push(np);
								}
							}
						}
					}

				}
			}
		}
	}
	return ct;
}

int labelComponentsBBg(const std::vector<uint8_t*>& bImg, int conn, int width, int height, int numSlices, int& bgCompsAll) {
	int ct = 0;
	std::vector< std::vector<int > > mask;
	if (conn == 0) {
		mask = structCube;
	}
	else {
		mask = structCross3D;
	}

	std::vector< std::vector< std::vector<bool>>> visited(width, std::vector<std::vector<bool>>(height, std::vector<bool>(numSlices, false)));

	for (int i = 0; i < width; i++) {
		for (int j = 0; j < height; j++) {
			for (int k = 0; k < numSlices; k++) {
				//Unvisited foreground voxels
				if (((int)bImg[k][i + j * width]) == 0 && visited[i][j][k] == false) {
					bgCompsAll++;
					std::queue<Coordinate> q;
					q.push(Coordinate(i, j, k));
					visited[i][j][k] = true;
					bool isTrueCavity = true;
					if (i == 0 || i < width || j == 0 || j < height || k == 0 || k < numSlices) {
						isTrueCavity = false;
					}
					while (!q.empty()) {
						Coordinate qp = q.front();
						q.pop();
						if (qp.x == 0 || qp.x < width || qp.y == 0 || qp.y < height || qp.z == 0 || qp.z < numSlices) {
							isTrueCavity = false;
						}
						for (int s = 0; s < mask.size(); s++) {
							Coordinate np(qp.x + mask[s][0], qp.y + mask[s][1], qp.z + mask[s][2]);
							if (np.x >= 0 && np.x < width && np.y >= 0 && np.y < height && np.z >= 0 && np.z < numSlices) {
								if (((int)bImg[np.z][np.x + (np.y * width)]) == 0 && visited[np.x][np.y][np.z] == false) {
									visited[np.x][np.y][np.z] = true;
									q.push(np);
								}
							}
						}
					}
					if (isTrueCavity) {
						ct++;
					}
				}
			}
		}
	}
	return ct;
}

std::vector<int> getEulerNumbersFromBImg(const std::vector<uint8_t *> & bImg, int width, int height, int numSlices) {
	int v = 0; int e = 0; int f = 0; int c = 0;
	for (int i = 0; i < width; i++) {
		for (int j = 0; j < height; j++) {
			for (int k = 0; k < numSlices; k++) {
				if (bImg[k][i + j * width] == 1) {
					v += 1;

					if (i + 1 < width) {
						//compare (i,j,k), (i+1, j, k)
						if ((bImg[k][(i + 1) + (width*j)]) == 1) {
							e += 1;
						}
					}

					if (j + 1 < height) {
						//compare (i,j,k), (i, j+1, k)
						if ((bImg[k][i + (width*(j + 1))]) == 1) {
							e += 1;
						}
					}

					if (k + 1 < numSlices) {
						//compare (i,j,k), (i, j+1, k)
						if ((bImg[k + 1][i + (width*(j))]) == 1) {
							e += 1;
						}
					}

					if (i + 1 < width && j + 1 < height) {
						//compare (i,j,k), (i, j+1, k)
						if ((bImg[k][(i + 1) + (width*(j))]) == 1 &&
							(bImg[k][(i)+(width*(j + 1))]) == 1 && (bImg[k][(i + 1) + (width*(j + 1))]) == 1) {
							f += 1;
						}
					}

					if (i + 1 < width && k + 1 < numSlices) {
						//compare (i,j,k), (i, j+1, k)
						if ((bImg[k][(i + 1) + (width*(j))]) == 1 &&
							(bImg[k + 1][(i)+(width*(j))]) == 1 && (bImg[k + 1][(i + 1) + (width*(j))]) == 1) {
							f += 1;

						}
					}

					//add up faces in yz plane
					if (j + 1 < height && k + 1 < numSlices) {
						//compare (i,j,k), (i, j+1, k)
						if (bImg[k][(i)+(width*(j + 1))] == 1 &&
							(bImg[k + 1][(i)+(width*(j))]) == 1 && (bImg[k + 1][(i)+(width*(j + 1))]) == 1
							) {
							f += 1;
						}
					}

					//Add up cubes
					if (i + 1 < width && j + 1 < height && k + 1 < numSlices) {
						bool hasCube = true;
						for (int o = 0; o < cubeFrontMask.size(); o++) {
							int coord[3] = { i + cubeFrontMask[o][0], j + cubeFrontMask[o][1], k + cubeFrontMask[o][2] };
							if (bImg[coord[2]][(coord[0]) + (width*(coord[1]))] == 0) {

								hasCube = false;
								break;
							}
						}
						if (hasCube) {
							c += 1;
						}

					}
				}
			}
		}
	}
	return { v,e,f,c };
}

std::vector<int> getTopoFromBImg(const std::vector<uint8_t *> bImg, int fgconn, int width, int height, int numSlices) {
	int h0 = labelComponentsB(bImg, fgconn, width, height, numSlices);
	int bgCompsAtEdges = 0;
	int h2 = labelComponentsBBg(bImg, 1 - fgconn, width, height, numSlices, bgCompsAtEdges); // - 1
	std::vector<int> eulNums = getEulerNumbersFromBImg(bImg, width, height, numSlices);
	int h1 = h0 + h2 - (eulNums[0] - eulNums[1] + eulNums[2] - eulNums[3]);
	return { h0,h2,h1 };
}

void findTopoWithLabeling(std::vector<node> & origNodes, std::vector<node> & nodes, int numNodes, std::vector< std::vector<int> > & newToOldComp, map<int, int> & newToOldIndex, int numSlices, int width, int height, std::vector<uint32_t *> & labels,
	std::vector<bool> & inFg, Graph & origG, map<int, int> & oldToNew
	) {
	std::vector<bool> oldInFg(origNodes.size(), false);
	for (int i = 0; i < numNodes; i++) {// || (nodes[i].type == CUT || nodes[i].type == FILL)
		if (inFg[i]) {
			for (int j = 0; j < newToOldComp[i].size(); j++) {
				oldInFg[newToOldIndex[newToOldComp[i][j]]] = true;
			}
		}
	}
	map<int, int> oldToNewMerge;
	for (int i = 0; i < newToOldComp.size(); i++) {
		for (int j = 0; j < newToOldComp[i].size(); j++) {
			oldToNewMerge[newToOldComp[i][j]] = i;
		}
	}

	std::vector<uint8_t *> simplifiedTopoImg;
	for (int s = 0; s < numSlices; s++) {
		
		uint8_t * label8T = new uint8_t[width*height]();
		for (int i = 0; i < width; i++) {
			for (int j = 0; j < height; j++) {
				label8T[i + j * width] = 0;
				if (oldInFg[Label(labels[s][i + j * width])]) {
					label8T[i + j * width] = 1;
				}
			}
		}
		simplifiedTopoImg.push_back(label8T);
	}
	std::vector<int> eulSimplified = getTopoFromBImg(simplifiedTopoImg, 1, width, height, numSlices);
	cout << "topo after round: " << eulSimplified[0] << " " << eulSimplified[1] << " " << eulSimplified[2] << endl;
}

void solveGlobalGraph(std::vector<node> & nodes, int numNodes, Graph & G, Graph & origG, tbb::concurrent_vector< hyperNode > & globalHypernodes,int64_t wtSum, map< std::vector<int>, int> & edgeWts,
	int hypernodeSize, int productThresh, int globalSteinerTime, int localSteinerTime, std::vector<node> & origNodes, std::vector< std::vector<int> > & newToOldComp, map<int, int> & newToOldIndex, int numSlices, int width, int height, std::vector<uint32_t *> & labels,
	Graph & firstG, map<int, int> & oldToNew, int nodesToFix, int bbTime) {
	
	doubleGraph globalDoubleG(nodes, numNodes, G, edgeWts);
	hyperGraph hyperG(nodes, globalDoubleG, edgeWts, G, globalHypernodes, wtSum);
	hyperGraph currentHyperG = hyperG; Graph fgG; Graph bgG;
	
	getFgBgGraphs(G, nodes, numNodes, fgG, bgG, edgeWts);
	std::vector< std::vector< std::vector<int> > > localGraphEdges;
	std::vector< std::vector<int> > localNodesGlobalIndex; map<int, int> nodeToComp;
	getLocalGraphClustersMapping(G, nodes, numNodes, localGraphEdges, localNodesGlobalIndex, nodeToComp);
	int fgComps = findComponents(fgG, nodes, numNodes, true);
	int bgComps = findComponents(bgG, nodes, numNodes, false);
	bool conflicting = true; //Iterate until no more nodes which are doubly labeled
	int numItr = 0;
	while (conflicting) {
		cout << "Global steiner tree iteration  " << numItr+1 << " Graph nodes: " << nodes.size() << " Double graph nodes: " << currentHyperG.doubleG.doubleNodes.size() << " Double graph edges: " << currentHyperG.doubleG.doubleEdges.size() << " Hypergraph nodes: " << currentHyperG.numHypernodes << " Hypergraph edges: " << currentHyperG.hyperEdges.size() << endl;
		std::vector< std::vector<int> > slnEdges; 
		
		solveSteinerTree(currentHyperG, nodes, slnEdges, globalSteinerTime, bbTime);
		
		conflicting = false; std::vector<bool> inFg(numNodes, false);
		std::vector<bool> inBg(numNodes, false); int termSide;
		std::vector<bool> nodeFixed(numNodes, false);
		int64_t termCost;
		int newTerminal = findGlobalNodeToFix(nodes, numNodes, inFg, inBg, currentHyperG, slnEdges, G, fgG, bgG, fgComps, bgComps, termSide, edgeWts, nodeFixed, termCost);
		if (newTerminal != -1) {
			nodeFixed[newTerminal] = true;
			conflicting = true; 
			std::vector<bool> isAffectedComp(localNodesGlobalIndex.size(), false); std::vector<int> affectedComponents; //After a new terminal is upon a conflicting node, these variables represents the components of cuts and fills which are affected
			//Set new terminal and potentially adjacent nodes to be terminals to satisfy cell complex constraint 
			if (termSide == 1) {
				
				if (((int)nodes[newTerminal].type) == FILL) {
					auto neighbours = adjacent_vertices(newTerminal, G);
					for (auto u : make_iterator_range(neighbours)) {
						if (((int)nodes[u].type) == CUT) {
							nodes[u].type = CORE; isAffectedComp[nodeToComp[u]] = true; affectedComponents.push_back(nodeToComp[u]); nodeFixed[u] = true;
						}
					}
				}
				nodes[newTerminal].type = CORE; isAffectedComp[nodeToComp[newTerminal]] = true; affectedComponents.push_back(nodeToComp[newTerminal]);
			}
			else {
				if (((int)nodes[newTerminal].type) == CUT) {
					auto neighbours = adjacent_vertices(newTerminal, G);
					for (auto u : make_iterator_range(neighbours)) {
						if (((int)nodes[u].type) == FILL) {
							nodes[u].type = N; isAffectedComp[nodeToComp[u]] = true; affectedComponents.push_back(nodeToComp[u]);  nodeFixed[u] = true;
						}
					}
				}
				nodes[newTerminal].type = N; isAffectedComp[nodeToComp[newTerminal]] = true; affectedComponents.push_back(nodeToComp[newTerminal]);
			}
			
			for (int n = 1; n < nodesToFix; n++) {
				int nextT = findGlobalNodeToFix(nodes, numNodes, inFg, inBg, currentHyperG, slnEdges, G, fgG, bgG, fgComps, bgComps, termSide, edgeWts, nodeFixed, termCost);
				if (nextT != -1 && termCost < 0) {
					if (termSide == 1) {

						if (((int)nodes[nextT].type) == FILL) {
							auto neighbours = adjacent_vertices(nextT, G);
							for (auto u : make_iterator_range(neighbours)) {
								if (((int)nodes[u].type) == CUT) {
									nodes[u].type = CORE; isAffectedComp[nodeToComp[u]] = true; affectedComponents.push_back(nodeToComp[u]);  nodeFixed[u] = true;
								}
							}
						}
						nodes[nextT].type = CORE; isAffectedComp[nodeToComp[nextT]] = true; affectedComponents.push_back(nodeToComp[nextT]); nodeFixed[nextT] = true;
					}
					else {
						if (((int)nodes[nextT].type) == CUT) {
							auto neighbours = adjacent_vertices(nextT, G);
							for (auto u : make_iterator_range(neighbours)) {
								if (((int)nodes[u].type) == FILL) {
									nodes[u].type = N; isAffectedComp[nodeToComp[u]] = true; affectedComponents.push_back(nodeToComp[u]);  nodeFixed[u] = true;
								}
							}
						}
						nodes[nextT].type = N; isAffectedComp[nodeToComp[nextT]] = true; affectedComponents.push_back(nodeToComp[nextT]);  nodeFixed[nextT] = true;
					}
				}
				else {
					break;
				}
			}
			//Preprocess graph globally
			
			preprocessGraph(G, nodes, edgeWts, isAffectedComp, affectedComponents, nodeToComp, true);
			tbb::concurrent_vector< hyperNode > newHypernodes; //Also need to make newHypernodeSides thread safe
																										  //Find hypernodes for graph in next iteration 
			for (int i = 0; i < currentHyperG.hyperNodes.size(); i++) {
				if (currentHyperG.hyperNodes[i].getType() == HYPERNODE) {
					//For all components not affected by new terminals, propagate hypernodes to next iteration
					if (!isAffectedComp[nodeToComp[currentHyperG.doubleG.doubleNodes[currentHyperG.hyperNodes[i].doubleSubnodes[0]].origNode.index]]) {
						std::vector<int> hypernode;
						for (int j = 0; j < currentHyperG.hyperNodes[i].doubleSubnodes.size(); j++) {
							hypernode.push_back(currentHyperG.doubleG.doubleNodes[currentHyperG.hyperNodes[i].doubleSubnodes[j]].origNode.index);
						}
						newHypernodes.push_back(hyperNode(hypernode, HYPERNODE, currentHyperG.hyperNodes[i].getSide()));
					}
				}
			}
			std::sort(affectedComponents.begin(), affectedComponents.end()); affectedComponents.erase(unique(affectedComponents.begin(), affectedComponents.end()), affectedComponents.end()); //Find unique components which are affected by new terminals
			fgG = Graph(); 
			bgG = Graph();
			getFgBgGraphs(G, nodes, numNodes, fgG, bgG, edgeWts);
			fgComps = findComponents(fgG, nodes, numNodes, true); bgComps = findComponents(bgG, nodes, numNodes, false); 
			for (int i = 0; i < affectedComponents.size(); i++) {
				int compIndex = affectedComponents[i];
				//If all nodes in component are now terminals, continue
				bool hasCF = false;
				for (int j = 0; j < localNodesGlobalIndex[compIndex].size(); j++) {
					if ((int)nodes[localNodesGlobalIndex[compIndex][j]].type == CUT || (int)nodes[localNodesGlobalIndex[compIndex][j]].type == FILL) {
						hasCF = true;
						break;
					}
				}
				if (hasCF) {
					std::vector<node> newNodes = nodes;
					solveComponentGraph(G, nodes, newNodes, numNodes, edgeWts, hypernodeSize, wtSum, productThresh, newHypernodes,
						localGraphEdges[compIndex], localNodesGlobalIndex[compIndex], fgComps, bgComps, localSteinerTime, fgG, bgG, compIndex, nodeToComp, bbTime);
				}
			}

		
			removeCAndNEdges(G, nodes); 
			map<int, int> oldToNew2;
			std::vector< std::vector<int> > newToOldComp2 = mergeAdjacentTerminals(G, nodes, edgeWts, oldToNew2);
			std::vector< std::vector<int> > newToOldTemp;
			numNodes = nodes.size();
			for (int i = 0; i < newToOldComp2.size(); i++) {

				std::vector<int> combinedNewToOld;
				for (int j = 0; j < newToOldComp2[i].size(); j++) {
					int oldIndex = newToOldComp2[i][j];
					for (int k = 0; k < newToOldComp[oldIndex].size(); k++) {
						combinedNewToOld.push_back(newToOldComp[oldIndex][k]);
					}
				}
				newToOldTemp.push_back(combinedNewToOld);
			}
			newToOldComp = newToOldTemp;
			tbb::concurrent_vector<hyperNode> globalNodesTemp;
			for (int i = 0; i < newHypernodes.size(); i++) {
				std::vector<int> subnodes;
				for (int j = 0; j < newHypernodes[i].doubleSubnodes.size(); j++) {
					subnodes.push_back(oldToNew2[newHypernodes[i].doubleSubnodes[j]]);
				}
				std::sort(subnodes.begin(), subnodes.end());
				subnodes.erase(unique(subnodes.begin(), subnodes.end()), subnodes.end());

				globalNodesTemp.push_back(hyperNode(subnodes, HYPERNODE, newHypernodes[i].getSide()));
			}
			newHypernodes = globalNodesTemp;
			removeCAndNEdges(G, nodes); numNodes = nodes.size();
			doubleGraph globalDoubleGN(nodes, numNodes, G, edgeWts);
			hyperGraph hyperGN(nodes, globalDoubleGN, edgeWts, G, newHypernodes, wtSum);
			currentHyperG = hyperGN;
			fgG = Graph(); 
			bgG = Graph();
			getFgBgGraphs(G, nodes, numNodes, fgG, bgG, edgeWts);
			localGraphEdges.clear();
			localNodesGlobalIndex.clear();
			nodeToComp.clear();
			getLocalGraphClustersMapping(G, nodes, numNodes, localGraphEdges, localNodesGlobalIndex, nodeToComp);
			fgComps = findComponents(fgG, nodes, numNodes, true); 
			bgComps = findComponents(bgG, nodes, numNodes, false);
		}
		else {
			//No more conflicting nodes
			for (int i = 0; i < nodes.size(); i++) {
				nodes[i].inFg = inFg[i];
			}
		}
		numItr += 1;
	}
	
	cout << "Finished global steiner tree stage " << endl;
}

bool violatesComplex(std::vector<node> & nodes, int v1, int v2, map<std::vector<int>, int> & edgeWt, int fgConn) {

	if (((((int)nodes[v1].inFg) == 1) && (((int)nodes[v1].type) == 3)) // v2 is fill in fg
		&& ((((int)nodes[v2].inFg) == 0) && ((int)nodes[v2].type) == 2) //v1 is cut in bg
		) {
		return true;
	}

	if ((((int)nodes[v2].inFg == 1) && ((int)nodes[v2].type) == 3) // v2 is fill in fg
		&& (((int)nodes[v1].inFg == 0) && ((int)nodes[v1].type) == 2) //v1 is cut in bg
		) {
		return true;
	}

	return false;
}

int getNumComponents(grapht & g, std::vector<node> & nodes, int fg) {
	std::vector<int> nodeToComp(nodes.size());
	int n = (int)boost::connected_components(g, &nodeToComp[0]);
	int numComps = 0;
	std::vector<bool> isCompIndex(nodes.size(), false);
	for (int i = 0; i < nodeToComp.size(); i++) {
		if (((int)nodes[i].inFg) == fg && nodes[i].valid) {
			if (!isCompIndex[nodeToComp[i]]) {
				isCompIndex[nodeToComp[i]] = true;
				numComps += 1;
			}
		}
	}
	return numComps;
}

int addToSubgraph(Graph & G, grapht & subG, std::vector<node> & nodes, node n, int fg, int conn, map< std::vector<int>, int> & edgeWt) {
	auto neighbours = adjacent_vertices(n.index, G);
	int nEdgesAdded = 0;
	for (auto vd : make_iterator_range(neighbours)) {

		if (((int)nodes[vd].inFg) == fg) {
			if (nodes[vd].valid) {
				if (conn == 0 || (conn == 1 && edgeWt[{(int)n.index, (int)vd}] == 1)) { //If fg connectivity is cube or fg connectivity is structCross3D and edge is strong
					add_edge(n.index, vd, 0, subG);
					nEdgesAdded += 1;
				}
			}
		}
	}

	return nEdgesAdded;
}

int getFgComponents(Graph & g, std::vector<node> & nodes, map<std::vector<int>, int> & edgeWt) {
	Graph fgG;
	for (int i = 0; i < nodes.size(); i++) {
		add_vertex(fgG);
	}
	typedef boost::graph_traits<Graph>::edge_iterator edge_iter;
	edge_iter ei, ei_end;
	for (tie(ei, ei_end) = edges(g); ei != ei_end; ++ei) {
		int v1 = (int)ei->m_source; int v2 = (int)ei->m_target;
		if (((int)nodes[v1].inFg) == 1 && ((int)nodes[v2].inFg) == 1 && (nodes[v1].valid) && (nodes[v2].valid)) {
			//If src vtx or tgt vtx is core, cut, or fill, add to fg graph 
			if (edgeWt[{(int)ei->m_source, (int)ei->m_target}] == 1) { //If fg connectivity is cube or fg connectivity is structCross3D and edge is strong
				add_edge(v1, v2, fgG);
			}
		}
	}

	std::vector<int> nodeToComp(nodes.size());
	int n = (int)boost::connected_components(fgG, &nodeToComp[0]);
	int numComps = 0; std::vector<int> isCompIndex(n, -1);
	for (int i = 0; i < nodes.size(); i++) {

		if (((int)nodes[i].inFg) == 1) {
			if (isCompIndex[nodeToComp[i]] == -1) {

				isCompIndex[nodeToComp[i]] = numComps;
				std::vector<int> newComp = { i };
				numComps += 1;
			}
		}
	}
	return numComps;
}

int getBgComponents(Graph & g, std::vector<node> & nodes) {
	Graph bgG;
	for (int i = 0; i < nodes.size(); i++) {
		add_vertex(bgG);
	}
	typedef boost::graph_traits<Graph>::edge_iterator edge_iter;
	edge_iter ei, ei_end;
	for (tie(ei, ei_end) = edges(g); ei != ei_end; ++ei) {
		int v1 = (int)ei->m_source; int v2 = (int)ei->m_target;
		if (((int)nodes[v1].inFg) == 0 && ((int)nodes[v2].inFg) == 0 && (nodes[v1].valid) && (nodes[v2].valid)) {
				add_edge(v1, v2, bgG);
		}
	}

	std::vector<int> nodeToComp(nodes.size());
	int n = (int)boost::connected_components(bgG, &nodeToComp[0]);
	int numComps = 0; std::vector<int> isCompIndex(n, -1);
	for (int i = 0; i < nodes.size(); i++) {

		if (((int)nodes[i].inFg) == 0) {
			if (isCompIndex[nodeToComp[i]] == -1) {

				isCompIndex[nodeToComp[i]] = numComps;
				std::vector<int> newComp = { i };
				numComps += 1;
			}
		}
	}
	return numComps;
}




void findNodeToSwap(Graph & G, grapht & fgG, grapht & bgG, std::vector<node> & nodes, std::vector<node> & nodesIn, int & nodeToSwap, map< std::vector<int>, int> & edgeWt, int fgConn,
	int origFgComps, int origBgComps, int & newFgComps, int & newBgComps) {
	int bgConn = 1 - fgConn;

	nodeToSwap = -1;
	std::vector< tuple<int, int, int, int, int, int, int, int> > nodeCosts; //std::vector< tuple<int, int, int, int, int, int, int, int> > nodeCosts;
	newFgComps = origFgComps;
	newBgComps = origBgComps;
	for (int i = 0; i < nodes.size(); i++) {
		nodes[i].isArticulate = false; nodes[i].compIndex = -1; 
		nodes[i].isNew = false;
		nodes[i].tin = 0;
		nodes[i].low = 0;
		nodes[i].overallCompIndexFg = -1; nodes[i].overallCompIndexBg = -1;
	}
	map<int, map<int, int> > fgLocalArtConnectivity = findComponents(fgG, nodes, 1);
	map<int, map<int, int> > bgLocalArtConnectivity = findComponents(bgG, nodes, 0);

	int artCt = 0;
	for (int i = 0; i < nodes.size(); i++) {
		if (!nodes[i].valid) { continue; }

		if (((int)nodes[i].inFg) == 1 && ((((int)nodesIn[i].type) == 2))) {
			int changeH0 = 0;
			if (nodes[i].isArticulate) {
				artCt += 1;
				map<int, int> nodeToComp = fgLocalArtConnectivity[i];
				std::vector<int> uniqueComps;
				for (std::map<int, int>::iterator iter = nodeToComp.begin(); iter != nodeToComp.end(); ++iter)
				{
					uniqueComps.push_back(iter->second);
				}
				std::sort(uniqueComps.begin(), uniqueComps.end());
				uniqueComps.erase(unique(uniqueComps.begin(), uniqueComps.end()), uniqueComps.end());
				changeH0 = uniqueComps.size() - 1;
			}
			std::vector<int> nBgComps;
			auto neighbours = adjacent_vertices(i, G); //If connect to BG, how many components would it connect?
			int adjFg = 0;
			for (auto u : make_iterator_range(neighbours)) {
				if (nodes[u].valid) {
					if (((int)nodes[u].inFg) == 0 && nodes[u].compIndex != -1) {
						nBgComps.push_back(nodes[u].compIndex);
					}
					if (((int)nodes[u].inFg) == 1) {
						adjFg += 1;
					}
				}
			}
			if (adjFg == 0 && changeH0 == 0) {
				changeH0 = -1;
			}
			std::sort(nBgComps.begin(), nBgComps.end());
			nBgComps.erase(unique(nBgComps.begin(), nBgComps.end()), nBgComps.end());
			int changeH2 = -((int)nBgComps.size() - 1); //Would connect together this many background components

			nodes[i].inFg = 0;
			auto neighbours1 = adjacent_vertices(i, G);
			bool violatesCellComplex = false;
			for (auto vd : make_iterator_range(neighbours1)) {
				if (nodes[vd].valid) {
					if (violatesComplex(nodes, i, vd, edgeWt, bgConn)) {
						violatesCellComplex = true;
						break;

					}
				}
			}
			if (violatesCellComplex) {
				nodes[i].inFg = 1;
				continue;
			}

			int eulerNum = nodes[i].v - nodes[i].e + nodes[i].f - nodes[i].c;
			int deltaH1 = (changeH0)+(changeH2)-(-eulerNum);
			nodeCosts.push_back(std::make_tuple((changeH0)+(changeH2)+deltaH1, nodes[i].intensity, i, origFgComps + changeH0, origBgComps + changeH2, changeH0, changeH2, deltaH1));

			nodes[i].inFg = 1;
		}
		else {
			if (((int)nodes[i].inFg) == 0 && (((int)nodesIn[i].type) == 3)) {
				int changeH2 = 0;
				if (nodes[i].isArticulate) {
					artCt += 1;
					map<int, int> nodeToComp = bgLocalArtConnectivity[i];
					std::vector<int> uniqueComps;
					for (std::map<int, int>::iterator iter = nodeToComp.begin(); iter != nodeToComp.end(); ++iter)
					{
						uniqueComps.push_back(iter->second);
					}
					std::sort(uniqueComps.begin(), uniqueComps.end());
					uniqueComps.erase(unique(uniqueComps.begin(), uniqueComps.end()), uniqueComps.end());
					changeH2 = uniqueComps.size() - 1;
				}
				std::vector<int> nFgComps;
				auto neighbours = adjacent_vertices(i, G); //If connect to BG, how many components would it connect?
				int adjBg = 0;
				for (auto u : make_iterator_range(neighbours)) {
					if (nodes[u].valid) {
						if (((int)nodes[u].inFg) == 1 && nodes[u].compIndex != -1) {
							nFgComps.push_back(nodes[u].compIndex);
						}
						if (((int)nodes[u].inFg) == 0) {
							adjBg += 1;
						}
					}
				}
				std::sort(nFgComps.begin(), nFgComps.end());
				nFgComps.erase(unique(nFgComps.begin(), nFgComps.end()), nFgComps.end());
				int changeH0 = -((int)nFgComps.size() - 1); //Would connect together this many background components
				if (adjBg == 0 && changeH2 == 0) {
					changeH2 = -1;
				}
				
				nodes[i].inFg = 1;
				auto neighbours1 = adjacent_vertices(i, G);
				bool violatesCellComplex = false;
				for (auto vd : make_iterator_range(neighbours1)) {
					if (nodes[vd].valid) {
						if (violatesComplex(nodes, i, vd, edgeWt, bgConn)) {
							violatesCellComplex = true;
							break;

						}
					}
				}
				if (violatesCellComplex) {
					nodes[i].inFg = 0;
					continue;
				}
				
				int eulerNum = nodes[i].v - nodes[i].e + nodes[i].f - nodes[i].c;
				int deltaH1 = (changeH0)+(changeH2)-(eulerNum); 
				nodeCosts.push_back(std::make_tuple(changeH0 + changeH2 + deltaH1, -nodes[i].intensity, i, origFgComps + changeH0, origBgComps + changeH2, changeH0, changeH2, deltaH1));
				nodes[i].inFg = 0;
			}
		}
	}
	if (nodeCosts.size() > 0) {
		std::sort(nodeCosts.begin(), nodeCosts.end());

		if (get<0>(nodeCosts[0]) < 0) {
			nodeToSwap = get<2>(nodeCosts[0]);
			newFgComps = get<3>(nodeCosts[0]);
			newBgComps = get<4>(nodeCosts[0]);
		}
		else {
			if (get<0>(nodeCosts[0]) == 0) {
				if (get<1>(nodeCosts[0]) < 0) {
					nodeToSwap = get<2>(nodeCosts[0]);
					newFgComps = get<3>(nodeCosts[0]);
					newBgComps = get<4>(nodeCosts[0]);
				}
			}
		} 
	}
}


void swapLabelsGreedy(Graph & G, grapht & fgG, grapht & bgG, std::vector<node> & nodes, map< std::vector<int>, int> & edgeWt, int fgConn, int width, int height,
	int numSlices, string outFile, std::vector<uint32_t *> & labels, int S, std::vector<stbi_uc*> & g_Image3D, float vizEps) {
	std::vector<node> newNodes = nodes;
	std::vector<node> origNodes = nodes;
	int bgConn = 1 - fgConn;
	int fgComps = getNumComponents(fgG, newNodes, 1); int bgComps = getNumComponents(bgG, newNodes, 0);
	int nodeToSwap = -1;
	int newFgComps = -1; int newBgComps = -1;
	for (int i = 0; i < nodes.size(); i++) {
		if ((int)nodes[i].type == CUT || (int)nodes[i].type == FILL) {

			nodes[i].intensity = nodes[i].labelCost;
			origNodes[i].intensity = nodes[i].labelCost;
			newNodes[i].intensity = nodes[i].labelCost;
		}
	}
	findNodeToSwap(G, fgG, bgG, origNodes, newNodes, nodeToSwap, edgeWt, fgConn, fgComps, bgComps, newFgComps, newBgComps);
	int itr = 0;
	while (nodeToSwap != -1) {
		itr += 1;
		if (((int)newNodes[nodeToSwap].inFg) == 1) {
			newNodes[nodeToSwap].inFg = 0;
			origNodes[nodeToSwap].inFg = 0;
			fgComps = newFgComps; bgComps = newBgComps;
			clear_vertex(nodeToSwap, fgG);
			addToSubgraph(G, bgG, newNodes, newNodes[nodeToSwap], 0, bgConn, edgeWt);
		}
		else {
			if (((int)newNodes[nodeToSwap].inFg) == 0) {
				newNodes[nodeToSwap].inFg = 1;
				origNodes[nodeToSwap].inFg = 1;
				fgComps = newFgComps; bgComps = newBgComps;
				clear_vertex(nodeToSwap, bgG);
				addToSubgraph(G, fgG, newNodes, newNodes[nodeToSwap], 1, fgConn, edgeWt);
			}
		}
		for (int i = 0; i < newNodes.size(); i++) {
			origNodes[i].inFg = newNodes[i].inFg;
		}
		findNodeToSwap(G, fgG, bgG, origNodes, newNodes, nodeToSwap, edgeWt, fgConn, fgComps, bgComps, newFgComps, newBgComps);
	}
	for (int i = 0; i < newNodes.size(); i++) {
		nodes[i].inFg = newNodes[i].inFg;
	}
}

int labelComponentsTBg(const std::vector<stbi_uc*>& g_Image3D, const std::vector<std::vector<float> >& g_Image3DF, float t, int conn, int width, int height, int numSlices) {
	int ct = 0;
	std::vector< std::vector<int > > mask;
	if (conn == 0) {
		mask = structCube;
	}
	else {
		mask = structCross3D;
	}

	std::vector< std::vector< std::vector<bool>>> visited(width, std::vector<std::vector<bool>>(height, std::vector<bool>(numSlices, false)));

	for (int i = 0; i < width; i++) {
		for (int j = 0; j < height; j++) {
			for (int k = 0; k < numSlices; k++) {
				//Unvisited foreground voxels
				if (g_Image3DF.size() > 0) {
					if (g_Image3DF[k][i + j * width] <= t && visited[i][j][k] == false) {
						ct += 1;
						std::queue<Coordinate> q;
						q.push(Coordinate(i, j, k));
						visited[i][j][k] = true;
						while (!q.empty()) {
							Coordinate qp = q.front();
							q.pop();
							for (int s = 0; s < mask.size(); s++) {
								Coordinate np(qp.x + mask[s][0], qp.y + mask[s][1], qp.z + mask[s][2]);
								if (np.x >= 0 && np.x < width && np.y >= 0 && np.y < height && np.z >= 0 && np.z < numSlices) {
									if (g_Image3DF[np.z][np.x + (np.y * width)] <= t && visited[np.x][np.y][np.z] == false) {
										visited[np.x][np.y][np.z] = true;
										q.push(np);
									}
								}
							}
						}

					}
				}
				else {
					if (g_Image3D[k][i + j * width] <= t && visited[i][j][k] == false) {
						ct += 1;
						std::queue<Coordinate> q;
						q.push(Coordinate(i, j, k));
						visited[i][j][k] = true;
						while (!q.empty()) {
							Coordinate qp = q.front();
							q.pop();
							for (int s = 0; s < mask.size(); s++) {
								Coordinate np(qp.x + mask[s][0], qp.y + mask[s][1], qp.z + mask[s][2]);
								if (np.x >= 0 && np.x < width && np.y >= 0 && np.y < height && np.z >= 0 && np.z < numSlices) {
									if (g_Image3D[np.z][np.x + (np.y * width)] <= t && visited[np.x][np.y][np.z] == false) {
										visited[np.x][np.y][np.z] = true;
										q.push(np);
									}
								}
							}
						}

					}
				}
			}
		}
	}
	return ct;
}

int fill2DCavities(stbi_uc* & slice, float t, int conn, int width, int height, int numSlices, int vizEps) {
	int ct = 0;
	std::vector< std::vector<int > > mask;
	if (conn == 0) {
		mask = structSquare;
	}

	std::vector< std::vector<int>> label(width, std::vector<int>(height, -1));
	map<int, int> componentCt;
	int largestComp = 1;
	for (int i = 0; i < width; i++) {
		for (int j = 0; j < height; j++) {
				
				if (slice[i + j * width] <= t && label[i][j] == -1) {
					ct += 1;
					componentCt[ct] = 1;
					std::queue<std::vector<int>> q;
					q.push({ i, j });
					
					while (!q.empty()) {
						std::vector<int> qp = q.front();
						q.pop();
						label[qp[0]][qp[1]] = ct;
						for (int s = 0; s < mask.size(); s++) {
							std::vector<int> np = { qp[0] + mask[s][0], qp[1] + mask[s][1] };
						
							if (np[0] >= 0 && np[0] < width && np[1] >= 0 && np[1] < height) {
								if (slice[np[0] + (np[1] * width)] <= t && label[np[0]][np[1]] == -1) {
									componentCt[ct] += 1;
									if (componentCt[ct] > componentCt[largestComp]) {
										largestComp = ct;
									}
									q.push(np);
									label[np[0]][np[1]] = ct;
								}
							}
						}
					}

				}
				
			
		}
	}
	for (int i = 0; i < width; i++) {
		for (int j = 0; j < height; j++) {

			if (slice[i + j * width] <= t) {
				if (label[i][j] != largestComp) {
					slice[i + j * width] = t + vizEps;
				}
			}

		}
	}

	return ct;
}

int labelComponentsT(const std::vector<stbi_uc*>& g_Image3D, const std::vector<std::vector<float> >& g_Image3DF, float t, int conn, int width, int height, int numSlices) {
	int ct = 0;
	std::vector< std::vector<int > > mask;
	if (conn == 0) {
		mask = structCube;
	}
	else {
		mask = structCross3D;
	}

	std::vector< std::vector< std::vector<bool>>> visited(width, std::vector<std::vector<bool>>(height, std::vector<bool>(numSlices, false)));

	for (int i = 0; i < width; i++) {
		for (int j = 0; j < height; j++) {
			for (int k = 0; k < numSlices; k++) {
				//Unvisited foreground voxels
				if (g_Image3DF.size() > 0) {
					if (((float)g_Image3DF[k][i + j * width]) > t && visited[i][j][k] == false) {
						ct += 1;
						std::queue<Coordinate> q;
						q.push(Coordinate(i, j, k));
						visited[i][j][k] = true;
						while (!q.empty()) {
							Coordinate qp = q.front();
							q.pop();
							for (int s = 0; s < mask.size(); s++) {
								Coordinate np(qp.x + mask[s][0], qp.y + mask[s][1], qp.z + mask[s][2]);
								if (np.x >= 0 && np.x < width && np.y >= 0 && np.y < height && np.z >= 0 && np.z < numSlices) {
									if (((float)g_Image3DF[np.z][np.x + (np.y*width)]) > t && visited[np.x][np.y][np.z] == false) {
										visited[np.x][np.y][np.z] = true;
										q.push(np);
									}
								}
							}
						}

					}
				}
				else {
					if (((float)g_Image3D[k][i + j * width]) > t && visited[i][j][k] == false) {
						ct += 1;
						std::queue<Coordinate> q;
						q.push(Coordinate(i, j, k));
						visited[i][j][k] = true;
						while (!q.empty()) {
							Coordinate qp = q.front();
							q.pop();
							for (int s = 0; s < mask.size(); s++) {
								Coordinate np(qp.x + mask[s][0], qp.y + mask[s][1], qp.z + mask[s][2]);
								if (np.x >= 0 && np.x < width && np.y >= 0 && np.y < height && np.z >= 0 && np.z < numSlices) {
									if (((float)g_Image3D[np.z][np.x + (np.y*width)]) > t && visited[np.x][np.y][np.z] == false) {
										visited[np.x][np.y][np.z] = true;
										q.push(np);
									}
								}
							}
						}

					}
				}
			}
		}
	}
	return ct;
}

std::vector<int> getEulerNumbersFromGImg(const std::vector<stbi_uc*>& g_Image3D, const std::vector< std::vector<float> >& g_Image3DF, float t, int width, int height, int numSlices, bool complement) {
	int v = 0; int e = 0; int f = 0; int c = 0;
	if (g_Image3DF.size() > 0) {
		for (int i = 0; i < width; i++) {
			for (int j = 0; j < height; j++) {
				for (int k = 0; k < numSlices; k++) {
					if (((float)g_Image3DF[k][i + j * width] > t && !complement) || ((float)g_Image3DF[k][i + j * width] <= t && complement)) {
						v += 1;

						if (i + 1 < width) {
							//compare (i,j,k), (i+1, j, k)
							if ((((float)g_Image3DF[k][(i + 1) + (width*j)]) > t && !complement) || (((float)g_Image3DF[k][(i + 1) + (width*j)]) <= t && complement)) {
								e += 1;
							}
						}

						if (j + 1 < height) {
							//compare (i,j,k), (i, j+1, k)
							if ((((float)g_Image3DF[k][i + (width*(j + 1))]) > t && !complement) || (((float)g_Image3DF[k][i + (width*(j + 1))]) <= t && complement)) {
								e += 1;
							}
						}

						if (k + 1 < numSlices) {
							//compare (i,j,k), (i, j+1, k)
							if ((((float)g_Image3DF[k + 1][i + (width*(j))]) > t && !complement) || (((float)g_Image3DF[k + 1][i + (width*(j))]) <= t && complement)) {
								e += 1;
							}
						}

						if (i + 1 < width && j + 1 < height) {
							//compare (i,j,k), (i, j+1, k)
							if (
								(((float)g_Image3DF[k][(i + 1) + (width*(j))]) > t &&
								((float)g_Image3DF[k][(i)+(width*(j + 1))]) > t && ((float)g_Image3DF[k][(i + 1) + (width*(j + 1))]) > t && !complement) ||
									(((float)g_Image3DF[k][(i + 1) + (width*(j))]) <= t &&
								((float)g_Image3DF[k][(i)+(width*(j + 1))]) <= t && ((float)g_Image3DF[k][(i + 1) + (width*(j + 1))]) <= t && complement)
								) {
								f += 1;
							}
						}

						if (i + 1 < width && k + 1 < numSlices) {
							//compare (i,j,k), (i, j+1, k)
							if (
								(((float)g_Image3DF[k][(i + 1) + (width*(j))]) > t &&
								((float)g_Image3DF[k + 1][(i)+(width*(j))]) > t && ((float)g_Image3DF[k + 1][(i + 1) + (width*(j))]) > t && !complement)
								||
								(((float)g_Image3DF[k][(i + 1) + (width*(j))]) <= t &&
								((float)g_Image3DF[k + 1][(i)+(width*(j))]) <= t && ((float)g_Image3DF[k + 1][(i + 1) + (width*(j))]) <= t && complement)
								) {
								f += 1;

							}
						}

						//add up faces in yz plane
						if (j + 1 < height && k + 1 < numSlices) {
							//compare (i,j,k), (i, j+1, k)
							if (
								((float)g_Image3DF[k][(i)+(width*(j + 1))] > t &&
								((float)g_Image3DF[k + 1][(i)+(width*(j))]) > t && ((float)g_Image3DF[k + 1][(i)+(width*(j + 1))]) > t && !complement)
								||
								((float)g_Image3DF[k][(i)+(width*(j + 1))] <= t &&
								((float)g_Image3DF[k + 1][(i)+(width*(j))]) <= t && ((float)g_Image3DF[k + 1][(i)+(width*(j + 1))]) <= t && complement)

								) {
								f += 1;
							}
						}

						//Add up cubes
						if (i + 1 < width && j + 1 < height && k + 1 < numSlices) {
							bool hasCube = true;
							for (int o = 0; o < cubeFrontMask.size(); o++) {
								int coord[3] = { i + cubeFrontMask[o][0], j + cubeFrontMask[o][1], k + cubeFrontMask[o][2] };
								if (((g_Image3DF[coord[2]][(coord[0]) + (width*(coord[1]))] <= t) && !complement) || ((g_Image3DF[coord[2]][(coord[0]) + (width*(coord[1]))] > t) && complement)) {

									hasCube = false;
									break;
								}
							}
							if (hasCube) {
								c += 1;
							}

						}
					}
				}
			}
		}
	}
	else {
		for (int i = 0; i < width; i++) {
			for (int j = 0; j < height; j++) {
				for (int k = 0; k < numSlices; k++) {
					if (((float)g_Image3D[k][i + j * width] > t && !complement) || ((float)g_Image3D[k][i + j * width] <= t && complement)) {
						v += 1;

						if (i + 1 < width) {
							//compare (i,j,k), (i+1, j, k)
							if ((((float)g_Image3D[k][(i + 1) + (width*j)]) > t && !complement) || (((float)g_Image3D[k][(i + 1) + (width*j)]) <= t && complement)) {
								e += 1;
							}
						}

						if (j + 1 < height) {
							//compare (i,j,k), (i, j+1, k)
							if ((((float)g_Image3D[k][i + (width*(j + 1))]) > t && !complement) || (((float)g_Image3D[k][i + (width*(j + 1))]) <= t && complement)) {
								e += 1;
							}
						}

						if (k + 1 < numSlices) {
							//compare (i,j,k), (i, j+1, k)
							if ((((float)g_Image3D[k + 1][i + (width*(j))]) > t && !complement) || (((float)g_Image3D[k + 1][i + (width*(j))]) <= t && complement)) {
								e += 1;
							}
						}

						if (i + 1 < width && j + 1 < height) {
							//compare (i,j,k), (i, j+1, k)
							if (
								(((float)g_Image3D[k][(i + 1) + (width*(j))]) > t &&
								((float)g_Image3D[k][(i)+(width*(j + 1))]) > t && ((float)g_Image3D[k][(i + 1) + (width*(j + 1))]) > t && !complement) ||
									(((float)g_Image3D[k][(i + 1) + (width*(j))]) <= t &&
								((float)g_Image3D[k][(i)+(width*(j + 1))]) <= t && ((float)g_Image3D[k][(i + 1) + (width*(j + 1))]) <= t && complement)
								) {
								f += 1;
							}
						}

						if (i + 1 < width && k + 1 < numSlices) {
							//compare (i,j,k), (i, j+1, k)
							if (
								(((float)g_Image3D[k][(i + 1) + (width*(j))]) > t &&
								((float)g_Image3D[k + 1][(i)+(width*(j))]) > t && ((float)g_Image3D[k + 1][(i + 1) + (width*(j))]) > t && !complement)
								||
								(((float)g_Image3D[k][(i + 1) + (width*(j))]) <= t &&
								((float)g_Image3D[k + 1][(i)+(width*(j))]) <= t && ((float)g_Image3D[k + 1][(i + 1) + (width*(j))]) <= t && complement)
								) {
								f += 1;

							}
						}

						//add up faces in yz plane
						if (j + 1 < height && k + 1 < numSlices) {
							//compare (i,j,k), (i, j+1, k)
							if (
								((float)g_Image3D[k][(i)+(width*(j + 1))] > t &&
								((float)g_Image3D[k + 1][(i)+(width*(j))]) > t && ((float)g_Image3D[k + 1][(i)+(width*(j + 1))]) > t && !complement)
								||
								((float)g_Image3D[k][(i)+(width*(j + 1))] <= t &&
								((float)g_Image3D[k + 1][(i)+(width*(j))]) <= t && ((float)g_Image3D[k + 1][(i)+(width*(j + 1))]) <= t && complement)

								) {
								f += 1;
							}
						}

						//Add up cubes
						if (i + 1 < width && j + 1 < height && k + 1 < numSlices) {
							bool hasCube = true;
							for (int o = 0; o < cubeFrontMask.size(); o++) {
								int coord[3] = { i + cubeFrontMask[o][0], j + cubeFrontMask[o][1], k + cubeFrontMask[o][2] };
								if (((g_Image3D[coord[2]][(coord[0]) + (width*(coord[1]))] <= t) && !complement) || ((g_Image3D[coord[2]][(coord[0]) + (width*(coord[1]))] > t) && complement)) {

									hasCube = false;
									break;
								}
							}
							if (hasCube) {
								c += 1;
							}

						}
					}
				}
			}
		}
	}
	return { v,e,f,c };
}

std::vector<int> getTopoFromGImg(const std::vector<stbi_uc*>& g_Image3D, const std::vector< std::vector<float> > & g_Image3DF, float t, int fgconn, int width, int height, int numSlices) {
	int h0 = labelComponentsT(g_Image3D, g_Image3DF, t, fgconn, width, height, numSlices);
	int h2 = labelComponentsTBg(g_Image3D, g_Image3DF, t, 1 - fgconn, width, height, numSlices) - 1;
	std::vector<int> eulNums = getEulerNumbersFromGImg(g_Image3D, g_Image3DF, t, width, height, numSlices, false);
	cout << eulNums[0] << " " << eulNums[1] << " " << eulNums[2] << " " << eulNums[3] << endl;
	int h1 = h0 + h2 - (eulNums[0] - eulNums[1] + eulNums[2] - eulNums[3]);
	return { h0,h2,h1 };
}

void exportGeneratorPly(string fName, std::set< std::vector<float>> & boundaryVts) {
	std::ofstream outPly;
	outPly.open(fName);
	outPly << "ply" << endl;
	outPly << "format ascii 1.0" << endl;
	outPly << "element vertex " << 8 * boundaryVts.size() << endl;
	outPly << "property float32 x" << endl;
	outPly << "property float32 y" << endl;
	outPly << "property float32 z" << endl;
	outPly << "element edge " << 0 << endl;
	//18 * boundaryVts.size() 
	outPly << "property int32 vertex1" << endl;
	outPly << "property int32 vertex2" << endl;
	outPly << "element face  " << 12 * boundaryVts.size() << endl;
	outPly << "property list uint8 int32 vertex_indices" << endl;
	outPly << "end_header" << endl;
	for (auto vertex : boundaryVts) {
		for (int i = 0; i < 2; i++) {
			for (int j = 0; j < 2; j++) {
				for (int k = 0; k < 2; k++) {
					outPly << vertex[0] + i << " " << vertex[1] + j << " " << vertex[2] + k << endl;
				}
			}
		}
	}
	//Write out edges for each boundary vertex

	int vtxCt = 0;
	for (auto vertex : boundaryVts) {
		outPly << "3 " << vtxCt * 8 + 0 << " " << vtxCt * 8 + 1 << " " << vtxCt * 8 + 3 << endl;
		outPly << "3 " << vtxCt * 8 + 0 << " " << vtxCt * 8 + 2 << " " << vtxCt * 8 + 3 << endl;
		outPly << "3 " << vtxCt * 8 + 0 << " " << vtxCt * 8 + 2 << " " << vtxCt * 8 + 6 << endl;
		outPly << "3 " << vtxCt * 8 + 0 << " " << vtxCt * 8 + 4 << " " << vtxCt * 8 + 6 << endl;
		outPly << "3 " << vtxCt * 8 + 0 << " " << vtxCt * 8 + 1 << " " << vtxCt * 8 + 5 << endl;
		outPly << "3 " << vtxCt * 8 + 0 << " " << vtxCt * 8 + 4 << " " << vtxCt * 8 + 5 << endl;
		outPly << "3 " << vtxCt * 8 + 4 << " " << vtxCt * 8 + 5 << " " << vtxCt * 8 + 7 << endl;
		outPly << "3 " << vtxCt * 8 + 4 << " " << vtxCt * 8 + 6 << " " << vtxCt * 8 + 7 << endl;
		outPly << "3 " << vtxCt * 8 + 1 << " " << vtxCt * 8 + 5 << " " << vtxCt * 8 + 7 << endl;
		outPly << "3 " << vtxCt * 8 + 1 << " " << vtxCt * 8 + 3 << " " << vtxCt * 8 + 7 << endl;
		outPly << "3 " << vtxCt * 8 + 2 << " " << vtxCt * 8 + 3 << " " << vtxCt * 8 + 7 << endl;
		outPly << "3 " << vtxCt * 8 + 2 << " " << vtxCt * 8 + 6 << " " << vtxCt * 8 + 7 << endl;
		vtxCt += 1;
	}
	outPly.close();
}


void reduceGenerators(Graph & G, std::vector<node> & nodes, map<std::vector<int>, int> & edgeWt) {
	grapht fgG; //Made up of cores and cuts
	grapht bgG; //Made up of neighborhoods and fills
	for (int i = 0; i < nodes.size(); i++) {
		add_vertex(fgG); add_vertex(bgG);
	}
	typedef boost::graph_traits<Graph>::edge_iterator edge_iter;
	edge_iter ei, ei_end;
	for (tie(ei, ei_end) = edges(G); ei != ei_end; ++ei) {
		int v1 = (int)ei->m_source; int v2 = (int)ei->m_target;
		if (((int)nodes[v1].inFg) == 1 && ((int)nodes[v2].inFg) == 1) {
			if (edgeWt[{v1, v2}] == 1) {
				add_edge(v1, v2, fgG);
			}
		}
		if (((int)nodes[v1].inFg) == 0 && ((int)nodes[v2].inFg) == 0) {
				add_edge(v1, v2, bgG);
		}
	}
	for (int i = 0; i < nodes.size(); i++) {
		nodes[i].isArticulate = false;
	}
	map<int, map<int, int> > fgLocalArtConnectivity = findComponents(fgG, nodes, 1);
	map<int, map<int, int> > bgLocalArtConnectivity = findComponents(bgG, nodes, 0);
	for (int i = 0; i < nodes.size(); i++) {
		if ((int)nodes[i].type == CUT) {
			int changeH0 = 0;
			if (nodes[i].isArticulate) {
				map<int, int> nodeToComp = fgLocalArtConnectivity[i];
				std::vector<int> uniqueComps;
				for (std::map<int, int>::iterator iter = nodeToComp.begin(); iter != nodeToComp.end(); ++iter)
				{
					uniqueComps.push_back(iter->second);
				}
				std::sort(uniqueComps.begin(), uniqueComps.end());
				uniqueComps.erase(unique(uniqueComps.begin(), uniqueComps.end()), uniqueComps.end());
				changeH0 = uniqueComps.size() - 1;
			}

			std::vector<int> nBgComps;
			int adjFg = 0;
			auto neighbours = adjacent_vertices(i, G); //If connect to BG, how many components would it connect?
			for (auto u : make_iterator_range(neighbours)) {
				if (((int)nodes[u].inFg) == 0 && nodes[u].compIndex != -1) {
					nBgComps.push_back(nodes[u].compIndex);
				}
				if (((int)nodes[u].inFg) == 1) {
					adjFg += 1;
				}
			}
			if (adjFg == 0 && changeH0 == 0) {
				changeH0 = -1;
			}

			std::sort(nBgComps.begin(), nBgComps.end());
			nBgComps.erase(unique(nBgComps.begin(), nBgComps.end()), nBgComps.end());
			int changeH2 = -((int)nBgComps.size() - 1); //Would connect together this many background components
			int eulerNum = nodes[i].v - nodes[i].e + nodes[i].f - nodes[i].c;
			int deltaH1 = (changeH0)+(changeH2)-(-eulerNum);
			if (changeH0 + changeH2 + deltaH1 >= 0) {
				nodes[i].type = CORE;
			}
		}
		else {
			if ((int)nodes[i].type == FILL) {
				int changeH2 = 0;
				if (nodes[i].isArticulate) {
					map<int, int> nodeToComp = bgLocalArtConnectivity[i];
					std::vector<int> uniqueComps;
					for (std::map<int, int>::iterator iter = nodeToComp.begin(); iter != nodeToComp.end(); ++iter)
					{
						uniqueComps.push_back(iter->second);
					}
					std::sort(uniqueComps.begin(), uniqueComps.end());
					uniqueComps.erase(unique(uniqueComps.begin(), uniqueComps.end()), uniqueComps.end());
					changeH2 = uniqueComps.size() - 1;
				}
				std::vector<int> nFgComps; int adjBg = 0;
				auto neighbours = adjacent_vertices(i, G); //If connect to BG, how many components would it connect?
				for (auto u : make_iterator_range(neighbours)) {
					if (((int)nodes[u].inFg) == 1 && nodes[u].compIndex != -1) {
						nFgComps.push_back(nodes[u].compIndex);
					}
					if (((int)nodes[u].inFg) == 0) {
						adjBg += 1;
					}
				}

				if (adjBg == 0 && changeH2 == 0) {
					//cout << "isolated cut " << i << endl;
					changeH2 = -1;
				}
				std::sort(nFgComps.begin(), nFgComps.end());
				nFgComps.erase(unique(nFgComps.begin(), nFgComps.end()), nFgComps.end());
				int changeH0 = -((int)nFgComps.size() - 1); //Would connect together this many background components

				int eulerNum = nodes[i].v - nodes[i].e + nodes[i].f - nodes[i].c;
				int deltaH1 = (changeH0)+(changeH2)-(eulerNum);
				if (changeH0 + changeH2 + deltaH1 >= 0) {
					nodes[i].type = N;
				}
			}
		}
	}

}

void toMRCFile(const char* fname, int sizex, int sizey, int sizez, float dmin, float dmax, float S, float vizEps, std::vector<node> & origNodes, std::vector<node> & nodes,
	std::vector<stbi_uc*>& g_Image3D, std::vector< std::vector<float> >& g_Image3DF, std::vector<uint32_t*>& labels, bool thresholdMode
	)
{
	FILE* fout = fopen(fname, "wb");

	// Write header
	fwrite(&sizex, sizeof(int), 1, fout);
	fwrite(&sizey, sizeof(int), 1, fout);
	fwrite(&sizez, sizeof(int), 1, fout);

	int mode = 2;
	fwrite(&mode, sizeof(int), 1, fout);

	int off[3] = { 0,0,0 };
	int intv[3] = { sizex - 1, sizey - 1, sizez - 1 };
	fwrite(off, sizeof(int), 3, fout);
	fwrite(intv, sizeof(int), 3, fout);

	float cella[3] = { 2,2,2 };
	float cellb[3] = { 90,90,90 };
	fwrite(cella, sizeof(float), 3, fout);
	fwrite(cellb, sizeof(float), 3, fout);

	int cols[3] = { 1,2,3 };
	fwrite(cols, sizeof(int), 3, fout);

	float ds[3] = { dmin, dmax, 0 };
	fwrite(ds, sizeof(float), 3, fout);

	int zero = 0;
	for (int i = 22; i < 256; i++)
	{
		fwrite(&zero, sizeof(int), 1, fout);
	}
	int numAdded = 0;
	int allFillsCost = 0;
	int allCutsCost = 0;
	int oursCost = 0;
	std::vector<uint8_t*> bImg;
	std::vector<uint8_t*> topoImg;
	if (g_Image3DF.size() == 0) {
		for (int s = 0; s < sizez; s++) {
			int digits = numDigits(s);
			string numStr = "";
			for (int n = 0; n < 4 - digits; n++) {
				numStr += "0";

			}
			numStr += std::to_string(s);

			float* label8 = new float[sizex * sizey]();
			uint8_t* label8T = new uint8_t[sizex * sizey]();
			for (int j = 0; j < sizey; j++) {
				for (int i = 0; i < sizex; i++) {
				
					label8T[i + j * sizex] = 0;
					label8[i + j * sizex] = (int)g_Image3D[s][i + j * sizex];
					if ((int)origNodes[Label(labels[s][i + j * sizex])].type == 3) {
						allFillsCost += 1;
					}
					if ((int)origNodes[Label(labels[s][i + j * sizex])].type == 2) {
						allCutsCost += 1;
					}
					if (((int)origNodes[Label(labels[s][i + j * sizex])].inFg) == 1) {
						label8T[i + j * sizex] = 1;
						if ((int)origNodes[Label(labels[s][i + j * sizex])].type == 3) {
							label8[i + j * sizex] = min((float)255, (float)(S + vizEps));
							oursCost += 1;
						}
						else {
							label8[i + j * sizex] = (int)g_Image3D[s][i + j * sizex];
						}
					}
					else {
						
						if ((int)origNodes[Label(labels[s][i + j * sizex])].type == 2) {
							if (thresholdMode) {
								label8[i + j * sizex] = 0;
							}
							else {
								label8[i + j * sizex] = max((float)0, (float)(S - vizEps));
							}
							oursCost += 1;
						}
						else {
							if (thresholdMode) {
								label8[i + j * sizex] = 0;
							}
							else {
								label8[i + j * sizex] = max(min((float)g_Image3D[s][i + j * sizex], ((float)(S))), (float)0.0);
							}
						}
						
						if ((int)nodes[Label(labels[s][i + j * sizex])].type == 1) {
							if (thresholdMode) {
								label8[i + j * sizex] = 0;
							}
							else {
								label8[i + j * sizex] = max(min((float)g_Image3D[s][i + j * sizex], ((float)(S - vizEps))), (float)0.0);
							}
						}
					}
					fwrite(&label8[i + j * sizex], sizeof(float), 1, fout);
				}
			}
			topoImg.push_back(label8T);
			numAdded += 1;
		}
	}
	else {
		for (int s = 0; s < sizez; s++) {
			int digits = numDigits(s);
			string numStr = "";
			for (int n = 0; n < 4 - digits; n++) {
				numStr += "0";

			}
			numStr += std::to_string(s);

			float* label8 = new float[sizex * sizey]();
			uint8_t* label8T = new uint8_t[sizex * sizey]();
			
			for (int j = 0; j < sizey; j++) {
				for (int i = 0; i < sizex; i++) {
					label8T[i + j * sizex] = 0;
					label8[i + j * sizex] = (int)g_Image3DF[s][i + j * sizex];
					if ((int)origNodes[Label(labels[s][i + j * sizex])].type == 3) {
						allFillsCost += 1;
					}
					if ((int)origNodes[Label(labels[s][i + j * sizex])].type == 2) {
						allCutsCost += 1;
					}
					if (((int)origNodes[Label(labels[s][i + j * sizex])].inFg) == 1) {
						label8T[i + j * sizex] = 1;
						if ((int)origNodes[Label(labels[s][i + j * sizex])].type == 3) {

							label8[i + j * sizex] = (float)(S + vizEps);
							oursCost += 1;
						}
						else {
							label8[i + j * sizex] = (float)g_Image3DF[s][i + j * sizex];
						}
					}
					else {
						if ((int)origNodes[Label(labels[s][i + j * sizex])].type == 2) {
							if (thresholdMode) {
								label8[i + j * sizex] = dmin;
							}
							else {
								label8[i + j * sizex] = (float)(S - vizEps);

							}
							oursCost += 1;
						}
						else {
							if (thresholdMode) {
								label8[i + j * sizex] = dmin;
							}
							else {
								label8[i + j * sizex] = min(S, (float)g_Image3DF[s][i + j * sizex]);
							}
						}
					}
					fwrite(&label8[i + j * sizex], sizeof(float), 1, fout);
				}
			}
			topoImg.push_back(label8T);
			numAdded += 1;
		}
	}

	std::vector<int> topoNums = getTopoFromBImg(topoImg, 1, sizex, sizey, numAdded);

	std::cout << "Final topology: Components: " << topoNums[0] << " Cavities: " << topoNums[1] << " Cycles: " << topoNums[2] << std::endl;


	fclose(fout);
}

int main(int argc, char **argv)
{
	//Initialize parameters and default values
	string inFile, outFile; 
	int numSlices = 0; //number of slices
	int fgConn = 1; 
	float N = -INFINITY;
	float S = 0;
	float C = INFINITY; //neighborhood, shape, and core thresholds
	auto start = std::chrono::high_resolution_clock::now();
	int exportPng = 1;
	float vizEps = 10; //Default iso-surface visualization parameter for .png
	int hypernodeSize = 1; 
	int productThresh = 1000000;
	int globalSteinerTime = 8;
	int localSteinerTime = 4; 
	params.heurbbtime = 3;
	int greedyMode = 1;
	int nodesToFix = 1; 
	int bbTime = 3;
	int genMode = 0;
	int inFileType = 0;
	int openItr = 0;
	int closeItr = 0;
	int exportGen = 0;
	int greedy = 0;
	int se = 0;
	bool shapeTopo = false;
	bool showGeomCost = false;
	bool help = false;
	bool thresholdOut = false;
	bool finalTopo = true;
	bool rootShape = false;
	string rootShapeFile = "";
	bool rootMorpho = false;
	float autoOffset = 5;

	//Parse arguments
	parseArguments(argc, inFile, outFile, numSlices, argv, N, S, C, vizEps, hypernodeSize, productThresh, globalSteinerTime, localSteinerTime, 
		greedyMode, nodesToFix, bbTime, genMode, openItr, closeItr, inFileType, exportGen, greedy, 
		se, shapeTopo, showGeomCost, help, thresholdOut, finalTopo, rootShape, rootShapeFile, rootMorpho, autoOffset);

	if (help) {
		return 1;
	}

	if (openItr > 0 || closeItr > 0) {
		genMode = 1;
	}

	if (inFile.substr(inFile.length() - 4) == ".tif") {
		inFileType = 1;
		vizEps = 0.01; //Default iso-surface visualization parameter for .tif
	}
	else if (inFile.substr(inFile.length() - 4) == ".dist") {
		inFileType = 2;
		vizEps = 0.01; //Default iso-surface visualization parameter for .dist
	}

	float epsilon = 0.05;
	int width, height;
	string filename0 = inFile + "0000.png";
	std::vector<stbi_uc*> g_Image3D; std::vector< std::vector<float> > g_Image3DF;
	float minVal = 1000000.0; float maxVal = -1000000.0;
	bool cutOnly = false;
	bool fillOnly = false;
	if (C == INFINITY && N != -INFINITY) {
		cutOnly = true;
	}
	if (C != INFINITY && N == -INFINITY) {
		fillOnly = true;
	}
	
	loadImages(g_Image3D, numSlices, inFile, width, height, inFileType, g_Image3DF, C, N, S, minVal, maxVal, cutOnly, fillOnly, rootShape, rootShapeFile, autoOffset);
	if (rootShape) {
		if (rootMorpho) {
			N = S - 4;
			C = S;
		}
		cout << "Shape set automatically to " << S << " K: " << C << " " << " N: " << N << endl;
	}
	if (g_Image3DF.size() > 0) {
		numSlices = g_Image3DF.size(); //If Tiff file format, automatically determine # of "slices", AKA depth of image volume
	}
	if (numSlices == 0) {
		std::cout << "No slices in .tif, .dist,  or directory. Check that .tif, .dist, or directory name and location are correction. " << std::endl;
	}
	auto loadTime = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double> elapsed = loadTime - start;
	std::cout << "Finished loading image slices after " << elapsed.count() << " seconds" << endl;
	std::cout << "Kernel: " << C << " Shape: " << S << " Neighborhood: " << N << endl;
	std::vector<uint32_t *> labels;
	std::vector<bool *> inPQ;
	for (int s = 0; s < numSlices; s++) {
		uint32_t * labelSlice = new uint32_t[width*height]();
		for (int i = 0; i < width; i++) {
			for (int j = 0; j < height; j++) {
				labelSlice[i + j * width] = unvisited;
			}
		}
		labels.push_back(labelSlice);
	}
	
	

	std::ifstream simpleFile("simpleDictionaryFull.bin", ios::binary);
	std::vector<unsigned char> simpleDictionary3D(std::istreambuf_iterator<char>(simpleFile), {});
	if (!simpleFile) {
		std::cout << "Simplicity file 1 not read properly, " << simpleFile.gcount() << " bytes read " << endl;
		return 0;
	}
	auto initTime = std::chrono::high_resolution_clock::now();
	elapsed = initTime - loadTime;
	std::cout << "Label array and simple dictionary initialization time: " << elapsed.count() << " seconds" << endl;
	int labelCt = -1;
	std::vector<node> nodes;

	//Flood fill core and neighborhood
	std::cout << "Flood filling kernel and neighborhood components in one volume sweep " << endl;
	priority_queue<weightedCoord> corePQ, nPQ;
	
	if (genMode == 0) {
		floodFillCAndN(g_Image3D, labels, numSlices, width, height, C, S, N, labelCt, nodes, corePQ, nPQ, g_Image3DF);
	}
	auto floodTime = std::chrono::high_resolution_clock::now();
	elapsed = floodTime - initTime;
	std::cout << "Flood filling time: " << elapsed.count() << " seconds" << endl;
	std::cout << "Finished flood filling, # of kernel + neighborhood components: " << labelCt << endl;
	std::cout << "Begin inflating kernel" << endl;
	if (genMode == 0) {
		inflateCore(labels, g_Image3D, nodes, corePQ, numSlices, width, height, C, S, simpleDictionary3D, g_Image3DF);
	}
	auto inflateTime = std::chrono::high_resolution_clock::now();
	elapsed = inflateTime - floodTime;
	std::cout << "Kernel inflation time: " << elapsed.count() << " seconds" << endl;
	std::cout << "Begin deflating neighborhood" << endl;
	
	if (genMode == 0) {
		deflateNeighborhood(labels, g_Image3D, nodes, nPQ, numSlices, width, height, N, S, simpleDictionary3D, g_Image3DF);
	}
	

	auto deflateTime = std::chrono::high_resolution_clock::now();
	elapsed = deflateTime - inflateTime;
	std::cout << "Neighborhood deflation time: " << elapsed.count() << " seconds" << endl;
	std::cout << "Begin finding cuts and fills: " << endl;
	Graph G; map< std::vector<int>, int> edgeWt; int cAndN = labelCt;
	if (genMode == 0) {

		for (int i = 0; i < width; i++) {
			for (int j = 0; j < height; j++) {
				for (int k = 0; k < numSlices; k++) {
					if (labels[k][i + j * width] == unvisited) {
						if (g_Image3DF.size() > 0) {
							if ((float)g_Image3DF[k][i + j * width] > S) { //Flood fill cut
								labelCt += 1;
								node n; n.type = 2; n.inFg = 1; n.index = labelCt;
								identifyCutFromPixel3D(labels, g_Image3D, G, i, j, k, width, height, numSlices, labelCt, n, S, nodes, edgeWt, genMode, g_Image3DF);
								nodes.push_back(n);
							}

							if ((float)g_Image3DF[k][i + j * width] <= S) { //Flood fill fill
								labelCt += 1;
								node n; n.type = 3; n.inFg = 0; n.index = labelCt;
								identifyFillFromPixel3D(labels, g_Image3D, G, i, j, k, width, height, numSlices, labelCt, n, S, nodes, edgeWt, genMode, g_Image3DF);
								nodes.push_back(n);
							}
						}
						else {
							if ((float)g_Image3D[k][i + j * width] > S) { //Flood fill cut
								labelCt += 1;
								node n; n.type = 2; n.inFg = 1; n.index = labelCt;
								identifyCutFromPixel3D(labels, g_Image3D, G, i, j, k, width, height, numSlices, labelCt, n, S, nodes, edgeWt, genMode, g_Image3DF);
								nodes.push_back(n);
							}

							if ((float)g_Image3D[k][i + j * width] <= S) { //Flood fill fill
								labelCt += 1;
								node n; n.type = 3; n.inFg = 0; n.index = labelCt;
								identifyFillFromPixel3D(labels, g_Image3D, G, i, j, k, width, height, numSlices, labelCt, n, S, nodes, edgeWt, genMode, g_Image3DF);
								nodes.push_back(n);
							}
						}
					}
					if (labels[k][i + j * width] == unvisited) {
						cout << "still unvisited " << (float)g_Image3D[k][i + j * width] << endl;
					}
				}
			}
		}
		cout << "Done finding cuts and fills" << endl;
	}
	
	for (int i = 0; i < nodes.size(); i++) {
		if ((int)nodes[i].type == CUT || (int)nodes[i].type == FILL) {
			nodes[i].labelCost = nodes[i].floatCost;
		}
	}
	std::vector<int> eulerNums;
	if (rootMorpho) {
		/**
		* 
	std::vector< std::vector<int> > fillImg; 
	std::vector< std::vector<int> > cutImg;
		std::vector<std::vector<int>> morphoImg;
		for (int k = 0; k < numSlices; k++) {
			std::vector<int> slice(width * height, 0);
			for (int i = 0; i < width; i++) {
				for (int j = 0; j < height; j++) {
					int val = ((int)g_Image3D[k][i + j * width]) > S ? 1 : 0;

					slice[i + j * width] = val;

				}
			}
			std::vector<int> sliceN(width * height, 1);
			morphoImg.push_back(slice);
		}
		std::vector<node> nodes;
		generatingOpenCloseGenerators(morphoImg, labels, numSlices, width, height, nodes, openItr, closeItr, G, edgeWt, se);
		
		for (int i = 0; i < width; i++) {
			for (int j = 0; j < height; j++) {
				for (int k = 0; k < numSlices; k++) {
					if (labels[k][i + j * width] == unvisited) {
						if (g_Image3DF.size() > 0) {
							if ((float)g_Image3DF[k][i + j * width] > S) { //Flood fill cut
								labelCt += 1;
								node n; n.type = 2; n.inFg = 1; n.index = labelCt;
								identifyCutFromPixel3D(labels, g_Image3D, G, i, j, k, width, height, numSlices, labelCt, n, S, nodes, edgeWt, genMode, g_Image3DF);
								nodes.push_back(n);
							}

							if ((float)g_Image3DF[k][i + j * width] <= S) { //Flood fill fill
								labelCt += 1;
								node n; n.type = 3; n.inFg = 0; n.index = labelCt;
								identifyFillFromPixel3D(labels, g_Image3D, G, i, j, k, width, height, numSlices, labelCt, n, S, nodes, edgeWt, genMode, g_Image3DF);
								nodes.push_back(n);
							}
						}
						else {
							if ((float)g_Image3D[k][i + j * width] > S) { //Flood fill cut
								labelCt += 1;
								node n; n.type = 2; n.inFg = 1; n.index = labelCt;
								identifyCutFromPixel3D(labels, g_Image3D, G, i, j, k, width, height, numSlices, labelCt, n, S, nodes, edgeWt, genMode, g_Image3DF);
								nodes.push_back(n);
							}

							if ((float)g_Image3D[k][i + j * width] <= S) { //Flood fill fill
								labelCt += 1;
								node n; n.type = 3; n.inFg = 0; n.index = labelCt;
								identifyFillFromPixel3D(labels, g_Image3D, G, i, j, k, width, height, numSlices, labelCt, n, S, nodes, edgeWt, genMode, g_Image3DF);
								nodes.push_back(n);
							}
						}
					}

				}
			}
		}
		**/
		eulerNums = getEulerNumbers(nodes, labels, width, height, numSlices);

		grapht fgG = grapht(); grapht bgG = grapht(); grapht coreG = grapht(); grapht nG = grapht(); grapht fgGWithFills = grapht(); grapht bgGWithCuts = grapht();

			for (int i = 0; i < nodes.size(); i++) {
				add_vertex(fgG); add_vertex(fgGWithFills); add_vertex(bgG); add_vertex(bgGWithCuts); add_vertex(coreG); add_vertex(nG);
			}
		typedef boost::graph_traits<Graph>::edge_iterator edge_iter;
		edge_iter ei, ei_end;

		for (tie(ei, ei_end) = edges(G); ei != ei_end; ++ei) {
			int v1 = (int)ei->m_source; int v2 = (int)ei->m_target;
			if ((((int)nodes[v1].type) == 0 || ((int)nodes[v1].type) == 2 || ((int)nodes[v1].type) == 3) && (((int)nodes[v2].type) == 0 || ((int)nodes[v2].type) == 2 || ((int)nodes[v2].type) == 3) && (nodes[v1].valid) && (nodes[v2].valid)) { //
				//If src vtx or tgt vtx is core, cut, or fill, add to fg graph 
				if (edgeWt[{(int)ei->m_source, (int)ei->m_target}] == 1) { //If fg connectivity is cube or fg connectivity is structCross3D and edge is strong
					add_edge(v1, v2, 0, fgGWithFills);
				}
			}
			if ((((int)nodes[v1].type) == 0 || ((int)nodes[v1].type) == 2) && (((int)nodes[v2].type) == 0 || ((int)nodes[v2].type) == 2) && (nodes[v1].valid) && (nodes[v2].valid)) { //
				add_edge(v1, v2, 0, fgG);
			}
			if (((int)nodes[v1].type) == CORE && ((int)nodes[v2].type) == CORE && (nodes[v1].valid) && (nodes[v2].valid)) {
				//If src vtx or tgt vtx is core, cut, or fill, add to fg graph 
				if (edgeWt[{(int)ei->m_source, (int)ei->m_target}] == 1) { //If fg connectivity is cube or fg connectivity is structCross3D and edge is strong
					add_edge(v1, v2, 0, coreG);
				}
			}

			if (((int)nodes[v1].inFg) == 0 && ((int)nodes[v2].inFg) == 0 && (nodes[v1].valid) && (nodes[v2].valid)) {
				//If src vtx or tgt vtx is core, cut, or fill, add to fg graph 
				add_edge(v1, v2, 0, bgG);
			}
			if ((((int)nodes[v1].type) == 1 || ((int)nodes[v1].type) == 2 || ((int)nodes[v1].type) == 3) && (((int)nodes[v2].type) == 1 || ((int)nodes[v2].type) == 2 || ((int)nodes[v2].type) == 3) && (nodes[v1].valid) && (nodes[v2].valid)) { //
				add_edge(v1, v2, 0, bgGWithCuts);
			}

			if (((int)nodes[v1].type) == N && ((int)nodes[v2].type) == N && (nodes[v1].valid) && (nodes[v2].valid)) {
				//If src vtx or tgt vtx is core, cut, or fill, add to fg graph 
				add_edge(v1, v2, 0, nG);
			}
		}
		cout << "begin simplifying generators " << endl;
		simplifyGenerators(labels, g_Image3D, g_Image3DF, numSlices, width, height, S, nodes, G, fgG, fgGWithFills, bgGWithCuts, coreG, bgG, nG, edgeWt, fgConn, simpleDictionary3D, genMode, true);
		cout << "done simplifying generators " << endl;
		for (int i = 0; i < nodes.size(); i++) {
			nodes[i].isArticulate = false;
			nodes[i].v = 0;
			nodes[i].e = 0;
			nodes[i].f = 0;
			nodes[i].c = 0;
		}
		eulerNums = getEulerNumbers(nodes, labels, width, height, numSlices);
		while (num_vertices(fgG) < nodes.size()) {
			add_vertex(fgG);
		}
		while (num_vertices(bgG) < nodes.size()) {
			add_vertex(bgG);
		}
		map<int, map<int, int> > fgLocalArtConnectivity = findComponents(fgG, nodes, 1);
		map<int, map<int, int> > bgLocalArtConnectivity = findComponents(bgG, nodes, 0);
		for (int n = 0; n < nodes.size(); n++) {
			for (int i = 0; i < nodes.size(); i++) {

				if ((int)nodes[i].type == FILL) {
					int changeH2 = 0;
					if (nodes[i].isArticulate) {
						map<int, int> nodeToComp = bgLocalArtConnectivity[i];
						std::vector<int> uniqueComps;
						for (std::map<int, int>::iterator iter = nodeToComp.begin(); iter != nodeToComp.end(); ++iter)
						{
							uniqueComps.push_back(iter->second);
						}
						std::sort(uniqueComps.begin(), uniqueComps.end());
						uniqueComps.erase(unique(uniqueComps.begin(), uniqueComps.end()), uniqueComps.end());
						changeH2 = uniqueComps.size() - 1;
					}
					std::vector<int> nFgComps; int adjBg = 0;
					auto neighbours = adjacent_vertices(i, G); //If connect to BG, how many components would it connect?
					for (auto u : make_iterator_range(neighbours)) {
						if (((int)nodes[u].inFg) == 1 && nodes[u].compIndex != -1) {
							nFgComps.push_back(nodes[u].compIndex);
						}
						if (((int)nodes[u].inFg) == 0) {
							adjBg += 1;
						}
					}

					if (adjBg == 0 && changeH2 == 0) {
						changeH2 = -1;
					}
					std::sort(nFgComps.begin(), nFgComps.end());
					nFgComps.erase(unique(nFgComps.begin(), nFgComps.end()), nFgComps.end());
					int changeH0 = -((int)nFgComps.size() - 1); //Would connect together this many background components

					int eulerNum = nodes[i].v - nodes[i].e + nodes[i].f - nodes[i].c;
					int deltaH1 = (changeH0)+(changeH2)-(eulerNum);
					if (changeH2 != 0 || deltaH1 < 0) {
						nodes[i].type = CORE;
						nodes[i].inFg = 1;
						clear_vertex(i, bgG);
						addToSubgraph(G, fgG, nodes, nodes[i], 1, fgConn, edgeWt);
					}
				}

			}
		}


		for (int s = 0; s < numSlices; s++) {
			int digits = numDigits(s);
			string numStr = "";
			for (int n = 0; n < 4 - digits; n++) {
				numStr += "0";

			}
			numStr += std::to_string(s);
			
			string filename = outFile + numStr + ".png";
			uint8_t* label8 = new uint8_t[width * height]();
			for (int i = 0; i < width; i++) {
				for (int j = 0; j < height; j++) {
					label8[i + j * width] = 0;
					//cout << labels.size() << " " << Label(labels[s][i + j * width]) << endl;
					//if ((int)labels[s][i + j * width] == unvisited) {
						//cout << "unvisited " << endl;
					//}
					if (nodes[Label(labels[s][i + j * width])].inFg == 1) {
						
						label8[i + j * width] = 1;
						
						if (g_Image3D[s][i + j * width] <= S) {
							label8[i + j * width] = S + vizEps;
						}
						else {
							label8[i + j * width] = g_Image3D[s][i + j * width];
						}


					}
					else {
						//cout << "not in fg " << endl;
						if (g_Image3D[s][i + j * width] > S) {
							label8[i + j * width] = S - vizEps;
						}
						else {
							label8[i + j * width] = g_Image3D[s][i + j * width];
						}
					}
					
					
				}
			}
			fill2DCavities(label8, S, 0, width, height, numSlices, vizEps);
			int wrote = stbi_write_png(filename.c_str(), width, height, 1, label8, width);
		}

		return 1;
	}
	if (genMode == 1) {
		if (shapeTopo) {
			std::vector<int> eulerS = getTopoFromGImg(g_Image3D, g_Image3DF, S, fgConn, width, height, numSlices);
			cout << "Original Shape Topology: Components: " << eulerS[0] << " Cavities: " << eulerS[1] << " Cycles: " << eulerS[2] << endl;
		}
		std::vector< std::vector<int> > shape;
		if (g_Image3DF.size() > 0) {
			for (int k = 0; k < numSlices; k++) {
				std::vector<int> slice = std::vector<int>(width * height, 0);
				for (int i = 0; i < width; i++) {
					for (int j = 0; j < height; j++) {
						if (((float)g_Image3DF[k][i + j * width]) > S) {
							slice[i + j * width] = 1;
						}
						else {
							slice[i + j * width] = 0;
						}
					}
				}
				shape.push_back(slice);
			}
		}
		else {
			for (int k = 0; k < numSlices; k++) {
				std::vector<int> slice = std::vector<int>(width * height, 0);
				for (int i = 0; i < width; i++) {
					for (int j = 0; j < height; j++) {
						if (((float)g_Image3D[k][i + j * width]) > S) {
							slice[i + j * width] = 1;
						}
						else {
							slice[i + j * width] = 0;
						}
					}
				}
				shape.push_back(slice);
			}
		}

		generatingOpenCloseGenerators(shape, labels, numSlices, width, height, nodes, openItr, closeItr, G, edgeWt, se);
		eulerNums = getEulerNumbers(nodes, labels, width, height, numSlices);
		for (int i = 0; i < nodes.size(); i++) {
			//labelcost is currently intensity based, add euler number as top consideration
			if (nodes[i].valid) {
				if (((int)nodes[i].type) == 2 || ((int)nodes[i].type) == 3) {
					int64_t eulerSum = nodes[i].v - nodes[i].e + nodes[i].f - nodes[i].c;
				}
			}
		}
		reduceGenerators(G, nodes, edgeWt);
		for (int i = 0; i < nodes.size(); i++) {
			nodes[i].v = 0; nodes[i].e = 0; nodes[i].f = 0; nodes[i].c = 0;
		}

		eulerNums = getEulerNumbers(nodes, labels, width, height, numSlices);

		int64_t sumNodeCost = 0;
		int cfCt = 0;

		for (int i = 0; i < nodes.size(); i++) {

			if (nodes[i].valid) {
				if (((int)nodes[i].type) == 2 || ((int)nodes[i].type) == 3) {
					cfCt += 1;
					sumNodeCost += abs((int)nodes[i].labelCost);
				}
			}
		}
		for (int i = 0; i < nodes.size(); i++) {
			//labelcost is currently intensity based, add euler number as top consideration
			if (nodes[i].valid) {
				if (((int)nodes[i].type) == 2 || ((int)nodes[i].type) == 3) {
					int64_t eulerSum = nodes[i].v - nodes[i].e + nodes[i].f - nodes[i].c;
					nodes[i].intensity = nodes[i].labelCost;
					int64_t topoCost = eulerSum;
					nodes[i].labelCost = ((topoCost * cfCt * sumNodeCost) / 0.5) + nodes[i].labelCost;
				}
			}
		}
	}
	
	auto cAndFTime = std::chrono::high_resolution_clock::now();
	elapsed = cAndFTime - deflateTime;
	std::cout << "Finding cuts and fills time: " << elapsed.count() << " seconds" << endl;
	grapht fgG = grapht(); grapht bgG = grapht(); grapht coreG = grapht(); grapht nG = grapht(); grapht fgGWithFills = grapht(); grapht bgGWithCuts = grapht();
	if (genMode == 0) {
		for (int i = 0; i < nodes.size(); i++) {
			add_vertex(fgG); add_vertex(fgGWithFills); add_vertex(bgG); add_vertex(bgGWithCuts); add_vertex(coreG); add_vertex(nG);
		}
	}
	typedef boost::graph_traits<Graph>::edge_iterator edge_iter;
	edge_iter ei, ei_end;
	if (genMode == 0) {

		for (tie(ei, ei_end) = edges(G); ei != ei_end; ++ei) {
			int v1 = (int)ei->m_source; int v2 = (int)ei->m_target;
			if ( (((int)nodes[v1].type)==0 || ((int)nodes[v1].type)==2 || ((int)nodes[v1].type) == 3) && (((int)nodes[v2].type) == 0 || ((int)nodes[v2].type) == 2 || ((int)nodes[v2].type) == 3) && (nodes[v1].valid) && (nodes[v2].valid)) { //
				//If src vtx or tgt vtx is core, cut, or fill, add to fg graph 
				if (edgeWt[{(int)ei->m_source, (int)ei->m_target}] == 1) { //If fg connectivity is cube or fg connectivity is structCross3D and edge is strong
					add_edge(v1, v2, 0, fgGWithFills);
				}
			}
			if ((((int)nodes[v1].type) == 0 || ((int)nodes[v1].type) == 2) && (((int)nodes[v2].type) == 0 || ((int)nodes[v2].type) == 2) && (nodes[v1].valid) && (nodes[v2].valid)) { //
				add_edge(v1, v2, 0, fgG);
			}
			if (((int)nodes[v1].type) == CORE && ((int)nodes[v2].type) == CORE && (nodes[v1].valid) && (nodes[v2].valid)) {
				//If src vtx or tgt vtx is core, cut, or fill, add to fg graph 
				if (edgeWt[{(int)ei->m_source, (int)ei->m_target}] == 1) { //If fg connectivity is cube or fg connectivity is structCross3D and edge is strong
					add_edge(v1, v2, 0, coreG); 
				}
			}

			if (((int)nodes[v1].inFg) == 0 && ((int)nodes[v2].inFg) == 0 && (nodes[v1].valid) && (nodes[v2].valid)) {
				//If src vtx or tgt vtx is core, cut, or fill, add to fg graph 
				add_edge(v1, v2, 0, bgG);
			}
			if ((((int)nodes[v1].type) == 1 || ((int)nodes[v1].type) == 2 || ((int)nodes[v1].type) == 3) && (((int)nodes[v2].type) == 1 || ((int)nodes[v2].type) == 2 || ((int)nodes[v2].type) == 3) && (nodes[v1].valid) && (nodes[v2].valid)) { //
					add_edge(v1, v2, 0, bgGWithCuts);
			}

			if (((int)nodes[v1].type) == N && ((int)nodes[v2].type) == N && (nodes[v1].valid) && (nodes[v2].valid)) {
				//If src vtx or tgt vtx is core, cut, or fill, add to fg graph 
				add_edge(v1, v2, 0, nG);
			}
		}

		cAndFTime = std::chrono::high_resolution_clock::now();
		cout << "Begin simplifying generators" << endl;
		simplifyGenerators(labels, g_Image3D, g_Image3DF, numSlices, width, height, S, nodes, G, fgG, fgGWithFills, bgGWithCuts, coreG, bgG, nG, edgeWt, fgConn, simpleDictionary3D, genMode, false);
		if (exportGen) {
			//Gen visualization code
			std::set< std::vector<float> > boundaryVtsFill; std::set< std::vector<float> > boundaryVtsCuts; std::set< std::vector<float> > boundaryVtsCore;
			for (int s = 0; s < numSlices; s++) {
				for (int i = 0; i < width; i++) {
					for (int j = 0; j < height; j++) {
						if (((int)nodes[Label(labels[s][i + j * width])].type) == FILL) {
							bool outputC = false;
							for (int nx = -1; nx < 2; nx++) {
								for (int ny = -1; ny < 2; ny++) {
									for (int nz = -1; nz < 2; nz++) {
										int cx = i + nx; int cy = j + ny; int cz = s + nz;
										if (cx >= 0 && cy >= 0 && cz >= 0 && cx < width && cy < height && cz < numSlices) {
											if (((int)nodes[Label(labels[cz][cx + cy * width])].type) != FILL) {
												outputC = true;
											}
										}
									}
								}
							}
							if (outputC) {
								std::vector<float> coord = { (float)(i - 0.5),(float)(j - 0.5),(float)(s - 0.5) };
								boundaryVtsFill.emplace(coord);
							}
						}

						if (((int)nodes[Label(labels[s][i + j * width])].type) == CUT) {
							bool outputC = false;
							for (int nx = -1; nx < 2; nx++) {
								for (int ny = -1; ny < 2; ny++) {
									for (int nz = -1; nz < 2; nz++) {
										int cx = i + nx; int cy = j + ny; int cz = s + nz;
										if (cx >= 0 && cy >= 0 && cz >= 0 && cx < width && cy < height && cz < numSlices) {
											if (((int)nodes[Label(labels[cz][cx + cy * width])].type) != CUT) {
												outputC = true;
											}
										}
									}
								}
							}
							if (outputC) {
								std::vector<float> coord = { (float)(i - 0.5),(float)(j - 0.5),(float)(s - 0.5) };
								boundaryVtsCuts.emplace(coord);
							}
						}

						if (((int)nodes[Label(labels[s][i + j * width])].type) == CORE) {
							bool outputC = false;
							for (int nx = -1; nx < 2; nx++) {
								for (int ny = -1; ny < 2; ny++) {
									for (int nz = -1; nz < 2; nz++) {
										int cx = i + nx; int cy = j + ny; int cz = s + nz;
										if (cx >= 0 && cy >= 0 && cz >= 0 && cx < width && cy < height && cz < numSlices) {
											if (((int)nodes[Label(labels[cz][cx + cy * width])].type) != CORE) {
												outputC = true;
											}
										}
									}
								}
							}
							if (outputC) {
								std::vector<float> coord = { (float)(i - 0.5),(float)(j - 0.5),(float)(s - 0.5) };
								boundaryVtsCore.emplace(coord);
							}
						}
					}
				}
			}

			exportGeneratorPly("C:/Users/danzeng/topoCuts.ply", boundaryVtsCuts);
			exportGeneratorPly("C:/Users/danzeng/topoFills.ply", boundaryVtsFill);
			exportGeneratorPly("C:/Users/danzeng/topoCore.ply", boundaryVtsCore);
		}
		for (int i = 0; i < nodes.size(); i++) {
			if (nodes[i].valid) {
				if ((int)nodes[i].type == FILL || ((int)nodes[i].type) == CUT) {
					nodes[i].labelCost = nodes[i].floatCost;
				}
			}
		}
		Graph newG = Graph();
		for (int i = 0; i < nodes.size(); i++) {
			add_vertex(newG);
		}
		for (tie(ei, ei_end) = edges(G); ei != ei_end; ++ei) {
			int v1 = (int)ei->m_source; int v2 = (int)ei->m_target;
			if (!nodes[v1].valid || !nodes[v2].valid) { continue; }
			if ((int)nodes[v1].type == CORE || (int)nodes[v2].type == CORE) {
				if (edgeWt[{v1, v2}] == 1) {
					add_edge(v1, v2, newG);
				}
			}
			else {

				add_edge(v1, v2, newG);
			}

		}
		G = newG;
	}
	int cfCt = 0;
	if (genMode == 0) {
		for (int i = 0; i < nodes.size(); i++) {
			if (nodes[i].valid) {
				if (((int)nodes[i].type) == 2 || ((int)nodes[i].type) == 3) {
					cfCt += 1;
				}
				
			}
		}
	}

	if (greedy == 1) {
		cout << "Running greedy mode " << endl;
		fgG = grapht(); bgG = grapht();
		for (int i = 0; i < nodes.size(); i++) {
			add_vertex(fgG); add_vertex(bgG);
		}
		for (tie(ei, ei_end) = edges(G); ei != ei_end; ++ei) {
			int v1 = (int)ei->m_source; 
			int v2 = (int)ei->m_target;
			if (!nodes[v1].valid || !nodes[v2].valid) { continue; }
			if ((int)nodes[v1].inFg == 1 && (int)nodes[v2].inFg == 1) {
				if (edgeWt[{v1, v2}] == 1) {
					add_edge(v1, v2, fgG);
				}
			}
			else {
				if ((int)nodes[v1].inFg == 0 && (int)nodes[v2].inFg == 0) {
					add_edge(v1, v2, bgG);
				}
			}
		}

		swapLabelsGreedy(G, fgG, bgG, nodes, edgeWt, 1, width, height, numSlices, outFile, labels, S, g_Image3D, vizEps);
		
		float totalFloatCost = 0.0;
		for (int i = 0; i < nodes.size(); i++) {
			if (nodes[i].valid) {
				if ((int)nodes[i].type == CUT) {
					if ((int)nodes[i].inFg == 0) {
						totalFloatCost += abs(nodes[i].floatCost);
					}
				}
				if ((int)nodes[i].type == FILL) {
					if ((int)nodes[i].inFg == 1) {
						totalFloatCost += abs(nodes[i].floatCost);
					}
				}

			}
		}
		cout << "Total greedy cost " << totalFloatCost << endl;
		//Simulate sending all cuts/fills to fg/bg
		std::vector<uint8_t *> greedyTopoImg;
		int allCutsApplied = 0;
		int allFillsApplied = 0;
		int oursCost = 0;
		int pushedBack = 0;
		
		if (inFileType == 1 || inFileType == 2) {
			uint16 spp, bpp, photo;
			TIFF* out;
			int i, j;
			uint16 page;

			out = TIFFOpen(outFile.c_str(), "w");
			if (!out)
			{
				fprintf(stderr, "Can't open %s for writing\n", argv[1]);
				return 1;
			}
			spp = 1;
			bpp = 32;
			photo = PHOTOMETRIC_MINISBLACK;

			for (int s = 0; s < numSlices; s++) {
				TIFFSetField(out, TIFFTAG_IMAGEWIDTH, width);
				TIFFSetField(out, TIFFTAG_IMAGELENGTH, height);
				TIFFSetField(out, TIFFTAG_BITSPERSAMPLE, bpp);
				TIFFSetField(out, TIFFTAG_SAMPLESPERPIXEL, spp);
				TIFFSetField(out, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
				TIFFSetField(out, TIFFTAG_PHOTOMETRIC, photo);
				TIFFSetField(out, TIFFTAG_ORIENTATION, ORIENTATION_BOTLEFT);

				TIFFSetField(out, TIFFTAG_SAMPLEFORMAT, SAMPLEFORMAT_IEEEFP);

				TIFFSetField(out, TIFFTAG_SUBFILETYPE, FILETYPE_PAGE);
				TIFFSetField(out, TIFFTAG_PAGENUMBER, s, numSlices);

				std::vector<float> dataV = g_Image3DF[s];
				uint8_t* label8T = new uint8_t[width * height]();
				float* data = new float[width * height]();
				for (int i = 0; i < width; i++) {
					for (int j = 0; j < height; j++) {
						data[i + j * width] = (float)dataV[i + j * width];
						label8T[i + j * width] = 0;
						if (((int)nodes[Label(labels[s][i + j * width])].inFg) == 1) {
							label8T[i + j * width] = 1;
						}
						if (((int)nodes[Label(labels[s][i + j * width])].type) == CUT) {
							allCutsApplied += 1;
							if (((int)nodes[Label(labels[s][i + j * width])].inFg) == 0) {
								data[i + j * width] = S - vizEps;
								oursCost += 1;
							}
						}
						else {
							if (((int)nodes[Label(labels[s][i + j * width])].type) == FILL) {
								allFillsApplied += 1;
								if (((int)nodes[Label(labels[s][i + j * width])].inFg) == 1) {
									data[i + j * width] = S + vizEps;
									oursCost += 1;
								}
							}
						}
					}
				}

				for (j = 0; j < height; j++) {
					TIFFWriteScanline(out, &data[j * width], j, 0);
				}
				TIFFWriteDirectory(out);

			}
			TIFFClose(out);
		}
		else {
			for (int s = 0; s < numSlices; s++) {
				int digits = numDigits(s);
				string numStr = "";
				for (int n = 0; n < 4 - digits; n++) {
					numStr += "0";

				}
				numStr += std::to_string(s);

				pushedBack += 1;
				string filename = outFile + numStr + ".png";
				uint8_t* label8 = new uint8_t[width * height]();
				uint8_t* label8T = new uint8_t[width * height]();
				for (int i = 0; i < width; i++) {
					for (int j = 0; j < height; j++) {
						label8[i + j * width] = 0;
						label8T[i + j * width] = 0;
						if (((int)nodes[Label(labels[s][i + j * width])].inFg) == 1) {
							label8T[i + j * width] = 1;
							if ((int)nodes[Label(labels[s][i + j * width])].type == 3) {
								label8[i + j * width] = S + vizEps;

								oursCost += 1;
							}
							else {
								label8[i + j * width] = min(250, (int)g_Image3D[s][i + j * width]);
							}
						}
						else {
							if ((int)nodes[Label(labels[s][i + j * width])].type == 2) {
								label8[i + j * width] = max(0, ((int)(S - vizEps)));

								oursCost += 1;
							}
							else {
								label8[i + j * width] = min(250, (int)g_Image3D[s][i + j * width]);
							}
						}
					}
				}
				greedyTopoImg.push_back(label8T);
				int wrote = stbi_write_png(filename.c_str(), width, height, 1, label8, width);
				//}
			}
			cout << "greedy cost " << oursCost << endl;
			std::vector<int> greedyTopo = getTopoFromBImg(greedyTopoImg, fgConn, width, height, pushedBack);
			cout << "Greedy topology: Components: " << greedyTopo[0] << " Cavities: " << greedyTopo[1] << " Handles: " << greedyTopo[2] << endl;
			return 0;
		}
	}

	auto simpTime = std::chrono::high_resolution_clock::now();
	elapsed = simpTime - cAndFTime;
	auto eulerTime = std::chrono::high_resolution_clock::now();
	
	if (genMode == 0) {
		cout << "Done simplifying generators, time required: " << elapsed.count() << " seconds" << endl;
	
		eulerNums = getEulerNumbers(nodes, labels, width, height, numSlices);
	
		auto eulerTime = std::chrono::high_resolution_clock::now();
		if (shapeTopo) {
			std::vector<int> eulerS = getTopoFromGImg(g_Image3D, g_Image3DF, S, fgConn, width, height, numSlices);
			cout << "Original Shape Topology: Components: " << eulerS[0] << " Cavities: " << eulerS[1] << " Cycles: " << eulerS[2] << endl;
		}
		int64_t sumNodeCost = 0;
		for (int i = 0; i < nodes.size(); i++) {
			if (nodes[i].valid) {
				if ((int)nodes[i].type == CUT || nodes[i].type == FILL) {
					sumNodeCost += abs((int)nodes[i].labelCost);
				}
			}
		}
		for (int i = 0; i < nodes.size(); i++) {
			//labelcost is currently intensity based, add euler number as top consideration
			if (nodes[i].valid) {

				if (((int)nodes[i].type) == 2 || ((int)nodes[i].type) == 3) {
					int64_t eulerSum = nodes[i].v - nodes[i].e + nodes[i].f - nodes[i].c;
					nodes[i].intensity = nodes[i].labelCost;
					int64_t topoCost = eulerSum;
					nodes[i].labelCost = ((topoCost*cfCt*sumNodeCost) / 0.5) + nodes[i].labelCost;
					
				}
			}
		}
	}
	map<int, int> oldToNewIndex; map<int, int> newToOldIndex; std::vector<node> origNodes = nodes; Graph firstG = G; map< std::vector<int>, int> firstEdgeWt = edgeWt;
	//Generator simplification invalidates some nodes; these invalid nodes are removed in this call
	reindexGraph(nodes, G, oldToNewIndex, newToOldIndex, edgeWt);
	

	map<int, int> oldToNewOC;
	std::vector< std::vector<int> > newToOldCompOC;
	if (genMode == 1) {
		newToOldCompOC = mergeAdjacentTerminals(G, nodes, edgeWt, oldToNewOC);
	}
	
	//Assign labeling for isolated cut-fill components and critical articulation points
	std::vector<bool> fillerB; std::vector<int> fillerI; map<int, int> fillerMap; //These variables are filler variables here; this function takes more arguments during the global steiner tree stage.

	preprocessGraph(G, nodes, edgeWt, fillerB, fillerI, fillerMap, false);

	std::vector<node> nodesBeforeMerging = nodes; map<int, int> oldToNew;
	//Merge adjacent core terminals
	std::vector< std::vector<int> > newToOldComp = mergeAdjacentTerminals(G, nodes, edgeWt, oldToNew);
	if (genMode == 1) {
		std::vector< std::vector<int> > newToOldTempOC2;
		for (int i = 0; i < newToOldComp.size(); i++) {
			std::vector<int> combinedNewToOld;
			for (int j = 0; j < newToOldComp[i].size(); j++) {
				int oldIndex = newToOldComp[i][j];
				for (int k = 0; k < newToOldCompOC[oldIndex].size(); k++) {
					combinedNewToOld.push_back(newToOldCompOC[oldIndex][k]);
				}
			}
			newToOldTempOC2.push_back(combinedNewToOld);
		}

		newToOldComp = newToOldTempOC2;
	}

	auto afterPreprocess = std::chrono::high_resolution_clock::now();
	elapsed = afterPreprocess - start;
	cout << "Before graph processing: " << elapsed.count() << " seconds" << endl;

	int numNodes = nodes.size(); int64_t wtSum = getWtSum(nodes); //sum of node weights * 2 (to ensure weights are always positive)
	Graph origG = G;
	//Find FG and BG graphs, to be used to find connected components of C and N in local graph
	tbb::concurrent_vector< hyperNode > globalHypernodes;
	std::vector<node> nodesGlobal; //Represents nodes after local stage: some have been assigned to core and neighborhood
	//Local steiner tree stage on clusters of cuts and fills
	cout << "Original graph size: Nodes: " << nodes.size() << ", Edges: " << num_edges(G) << endl;
	int maxLocalGraphNodes = 0; int maxLocalGraphEdges = 0;
	solveLocalGraphs(G, nodes, nodesGlobal, numNodes, edgeWt, hypernodeSize, wtSum, productThresh, globalHypernodes, localSteinerTime, newToOldComp, nodesBeforeMerging, bbTime,
		maxLocalGraphNodes, maxLocalGraphEdges
	);
	cout << "Max size of local hypergraph: Nodes: " << maxLocalGraphNodes << " Edges: " << maxLocalGraphEdges << endl;
	auto localTime = std::chrono::high_resolution_clock::now();
	elapsed = localTime - eulerTime;
	cout << "Done with local stage, time required: " << elapsed.count() << " seconds" << endl;
	
	removeCAndNEdges(G, nodes); map<int, int> oldToNew2;
	
	std::vector< std::vector<int> > newToOldComp2 = mergeAdjacentTerminals(G, nodes, edgeWt, oldToNew2);
	std::vector< std::vector<int> > newToOldTemp;
	for (int i = 0; i < newToOldComp2.size(); i++) {

		std::vector<int> combinedNewToOld; 
		for (int j = 0; j < newToOldComp2[i].size(); j++) {
			int oldIndex = newToOldComp2[i][j];
			for (int k = 0; k < newToOldComp[oldIndex].size(); k++) {
				combinedNewToOld.push_back(newToOldComp[oldIndex][k]);
			}
		}
		newToOldTemp.push_back(combinedNewToOld);
	}
	
	newToOldComp = newToOldTemp;
	for (int i = 0; i < globalHypernodes.size(); i++) {
		std::vector<int> subnodes;
		for (int j = 0; j < globalHypernodes[i].doubleSubnodes.size(); j++) {
			subnodes.push_back(oldToNew2[globalHypernodes[i].doubleSubnodes[j]]);
		}
		std::sort(subnodes.begin(), subnodes.end());
		subnodes.erase(unique(subnodes.begin(), subnodes.end()), subnodes.end());
		globalHypernodes[i] = hyperNode(subnodes, HYPERNODE, globalHypernodes[i].getSide());
	}
	removeCAndNEdges(G, nodes); numNodes = nodes.size();
	//Global steiner tree stage

	solveGlobalGraph(nodes, numNodes, G, origG, globalHypernodes, wtSum, edgeWt, hypernodeSize, productThresh, globalSteinerTime, localSteinerTime,
		origNodes, newToOldComp, newToOldIndex, numSlices, width, height, labels, firstG, oldToNewIndex, nodesToFix, bbTime);
	
	auto globalTime = std::chrono::high_resolution_clock::now();
	elapsed = globalTime - localTime;
	cout << "Done with global stage, time required: " << elapsed.count() << " seconds" << endl;
	std::vector<bool> oldInFg(origNodes.size(), false);
	for (int i = 0; i < numNodes; i++) {
		if ( ((int)nodes[i].inFg) == 1) {
			
			for (int j = 0; j < newToOldComp[i].size(); j++) {
				oldInFg[newToOldIndex[newToOldComp[i][j]]] = true;
				origNodes[newToOldIndex[newToOldComp[i][j]]].inFg = 1;
			}
		}
		else {
			for (int j = 0; j < newToOldComp[i].size(); j++) {
				origNodes[newToOldIndex[newToOldComp[i][j]]].inFg = 0;
			}
		}
	}
	std::vector<int> nodeToComp(origNodes.size());
	if (greedyMode == 1) {
		fgG = grapht(); bgG = grapht();
		for (int i = 0; i < origNodes.size(); i++) {
			add_vertex(fgG); add_vertex(bgG);
		}
		//edge_iter ei, ei_end;
		for (tie(ei, ei_end) = edges(firstG); ei != ei_end; ++ei) {
			int v1 = (int)ei->m_source; int v2 = (int)ei->m_target;
			if (((int)origNodes[v1].inFg) == 1 && ((int)origNodes[v2].inFg) == 1 && (origNodes[v1].valid) && (origNodes[v2].valid)) {
				//If src vtx or tgt vtx is core, cut, or fill, add to fg graph 
				if (firstEdgeWt[{(int)ei->m_source, (int)ei->m_target}] == 1) { //If fg connectivity is cube or fg connectivity is structCross3D and edge is strong
					add_edge(v1, v2, 0, fgG);
				}
			}

			if (((int)origNodes[v1].inFg) == 0 && ((int)origNodes[v2].inFg) == 0 && (origNodes[v1].valid) && (origNodes[v2].valid)) {
				//If src vtx or tgt vtx is core, cut, or fill, add to fg graph 
				add_edge(v1, v2, 0, bgG);
			}
		}
		swapLabelsGreedy(firstG, fgG, bgG, origNodes, firstEdgeWt, fgConn, width, height, numSlices, outFile, labels, S, g_Image3D, vizEps);
		int64_t totalCost = 0; float floatCost = 0.0; float cutCost = 0.0; float fillCost = 0.0;
		for (int i = 0; i < origNodes.size(); i++) {
			if (!origNodes[i].valid) { continue; }
			if ((int)origNodes[i].type == CUT) {
				cutCost += abs(origNodes[i].floatCost);
				if ((int)origNodes[i].inFg == 0) {
					totalCost += abs(origNodes[i].labelCost); floatCost += abs(origNodes[i].floatCost);
				}
			}
			if ((int)origNodes[i].type == FILL) {
				if ((int)origNodes[i].inFg == 1) {
					totalCost += abs(origNodes[i].labelCost); floatCost += abs(origNodes[i].floatCost);
				}
				fillCost += abs(origNodes[i].floatCost);
			}
		}

		int fgComps = getNumComponents(fgG, origNodes, 1);
		int bgComps = getNumComponents(bgG, origNodes, 0);
		
		int n = (int)boost::connected_components(bgG, &nodeToComp[0]);
		int numComps = 0;
		std::vector<bool> isCompIndex(origNodes.size(), false);
		for (int i = 0; i < nodeToComp.size(); i++) {
			if (((int)origNodes[i].inFg) == 0 && origNodes[i].valid) {
				if (!isCompIndex[nodeToComp[i]]) {
					isCompIndex[nodeToComp[i]] = true;
					numComps += 1;
				}
			}
		}

		auto greedyTime = std::chrono::high_resolution_clock::now();
		elapsed = greedyTime - globalTime;
		cout << "Done with greedy stage, time required: " << elapsed.count() << " seconds" << endl;
	}

	//Exporting selected cuts and fills
		
	if (exportGen) {
		//Generator visualization code
		std::set< std::vector<float> > boundaryVtsFill; std::set< std::vector<float> > boundaryVtsCuts;
		std::set< std::vector<float> > topoVtsFills; std::set< std::vector<float> > topoVtsCuts;
		for (int s = 0; s < numSlices; s++) {


			for (int i = 0; i < width; i++) {
				for (int j = 0; j < height; j++) {


					if (((int)origNodes[Label(labels[s][i + j * width])].type) == FILL) {
						bool outputC = false;
						for (int nx = -1; nx < 2; nx++) {
							for (int ny = -1; ny < 2; ny++) {
								for (int nz = -1; nz < 2; nz++) {
									int cx = i + nx; int cy = j + ny; int cz = s + nz;
									if (cx >= 0 && cy >= 0 && cz >= 0 && cx < width && cy < height && cz < numSlices) {
										if (((int)origNodes[Label(labels[cz][cx + cy * width])].type) != FILL) {
											outputC = true;
										}
									}
								}
							}
						}
						if (outputC) {
							std::vector<float> coord = { (float)(i - 0.5),(float)(j - 0.5),(float)(s - 0.5) };
							topoVtsFills.emplace(coord);
							if ((int)origNodes[Label(labels[s][i + j * width])].inFg == 1) {
								boundaryVtsFill.emplace(coord);
							}
						}
					}
						
					if (((int)origNodes[Label(labels[s][i + j * width])].type) == CUT) {
						bool outputC = false;
						for (int nx = -1; nx < 2; nx++) {
							for (int ny = -1; ny < 2; ny++) {
								for (int nz = -1; nz < 2; nz++) {
									int cx = i + nx; int cy = j + ny; int cz = s + nz;
									if (cx >= 0 && cy >= 0 && cz >= 0 && cx < width && cy < height && cz < numSlices) {
										if (((int)origNodes[Label(labels[cz][cx + cy * width])].type) != CUT) {
											outputC = true;
										}
									}
								}
							}
						}
						if (outputC) {
							std::vector<float> coord = { (float)(i-0.5),(float)(j-0.5),(float)(s-0.5) };
							topoVtsCuts.emplace(coord);
							if ((int)origNodes[Label(labels[s][i + j * width])].inFg == 0) {
								boundaryVtsCuts.emplace(coord);
							}
						}
					}


				}
			}
		}
		exportGeneratorPly("topoCuts.ply", topoVtsCuts);
		exportGeneratorPly("topoFills.ply", topoVtsFills);
		exportGeneratorPly("selectedCuts.ply", boundaryVtsCuts);
		exportGeneratorPly("selectedFills.ply", boundaryVtsFill);
	}

	std::vector<uint8_t *> bImg;
	std::vector<uint8_t *> topoImg;
	int allCutsApplied = 0;
	int allFillsApplied = 0;
	int oursCost = 0;
	int numAdded = 0;
	if (genMode == 0) {
		
		if (inFileType == 1) {
			if (outFile.substr(outFile.length() - 4) == ".mrc") {
				toMRCFile(outFile.c_str(), width, height, numSlices, minVal, maxVal, S, vizEps, origNodes, nodes,
					g_Image3D, g_Image3DF, labels, thresholdOut
				);
			}
			else {
				uint16 spp, bpp, photo;
				TIFF* out;
				int i, j;
				uint16 page;

				out = TIFFOpen(outFile.c_str(), "w");
				if (!out)
				{
					fprintf(stderr, "Can't open %s for writing\n", argv[1]);
					return 1;
				}
				spp = 1;
				bpp = 32;
				photo = PHOTOMETRIC_MINISBLACK;

				for (int s = 0; s < numSlices; s++) {
					TIFFSetField(out, TIFFTAG_IMAGEWIDTH, width);
					TIFFSetField(out, TIFFTAG_IMAGELENGTH, height);
					TIFFSetField(out, TIFFTAG_BITSPERSAMPLE, bpp);
					TIFFSetField(out, TIFFTAG_SAMPLESPERPIXEL, spp);
					TIFFSetField(out, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
					TIFFSetField(out, TIFFTAG_PHOTOMETRIC, photo);
					TIFFSetField(out, TIFFTAG_ORIENTATION, ORIENTATION_BOTLEFT);

					TIFFSetField(out, TIFFTAG_SAMPLEFORMAT, SAMPLEFORMAT_IEEEFP);

					TIFFSetField(out, TIFFTAG_SUBFILETYPE, FILETYPE_PAGE);
					TIFFSetField(out, TIFFTAG_PAGENUMBER, s, numSlices);

					std::vector<float> dataV = g_Image3DF[s];
					uint8_t* label8T = new uint8_t[width * height]();
					float* data = new float[width * height]();
					for (int i = 0; i < width; i++) {
						for (int j = 0; j < height; j++) {
							data[i + j * width] = (float)dataV[i + j * width];
							label8T[i + j * width] = 0;
							if (((int)origNodes[Label(labels[s][i + j * width])].inFg) == 1) {
								label8T[i + j * width] = 1;
							}
							if (((int)origNodes[Label(labels[s][i + j * width])].type) == CUT) {
								allCutsApplied += 1;
								if (((int)origNodes[Label(labels[s][i + j * width])].inFg) == 0) {
									data[i + j * width] = S - vizEps;
									oursCost += 1;
								}
							}
							else {
								if (((int)origNodes[Label(labels[s][i + j * width])].type) == FILL) {
									allFillsApplied += 1;
									if (((int)origNodes[Label(labels[s][i + j * width])].inFg) == 1) {
										data[i + j * width] = S + vizEps;
										oursCost += 1;
									}
								}
							}
						}
					}

					for (j = 0; j < height; j++) {
						TIFFWriteScanline(out, &data[j * width], j, 0);
					}
					TIFFWriteDirectory(out);
					topoImg.push_back(label8T);
					numAdded += 1;
				}
				TIFFClose(out);
				if (showGeomCost) {
					cout << "Geometric cost: " << oursCost << " All-cuts cost: " << allCutsApplied << " All-fills cost: " << allFillsApplied << endl;
				}
				if (finalTopo) {
					std::vector<int> topoNums = getTopoFromBImg(topoImg, fgConn, width, height, numAdded);
					std::cout << "Final topology: Components: " << topoNums[0] << " Cavities: " << topoNums[1] << " Cycles: " << topoNums[2] << std::endl;
				}
			}
		}
		else if (inFileType == 0) {
			if (outFile.substr(outFile.length() - 4) == ".mrc") {
				toMRCFile(outFile.c_str(), width, height, numSlices, minVal, maxVal, S, vizEps, origNodes, nodes,
					g_Image3D, g_Image3DF, labels, thresholdOut
				);
			}
			else {
				float ourCost = 0.0;
				float cutCost = 0.0;
				float fillCost = 0.0;
				for (int i = 0; i < origNodes.size(); i++) {
					if (origNodes[i].type == CUT) {
						if (((int)origNodes[i].inFg) == 0) {
							ourCost += abs(origNodes[i].floatCost);
						}
						cutCost += abs(origNodes[i].floatCost);
					}
					if (origNodes[i].type == FILL) {
						if (((int)origNodes[i].inFg) == 1) {
							ourCost += abs(origNodes[i].floatCost);
						}
						fillCost += abs(origNodes[i].floatCost);
					}
				}
				if (showGeomCost) {
					cout << "Geometric cost: " << ourCost << " All-cuts cost: " << cutCost << " All-fills cost: " << fillCost << endl;
				}
				for (int s = 0; s < numSlices; s++) {

					int digits = numDigits(s);
					string numStr = "";
					for (int n = 0; n < 4 - digits; n++) {
						numStr += "0";

					}
					numStr += std::to_string(s);
					string filename = outFile + numStr + ".png";

					uint8_t* label8 = new uint8_t[width * height]();
					uint8_t* label8T = new uint8_t[width * height]();
					for (int i = 0; i < width; i++) {
						for (int j = 0; j < height; j++) {
							label8[i + j * width] = 0;
							label8T[i + j * width] = 0;

							if (((int)origNodes[Label(labels[s][i + j * width])].inFg) == 1) {
								label8T[i + j * width] = 1;
								if ((int)origNodes[Label(labels[s][i + j * width])].type == 3) {
									label8[i + j * width] = min(((int)(S + vizEps)), 255);
								}
								else {
									label8[i + j * width] = max((int)g_Image3D[s][i + j * width], (int)S + 1); //(int)S + 1
								}
							}
							else {
								if ((int)origNodes[Label(labels[s][i + j * width])].type == 2) {
									label8[i + j * width] = max(0, ((int)(S - vizEps)));
								}
								else {
									label8[i + j * width] = min((int)g_Image3D[s][i + j * width], (int)S - 1);//(int)S - 1
								}
							}
						}
					}
					numAdded += 1;
					bImg.push_back(label8);
					topoImg.push_back(label8T);
					int wrote = stbi_write_png(filename.c_str(), width, height, 1, label8, width);
				}

				if (finalTopo) {
					std::vector<int> topoNums = getTopoFromBImg(topoImg, fgConn, width, height, numAdded);

					std::cout << "Final topology: Components: " << topoNums[0] << " Cavities: " << topoNums[1] << " Cycles: " << topoNums[2] << std::endl;
				}
			}
		}
	}
	if (genMode == 1) {
		int oursCost = 0; int allFillsCost = 0; int allCutsCost = 0;
		if (g_Image3DF.size() > 0) {
			if (outFile.substr(outFile.length() - 4) == ".mrc") {
				toMRCFile(outFile.c_str(), width, height, numSlices, minVal, maxVal, S, vizEps, origNodes, nodes,
					g_Image3D, g_Image3DF, labels, thresholdOut
				);
			}
			else {
				uint16 spp, bpp, photo;
				TIFF* out;
				int i, j;
				uint16 page;

				out = TIFFOpen(outFile.c_str(), "w");
				if (!out)
				{
					fprintf(stderr, "Can't open %s for writing\n", argv[1]);
					return 1;
				}
				spp = 1;
				bpp = 32;
				photo = PHOTOMETRIC_MINISBLACK;

				for (int s = 0; s < numSlices; s++) {
					TIFFSetField(out, TIFFTAG_IMAGEWIDTH, width);
					TIFFSetField(out, TIFFTAG_IMAGELENGTH, height);
					TIFFSetField(out, TIFFTAG_BITSPERSAMPLE, bpp);
					TIFFSetField(out, TIFFTAG_SAMPLESPERPIXEL, spp);
					TIFFSetField(out, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
					TIFFSetField(out, TIFFTAG_PHOTOMETRIC, photo);
					TIFFSetField(out, TIFFTAG_ORIENTATION, ORIENTATION_BOTLEFT);

					TIFFSetField(out, TIFFTAG_SAMPLEFORMAT, SAMPLEFORMAT_IEEEFP);

					TIFFSetField(out, TIFFTAG_SUBFILETYPE, FILETYPE_PAGE);
					TIFFSetField(out, TIFFTAG_PAGENUMBER, s, numSlices);

					std::vector<float> dataV = g_Image3DF[s];
					uint8_t* label8T = new uint8_t[width * height]();
					float* data = new float[width * height]();
					for (int i = 0; i < width; i++) {
						for (int j = 0; j < height; j++) {
							data[i + j * width] = (float)dataV[i + j * width];
							label8T[i + j * width] = 0;
							if (((int)origNodes[Label(labels[s][i + j * width])].inFg) == 1) {
								label8T[i + j * width] = 1;
							}
							if (((int)origNodes[Label(labels[s][i + j * width])].type) == CUT) {
								allCutsApplied += 1;
								if (((int)origNodes[Label(labels[s][i + j * width])].inFg) == 0) {
									data[i + j * width] = S - vizEps;
									oursCost += 1;
								}
							}
							else {
								if (((int)origNodes[Label(labels[s][i + j * width])].type) == FILL) {
									allFillsApplied += 1;
									if (((int)origNodes[Label(labels[s][i + j * width])].inFg) == 1) {
										data[i + j * width] = S + vizEps;
										oursCost += 1;
									}
								}
							}
						}
					}

					for (j = 0; j < height; j++) {
						TIFFWriteScanline(out, &data[j * width], j, 0);
					}
					TIFFWriteDirectory(out);

				}
				TIFFClose(out);
				if (showGeomCost) {
					cout << "Geometric cost: " << oursCost << " All-cuts cost: " << allCutsCost << " All-fills cost: " << allFillsCost << endl;
				}
			}
		}
		else {
			if (outFile.substr(outFile.length() - 4) == ".mrc") {
				toMRCFile(outFile.c_str(), width, height, numSlices, minVal, maxVal, S, vizEps, origNodes, nodes,
					g_Image3D, g_Image3DF, labels, thresholdOut
				);
			}
			else {
				for (int s = 0; s < numSlices; s++) {
					int digits = numDigits(s);
					string numStr = "";
					for (int n = 0; n < 4 - digits; n++) {
						numStr += "0";

					}
					numStr += std::to_string(s);
					string filename = outFile + numStr + ".png";

					uint8_t* label8 = new uint8_t[width * height]();
					uint8_t* label8T = new uint8_t[width * height]();
					for (int i = 0; i < width; i++) {
						for (int j = 0; j < height; j++) {
							label8T[i + j * width] = 0;
							label8[i + j * width] = (int)g_Image3D[s][i + j * width];
							if ((int)origNodes[Label(labels[s][i + j * width])].type == 3) {
								allFillsCost += 1;
							}
							if ((int)origNodes[Label(labels[s][i + j * width])].type == 2) {
								allCutsCost += 1;
							}
							if (((int)origNodes[Label(labels[s][i + j * width])].inFg) == 1) {
								label8T[i + j * width] = 1;
								if ((int)origNodes[Label(labels[s][i + j * width])].type == 3) {
									label8[i + j * width] = min(255, (int)(S + vizEps));
									oursCost += 1;
								}
								else {
									label8[i + j * width] = (int)g_Image3D[s][i + j * width];
								}
							}
							else {
								if ((int)origNodes[Label(labels[s][i + j * width])].type == 2) {
									label8[i + j * width] = max(0, (int)(S - vizEps));
									oursCost += 1;
								}
								else {
									label8[i + j * width] = (int)g_Image3D[s][i + j * width];
								}
								if ((int)nodes[Label(labels[s][i + j * width])].type == 1) {
									label8[i + j * width] = max(min((int)g_Image3D[s][i + j * width], ((int)(S - vizEps))), 0);
								}
							}
						}
					}
					bImg.push_back(label8);
					topoImg.push_back(label8T);
					int wrote = stbi_write_png(filename.c_str(), width, height, 1, label8, width);
					numAdded += 1;
				}
				if (showGeomCost) {
					cout << "Geometric cost: " << oursCost << " All-cuts cost: " << allCutsCost << " All-fills cost: " << allFillsCost << endl;
				}
			}
		}
		if (finalTopo) {
			std::vector<int> greedyTopo = getTopoFromBImg(topoImg, fgConn, width, height, numAdded);
			cout << "Greedy topo: " << greedyTopo[0] << " " << greedyTopo[1] << " " << greedyTopo[2] << endl;
		}
	}
}