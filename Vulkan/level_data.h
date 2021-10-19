#pragma once
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include "h2bParser.h"
using namespace std;
#define MAX_SUBMESH_PER_DRAW 1024
namespace LEVEL {

	class LevelData {
		string level_file;
		fstream myFile;
		GW::MATH::GMatrix MatrixMath;
		// Other Stuff
		H2B::Parser classParser;

	public:
		// Information we will need the Graphics Card to have eventually
		GW::MATH::GMATRIXF worldMatrices[MAX_SUBMESH_PER_DRAW];
		string meshNames[MAX_SUBMESH_PER_DRAW];

		vector<H2B::VERTEX> m_vertices;
		vector<unsigned> mesh_vert_offsets;
		unsigned int mesh_vertex_counts[MAX_SUBMESH_PER_DRAW];

		vector<unsigned> m_indices;
		vector<unsigned> mesh_index_offsets;
		unsigned int mesh_index_counts[MAX_SUBMESH_PER_DRAW];

		vector<H2B::MATERIAL> m_materials;
		vector<unsigned> mesh_material_offsets;
		unsigned int mesh_material_counts[MAX_SUBMESH_PER_DRAW];

		vector<H2B::MESH> m_submeshes;
		vector<unsigned> mesh_submesh_offsets;
		unsigned int mesh_submesh_counts[MAX_SUBMESH_PER_DRAW];

		int num_mesh = 0;


		// Mutators
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
			mesh_material_offsets.resize(num_mesh);
			mesh_submesh_offsets.resize(num_mesh);

			// Load .mtl / .obj info with .h2b parser
			string filePath;
			int index = 0;
			int curr_vertice_offset = 0;
			int curr_indice_offset = 0;
			int curr_material_offset = 0;
			int curr_mesh_offset = 0;
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
					mesh_vertex_counts[index] = classParser.vertexCount;					// Store how many vertices there are
					m_vertices.resize(classParser.vertexCount + curr_vertice_offset);	// Make room to store vertices
					for (int i = 0; i < classParser.vertexCount; i++)								// Loop to move each vertex from .h2b to class m_vertices
					{
						m_vertices[i + curr_vertice_offset] = classParser.vertices[i];		// Store parsedVerts in m_vertices in proper position!!
					}
					mesh_vert_offsets[index] = curr_vertice_offset;								// Add offset to mesh_vert_offsets
					curr_vertice_offset += classParser.vertexCount;								// Update curr_offset to match where we are in the m_vertices array (vector)

					// Index Data
					mesh_index_counts[index] = classParser.indexCount;
					m_indices.resize(classParser.indexCount + curr_indice_offset);		// Repeat for Indices
					for (int i = 0; i < classParser.indexCount; i++)
					{
						m_indices[i + curr_indice_offset] = classParser.indices[i];
					}
					mesh_index_offsets[index] = curr_indice_offset;
					curr_indice_offset += classParser.indexCount;

					// Material Data
					mesh_material_counts[index] = classParser.materialCount;
					m_materials.resize(classParser.materialCount + curr_material_offset);
					for (int i = 0; i < classParser.materialCount; i++)
					{
						m_materials[i + curr_material_offset] = classParser.materials[i];
					}
					mesh_material_offsets[index] = curr_material_offset;
					curr_material_offset += classParser.materialCount;

					// Mesh Data
					mesh_submesh_counts[index] = classParser.meshCount;
					m_submeshes.resize(classParser.meshCount + curr_mesh_offset);
					for (int i = 0; i < classParser.meshCount; i++)
					{
						m_submeshes[i + curr_mesh_offset] = classParser.meshes[i];
					}
					mesh_material_offsets[index] = curr_mesh_offset;
					curr_mesh_offset += classParser.meshCount;
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
}