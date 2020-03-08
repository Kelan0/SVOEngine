#pragma once
#include "core/pch.h"
#include "core/renderer/geometry/Mesh.h"

class Material;
struct MaterialConfiguration;

namespace MeshLoader {
	class OBJ {
	public:
		static const uint32_t npos;

		struct index {
			union {
				struct {
					uint32_t p, t, n;
				};
				uvec3 k;
				uint32_t i[3];
			};

			bool operator==(const index& other) const;

			bool operator!=(const index& other) const;

			bool operator<(const index& other) const;
		};

		struct face { // OBJ face. Contains no positions, normals or textures, only indices into those arrays.
			union {
				struct {
					index v0, v1, v2;
				};
				index v[3];
			};

			bool operator==(const face& other) const;

			bool operator!=(const face& other) const;
		};

		struct material {
			vec3 ambient = vec3(0.0F);                  // Ka
			vec3 diffuse = vec3(0.0F);                  // Kd
			vec3 specular = vec3(0.0F);                 // Ks
			vec3 transmission = vec3(0.0F);             // Kt/Tf (1 - dissolve)

			vec3 emission = vec3(0.0F);                 // Ke -- PBR

			float refractionIndex = 0.0F;               // Ni
			float specularExponent = 0.0F;              // Ns

			float roughness = 1.0F;                     // Pr -- PBR
			float metalness = 0.0F;                     // Pm -- PBR
			float clearcoatThickness = 0.0F;            // Pc -- PBR
			float clearcoatRoughness = 1.0F;            // Pcr -- PBR
			float anisotropy = 0.0F;                    // aniso -- PBR
			float anisotropicAngle = 0.0F;              // anisor -- PBR

			std::string ambientMapPath = "";          // map_Ka
			std::string diffuseMapPath = "";          // map_Kd
			std::string specularMapPath = "";         // map_Ks
			std::string specularExponentMapPath = ""; // map_Ns
			std::string bumpMapPath = "";             // map_bump/map_Bump
			std::string alphaMapPath = "";            // map_d
			std::string displacementMapPath = "";     // disp/(map_disp?)
			std::string reflectionMapPath = "";       // refl/(map_refl?)
			std::string roughnessMapPath = "";        // map_Pr -- PBR
			std::string metalnessMapPath = "";        // map_Pm -- PBR
			std::string sheenMapPath = "";            // map_Ps -- PBR
			std::string emissiveMapPath = "";         // map_Ke -- PBR
			std::string normalMapPath = "";           // norm/(map_norm?) -- PBR
		};

		class Object {
			friend class OBJ;
		public:
			std::string getObjectName() const;

			std::string getGroupName() const;

			std::string getMaterialName() const;

			uint32_t getTriangleBeginIndex() const;

			uint32_t getTriangleEndIndex() const;

			OBJ::material getMaterial() const;

			Mesh* createMesh(bool allocateGPU = true, bool deallocateCPU = true);

			void fillMeshBuffers(std::vector<Mesh::vertex>& vertices, std::vector<Mesh::triangle>& triangles);

			Material* createMaterial();

			MaterialConfiguration* createMaterialConfiguration();

			void initializeMaterialIndices();

		private:
			Object(OBJ* obj, std::string objectName, std::string groupName, std::string materialName);

			~Object();

			bool writeBinaryData(std::ofstream& stream);

			bool readBinaryData(std::ifstream& stream);

			OBJ* m_obj;
			std::string m_objectName;
			std::string m_groupName;
			std::string m_materialName;
			uint32_t m_triangleBeginIndex;
			uint32_t m_triangleEndIndex;
		};

		class MaterialSet {
			friend class OBJ;
		public:
			using material_map = typename std::map<std::string, material>;
			using iterator = material_map::iterator;
			using const_iterator = material_map::const_iterator;

			std::string getRootDirectory() const;

			uint32_t size() const;

			const_iterator begin() const;

			const_iterator end() const;

			bool hasMaterial(std::string name) const;

			material getMaterial(std::string name) const;

			Material* createMaterial(std::string name) const;

			MaterialConfiguration* createMaterialConfiguration(std::string name) const;

			MaterialConfiguration* createMaterialConfiguration(material material) const;

		private:
			MaterialSet(std::string rootDirectory);

			~MaterialSet();

			bool writeBinaryData(std::ofstream& stream);

			bool readBinaryData(std::ifstream& stream);

			std::string m_rootDirectory;
			material_map m_materials;
		};

		~OBJ();

		static OBJ* load(std::string file, bool readMdl = true, bool writeMdl = true);

		static MaterialSet* readMTL(std::string file);

		static OBJ* readOBJ(std::string file, std::string mtlDir = "");

		static OBJ* readMDL(std::string file, std::string mtlDir = "");

		bool writeMDL(std::string file);

		uint32_t getObjectCount() const;

		uint32_t getMaterialSetCount() const;

		uint32_t getMaterialCount() const;

		uint32_t getVertexCount() const;

		uint32_t getTriangleCount() const;

		Object* getObject(uint32_t index = 0) const;

		Object* getObject(std::string name, uint32* index = NULL) const;

		MaterialSet* getMaterialSet(uint32_t index = 0) const;

		material getMaterial(std::string name) const;

		bool hasMaterial(std::string name) const;

		std::map<std::string, std::vector<Object*>> createMaterialObjectMap() const;

		const std::vector<Mesh::vertex>& getVertices() const;

		const std::vector<Mesh::triangle>& getTriangles() const;

		void initializeMaterialIndices();

		Mesh* createMesh(bool allocateGPU = true, bool deallocateCPU = true);

		Material* createMaterial(std::string name) const;

		MaterialConfiguration* createMaterialConfiguration(std::string name) const;
	private:
		OBJ();

		static void compileObject(Object* currentObject, std::vector<Mesh::vertex>& vertices, std::vector<Mesh::triangle>& triangles, std::vector<face>& faces, std::vector<vec3>& positions, std::vector<vec2>& textures, std::vector<vec3>& normals, std::unordered_map<uvec3, uint32_t>& mappedIndices);

		std::vector<Object*> m_objects;
		std::vector<MaterialSet*> m_materialSets;
		std::vector<Mesh::vertex> m_vertices;
		std::vector<Mesh::triangle> m_triangles;
	};
};
