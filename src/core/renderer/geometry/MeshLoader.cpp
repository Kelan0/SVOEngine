#include "core/renderer/geometry/MeshLoader.h"
#include "core/renderer/Material.h"
#include "core/renderer/MaterialManager.h"
#include "core/scene/Scene.h"
#include "core/Engine.h"


const uint32_t MeshLoader::OBJ::npos = -1; // Underflow, gives max unsigned integer.

inline std::string trim(std::string string) {
	const char* t = " \t\n\r\f\v";
	string.erase(0, string.find_first_not_of(t));
	string.erase(string.find_last_not_of(t) + 1);
	return string;
}

inline std::vector<std::string> split(std::string string, const char delimiter, bool ignoreEmpty = true, bool doTrim = true) {
	std::vector<std::string> comps;

	size_t start = 0;
	size_t end = string.find_first_of(delimiter);

	while (true) {
		std::string comp = string.substr(start, end - start);
		
		if (doTrim) 
			comp = trim(comp);

		if (ignoreEmpty || !comp.empty())
			comps.emplace_back(comp);

		if (end == std::string::npos)
			break;

		start = end + 1;
		end = string.find_first_of(delimiter, start);
	}

	return comps;
}

inline void writeString(std::ofstream& stream, std::string& str) {
	std::string::size_type len = str.size();
	stream.write(reinterpret_cast<char*>(&len), sizeof(std::string::size_type));
	stream.write(str.c_str(), len);
}

inline void readString(std::ifstream& stream, std::string& str) {
	std::string::size_type nameLength;
	stream.read(reinterpret_cast<char*>(&nameLength), sizeof(std::string::size_type));

	str.resize(nameLength);
	stream.read(reinterpret_cast<char*>(&str[0]), sizeof(char) * nameLength);
}

bool MeshLoader::OBJ::index::operator==(const index& other) const {
	if (this->p != other.p) return false;
	if (this->t != other.t) return false;
	if (this->n != other.n) return false;
	return true;
}

bool MeshLoader::OBJ::index::operator!=(const index& other) const {
	return !(*this == other);
}

bool MeshLoader::OBJ::index::operator<(const index& other) const {
	return this->p < other.p;
}

bool MeshLoader::OBJ::face::operator==(const face& other) const {
	if (this->v0 != other.v0) return false;
	if (this->v1 != other.v1) return false;
	if (this->v2 != other.v2) return false;
	return true;
}

bool MeshLoader::OBJ::face::operator!=(const face& other) const {
	return !(*this == other);
}

MeshLoader::OBJ::OBJ() {

}

MeshLoader::OBJ::~OBJ() {
	for (int i = 0; i < m_objects.size(); i++)
		delete m_objects[i];
	for (int i = 0; i < m_materialSets.size(); i++)
		delete m_materialSets[i];

	m_vertices.clear();
	m_triangles.clear();
	m_objects.clear();
}

MeshLoader::OBJ* MeshLoader::OBJ::load(std::string file, bool readMdl, bool writeMdl) {
	OBJ* obj = NULL;
	uint32_t extensionIndex = file.find_last_of('.');

	if (extensionIndex < file.size()) {
		std::string extension = file.substr(extensionIndex + 1);
		file = file.substr(0, extensionIndex);

		std::transform(extension.begin(), extension.end(), extension.begin(), std::tolower); // extension to lower case

		if (extension == "obj") {
			obj = OBJ::readOBJ(file + ".obj");
		} else if (extension == "mdl") {
			obj = OBJ::readMDL(file + ".mdl");
			if (obj != NULL)
				writeMdl = false; // We just read from the cache, don't re-write it.
		}
	} else {
		if (readMdl) {
			obj = OBJ::readMDL(file + ".mdl");
			if (obj != NULL)
				writeMdl = false;
		}

		if (obj == NULL) { // If we did not read a .mdl file, or failed to do so, try loading as a .obj file.
			obj = OBJ::readOBJ(file + ".obj");
		}
	}

	if (obj != NULL && writeMdl) {
		obj->writeMDL(file + ".mdl");
	}

	if (obj == NULL) {
		error("An error occurred while opening or parsing the model data for \"%s\"\n", file.c_str());
	}

	return obj;
}

MeshLoader::OBJ::MaterialSet* MeshLoader::OBJ::readMTL(std::string file) {
	const char* materialResource = RESOURCE_PATH(file).c_str();
	info("Parsing MTL file \"%s\"\n", materialResource);
	std::ifstream stream(materialResource, std::ifstream::in);

	if (!stream.is_open()) {
		return NULL;
	}

	std::string mtlDir = "";
	uint32_t mtlDirIdx = file.find_last_of("/");
	if (mtlDirIdx != std::string::npos)
		mtlDir = file.substr(0, mtlDirIdx);

	MaterialSet* mtl = new MaterialSet(mtlDir);

	std::string materialName = "";
	OBJ::material currentMaterial;

	std::string line;
	while (std::getline(stream, line)) {
		line = trim(line);

		if (line.empty()) {
			continue;
		}

		try {
			if (line.rfind("newmtl ", 0) == 0) {
				std::string nextName = line.substr(7);
				if (!materialName.empty()) {
					mtl->m_materials[materialName] = currentMaterial;
				}

				materialName = nextName;
				currentMaterial = {}; // reinitialize currentMaterial to an empty material.
			} else if (line.rfind("Ka ", 0) == 0) { // ambient RGB colour
				std::vector<std::string> comps = split(line, ' ', false);
				float r = std::stof(comps[1]);
				float g = std::stof(comps[2]);
				float b = std::stof(comps[3]);
				currentMaterial.ambient = vec3(r, g, b);
			} else if (line.rfind("Kd ", 0) == 0) { // diffuse RGB colour
				std::vector<std::string> comps = split(line, ' ', false);
				float r = std::stof(comps[1]);
				float g = std::stof(comps[2]);
				float b = std::stof(comps[3]);
				currentMaterial.diffuse = vec3(r, g, b);
			} else if (line.rfind("Ks ", 0) == 0) { // specular RGB colour
				std::vector<std::string> comps = split(line, ' ', false);
				float r = std::stof(comps[1]);
				float g = std::stof(comps[2]);
				float b = std::stof(comps[3]);
				currentMaterial.specular = vec3(r, g, b);
			} else if (line.rfind("Kt ", 0) == 0 || line.rfind("Tf ", 0) == 0) {
				std::vector<std::string> comps = split(line, ' ', false);
				float r = std::stof(comps[1]);
				float g = std::stof(comps[2]);
				float b = std::stof(comps[3]);
				currentMaterial.transmission = 1.0F - vec3(r, g, b);
			} else if (line.rfind("Tr ", 0) == 0) { // transmission
				std::vector<std::string> comps = split(line, ' ', false);
				currentMaterial.transmission = vec3(std::stof(comps[1]));
			} else if (line.rfind("d ", 0) == 0) { // dissolve (1 - transmission)
				std::vector<std::string> comps = split(line, ' ', false);
				currentMaterial.transmission = 1.0F - vec3(std::stof(comps[1]));
			} else if (line.rfind("Ke ", 0) == 0) { // emissive RGB colour
				std::vector<std::string> comps = split(line, ' ', false);
				float r = std::stof(comps[1]);
				float g = std::stof(comps[2]);
				float b = std::stof(comps[3]);
				currentMaterial.emission = vec3(r, g, b);
			} else if (line.rfind("Ni ", 0) == 0) { // reference of refraction
				std::vector<std::string> comps = split(line, ' ', false);
				currentMaterial.refractionIndex = std::stof(comps[1]);
			} else if (line.rfind("Ns ", 0) == 0) { // specular exponent
				std::vector<std::string> comps = split(line, ' ', false);
				currentMaterial.specularExponent = std::stof(comps[1]);
			} else if (line.rfind("Pr ", 0) == 0) { // PBR roughness
				std::vector<std::string> comps = split(line, ' ', false);
				currentMaterial.roughness = std::stof(comps[1]);
			} else if (line.rfind("Pm ", 0) == 0) { // PBR metalness
				std::vector<std::string> comps = split(line, ' ', false);
				currentMaterial.metalness = std::stof(comps[1]);
			} else if (line.rfind("Ps ", 0) == 0) { // PBR sheen (1 - roughness)
				std::vector<std::string> comps = split(line, ' ', false);
				currentMaterial.roughness = 1.0F - std::stof(comps[1]);
			} else if (line.rfind("Pc ", 0) == 0) { // PBR clear-coat thickness
				std::vector<std::string> comps = split(line, ' ', false);
				currentMaterial.clearcoatThickness = std::stof(comps[1]);
			} else if (line.rfind("Pcr ", 0) == 0) { // PBR clear-coat roughness
				std::vector<std::string> comps = split(line, ' ', false);
				currentMaterial.clearcoatRoughness = std::stof(comps[1]);
			} else if (line.rfind("aniso ", 0) == 0) { // PBR anisotropy
				std::vector<std::string> comps = split(line, ' ', false);
				currentMaterial.anisotropy = std::stof(comps[1]);
			} else if (line.rfind("anisor ", 0) == 0) { // PBR anisotropic rotation angle
				std::vector<std::string> comps = split(line, ' ', false);
				currentMaterial.anisotropicAngle = std::stof(comps[1]);
			} else if (line.rfind("map_Ka ", 0) == 0) { // ambient texture file path
				std::vector<std::string> comps = split(line, ' ', false);
				currentMaterial.ambientMapPath = comps[comps.size() - 1]; //_strdup(comps[comps.size() - 1].c_str());
			} else if (line.rfind("map_Kd ", 0) == 0) { // diffuse texture file path
				std::vector<std::string> comps = split(line, ' ', false);
				currentMaterial.diffuseMapPath = comps[comps.size() - 1]; //_strdup(comps[comps.size() - 1].c_str());
			} else if (line.rfind("map_Ks ", 0) == 0) { // specular texture file path
				std::vector<std::string> comps = split(line, ' ', false);
				currentMaterial.specularMapPath = comps[comps.size() - 1]; //_strdup(comps[comps.size() - 1].c_str());
			} else if (line.rfind("map_Ns ", 0) == 0) { // specular exponent texture file path
				std::vector<std::string> comps = split(line, ' ', false);
				currentMaterial.specularExponentMapPath = comps[comps.size() - 1]; //_strdup(comps[comps.size() - 1].c_str());
			} else if (line.rfind("map_bump ", 0) == 0 || line.rfind("map_Bump ", 0) == 0) { // bump (or normal?) map file path
				std::vector<std::string> comps = split(line, ' ', false);
				currentMaterial.bumpMapPath = comps[comps.size() - 1]; //_strdup(comps[comps.size() - 1].c_str());
			} else if (line.rfind("map_d ", 0) == 0) { // alpha (dissolve) map file path
				std::vector<std::string> comps = split(line, ' ', false);
				currentMaterial.alphaMapPath = comps[comps.size() - 1]; //_strdup(comps[comps.size() - 1].c_str());
			} else if (line.rfind("disp ", 0) == 0 || line.rfind("map_disp ", 0) == 0) { // displacement map file path
				std::vector<std::string> comps = split(line, ' ', false);
				currentMaterial.displacementMapPath = comps[comps.size() - 1]; //_strdup(comps[comps.size() - 1].c_str());
			} else if (line.rfind("refl ", 0) == 0 || line.rfind("map_refl ", 0) == 0) { // reflection map file path
				std::vector<std::string> comps = split(line, ' ', false);
				currentMaterial.reflectionMapPath = comps[comps.size() - 1]; //_strdup(comps[comps.size() - 1].c_str());
			} else if (line.rfind("map_Pr ", 0) == 0) { // PBR roughness map file path
				std::vector<std::string> comps = split(line, ' ', false);
				currentMaterial.roughnessMapPath = comps[comps.size() - 1]; //_strdup(comps[comps.size() - 1].c_str());
			} else if (line.rfind("map_Pm ", 0) == 0) { // PBR metalness map file path
				std::vector<std::string> comps = split(line, ' ', false);
				currentMaterial.metalnessMapPath = comps[comps.size() - 1]; //_strdup(comps[comps.size() - 1].c_str());
			} else if (line.rfind("map_Ps ", 0) == 0) { // PBR sheen map file path (1 - roughness)
				std::vector<std::string> comps = split(line, ' ', false);
				currentMaterial.sheenMapPath = comps[comps.size() - 1]; //_strdup(comps[comps.size() - 1].c_str());
			} else if (line.rfind("map_Ke ", 0) == 0) { // PBR emissive map file path
				std::vector<std::string> comps = split(line, ' ', false);
				currentMaterial.emissiveMapPath = comps[comps.size() - 1]; //_strdup(comps[comps.size() - 1].c_str());
			} else if (line.rfind("norm ", 0) == 0 || line.rfind("map_norm ", 0) == 0) { // PBR normal map file path
				std::vector<std::string> comps = split(line, ' ', false);
				currentMaterial.normalMapPath = comps[comps.size() - 1]; //_strdup(comps[comps.size() - 1].c_str());
			}
		} catch (std::exception e) {
			warn("Error while parsing MTL line \"%s\" - %s\n", line.c_str(), e.what());
		}
	}

	// Handle last material.
	if (!materialName.empty()) {
		mtl->m_materials[materialName] = currentMaterial;
	}

	return mtl;
}

MeshLoader::OBJ* MeshLoader::OBJ::readOBJ(std::string file, std::string mtlDir) {
	std::ifstream stream(RESOURCE_PATH(file).c_str(), std::ifstream::in);

	if (mtlDir.empty()) {
		uint32_t idx = file.find_last_of("/");
		if (idx != std::string::npos)
			mtlDir = file.substr(0, idx);
	}

	uint64_t t0 = Engine::instance()->getCurrentTime();
	if (!stream.is_open()) {
		return NULL;
	}

	uint32_t nextMaterialId = 0;

	OBJ* obj = new OBJ();

	std::vector<vec3> positions;
	std::vector<vec2> textures;
	std::vector<vec3> normals;
	std::vector<face> faces;

	std::vector<Mesh::vertex> vertices;
	std::vector<Mesh::triangle> triangles;
	std::unordered_map<uvec3, uint32_t> mappedIndices;
	std::unordered_map<std::string, MaterialSet*> loadedMaterialSets;

	std::string currentObjectName = "default";
	std::string currentGroupName = "default";
	std::string currentMaterialName = "default";
	OBJ::Object* currentObject = new Object(obj, currentObjectName, currentGroupName, currentMaterialName);
	std::vector<Object*> objects;

	// TODO: can this be efficiently parallellized ?

	uint32_t lineNumber = 0;
	std::string line;
	while (std::getline(stream, line)) {
		line = trim(line);
		lineNumber++;

		if (line.empty()) {
			continue;
		}

		try {
			if (line.rfind("v ", 0) == 0) { // line is a vertex position
				std::vector<std::string> comps = split(line, ' ', false);
				float x = std::stof(comps[1]);
				float y = std::stof(comps[2]);
				float z = std::stof(comps[3]);
				positions.push_back(fvec3(x, y, z));
			} else if (line.rfind("vt ", 0) == 0) { // line is a vertex texture
				std::vector<std::string> comps = split(line, ' ', false);
				float x = std::stof(comps[1]);
				float y = std::stof(comps[2]);
				textures.push_back(fvec2(x, y));
			} else if (line.rfind("vn ", 0) == 0) { // line is a vertex normal
				std::vector<std::string> comps = split(line, ' ', false);
				float x = std::stof(comps[1]);
				float y = std::stof(comps[2]);
				float z = std::stof(comps[3]);
				normals.push_back(fvec3(x, y, z));
			} else if (line.rfind("f ", 0) == 0) { // line is a face definition
				std::vector<std::string> faceComps = split(line, ' ');

				uint32_t faceSize = faceComps.size() - 1;

				if (faceSize < 3) {
					warn("Degenerate face - %d vertices\n");
					continue;
				}

				OBJ::index* indices = new OBJ::index[faceSize];
				for (int i = 0; i < faceSize; i++) {
					std::vector<std::string> vertComps = split(faceComps[i + 1], '/');

					indices[i].p = OBJ::npos;
					indices[i].t = OBJ::npos;
					indices[i].n = OBJ::npos;
					if (vertComps.size() == 3) { // p/t/n | p//n
						indices[i].p = std::stoi(vertComps[0]) - 1;
						indices[i].n = std::stoi(vertComps[2]) - 1;
						if (!vertComps[1].empty()) {
							indices[i].t = std::stoi(vertComps[1]) - 1;
						}
					} else if (vertComps.size() == 2) { // p/t
						indices[i].p = std::stoi(vertComps[0]) - 1;
						indices[i].t = std::stoi(vertComps[1]) - 1;
					} else if (vertComps.size() == 1) { // p
						indices[i].p = std::stoi(vertComps[0]) - 1;
					} else {
						throw std::exception("Invalid or unsupported face vertex definition format");
					}

					if (indices[i].p == OBJ::npos) { // position reference is required
						throw std::exception("Invalid or missing face vertex position index");
					}

					mappedIndices[indices[i].k] = OBJ::npos; // initialize to invalid reference
				}

				// triangle fan
				for (int i = 1; i < faceSize - 1; i++) {
					OBJ::face face;
					face.v0 = indices[0];
					face.v1 = indices[i];
					face.v2 = indices[i + 1];
					faces.push_back(face);
				}

				//if (faceSize >= 3) {
				//	OBJ::face face;
				//	face.v0 = indices[0];
				//	face.v1 = indices[1];
				//	face.v2 = indices[2];
				//	faces.push_back(face);
				//
				//	if (faceSize >= 4) {
				//		face.v0 = indices[0];
				//		face.v1 = indices[2];
				//		face.v2 = indices[3];
				//		faces.push_back(face);
				//
				//		if (faceSize >= 5) {
				//			info("[%d] INVALID FACE - %d VERTICES\n", lineNumber, faceSize);
				//		}
				//	}
				//}

				delete[] indices;
			} else if (line.rfind("o ", 0) == 0) { // line is an mesh definition
				currentObjectName = line.substr(2);

				if (!faces.empty()) {
					OBJ::compileObject(currentObject, vertices, triangles, faces, positions, textures, normals, mappedIndices);
					objects.push_back(currentObject);
				}

				currentObject = new Object(obj, currentObjectName, currentGroupName, currentMaterialName);
				mappedIndices.clear(); // Clear mapped indices - we don't want shared vertices across mesh boundries.
				faces.clear();
			} else if (line.rfind("g ", 0) == 0) { // line is a group definition
				currentGroupName = line.substr(2);

				if (!faces.empty()) {
					OBJ::compileObject(currentObject, vertices, triangles, faces, positions, textures, normals, mappedIndices);
					objects.push_back(currentObject);
				}

				currentObject = new Object(obj, currentObjectName, currentGroupName, currentMaterialName);
				faces.clear();
			} else if (line.rfind("usemtl ", 0) == 0) { // Line defines material to use for all folowing polygons
				std::string materialName = line.substr(7);

				if (materialName.empty())
					throw std::exception("Using material with invalid name");

				if (currentMaterialName != materialName) {
					currentMaterialName = materialName;

					if (!faces.empty()) {
						OBJ::compileObject(currentObject, vertices, triangles, faces, positions, textures, normals, mappedIndices);
						objects.push_back(currentObject);
					}

					currentObject = new Object(obj, currentObjectName, currentGroupName, currentMaterialName);
					//mappedIndices.clear(); // Clear mapped indices - we don't want shared vertices across material boundries.
					faces.clear();
				}
			} else if (line.rfind("mtllib ", 0) == 0) { // Line is a material loading command
				std::vector<std::string> materialPaths = split(line, ' ');

				for (int i = 1; i < materialPaths.size(); i++) {
					auto it = loadedMaterialSets.find(materialPaths[i]);

					if (it != loadedMaterialSets.end()) { // already loaded, don't re-load it.
						continue;
					}

					std::string mtlPath = mtlDir + "/" + materialPaths[i];
					MaterialSet* mtl = OBJ::readMTL(mtlPath);
					if (mtl == NULL) {
						warn("Failed to read or parse MTL file \"%s\"\n", materialPaths[i].c_str());
						continue;
					}
					obj->m_materialSets.push_back(mtl);
					loadedMaterialSets[materialPaths[i]] = mtl;
				}
			}
		} catch (std::exception e) {
			warn("Error while parsing OBJ line \"%s\" - %s\n", line.c_str(), e.what());
		}
	}

	if (currentObject != NULL && !faces.empty()) {
		OBJ::compileObject(currentObject, vertices, triangles, faces, positions, textures, normals, mappedIndices);
		objects.push_back(currentObject);
	}

	obj->m_objects.swap(objects);
	obj->m_vertices.swap(vertices);
	obj->m_triangles.swap(triangles);

	uint64_t t1 = Engine::instance()->getCurrentTime();
	info("Took %f seconds to load OBJ file \"%s\"\n", (t1 - t0) / 1000000000.0, file.c_str());

	return obj;
}

void MeshLoader::OBJ::compileObject(Object* currentObject, std::vector<Mesh::vertex>& vertices, std::vector<Mesh::triangle>& triangles, std::vector<face>& faces, std::vector<vec3>& positions, std::vector<vec2>& textures, std::vector<vec3>& normals, std::unordered_map<uvec3, uint32_t>& mappedIndices) {
	info("Compiling object group \"%s\" with %d faces ", currentObject->m_objectName.c_str(), faces.size());
	uint64_t t0 = Engine::instance()->getCurrentTime();
	currentObject->m_triangleBeginIndex = triangles.size();

	//size_t startVertexCount = vertices.size();

	for (int i = 0; i < faces.size(); i++) {
		OBJ::face& face = faces[i];
		Mesh::triangle tri;

		for (int j = 0; j < 3; j++) {
			OBJ::index& index = face.v[j];

			uint32_t mappedIndex = mappedIndices[index.k];

			if (mappedIndex == OBJ::npos) {
				Mesh::vertex vertex;
				vertex.position = index.p != OBJ::npos ? positions[index.p] : vec3(0.0);
				vertex.normal = index.n != OBJ::npos ? normals[index.n] : vec3(0.0);
				vertex.texture = index.t != OBJ::npos ? textures[index.t] : vec3(0.0);
				mappedIndex = vertices.size();
				mappedIndices[index.k] = mappedIndex;
				vertices.push_back(vertex);
			}

			tri.indices[j] = mappedIndex;
		}

		Mesh::vertex& v0 = vertices[tri.i0];
		Mesh::vertex& v1 = vertices[tri.i1];
		Mesh::vertex& v2 = vertices[tri.i2];

		vec3 e0 = v1.position - v0.position;
		vec3 e1 = v2.position - v0.position;

		double du0 = (double) v1.texture.x - v0.texture.x;
		double dv0 = (double) v1.texture.y - v0.texture.y;
		double du1 = (double) v2.texture.x - v0.texture.x;
		double dv1 = (double) v2.texture.y - v0.texture.y;

		double f = 1.0 / (du0 * dv1 - du1 * dv0);

		vec3 tangent;
		tangent.x = f * (dv1 * e0.x - dv0 * e1.x);
		tangent.y = f * (dv1 * e0.y - dv0 * e1.y);
		tangent.z = f * (dv1 * e0.z - dv0 * e1.z);

		v0.tangent += tangent;
		v1.tangent += tangent;
		v2.tangent += tangent;

		triangles.push_back(tri);
	}

	currentObject->m_triangleEndIndex = triangles.size();

	// normalize tangents of all added vertices
	for (int i = currentObject->m_triangleBeginIndex; i < currentObject->m_triangleEndIndex; i++) {
		Mesh::triangle& tri = triangles[i];
		vertices[tri.i0].tangent = normalize(vertices[tri.i0].tangent);
		vertices[tri.i1].tangent = normalize(vertices[tri.i1].tangent);
		vertices[tri.i2].tangent = normalize(vertices[tri.i2].tangent);
	}

	uint64_t t1 = Engine::instance()->getCurrentTime();
	info("- Took %f msec\n", (t1 - t0) / 1000000.0);
}

MeshLoader::OBJ* MeshLoader::OBJ::readMDL(std::string file, std::string mtlDir) {
	std::ifstream stream(RESOURCE_PATH(file).c_str(), std::ifstream::in | std::ofstream::binary);
	if (!stream.is_open()) {
		return NULL;
	}

	info("Reading MDL file \"%s\"\n", file.c_str());
	OBJ* obj = new OBJ();

	size_t vertexCount, triangleCount, objectCount, materialSetCount;
	stream.read(reinterpret_cast<char*>(&vertexCount), sizeof(size_t));
	stream.read(reinterpret_cast<char*>(&triangleCount), sizeof(size_t));
	stream.read(reinterpret_cast<char*>(&objectCount), sizeof(size_t));
	stream.read(reinterpret_cast<char*>(&materialSetCount), sizeof(size_t));

	//info("Read %d vertices, %d triangles, %d mesh groups, %d material sets\n", vertexCount, triangleCount, objectCount, materialSetCount);

	obj->m_vertices.resize(vertexCount);
	obj->m_triangles.resize(triangleCount);
	obj->m_objects.resize(objectCount);
	obj->m_materialSets.resize(materialSetCount);

	stream.read(reinterpret_cast<char*>(&obj->m_vertices[0]), sizeof(Mesh::vertex) * vertexCount);
	stream.read(reinterpret_cast<char*>(&obj->m_triangles[0]), sizeof(Mesh::triangle) * triangleCount);

	bool error = false;
	for (int i = 0; i < objectCount; i++) {
		std::string tempName = std::to_string(i); // Names should be overwritten by readBinaryData... if it is not, something went wrong
		obj->m_objects[i] = new OBJ::Object(obj, tempName, tempName, tempName);

		if (!obj->m_objects[i]->readBinaryData(stream)) {
			error = true;
			break;
		}

		//info("Object %d - %s : %s : %s\n", i, obj->m_objects[i]->m_objectName.c_str(), obj->m_objects[i]->m_groupName.c_str(), obj->m_objects[i]->m_materialName.c_str());
	}

	if (mtlDir.empty()) {
		int mtlDirIdx = file.find_last_of('/');
		if (mtlDirIdx != std::string::npos) {
			mtlDir = file.substr(0, mtlDirIdx);
			info("MATERIAL DIRECTORY %s\n", mtlDir.c_str());
		}
	}

	for (int i = 0; i < materialSetCount; i++) {
		obj->m_materialSets[i] = new MaterialSet(mtlDir);
		if (!obj->m_materialSets[i]->readBinaryData(stream)) {
			error = true;
			break;
		}
	}

	if (error) {
		error("An error occurred while reading MDL file \"%s\"\n", file.c_str());
		delete obj;
		stream.close();
		return NULL;
	}

	return obj;
}

bool MeshLoader::OBJ::writeMDL(std::string file) {
	// TODO: write file header containing vertex format, face format, and other data so that adding some saved state does not invalidate previously saved MDLs
	info("Writing MDL file \"%s\" with %d vertices, %d triangles\n", file.c_str(), m_vertices.size(), m_triangles.size());
	const char* resourcePath = RESOURCE_PATH(file).c_str();
	std::ofstream stream(resourcePath, std::ofstream::out | std::ofstream::binary);
	if (!stream.is_open()) {
		info("Failed to write MDL file - could not open stream\n");
		return false;
	}

	size_t vertexCount = m_vertices.size();
	size_t triangleCount = m_triangles.size();
	size_t objectCount = m_objects.size();
	size_t materialSetCount = m_materialSets.size();
	stream.write(reinterpret_cast<char*>(&vertexCount), sizeof(size_t));
	stream.write(reinterpret_cast<char*>(&triangleCount), sizeof(size_t));
	stream.write(reinterpret_cast<char*>(&objectCount), sizeof(size_t));
	stream.write(reinterpret_cast<char*>(&materialSetCount), sizeof(size_t));
	stream.write(reinterpret_cast<char*>(&m_vertices[0]), sizeof(Mesh::vertex) * vertexCount);
	stream.write(reinterpret_cast<char*>(&m_triangles[0]), sizeof(Mesh::triangle) * triangleCount);

	for (int i = 0; i < m_objects.size(); i++) {
		if (!m_objects[i]->writeBinaryData(stream)) {
			stream.close();
			std::remove(resourcePath);
			return false;
		}
	}

	for (int i = 0; i < m_materialSets.size(); i++) {
		if (!m_materialSets[i]->writeBinaryData(stream)) {
			stream.close();
			std::remove(resourcePath);
			return false;
		}
	}

	stream.close();
	return true;
}

uint32_t MeshLoader::OBJ::getObjectCount() const {
	return m_objects.size();
}

uint32_t MeshLoader::OBJ::getMaterialSetCount() const {
	return m_materialSets.size();
}

uint32_t MeshLoader::OBJ::getMaterialCount() const {
	uint32_t count = 0;
	for (int i = 0; i < m_materialSets.size(); i++) {
		count += m_materialSets[i]->size();
	}
	return count;
}

uint32_t MeshLoader::OBJ::getVertexCount() const {
	return m_vertices.size();
}

uint32_t MeshLoader::OBJ::getTriangleCount() const {
	return m_triangles.size();
}

MeshLoader::OBJ::Object* MeshLoader::OBJ::getObject(uint32_t index) const {
	assert(index >= 0 && index < m_objects.size());
	return m_objects[index];
}

MeshLoader::OBJ::Object* MeshLoader::OBJ::getObject(std::string name, uint32* index) const {
	for (int i = 0; i < m_objects.size(); i++) {
		if (m_objects[i]->m_objectName == name) {
			if (index != NULL) *index = i;
			return m_objects[i];
		}
	}
	return NULL;
}

MeshLoader::OBJ::MaterialSet* MeshLoader::OBJ::getMaterialSet(uint32_t index) const {
	assert(index >= 0 && index < m_materialSets.size());
	return m_materialSets[index];
}

MeshLoader::OBJ::material MeshLoader::OBJ::getMaterial(std::string name) const {
	for (int i = 0; i < m_materialSets.size(); i++) {
		MaterialSet* materialSet = m_materialSets[i];

		MaterialSet::const_iterator it = materialSet->m_materials.find(name);
		if (it != materialSet->m_materials.end()) {
			return it->second;
		}
	}

	return material {}; // Empty material
}

bool MeshLoader::OBJ::hasMaterial(std::string name) const {
	for (int i = 0; i < m_materialSets.size(); i++) {
		if (m_materialSets[i]->hasMaterial(name)) {
			return true;
		}
	}

	return false;
}

std::map<std::string, std::vector<MeshLoader::OBJ::Object*>> MeshLoader::OBJ::createMaterialObjectMap() const {
	std::map<std::string, std::vector<Object*>> materialObjectMap;

	for (int i = 0; i < m_objects.size(); i++) {
		materialObjectMap[m_objects[i]->getMaterialName()].push_back(m_objects[i]);
	}

	return materialObjectMap;
}

const std::vector<Mesh::vertex>& MeshLoader::OBJ::getVertices() const {
	return m_vertices;
}

const std::vector<Mesh::triangle>& MeshLoader::OBJ::getTriangles() const {
	return m_triangles;
}

void MeshLoader::OBJ::initializeMaterialIndices() {
	for (int i = 0; i < this->getObjectCount(); i++) {
		this->getObject(i)->initializeMaterialIndices();
	}
}

Mesh* MeshLoader::OBJ::createMesh(bool allocateGPU, bool deallocateCPU) {
	Mesh* mesh = new Mesh(m_vertices, m_triangles);
	if (allocateGPU) {
		mesh->allocateGPU();
		mesh->uploadGPU();
	
		if (deallocateCPU) {
			mesh->deallocateCPU();
		}
	}

	//m_objects[0]->createMesh();

	return mesh;
}

Material* MeshLoader::OBJ::createMaterial(std::string name) const {
	for (int i = 0; i < m_materialSets.size(); i++) {
		MaterialSet* materialSet = m_materialSets[i];

		if (materialSet != NULL) {
			Material* material = materialSet->createMaterial(name);
			if (material != NULL) {
				return material;
			}
		}
	}

	return NULL;
}

MaterialConfiguration* MeshLoader::OBJ::createMaterialConfiguration(std::string name) const {
	for (int i = 0; i < m_materialSets.size(); i++) {
		MaterialSet* materialSet = m_materialSets[i];

		if (materialSet != NULL) {
			MaterialConfiguration* configuration = materialSet->createMaterialConfiguration(name);
			if (configuration != NULL) {
				return configuration;
			}
		}
	}

	return NULL;
}

std::string MeshLoader::OBJ::Object::getObjectName() const {
	return m_objectName;
}

std::string MeshLoader::OBJ::Object::getGroupName() const {
	return m_groupName;
}

std::string MeshLoader::OBJ::Object::getMaterialName() const {
	return m_materialName;
}

uint32_t MeshLoader::OBJ::Object::getTriangleBeginIndex() const {
	return m_triangleBeginIndex;
}

uint32_t MeshLoader::OBJ::Object::getTriangleEndIndex() const {
	return m_triangleEndIndex;
}

MeshLoader::OBJ::material MeshLoader::OBJ::Object::getMaterial() const {
	return m_obj->getMaterial(m_materialName);
}

Mesh* MeshLoader::OBJ::Object::createMesh(bool allocateGPU, bool deallocateCPU) {
	std::vector<Mesh::vertex> vertices;
	std::vector<Mesh::triangle> triangles;
	fillMeshBuffers(vertices, triangles);

	Mesh* mesh = new Mesh(vertices, triangles);
	if (allocateGPU) {
		mesh->allocateGPU();
		mesh->uploadGPU();
	
		if (deallocateCPU) {
			mesh->deallocateCPU();
		}
	}

	vertices.clear();
	triangles.clear();
	return mesh;
}

void MeshLoader::OBJ::Object::fillMeshBuffers(std::vector<Mesh::vertex>& vertices, std::vector<Mesh::triangle>& triangles) {
	uint32_t baseVertex = vertices.size();
	uint32_t minIndex = -1; // underflow to max
	uint32_t maxIndex = 0;

	for (int i = m_triangleBeginIndex; i <= m_triangleEndIndex; i++) {
		Mesh::triangle& tri = m_obj->m_triangles[i];
		for (int j = 0; j < 3; j++) {
			minIndex = std::min(minIndex, tri.indices[j]);
			maxIndex = std::max(maxIndex, tri.indices[j] + 1);
		}
	}

	//std::vector<Mesh::vertex> vertices(&m_obj->m_vertices[minIndex], &m_obj->m_vertices[maxIndex]);
	//std::vector<Mesh::triangle> triangles;
	vertices.reserve(vertices.size() + (maxIndex - minIndex));
	triangles.reserve(triangles.size() + (m_triangleEndIndex - m_triangleBeginIndex));

	vertices.insert(vertices.end(), &m_obj->m_vertices[minIndex], &m_obj->m_vertices[maxIndex]);

	for (int i = m_triangleBeginIndex; i <= m_triangleEndIndex; i++) {
		Mesh::triangle tri = Mesh::triangle(m_obj->m_triangles[i]); // copy
		tri.i0 = (tri.i0 - minIndex) + baseVertex;
		tri.i1 = (tri.i1 - minIndex) + baseVertex;
		tri.i2 = (tri.i2 - minIndex) + baseVertex;
		triangles.push_back(tri);
	}
}

Material* MeshLoader::OBJ::Object::createMaterial() {
	if (m_obj != NULL) {
		Material* material = m_obj->createMaterial(m_materialName);
		return material;
	}

	return NULL;
}

MaterialConfiguration* MeshLoader::OBJ::Object::createMaterialConfiguration() {
	if (m_obj != NULL) {
		MaterialConfiguration* configuration = m_obj->createMaterialConfiguration(m_materialName);
		return configuration;
	}

	return NULL;
}

void MeshLoader::OBJ::Object::initializeMaterialIndices() {
	MaterialManager* materialManager = Engine::scene()->getMaterialManager();
	MaterialConfiguration* materialConfiguration = this->createMaterialConfiguration();

	uint32_t materialIndex = materialManager->loadNamedMaterial(m_materialName, *materialConfiguration);

	for (uint32_t i = m_triangleBeginIndex; i < m_triangleEndIndex; ++i) {
		m_obj->m_vertices[m_obj->m_triangles[i].indices[0]].material = materialIndex;
		m_obj->m_vertices[m_obj->m_triangles[i].indices[1]].material = materialIndex;
		m_obj->m_vertices[m_obj->m_triangles[i].indices[2]].material = materialIndex;
	}

	delete materialConfiguration;
}

MeshLoader::OBJ::Object::Object(OBJ* obj, std::string objectName, std::string groupName, std::string materialName):
	m_obj(obj),
	m_objectName(objectName),
	m_groupName(groupName),
	m_materialName(materialName),
	m_triangleBeginIndex(0),
	m_triangleEndIndex(0) {
}

MeshLoader::OBJ::Object::~Object() {}

bool MeshLoader::OBJ::Object::writeBinaryData(std::ofstream& stream) {
	try {
		stream.write(reinterpret_cast<char*>(&m_triangleBeginIndex), sizeof(uint32_t));
		stream.write(reinterpret_cast<char*>(&m_triangleEndIndex), sizeof(uint32_t));

		writeString(stream, m_objectName);
		writeString(stream, m_groupName);
		writeString(stream, m_materialName);

		return true;
	} catch (std::exception e) {
		return false;
	}
}

bool MeshLoader::OBJ::Object::readBinaryData(std::ifstream& stream) {
	try {
		stream.read(reinterpret_cast<char*>(&m_triangleBeginIndex), sizeof(uint32_t));
		stream.read(reinterpret_cast<char*>(&m_triangleEndIndex), sizeof(uint32_t));

		readString(stream, m_objectName);
		readString(stream, m_groupName);
		readString(stream, m_materialName);

		return true;
	} catch (std::exception e) {
		return false;
	}
}

MeshLoader::OBJ::MaterialSet::MaterialSet(std::string rootDirectory):
	m_rootDirectory(rootDirectory) {
}

MeshLoader::OBJ::MaterialSet::~MaterialSet() {
	//for (const_iterator it = m_materials.begin(); it != m_materials.end(); it++) {
	//	OBJ::material mat = it->second;
	//
	//	if (mat.ambientMapPath != NULL) free(mat.ambientMapPath);
	//	if (mat.diffuseMapPath != NULL) free(mat.diffuseMapPath);
	//	if (mat.specularMapPath != NULL) free(mat.specularMapPath);
	//	if (mat.specularExponentMapPath != NULL) free(mat.specularExponentMapPath);
	//	if (mat.bumpMapPath != NULL) free(mat.bumpMapPath);
	//	if (mat.alphaMapPath != NULL) free(mat.alphaMapPath);
	//	if (mat.displacementMapPath != NULL) free(mat.displacementMapPath);
	//	if (mat.reflectionMapPath != NULL) free(mat.reflectionMapPath);
	//	if (mat.roughnessMapPath != NULL) free(mat.roughnessMapPath);
	//	if (mat.metalnessMapPath != NULL) free(mat.metalnessMapPath);
	//	if (mat.sheenMapPath != NULL) free(mat.sheenMapPath);
	//	if (mat.emissiveMapPath != NULL) free(mat.emissiveMapPath);
	//	if (mat.normalMapPath != NULL) free(mat.normalMapPath);
	//}
}

bool MeshLoader::OBJ::MaterialSet::writeBinaryData(std::ofstream& stream) {
	try {
		size_t materialCount = m_materials.size();
		std::vector<OBJ::material> materials;
		materials.reserve(materialCount);

		stream.write(reinterpret_cast<char*>(&materialCount), sizeof(size_t));

		for (const_iterator it = m_materials.begin(); it != m_materials.end(); it++) {
			materials.push_back(it->second);

			size_t nameLength = it->first.size(); // maybe 16 bit or 8 bit int? names wont be that long

			stream.write(reinterpret_cast<char*>(&nameLength), sizeof(size_t));
			stream.write(it->first.c_str(), sizeof(char) * nameLength); // sizeof(char) redundant
		}

		for (int i = 0; i < materials.size(); i++) {
			material& mat = materials[i];
			stream.write(reinterpret_cast<char*>(&mat.ambient), sizeof(vec3));
			stream.write(reinterpret_cast<char*>(&mat.diffuse), sizeof(vec3));
			stream.write(reinterpret_cast<char*>(&mat.specular), sizeof(vec3));
			stream.write(reinterpret_cast<char*>(&mat.transmission), sizeof(vec3));

			stream.write(reinterpret_cast<char*>(&mat.emission), sizeof(vec3));

			stream.write(reinterpret_cast<char*>(&mat.refractionIndex), sizeof(float));
			stream.write(reinterpret_cast<char*>(&mat.specularExponent), sizeof(float));

			stream.write(reinterpret_cast<char*>(&mat.roughness), sizeof(float));
			stream.write(reinterpret_cast<char*>(&mat.metalness), sizeof(float));
			stream.write(reinterpret_cast<char*>(&mat.clearcoatThickness), sizeof(float));
			stream.write(reinterpret_cast<char*>(&mat.clearcoatRoughness), sizeof(float));
			stream.write(reinterpret_cast<char*>(&mat.anisotropy), sizeof(float));
			stream.write(reinterpret_cast<char*>(&mat.anisotropicAngle), sizeof(float));

			writeString(stream, mat.ambientMapPath);
			writeString(stream, mat.diffuseMapPath);
			writeString(stream, mat.specularMapPath);
			writeString(stream, mat.specularExponentMapPath);
			writeString(stream, mat.bumpMapPath);
			writeString(stream, mat.alphaMapPath);
			writeString(stream, mat.displacementMapPath);
			writeString(stream, mat.reflectionMapPath);
			writeString(stream, mat.roughnessMapPath);
			writeString(stream, mat.metalnessMapPath);
			writeString(stream, mat.sheenMapPath);
			writeString(stream, mat.emissiveMapPath);
			writeString(stream, mat.normalMapPath);
		}
		
		//stream.write(reinterpret_cast<char*>(&materials[0]), sizeof(material)* materialCount);

		return true;
	} catch (std::exception e) {
		return false;
	}
}

bool MeshLoader::OBJ::MaterialSet::readBinaryData(std::ifstream& stream) {
	try {
		size_t materialCount;
		stream.read(reinterpret_cast<char*>(&materialCount), sizeof(size_t));

		std::vector<std::string> materialNames;
		std::vector<OBJ::material> materials;
		materialNames.resize(materialCount);
		materials.resize(materialCount);

		for (int i = 0; i < materialCount; i++) {
			readString(stream, materialNames[i]);
		}

		for (int i = 0; i < materials.size(); i++) {
			material& mat = materials[i];
			stream.read(reinterpret_cast<char*>(&mat.ambient), sizeof(vec3));
			stream.read(reinterpret_cast<char*>(&mat.diffuse), sizeof(vec3));
			stream.read(reinterpret_cast<char*>(&mat.specular), sizeof(vec3));
			stream.read(reinterpret_cast<char*>(&mat.transmission), sizeof(vec3));

			stream.read(reinterpret_cast<char*>(&mat.emission), sizeof(vec3));

			stream.read(reinterpret_cast<char*>(&mat.refractionIndex), sizeof(float));
			stream.read(reinterpret_cast<char*>(&mat.specularExponent), sizeof(float));

			stream.read(reinterpret_cast<char*>(&mat.roughness), sizeof(float));
			stream.read(reinterpret_cast<char*>(&mat.metalness), sizeof(float));
			stream.read(reinterpret_cast<char*>(&mat.clearcoatThickness), sizeof(float));
			stream.read(reinterpret_cast<char*>(&mat.clearcoatRoughness), sizeof(float));
			stream.read(reinterpret_cast<char*>(&mat.anisotropy), sizeof(float));
			stream.read(reinterpret_cast<char*>(&mat.anisotropicAngle), sizeof(float));

			readString(stream, mat.ambientMapPath);
			readString(stream, mat.diffuseMapPath);
			readString(stream, mat.specularMapPath);
			readString(stream, mat.specularExponentMapPath);
			readString(stream, mat.bumpMapPath);
			readString(stream, mat.alphaMapPath);
			readString(stream, mat.displacementMapPath);
			readString(stream, mat.reflectionMapPath);
			readString(stream, mat.roughnessMapPath);
			readString(stream, mat.metalnessMapPath);
			readString(stream, mat.sheenMapPath);
			readString(stream, mat.emissiveMapPath);
			readString(stream, mat.normalMapPath);
		}

		//stream.read(reinterpret_cast<char*>(&materials[0]), sizeof(material) * materialCount);

		for (int i = 0; i < materialCount; i++) {
			m_materials[materialNames[i]] = materials[i];
		}

		return true;
	} catch (std::exception e) {
		return false;
	}
}

std::string MeshLoader::OBJ::MaterialSet::getRootDirectory() const {
	return m_rootDirectory;
}

uint32_t MeshLoader::OBJ::MaterialSet::size() const {
	return m_materials.size();
}

MeshLoader::OBJ::MaterialSet::const_iterator MeshLoader::OBJ::MaterialSet::begin() const {
	return m_materials.cbegin();
}

MeshLoader::OBJ::MaterialSet::const_iterator MeshLoader::OBJ::MaterialSet::end() const {
	return m_materials.cend();
}

bool MeshLoader::OBJ::MaterialSet::hasMaterial(std::string name) const {
	return m_materials.find(name) != m_materials.end();
}

MeshLoader::OBJ::material MeshLoader::OBJ::MaterialSet::getMaterial(std::string name) const {
	const_iterator it = m_materials.find(name);
	if (it != m_materials.end()) {
		return it->second;
	}
	return material {}; // empty material
}

Material* MeshLoader::OBJ::MaterialSet::createMaterial(std::string name) const {
	MaterialConfiguration* configuration = this->createMaterialConfiguration(name);
	if (configuration == NULL)
		return NULL;

	Material* material = new Material(*configuration);
	delete configuration;
	return material;
}

MaterialConfiguration* MeshLoader::OBJ::MaterialSet::createMaterialConfiguration(std::string name) const {
	const_iterator it = m_materials.find(name);
	if (it != m_materials.end()) {
		return createMaterialConfiguration(it->second);
	}

	return NULL;
}

MaterialConfiguration* MeshLoader::OBJ::MaterialSet::createMaterialConfiguration(material material) const {
	std::string mtlDir = m_rootDirectory + "/";

	MaterialConfiguration* configuration = new MaterialConfiguration();
	configuration->albedo = material.diffuse;
	configuration->transmission = material.transmission;
	configuration->emission = material.emission;

	if (!material.diffuseMapPath.empty()) configuration->albedoMapPath = mtlDir + material.diffuseMapPath;

	if (!material.normalMapPath.empty()) configuration->normalMapPath = mtlDir + material.normalMapPath;
	else if (!material.bumpMapPath.empty()) configuration->normalMapPath = mtlDir + material.bumpMapPath;

	if (!material.specularMapPath.empty()) {
		configuration->roughnessMapPath = mtlDir + material.specularMapPath;
		configuration->roughnessInverted = true;
	}

	if (!material.alphaMapPath.empty()) {
		configuration->alphaMapPath = mtlDir + material.alphaMapPath;
	}

	// if normal map is only one channel, assume it is a grayscale bump map and perform a conversion

	return configuration;
}
