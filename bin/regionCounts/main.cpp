#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <math.h>
#include <vector>
#include <map>
#include <thread>

using namespace std;

class BedNode {
public:
	string chr;
	int start;
	int stop;
};

//Move to util header
std::vector<std::string> &split(const std::string &s, char delim, std::vector<std::string> &elems) {
    std::stringstream ss(s);
        std::string item;
        while (std::getline(ss, item, delim)) {
            elems.push_back(item);
        }
        return elems;
}


std::vector<std::string> split(const std::string &s, char delim) {
    std::vector<std::string> elems;
        split(s, delim, elems);
        return elems;
}

bool bedSort(BedNode i, BedNode j) {
	return i.start < j.start;
}

int binary_search(vector<BedNode>* chr, int first, int last, BedNode bed) {
	int index;
	cout<<first<<endl;
	if(first > last) index=-1;
	else {
		int mid = (first+last)/2;
		BedNode temp = chr->at(mid);
		if((temp.start > bed.start && temp.start < bed.stop) || (temp.stop > bed.start && temp.stop < bed.stop) || (temp.start < bed.start && temp.stop > bed.stop) || (temp.start > bed.start && temp.stop < bed.stop)) {
			index = mid;
		} else {
			if(temp.stop > bed.start)
				index = binary_search(chr, first, mid-1, bed);
			else
				index = binary_search(chr, mid+1, last, bed);
		}
	}
	cout<<"returning "<<index<<endl;
	return index;
}

void regionCount(map<string, vector<BedNode> >* allPartData, string filename, vector<vector<vector<int> > >* counts, int i, vector<string>* chrList) {
	cout<<"Started "<<filename<<endl;
	ifstream rawDataFile(filename.c_str());
    string line;
    vector<BedNode> raw;
	vector<int> chrpos;
    while(getline(rawDataFile, line)) {
        if(line[0]=='@') continue;
        vector<string> splitz = split(line, '\t');
        BedNode temp;
        temp.chr = splitz[2];
        if(temp.chr.compare("*")==0) continue;
		bool found = false;
		for(int j = 0; j < chrList->size(); j++) {
			if(chrList->at(j).compare(temp.chr)==0) {
				found = true;
				chrpos.push_back(j);
				break;
			}
		}
		if(!found) continue;
        istringstream(splitz[3])>>temp.start;
        istringstream(splitz[8])>>temp.stop;
        temp.stop = temp.start+temp.stop;
        raw.push_back(temp);
    }
    rawDataFile.close();
    vector<vector<int> > regionCount;
    for(int j = 0; j < allPartData->size(); j++) {
		vector<int> temp;
		for(int k = 0; k < allPartData->at(chrList->at(j)).size(); k++) {
			temp.push_back(0);
		}
		regionCount.push_back(temp);
    }
	cout<<"Counting: "<<filename<<endl;
    for(int j = 0; j < raw.size(); j++) {
        BedNode temp = raw[j];
		cout<<"binary"<<'\t'<<filename<<'\t'<<j<<endl;
		int index = binary_search(&(allPartData->at(temp.chr)), 0, allPartData->at(temp.chr).size()-1, temp);
		cout<<"returned "<<index<<"/"<<regionCount[chrpos[j]].size()<<endl;
		if(index != -1) {
			regionCount[chrpos[j]][index]++;
		}
		/*for(int k = 0; k < allPartData->at(temp.chr).size(); k++) {
            BedNode part = allPartData->at(temp.chr).at(k);
            if(temp.chr.compare(part.chr)==0 && ((temp.start > part.start && temp.start < part.stop) || (temp.stop > part.start && temp.stop < part.stop) || (temp.start < part.start && temp.stop > part.stop))) {
                regionCount[k]++;
            }
        }*/
		if(j%10000==0) cout<<j<<"/"<<raw.size()<<'\t'<<filename<<endl;
    }
    counts->at(i) = regionCount;
	cout<<"Finished "<<filename<<endl;
	return;
}

int main(int argc, char *argv[]) {
    if(argc < 2) {
        cout << "Usage: ./regionCounts [options] -RawDataFile <raw file list location> -Partitions <partition file> -Output <output file location>" <<endl;
        cout << "Options: <default>" <<endl;
        cout << "-MergeRegion: Regions this close together will be merged <0>"<<endl;
        cout << "-MinFeature: Size of smallest partition. <25>"<<endl;
        cout << "-IgnoreRandom: Ignores random chromosomes.  <Off> [Off, On]"<<endl;
        cout << "-PadRegion: Pads regions to this minimum size. <0>"<<endl;

		return 0;
    }
    int mergeRegion=0;
    int minFeature=25;
	int ignoreRandom = 0;
	int padRegion = 0;
	string rawDataFileName;
	string peakDataFileName;
    string trainingFileName;
    for(int i = 0; i < argc; i++) {
        string temp = argv[i];
        if(temp.compare("-MergeRegion")==0)
            istringstream(argv[i+1])>>mergeRegion;
        if(temp.compare("-MinFeature")==0)
            istringstream(argv[i+1])>>minFeature;
        if(temp.compare("-Output")==0)
            trainingFileName=argv[i+1];
        if(temp.compare("-RawDataFile")==0)
            rawDataFileName=argv[i+1];
		if(temp.compare("-Partitions")==0)
			peakDataFileName=argv[i+1];
        if(temp.compare("-IgnoreRandom")==0) {
			string option = argv[i+1];
			if(option.compare("On") == 0) {
				ignoreRandom=1;
			}
		}   
        if(temp.compare("-PadRegion")==0)
            istringstream(argv[i+1])>>padRegion;
    }
	
	cout<<"Reading Partition Data"<<endl;
	
	ifstream partDataFiles(peakDataFileName.c_str());
	string line1;
	map<string, vector<BedNode> > allPartData;
	vector<string> chrList;
	int parts = 0;
		//vector<BedNode> peakData;
		while(getline(partDataFiles,line1)) {
			if(line1[0] == '#') continue;
			vector<string> splitz = split(line1, ':');
			if(splitz.size() < 2) continue;
			BedNode temp;
			temp.chr = splitz[0];
			bool found = false;
			for(int i = 0; i < chrList.size(); i++) {
				if(chrList[i].compare(temp.chr)==0) {
					found = true;
					break;
				}
			}
			if(!found) {
				chrList.push_back(temp.chr);
			}
			vector<string> splitz2 = split(splitz[1], '-');
			istringstream(splitz2[0])>>temp.start;
			istringstream(splitz2[1])>>temp.stop;
			allPartData[temp.chr].push_back(temp);
			parts++;
		}
		partDataFiles.close();
		//allPeakData.push_back(peakData);
	cout<<parts<<" partitions loaded."<<endl;
	cout<<"Reading Raw Data"<<endl;
	vector<string> fileNames;
	vector<vector<BedNode> > rawData;
	ifstream rawDataFiles(rawDataFileName.c_str());
	int count = 0;
	vector<thread> threads;
	vector<vector<vector<int> > > regionCounts;
	while(getline(rawDataFiles,line1)) {
		threads.push_back(thread(regionCount, &allPartData, line1, &regionCounts, count, &chrList));
		regionCounts.push_back(vector<vector<int> >());
		count++;
	}
	rawDataFiles.close();
	for (auto& th : threads) th.join();
	
	cout<<"Outputing Training Matrix"<<endl;
	ofstream outfile(trainingFileName.c_str());
	for(int i = 0; i < chrList.size(); i++) {
		for(int k = 0; k < allPartData[chrList[i]].size(); k++) {
			BedNode part = allPartData[chrList[i]][k];
			outfile<<part.chr<<':'<<part.start<<'-'<<part.stop;
			for(int j = 0; j < regionCounts.size(); j++) {
				for(int k = 0; k < regionCounts[j].size(); k++) {
					double RPKM = ((double)regionCounts[j][k][i])/((double)rawData[j].size())/(1000000.0);
					outfile<<'\t'<<RPKM;
				}
			}
			outfile<<endl;
		}
	}
	outfile.close();
}