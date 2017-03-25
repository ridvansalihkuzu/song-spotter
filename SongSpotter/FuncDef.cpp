#include "stdafx.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <io.h>
#include <direct.h>

#include <string>
using namespace std;

#include "FuncDef.h"
#include "Wave.h"
#include "StringList.h"
#include "EnergyContour.h"
#include "Songs.h"

double mydrand(void)
//returns a floating point number between -0.5 and 0.5 
{
    if (1)
        return 0.5 - ((double) rand()) / RAND_MAX;
    else
        return 0.0;
}

void write_energy_file(const float *en, int enLen, const char *energyFile, float ws, float ss)
{
	FILE *fid = fopen(energyFile, "wb");
	fwrite(&enLen, sizeof(int), 1, fid);
	fwrite(&ws, sizeof(float), 1, fid);
	fwrite(&ss, sizeof(float), 1, fid);
	fwrite(en, sizeof(float), enLen, fid);
	fclose(fid);
}

//Time domain filtering
// a[0]*y[n] = b[0]*x[n] + b[1]*x[n-1] + ... + b[nb]*x[n-nb]
//                         - a[1]*y[n-1] - ... - a[na]*y[n-na]	
// if bNormalize is true then:
//     all the coeffs are normalized with a[0]
// b and a are filter coefficients (impulse response of the filter)
// lenb and lena are the length of b and a respectively
// xLen reference to the variable holding the length of x
//
// Note :  x is CHANGED by this function 
void filter(float *b, int lenb, float *a, int lena, float *x, int xLen)
{
    int n;
    float x_terms;
    float y_terms;
    int ind;
    
    float *y;
    y = new float [xLen];
    
    int nb = lenb - 1;
    int na = lena - 1;
    
    for (n = 0; n < xLen; n++)
    {
        x_terms = (float)0;
        for (ind = n; ind > n - nb - 1; ind--)
        {
            if (ind >= 0)
                x_terms += b[n - ind] * x[ind];
        }
        
        y_terms = (float)0;
        for (ind = n - 1; ind > n - na - 1; ind--)
        {
            if (ind >= 0)
                y_terms += -a[n - ind] * y[ind];
        }
        
        y[n] = x_terms + y_terms;
    }
    
    for (n = 0; n < xLen; n++)
        x[n] = y[n];
    
    delete [] y;
}

void energy(const char* wavFile, float windowSize, float skipSize, float **en, int &numfrm, int isSmooth)
{
	Wave *w = Wave::openForRead(wavFile);

	if (w->state == Wave::OPENFOR_READ)
	{
		int ss = (int)(floor(skipSize*w->fs+0.5));
		int ws = (int)(floor(windowSize*w->fs+0.5));
		
		numfrm = (int)(floor((((double)w->numSamples)-ws)/ss+0.5));
		if (numfrm>0)
		{
			*en = new float[numfrm];
			short *x = new short[ws];

			float tmp;
			int i,j;
			for (i=0; i<numfrm; i++)
			{
				if (i==0)
					w->readMonoData16(x, ws);
				else
				{
					for (j=ss; j<ws; j++)
						x[j-ss] = x[j];

					w->readMonoData16(&(x[ws-ss]), ss);
				}
				
				(*en)[i] = 0.0f;
				for (j=0; j<ws; j++)
				{
					tmp = x[j]+(float)(1e-5*mydrand());
					(*en)[i] = (*en)[i] + tmp*tmp;
				}

				(*en)[i] = (float)(10*log10((*en)[i]/ws));
			}

			delete [] x;

			if (isSmooth)
			{
				float b[11] = {0.0099673f,
							   0.024862f,
							   0.066837f,
							   0.12489f,
							   0.17559f,
							   0.19571f,
							   0.17559f,
							   0.12489f,
							   0.066837f,
							   0.024862f,
							   0.0099673f};
	
				float a[1] = {1.0f};

				filter(b, 11 , a, 1, *en, numfrm);
			}
		}
		else
			printf("No frames found\n");

		w->close();
		delete [] w;
	}
	else
		printf("Cannot open file %s\n", wavFile);
}

float mean(float *x, int xLen)
{
	float m =0.0f;
	for (int i=0; i<xLen; i++)
		m += x[i];
	m /= xLen;

	return m;
}

float var(float *x, int xLen)
{
	float v = 0.0f;
	float m = mean(x, xLen);

	for (int i=0; i<xLen; i++)
		v += (x[i]-m)*(x[i]-m);

	if (xLen>1)
		return v/(xLen-1.0f); //unbiased estimate of variance
	else
		return v/xLen; //biased estimate of variance
}

float stdev(float *x, int xLen)
{
	return (float)sqrt(var(x, xLen));
}

//Sorts an array arr[1..n] into ascending order using Quicksort, while making the corresponding
// rearrangement of the array brr[1..n].
void sort(float *arr, int *brr, int n)
{
	int i,ir=n,j,k,l=1,*istack;
	int jstack=0;
	float a, temp_float;
	int temp_int;
	int b;
	istack=new int[NSTACK];
	for (;;) 
	{
		if (ir-l < MSTACK) 
		{
			for (j=l+1;j<=ir;j++) 
			{
				a=arr[j];
				b=brr[j];

				for (i=j-1;i>=l;i--) 
				{
					if (arr[i] <= a) break;
					arr[i+1]=arr[i];
					brr[i+1]=brr[i];
				}
				arr[i+1]=a;
				brr[i+1]=b;
			}

			if (!jstack) 
			{
				delete [] istack;
				return;
			}

			ir=istack[jstack];
			l=istack[jstack-1];
			jstack -= 2;
		} 
		else 
		{
			k=(l+ir) >> 1;
			SWAPFLOAT(arr[k],arr[l+1])
			SWAPINT(brr[k],brr[l+1])
			if (arr[l] > arr[ir]) 
			{
				SWAPFLOAT(arr[l],arr[ir])
				SWAPINT(brr[l],brr[ir])
			}
			if (arr[l+1] > arr[ir]) 
			{
				SWAPFLOAT(arr[l+1],arr[ir])
				SWAPINT(brr[l+1],brr[ir])
			}
			if (arr[l] > arr[l+1]) 
			{
				SWAPFLOAT(arr[l],arr[l+1])
				SWAPINT(brr[l],brr[l+1])
			}
			i=l+1; 
			j=ir;
			a=arr[l+1];
			b=brr[l+1];
			for (;;) 
			{
				do i++; while (arr[i] < a); 
				do j--; while (arr[j] > a);
				
				if (j < i) 
					break;
				
				SWAPFLOAT(arr[i],arr[j])
				SWAPINT(brr[i],brr[j])
			} 
			arr[l+1]=arr[j];
			arr[j]=a;
			brr[l+1]=brr[j];
			brr[j]=b;
			jstack += 2;
			if (jstack > NSTACK) 
				printf("NSTACK too small in sort2.");
			if (ir-i+1 >= j-l) 
			{
				istack[jstack]=ir;
				istack[jstack-1]=i;
				ir=j-1;
			} else 
			{
				istack[jstack]=j-1;
				istack[jstack-1]=l;
				l=i;
			}
		}
	}
}

int totalLess(float *seg, int segLen, float th)
{
	int total = 0;
	for (int i=0; i<segLen; i++)
	{
		if (seg[i]<th)
			total++;
	}

	return total;
}



string modifyExtension(string inFile, string newExt)
{
	string outFile;
	int ind = inFile.rfind('.', -1);
	if (ind>-1)
		outFile = inFile.substr(0, ind-1) + newExt;
	else
		outFile = "";

	return outFile;
}

string checkLastSlash(string inFolder)
{
	string outFolder;

	int ind = inFolder.rfind('\\', -1);
	int ind2 = inFolder.rfind('/', -1);
	if ((ind>ind2 && ind<inFolder.length()-1) || (ind2>ind && ind2<inFolder.length()-1))
		outFolder = inFolder + "\\";
	else
		outFolder = inFolder;

	return outFolder;
}

string getFileName(string inFile, bool bExt)
{
	string outFile;

	int ind = inFile.rfind('\\', -1);
	int ind2 = inFile.rfind('/', -1);

	if (ind2>ind)
		ind=ind2;
	
	ind2 = inFile.rfind('.', -1);

	if (ind>-1)
	{
		if(ind2>ind && !bExt)
			outFile = inFile.substr(ind+1, ind2-1-ind);
		else
			outFile = inFile.substr(ind+1, inFile.length()-1-ind);
	}
	else
	{
		if(ind2>-1 && !bExt)
			outFile = inFile.substr(0, ind2-1);
		else
			outFile = inFile;
	}

	return outFile;
}

void readFileList(StringList &songs, const char* listFile)
{
	int numSongs = getFileListLen(listFile);
	if (numSongs>0)
		songs.Allocate(numSongs);
	else
	{
		songs.Allocate(0);
		return;
	}
	
	FILE *fp = fopen(listFile, "r");
	int count = 0;
	char tmp[2000];
	string tmp2;
	
	while (!feof(fp))
	{
		fgets(tmp, 2000, fp);
		tmp2 = tmp;
		tmp2 = tmp2.substr(0, tmp2.length()-1); 

		if (_access(tmp2.c_str(), 0) != -1)
		{
			if (count<numSongs)
				songs.names[count] = tmp2;
			count++;
		}
	}
	fclose(fp);
}

void readFileList(EnergyContour **songs, int &numSongs, const char* listFile)
{
	numSongs = getFileListLen(listFile);
	if (numSongs>0)
		*songs = new EnergyContour[numSongs];
	else
	{
		numSongs = 0;
		*songs = NULL;
		return;
	}
	
	FILE *fp = fopen(listFile, "r");
	int count = 0;
	char tmp[2000];
	string tmp2;

	while (!feof(fp))
	{
		fgets(tmp, 2000, fp);
		tmp2 = tmp;
		tmp2 = tmp2.substr(0, tmp2.length()-1); 

		if (_access(tmp2.c_str(), 0) != -1)
		{
			if (count<numSongs)
				(*songs)[count].read_segment_file(tmp2.c_str());
			count++;
		}
	}
	fclose(fp);
}


int getFileListLen(const char *fileList)
{
	FILE *fp = fopen(fileList, "r");
	char tmp[2000];
	string tmp2;
	int count = 0;
	
	while (!feof(fp))
	{
		if (fgets(tmp, 2000, fp)!=NULL)
		{
			tmp2 = tmp;
			tmp2 = tmp2.substr(0, tmp2.length()-1);
			if (_access(tmp2.c_str(), 0) != -1)
				count++;
		}
	}
	fclose(fp);
	
	return count;
}

//Create all directories in str in a recursive manner
int makeDir(string str)
{
	str = checkLastSlash(str);

	int ret = 0;
	int found = 0;
	int start = 0;

	while(1)
	{
		string currentFolder;
		int slashInd = str.find('\\', start);
		if (slashInd > 0)
		{
			found++;
			if (found > 1)
				currentFolder = str.substr(0, slashInd);

			start = slashInd + 1;

			ret = _mkdir(currentFolder.c_str());
		}
		else
			break;
	}

	return ret;
}

// Returns the normalized correlation coefficient of two vectors x and y of equal length len
// Optional <step> can be used for resampling the input vectors uniformly
// step=1 by default
float normalized_corr2(float *x, float *y, int &len, int &step)
{
	//float mx = mean(x, len);
	//float my = mean(y, len);
	
	float rnum = 0.0f;
	float rden = 0.0f;
	float rden2 = 0.0f;

	for (int i=0; i<len; i+=step)
	{
		//rnum += (x[i]-mx)*(y[i]-my);
		rnum += x[i]*y[i];
		//rden += (x[i]-mx)*(x[i]-mx);
		rden += x[i]*x[i];
		//rden2 += (y[i]-my)*(y[i]-my);
		rden2 += y[i]*y[i];
	}
	
	rden = (float)(sqrt(rden*rden2));
	
	return rnum/rden;
}

int getMaxCorr(Songs &songs)
{
	int tmpInd = -1;
	float tmpMax = 0.0f;

	if (songs.songInd>-1)
	{
		for (int i=0; i<=songs.songInd; i++)
		{
			if (songs.corrVals[i]>tmpMax)
			{
				tmpMax = songs.corrVals[i];
				tmpInd = i;
			}
		}
	}

	return tmpInd;
}

//Normalize energy level
void NormalizeEnergy(float *x, int xlen, float val)
{
	//Remove mean
	float tmp = mean(x, xlen);
	for (int j=0; j<xlen; j++)
		x[j] -= tmp;
	
	//Compute energy
	tmp  = 0.0f;
	for (int ij=0; ij<xlen; ij++)
		tmp += x[ij]*x[ij];
	
	//Scale energy
	tmp = (float)(sqrt(val)/sqrt(tmp));
	for (int j=0; j<xlen; j++)
		x[j] = tmp*x[j];
}

//Compute delta parameters
void ExtractDelta(float *x, int xlen)
{	
	for (int j=0; j<xlen-1; j++)
		x[j] = x[j+1]-x[j];
	
	x[xlen-1] = 0.0f;
}