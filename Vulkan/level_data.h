#pragma once
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
using namespace std;
#define MAX_SUBMESH_PER_DRAW 1024

class LevelData {
	string level_file;
	fstream myFile;
	GW::MATH::GMatrix MatrixMath;
	GW::MATH::GMATRIXF worldMatrices[MAX_SUBMESH_PER_DRAW];
	string meshNames[MAX_SUBMESH_PER_DRAW];

public:
	void SetLevel(string level_file_path)
	{
		level_file = level_file_path;
	}
	void LoadLevel()
	{
		myFile.open(level_file, ios::in);
		if (myFile.is_open())
		{
			string line;
			cout << "Successfully accessed Level File" << endl;
			// Iterator to Store Data
			int index = 0;
			while (getline(myFile, line, '\n'))
			{
				if (line == "MESH")
				{
					// Parse Name of Mesh
					getline(myFile, line);
					int pos = line.find('.');
					if (pos > -1)
					{
						line.resize(pos);
					}
					meshNames[index] = line;

					GW::MATH::GVECTORF matrix_rows[4];
					// Parse wMatrix Data
					for (int i = 0; i < 4; i++)
					{
						getline(myFile, line, '(');
						getline(myFile, line, ',');
						matrix_rows[i].x = atof(line.c_str());
						getline(myFile, line, ',');
						matrix_rows[i].y = atof(line.c_str());
						getline(myFile, line, ',');
						matrix_rows[i].z = atof(line.c_str());
						getline(myFile, line, ')');
						matrix_rows[i].w = atof(line.c_str());
					}
					worldMatrices[index].row1 = matrix_rows[0];
					worldMatrices[index].row2 = matrix_rows[1];
					worldMatrices[index].row3 = matrix_rows[2];
					worldMatrices[index].row4 = matrix_rows[3];

					++index;
				}
			}
			myFile.close();
		}
		else
			cout << "Level Was Not Found" << endl;
	}
	LevelData()
	{
		level_file = "../../TestLevel.txt";
	}
};