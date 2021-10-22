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
		H2B::Parser classParser;
		
		struct PARSED_DATA
		{
			unsigned vertexCount;
			unsigned indexCount;
			unsigned materialCount;
			unsigned meshCount;
			std::vector<H2B::VERTEX> vertices;
			std::vector<unsigned> indices;
			std::vector<H2B::MATERIAL> materials;
			std::vector <H2B::BATCH > batches;
			std::vector<H2B::MESH> meshes;
		};
		vector<PARSED_DATA> ParsedObjects;

		vector<string> meshNames;

		vector<GW::MATH::GVECTORF> FirePointLightPositions;

		// What I'll for Vertex and Index Buffers
		vector<H2B::VERTEX>		toVertexBuffer;
		vector<unsigned>			toIndexBuffer;

		// What I'll Need for Storage Buffer
		GW::MATH::GMATRIXF worldMatrices[MAX_SUBMESH_PER_DRAW];
		vector<H2B::ATTRIBUTES> materials;

		// What I'll Need for Drawing
		vector<unsigned> firstIndex;
		vector<unsigned> firstVertex;
		vector<unsigned> firstMaterial;
		
		int num_mesh = 0;

		struct Care_Package
		{
			vector<unsigned>			firstIndex;
			vector<unsigned>			firstVertex;
			vector<unsigned>			firstMaterial;
			vector<H2B::VERTEX>		toVertexBuffer;
			vector<unsigned int>		toIndexBuffer;
			vector<PARSED_DATA> ParsedObjects;
			vector<GW::MATH::GVECTORF> FirePointLightPositions;
			int num_mesh;
			GW::MATH::GMATRIXF	worldMatrices[MAX_SUBMESH_PER_DRAW];
			H2B::ATTRIBUTES				materials[MAX_SUBMESH_PER_DRAW];
		};

	public:
		Care_Package access;
		LevelData()
		{
			level_file = "../../TestLevel.txt";
		}
		void SetLevel(string level_file_path)
		{
			level_file = level_file_path;
		}
		void LoadWorldMatrixData()
		{
			fstream myFile;
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
						meshNames.push_back(line);

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

						if (meshNames[index] == "WoodFire")
						{
							FirePointLightPositions.push_back(worldMatrices[index].row4);
						}
						++index;
					}
				}
				myFile.close();
				num_mesh = index;
			}
			else
				cout << "Level Was Not Found" << endl;

			ParsedObjects.resize(num_mesh);
		}
		void H2BParse()
		{
			// Load .mtl / .obj info with .h2b parser
			string filePath;
			int index = 0;
			bool loop = true;
			for (int name = 0; name < meshNames.size(); name++)
			{
				filePath = "../../Assets/";
				filePath.append(meshNames[index]);
				filePath.append(".h2b");
				if (classParser.Parse(filePath.c_str()))
				{
					// For Each Object
					// Copy Literally Everything From classParser to a PARSED_DATA structure
					cout << filePath << " successfully parsed." << endl;

					ParsedObjects[index].vertices = classParser.vertices;
					ParsedObjects[index].vertexCount = classParser.vertexCount;

					ParsedObjects[index].indices = classParser.indices;
					ParsedObjects[index].indexCount = classParser.indexCount;

					ParsedObjects[index].materials = classParser.materials;
					ParsedObjects[index].materialCount = classParser.materialCount;

					ParsedObjects[index].meshes = classParser.meshes;
					ParsedObjects[index].meshCount = classParser.meshCount;

					ParsedObjects[index].batches = classParser.batches;

					if (meshNames[index] == "WoodFire")
					{
						ParsedObjects[index].materials[1].attrib.Ka = { 100, 40, 0 };
					}

					++index;
				}
				else
				{
					cout << "Not able to parse: " << filePath << endl;
				}
			}
		}
		void OneArray()
		{
			// Create Vertex Array for Vertex Buffer (storing firstVertex values along the way)
			unsigned int current_offset = 0;
			for (int i = 0; i < ParsedObjects.size(); i++)
			{
				// Adjust Size of VertexBuffer to hold more VertexData
				toVertexBuffer.resize(ParsedObjects[i].vertexCount + toVertexBuffer.size()); 
				for (int j = 0; j < ParsedObjects[i].vertexCount; j++)
				{
					// Starting at the Offset, insert each new vertice into Vertex Buffer
					toVertexBuffer[j +current_offset] = ParsedObjects[i].vertices[j];
				}
				firstVertex.push_back(current_offset);
				current_offset += ParsedObjects[i].vertexCount;
			}
			// Do the Same for the Index Buffer
			current_offset = 0;
			for (int i = 0; i < ParsedObjects.size(); i++)
			{
				toIndexBuffer.resize(ParsedObjects[i].indexCount + toIndexBuffer.size());
				for (int j = 0; j < ParsedObjects[i].indexCount; j++)
				{
					toIndexBuffer[j + current_offset] = ParsedObjects[i].indices[j];
				}
				firstIndex.push_back(current_offset);
				current_offset += ParsedObjects[i].indexCount;
			}
			// Do the Same for Materials Array
			current_offset = 0;
			for (int i = 0; i < ParsedObjects.size(); i++)
			{
				materials.resize(ParsedObjects[i].materialCount + materials.size());
				for (int j = 0; j < ParsedObjects[i].materialCount; j++)
				{
					materials[j + current_offset] = ParsedObjects[i].materials[j].attrib;
				}
				firstMaterial.push_back(current_offset);
				current_offset += ParsedObjects[i].materialCount;
			}

			// Populate Access
			access =
			{
				firstIndex,
				firstVertex,
				firstMaterial,
				toVertexBuffer,
				toIndexBuffer,
				ParsedObjects,
				FirePointLightPositions,
				num_mesh
			};
			for (int i = 0; i < MAX_SUBMESH_PER_DRAW; i++)
			{
				access.worldMatrices[i] = worldMatrices[i];
			}
			for (int i = 0; i < materials.size(); i++)
			{
				access.materials[i] = materials[i];
			}
		}
	};
}