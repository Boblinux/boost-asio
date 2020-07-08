#include <fstream>
#include <iostream>
using namespace std;
 //g++ -std=c++11 GetTestFile.cc -o GetTestFile
int main()
{
	//input data to file1.txt
	ofstream outfile;
	outfile.open("kvmap_1.txt");
    std::string message;
    long long n = 200000000;
    while(n--)
    {
        std::string message =  std::to_string(n) + ':'+ std::to_string(n);
        outfile << message << endl;
        message.clear();
    }
 
	outfile.close();
	return 0;
}