#ifndef _FUNCDEF_H_INCLUDED
#define _FUNCDEF_H_INCLUDED

#define MSTACK 7
#define NSTACK 50

#define TINY 1e-10f

#define MIN(A, B) ((A) < (B) ? (A) : (B))
#define MAX(A, B) ((A) > (B) ? (A) : (B))
#define SWAPFLOAT(a,b) temp_float=(a);(a)=(b);(b)=temp_float;
#define SWAPINT(a,b) temp_int=(a);(a)=(b);(b)=temp_int;

#include <string>
using namespace std;

class Songs;
class EnergyContour;
class StringList;

//Statistics
float mean(float *x, int xLen);
float var(float *x, int xLen);
float stdev(float *x, int xLen);
float normalized_corr2(float *x, float *y, int &len, int &step);
int getMaxCorr(Songs &songs);
//

//Signal processing
void energy(const char* wavFile, float windowSize, float skipSize, float **en, int &numfrm, int isSmooth);
void filter(float *b, int lenb, float *a, int lena, float *x, int xLen);
void NormalizeEnergy(float *x, int xlen, float val=40000.0f);
//

//Algorithms
void sort(float *arr, int *brr, int n);
int totalLess(float *seg, int segLen, float th);
double mydrand(void);
void ExtractDelta(float *x, int xlen);
//

//String manipulation
class StringList;
void readFileList(StringList &songs, const char* listFile);
void readFileList(EnergyContour **songs, int &numSongs, const char* listFile);
int getFileListLen(const char *fileList);
string modifyExtension(string inFile, string newExt);
//

//File and directory name processing
string checkLastSlash(string inFolder);
string getFileName(string inFile, bool bExt);
int makeDir(string str);
//

#endif
