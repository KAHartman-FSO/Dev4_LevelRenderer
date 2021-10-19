#pragma once
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
using namespace std;
#define MAX_SUBMESH_PER_DRAW 1024

class LevelData {
	fstream myFile;
	GW::MATH::GMatrix MatrixMath;
	GW::MATH::GMATRIXF worldMatrices[MAX_SUBMESH_PER_DRAW];
public:
	LevelData()
	{
		myFile.open("../../TestLevel.txt", ios::in);
		if (myFile.is_open())
		{
			string line;
			cout << "We accessed the Text File" << endl;
			while (getline(myFile, line, '\n'))
			{
				cout << line << endl;
			}
			myFile.close();
		}
		else
			cout << "Bruhhh" << endl;
	}
};