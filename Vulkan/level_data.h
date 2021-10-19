#pragma once
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include "h2bParser.h"
using namespace std;
#define MAX_SUBMESH_PER_DRAW 1024

class LevelData {
	string level_file;
	fstream myFile;
	GW::MATH::GMatrix MatrixMath;

	// Information we will need the Graphics Card to have eventually
	GW::MATH::GMATRIXF worldMatrices[MAX_SUBMESH_PER_DRAW];
	string meshNames[MAX_SUBMESH_PER_DRAW];

	vector<H2B::VERTEX> m_vertices;
	vector<int> mesh_vert_offsets;

	vector<unsigned> m_indices;
	vector<int> mesh_index_offsets;

	// Other Stuff
	int num_mesh = 0;
	H2B::Parser classParser;

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
			num_mesh = index;
		}
		else
			cout << "Level Was Not Found" << endl;

		mesh_vert_offsets.resize(num_mesh);
		mesh_index_offsets.resize(num_mesh);

		// Load .mtl / .obj info with .h2b parser
		string filePath;
		int index = 0;
		int curr_v_offset = 0;
		int curr_i_offset = 0;
		int curr_m_offset = 0;
		bool loop = true;
		while (loop)
		{
			filePath = "../../Assets/";
			filePath.append(meshNames[index]);
			filePath.append(".h2b");
			if (classParser.Parse(filePath.c_str()))
			{
				cout << filePath << " successfully parsed." << endl;

				// Store Data in Class / Append to same array
				//	Vertex Data
				m_vertices.resize(classParser.vertexCount + curr_v_offset);	// Make room to store vertices
				for (int i = 0; i < classParser.vertexCount; i++)						// Loop to move each vertex from .h2b to class m_vertices
				{
					m_vertices[i + curr_v_offset] = classParser.vertices[i];			// Store parsedVerts in m_vertices in proper position!!
				}
				mesh_vert_offsets[index] = curr_v_offset;								// Add offset to mesh_vert_offsets
				curr_v_offset += classParser.vertexCount;								// Update curr_offset to match where we are in the m_vertices array (vector)

				// Index Data
				m_indices.resize(classParser.indexCount + curr_i_offset);
				for (int i = 0; i < classParser.indexCount; i++)
				{
					m_indices[i + curr_i_offset] = classParser.indices[i];
				}
				mesh_index_offsets[index] = curr_i_offset;
				curr_i_offset += classParser.indexCount;

				++index;
			}
			else
			{
				cout << "Could not parse " << filePath << endl;
				loop = false;
			}
		}
	
		int x = 0;
		
	}
	LevelData()
	{
		level_file = "../../TestLevel.txt";
	}
};