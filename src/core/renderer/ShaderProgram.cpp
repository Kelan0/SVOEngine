#include "core/renderer/ShaderProgram.h"
#include "core/util/FileUtils.h"
#include "core/util/StringUtils.h"
#include "core/Engine.h"
#include <GL/glew.h>

ShaderProgram::ShaderProgram(FragmentOutput fragmentOutput) :
	m_programID(0), m_completed(false) {
	this->setDataLocations(fragmentOutput);
}


ShaderProgram::~ShaderProgram() {
	for (int i = 0; i < m_shaders.size(); i++) {
		if (m_shaders[i] != NULL) {
			delete m_shaders[i];
		}
	}

	glDeleteProgram(m_programID);
}

void ShaderProgram::addShader(Shader* shader) {
	if (m_completed) {
		error("Cannot add a GLSL shader object to this shader program, the program has already been linked\n");
		return;
	}

	if (shader == NULL) {
		error("Failed to add GLSL shader object to program ID %d The shader was invalid or NULL\n", m_programID);
		return;
	}

	if (m_shaders.count(shader->getType()) != 0) {
		error("This shader program already has a %s attached\n", Shader::getShaderAsString(shader->getType()).c_str());
		return;
	}

	m_shaders.insert(std::pair<uint32, Shader*>(shader->getType(), shader));
}

void ShaderProgram::addShader(uint32 type, std::string file) {
	this->addShader(new Shader(type, file)); // this is a memory leak...
}

void ShaderProgram::addAttribute(int32 index, std::string name) {
	if (name.length() == 0 || m_attributes.count(name) != 0)
		return;

	m_attributes.insert(std::make_pair(name, index));
}

void ShaderProgram::addDataLocation(int32 index, std::string name) {
	if (name.length() == 0 || m_dataLocations.count(name) != 0)
		return;

	m_dataLocations.insert(std::make_pair(name, index));
}

void ShaderProgram::setDataLocations(FragmentOutput locations) {
	m_dataLocations.clear();

	for (auto it = locations.dataLocations.begin(); it != locations.dataLocations.end(); it++) {
		this->addDataLocation(it->index, it->name);
	}
}

void ShaderProgram::completeProgram() {
	if (m_completed) {
		error("Cannot complete shader program, the program has already been linked\n");
	}

	m_programID = glCreateProgram();
	info("Attaching shaders\n");
	for (std::pair<uint32, Shader*> entry : m_shaders) {
		entry.second->attachTo(m_programID);
	}

	info("Binding attribute locations\n");
	for (auto it = m_attributes.begin(); it != m_attributes.end(); ++it) {
		info("Adding attribute \"%s\" at location %d\n", it->first.c_str(), it->second);
		glBindAttribLocation(m_programID, it->second, it->first.c_str());
	}

	for (auto it = m_dataLocations.begin(); it != m_dataLocations.end(); it++) {
		info("Adding fragment data location \"%s\" at location %d\n", it->first.c_str(), it->second);
		glBindFragDataLocation(m_programID, it->second, it->first.c_str());
	}


	int32 status = GL_FALSE;
	int logLength;

	glLinkProgram(m_programID);
	glGetProgramiv(m_programID, GL_LINK_STATUS, &status);
	glGetProgramiv(m_programID, GL_INFO_LOG_LENGTH, &logLength);

	if (logLength > 0) {
		GLchar* programError = new GLchar[logLength];
		glGetProgramInfoLog(m_programID, logLength, nullptr, programError);
		if (status) {
			warn("Successfully linked shader program with warnings:\n%s\n", programError);
		} else {
			error("Failed to link shader program:\n%s\n", programError);
		}
	}

	glValidateProgram(m_programID);
	glGetProgramiv(m_programID, GL_VALIDATE_STATUS, &status);
	glGetProgramiv(m_programID, GL_INFO_LOG_LENGTH, &logLength);

	if (logLength > 0) {
		GLchar* programError = new GLchar[logLength];
		glGetProgramInfoLog(m_programID, logLength, nullptr, programError);
		if (status) {
			warn("Successfully validated shader program with warnings:\n%s\n", programError);
		} else {
			error("Failed to validate shader program:\n%s\n", programError);
		}
	}

	m_completed = true;
}

void ShaderProgram::useProgram(bool use) const {
	if (use) {
		ShaderProgram::use(this);
	} else {
		ShaderProgram::use(NULL);
	}
}

uint32 ShaderProgram::getProgramID() const {
	return m_programID;
}

bool ShaderProgram::isComplete() const {
	return m_completed;
}

void ShaderProgram::setUniform(std::string uniform, float f) {
	int32 id = getUniform(uniform);

	if (id >= 0) {
		glUniform1f(id, f);
	}
}

void ShaderProgram::setUniform(std::string uniform, float f, float f1) {
	int32 id = getUniform(uniform);

	if (id >= 0) {
		glUniform2f(id, f, f1);
	}
}

void ShaderProgram::setUniform(std::string uniform, float f, float f1, float f2) {
	int32 id = getUniform(uniform);

	if (id >= 0) {
		glUniform3f(id, f, f1, f2);
	}
}

void ShaderProgram::setUniform(std::string uniform, float f, float f1, float f2, float f3) {
	int32 id = getUniform(uniform);

	if (id >= 0) {
		glUniform4f(id, f, f1, f2, f3);
	}
}

void ShaderProgram::setUniform(std::string uniform, double d) {
	int32 id = getUniform(uniform);

	if (id >= 0) {
		glUniform1d(id, d);
	}
}

void ShaderProgram::setUniform(std::string uniform, double d, double d1) {
	int32 id = getUniform(uniform);

	if (id >= 0) {
		glUniform2d(id, d, d1);
	}
}

void ShaderProgram::setUniform(std::string uniform, double d, double d1, double d2) {
	int32 id = getUniform(uniform);

	if (id >= 0) {
		glUniform3d(id, d, d1, d2);
	}
}

void ShaderProgram::setUniform(std::string uniform, double d, double d1, double d2, double d3) {
	int32 id = getUniform(uniform);

	if (id >= 0) {
		glUniform4d(id, d, d1, d2, d3);
	}
}

void ShaderProgram::setUniform(std::string uniform, int i) {
	int32 id = getUniform(uniform);

	if (id >= 0) {
		glUniform1i(id, i);
	}
}

void ShaderProgram::setUniform(std::string uniform, int i, int i1) {
	int32 id = getUniform(uniform);

	if (id >= 0) {
		glUniform2i(id, i, i1);
	}
}

void ShaderProgram::setUniform(std::string uniform, int i, int i1, int i2) {
	int32 id = getUniform(uniform);

	if (id >= 0) {
		glUniform3i(id, i, i1, i2);
	}
}

void ShaderProgram::setUniform(std::string uniform, int i, int i1, int i2, int i3) {
	int32 id = getUniform(uniform);

	if (id >= 0) {
		glUniform4i(id, i, i1, i2, i3);
	}
}

void ShaderProgram::setUniform(std::string uniform, unsigned int i) {
	int32 id = getUniform(uniform);

	if (id >= 0) {
		glUniform1ui(id, i);
	}
}

void ShaderProgram::setUniform(std::string uniform, unsigned int i, unsigned int i1) {
	int32 id = getUniform(uniform);

	if (id >= 0) {
		glUniform2ui(id, i, i1);
	}
}

void ShaderProgram::setUniform(std::string uniform, unsigned int i, unsigned int i1, unsigned int i2) {
	int32 id = getUniform(uniform);

	if (id >= 0) {
		glUniform3ui(id, i, i1, i2);
	}
}

void ShaderProgram::setUniform(std::string uniform, unsigned int i, unsigned int i1, unsigned int i2, unsigned int i3) {
	int32 id = getUniform(uniform);

	if (id >= 0) {
		glUniform4ui(id, i, i1, i2, i3);
	}
}

void ShaderProgram::setUniform(std::string uniform, bool b) {
	int32 id = getUniform(uniform);

	if (id >= 0) {
		glUniform1i(id, b);
	}
}

void ShaderProgram::setUniform(std::string uniform, fvec1 v) {
	setUniform(uniform, v.x);
}

void ShaderProgram::setUniform(std::string uniform, fvec2 v) {
	setUniform(uniform, v.x, v.y);
}

void ShaderProgram::setUniform(std::string uniform, fvec3 v) {
	setUniform(uniform, v.x, v.y, v.z);
}

void ShaderProgram::setUniform(std::string uniform, fvec4 v) {
	setUniform(uniform, v.x, v.y, v.z, v.w);
}

void ShaderProgram::setUniform(std::string uniform, fmat2x2 m) {
	int32 id = getUniform(uniform);

	if (id >= 0) {
		glUniformMatrix2fv(id, 1, GL_FALSE, glm::value_ptr(m));
	}
}

void ShaderProgram::setUniform(std::string uniform, fmat2x3 m) {
	int32 id = getUniform(uniform);

	if (id >= 0) {
		glUniformMatrix2x3fv(id, 1, GL_FALSE, glm::value_ptr(m));
	}
}

void ShaderProgram::setUniform(std::string uniform, fmat2x4 m) {
	int32 id = getUniform(uniform);

	if (id >= 0) {
		glUniformMatrix2x4fv(id, 1, GL_FALSE, glm::value_ptr(m));
	}
}

void ShaderProgram::setUniform(std::string uniform, fmat3x2 m) {
	int32 id = getUniform(uniform);

	if (id >= 0) {
		glUniformMatrix3x2fv(id, 1, GL_FALSE, glm::value_ptr(m));
	}
}

void ShaderProgram::setUniform(std::string uniform, fmat3x3 m) {
	int32 id = getUniform(uniform);

	if (id >= 0) {
		glUniformMatrix3fv(id, 1, GL_FALSE, glm::value_ptr(m));
	}
}

void ShaderProgram::setUniform(std::string uniform, fmat3x4 m) {
	int32 id = getUniform(uniform);

	if (id >= 0) {
		glUniformMatrix3x4fv(id, 1, GL_FALSE, glm::value_ptr(m));
	}
}

void ShaderProgram::setUniform(std::string uniform, fmat4x2 m) {
	int32 id = getUniform(uniform);

	if (id >= 0) {
		glUniformMatrix4x2fv(id, 1, GL_FALSE, glm::value_ptr(m));
	}
}

void ShaderProgram::setUniform(std::string uniform, fmat4x3 m) {
	int32 id = getUniform(uniform);

	if (id >= 0) {
		glUniformMatrix4x3fv(id, 1, GL_FALSE, glm::value_ptr(m));
	}
}

void ShaderProgram::setUniform(std::string uniform, fmat4x4 m) {
	int32 id = getUniform(uniform);

	if (id >= 0) {
		glUniformMatrix4fv(id, 1, GL_FALSE, glm::value_ptr(m));
	}
}

void ShaderProgram::setUniform(std::string uniform, dvec1 v) {
#ifdef DOUBLE_PRECISION_UNIFORMS_ENABLED
	setUniform(uniform, v.x);
#else
	setUniform(uniform, fvec1(v));
#endif
}

void ShaderProgram::setUniform(std::string uniform, dvec2 v) {
#ifdef DOUBLE_PRECISION_UNIFORMS_ENABLED
	setUniform(uniform, v.x, v.y);
#else
	setUniform(uniform, fvec2(v));
#endif
}

void ShaderProgram::setUniform(std::string uniform, dvec3 v) {
#ifdef DOUBLE_PRECISION_UNIFORMS_ENABLED
	setUniform(uniform, v.x, v.y, v.z);
#else
	setUniform(uniform, fvec3(v));
#endif
}

void ShaderProgram::setUniform(std::string uniform, dvec4 v) {
#ifdef DOUBLE_PRECISION_UNIFORMS_ENABLED
	setUniform(uniform, v.x, v.y, v.z, v.w);
#else
	setUniform(uniform, fvec4(v));
#endif
}

void ShaderProgram::setUniform(std::string uniform, dmat2x2 m) {
#ifdef DOUBLE_PRECISION_UNIFORMS_ENABLED
	int32 id = getUniform(uniform);

	if (id >= 0) {
		glUniformMatrix2dv(id, 1, GL_FALSE, glm::value_ptr(m));
	}
#else
	setUniform(uniform, fmat2x2(m));
#endif
}

void ShaderProgram::setUniform(std::string uniform, dmat2x3 m) {
#ifdef DOUBLE_PRECISION_UNIFORMS_ENABLED
	int32 id = getUniform(uniform);

	if (id >= 0) {
		glUniformMatrix2x3dv(id, 1, GL_FALSE, glm::value_ptr(m));
	}
#else
	setUniform(uniform, fmat2x3(m));
#endif
}

void ShaderProgram::setUniform(std::string uniform, dmat2x4 m) {
#ifdef DOUBLE_PRECISION_UNIFORMS_ENABLED
	int32 id = getUniform(uniform);

	if (id >= 0) {
		glUniformMatrix2x4dv(id, 1, GL_FALSE, glm::value_ptr(m));
	}
#else
	setUniform(uniform, fmat2x4(m));
#endif
}

void ShaderProgram::setUniform(std::string uniform, dmat3x2 m) {
#ifdef DOUBLE_PRECISION_UNIFORMS_ENABLED
	int32 id = getUniform(uniform);

	if (id >= 0) {
		glUniformMatrix3x2dv(id, 1, GL_FALSE, glm::value_ptr(m));
	}
#else
	setUniform(uniform, fmat3x2(m));
#endif
}

void ShaderProgram::setUniform(std::string uniform, dmat3x3 m) {
#ifdef DOUBLE_PRECISION_UNIFORMS_ENABLED
	int32 id = getUniform(uniform);

	if (id >= 0) {
		glUniformMatrix3dv(id, 1, GL_FALSE, glm::value_ptr(m));
	}
#else
	setUniform(uniform, fmat3x3(m));
#endif
}

void ShaderProgram::setUniform(std::string uniform, dmat3x4 m) {
#ifdef DOUBLE_PRECISION_UNIFORMS_ENABLED
	int32 id = getUniform(uniform);

	if (id >= 0) {
		glUniformMatrix3x4dv(id, 1, GL_FALSE, glm::value_ptr(m));
	}
#else
	setUniform(uniform, fmat3x4(m));
#endif
}

void ShaderProgram::setUniform(std::string uniform, dmat4x2 m) {
#ifdef DOUBLE_PRECISION_UNIFORMS_ENABLED
	int32 id = getUniform(uniform);

	if (id >= 0) {
		glUniformMatrix4x2dv(id, 1, GL_FALSE, glm::value_ptr(m));
	}
#else
	setUniform(uniform, fmat4x2(m));
#endif
}

void ShaderProgram::setUniform(std::string uniform, dmat4x3 m) {
#ifdef DOUBLE_PRECISION_UNIFORMS_ENABLED
	int32 id = getUniform(uniform);

	if (id >= 0) {
		glUniformMatrix4x3dv(id, 1, GL_FALSE, glm::value_ptr(m));
	}
#else
	setUniform(uniform, fmat4x3(m));
#endif
}

void ShaderProgram::setUniform(std::string uniform, dmat4x4 m) {
#ifdef DOUBLE_PRECISION_UNIFORMS_ENABLED
	int32 id = getUniform(uniform);

	if (id >= 0) {
		glUniformMatrix4dv(id, 1, GL_FALSE, glm::value_ptr(m));
	}
#else
	setUniform(uniform, fmat4x4(m));
#endif
}

void ShaderProgram::setUniform(std::string uniform, ivec1 v) {
	setUniform(uniform, v.x);
}

void ShaderProgram::setUniform(std::string uniform, ivec2 v) {
	setUniform(uniform, v.x, v.y);
}

void ShaderProgram::setUniform(std::string uniform, ivec3 v) {
	setUniform(uniform, v.x, v.y, v.z);
}

void ShaderProgram::setUniform(std::string uniform, ivec4 v) {
	setUniform(uniform, v.x, v.y, v.z, v.w);
}

void ShaderProgram::setUniform(std::string uniform, uvec1 v) {
	setUniform(uniform, v.x);
}

void ShaderProgram::setUniform(std::string uniform, uvec2 v) {
	setUniform(uniform, v.x, v.y);
}

void ShaderProgram::setUniform(std::string uniform, uvec3 v) {
	setUniform(uniform, v.x, v.y, v.z);
}

void ShaderProgram::setUniform(std::string uniform, uvec4 v) {
	setUniform(uniform, v.x, v.y, v.z, v.w);
}

int32 ShaderProgram::getUniform(std::string uniform) {
	int32 id = 0;
	if (m_uniforms.count(uniform) != 1) {
		id = glGetUniformLocation(m_programID, uniform.c_str());

		// info("Caching uniform loaction \"%s\" with id %d\n", uniform.c_str(), id);

		m_uniforms.insert(std::make_pair(uniform, id));
	} else {
		id = m_uniforms.at(uniform);
	}

	return id;
}

void ShaderProgram::use(const ShaderProgram* program) {
	if (program != NULL) {
		glUseProgram(program->m_programID);
	} else {
		glUseProgram(0);
	}
}

Shader::Shader(uint32 type, std::string file) {
	info("Loading shader file %s\n", file.c_str());
	if (type != GL_VERTEX_SHADER && 
		type != GL_FRAGMENT_SHADER && 
		type != GL_GEOMETRY_SHADER &&
		type != GL_COMPUTE_SHADER &&
		type != GL_TESS_EVALUATION_SHADER &&
		type != GL_TESS_CONTROL_SHADER
		) {

		warn("The shader type %s (%d) was not valid or was not supported by this version. Shader file %s will not be loaded.\n", Shader::getShaderAsString(type).c_str(), type, file.c_str());
		throw std::invalid_argument("Invalid shader type");
	}

	int32 flag = GL_FALSE;
	int logLength;


	std::string fileRaw;
	if (!Shader::loadShaderFile(file, fileRaw)) {
		warn("An error occurred while loading this shader\n");
		throw std::exception("Failed to open shader source file");
	}

	std::stringstream parsedSource;
	parsedSource << "#version 440 core\n";
	parsedSource << "#extension GL_ARB_bindless_texture : require\n";
	parsedSource << "#line 1\n";

	std::vector<std::string> includes;
	int count = Shader::parseIncludes(fileRaw, parsedSource, includes);
	if (count < 0) {
		warn("An error occurred while parsing this shader\n");
		throw std::exception("Failed to parse shader source file");
	}

	parsedSource << "\0"; // NUL

	std::string glslSrcStr = parsedSource.str(); // needed otherwise garbage memory
	const char* glslSrc = glslSrcStr.c_str();

	info("Compiling shader\n");
	uint32 shaderID = glCreateShader(type);
	glShaderSource(shaderID, 1, &glslSrc, nullptr);
	glCompileShader(shaderID);
	glGetShaderiv(shaderID, GL_COMPILE_STATUS, &flag);
	glGetShaderiv(shaderID, GL_INFO_LOG_LENGTH, &logLength);
	char* shaderLog = new char[logLength];
	glGetShaderInfoLog(shaderID, logLength, nullptr, shaderLog);

	if (flag == GL_FALSE) { // error
		error("Failed to compile shader: %s\n", shaderLog);
		int lineNum = 0;
		for (std::string line; std::getline(parsedSource, line); ) {
			info("[%d]\t%s\n", lineNum, line.c_str());
			if (line.rfind("#line ", 0) == 0) {
				line = line.substr(6);
				line = StringUtils::trim(line);
				lineNum = std::stoi(line);
			} else {
				lineNum++;
			}
		}
		glDeleteShader(shaderID);
	} else if (logLength > 0) { // warning or info about shader, not necessarily a failure.
		info("%s\n", shaderLog);
	}

	m_type = type;
	m_id = shaderID;
	m_file = file;
	m_source = fileRaw;
}

Shader::Shader(uint32 type, uint32 id, std::string file, std::string source) :
	m_program(0), m_id(id), m_type(type), m_file(file), m_source(source) {}

Shader::~Shader() {
	glDetachShader(m_program, m_id);
	glDeleteShader(m_id);
}

uint32 Shader::getPorgram() const {
	return m_program;
}

uint32 Shader::getType() const {
	return m_type;
}

uint32 Shader::getID() const {
	return m_id;
}

std::string Shader::getFile() const {
	return m_file;
}

std::string Shader::getSource() const {
	return m_source;
}

void Shader::attachTo(uint32 program) {
	m_program = program;
	glAttachShader(program, m_id);
}

std::string Shader::getShaderAsString(uint32 type) {
	if (type == GL_VERTEX_SHADER) return "GL_VERTEX_SHADER";
	if (type == GL_FRAGMENT_SHADER) return "GL_FRAGMENT_SHADER";
	if (type == GL_GEOMETRY_SHADER) return "GL_GEOMETRY_SHADER";
	if (type == GL_COMPUTE_SHADER) return "GL_COMPUTE_SHADER";
	if (type == GL_TESS_EVALUATION_SHADER) return "GL_TESS_EVALUATION_SHADER";
	if (type == GL_TESS_CONTROL_SHADER) return "GL_TESS_CONTROL_SHADER";

	return "INVALID_SHADER_TYPE";
}

bool Shader::loadShaderFile(std::string filePath, std::string& dest) {
	std::vector<std::string> paths = {
		filePath,
		filePath + ".glsl",
		RESOURCE_PATH(filePath),
		RESOURCE_PATH(filePath + ".glsl"),
		RESOURCE_PATH("res/" + filePath),
		RESOURCE_PATH("res/" + filePath + ".glsl"),
		RESOURCE_PATH("shaders/" + filePath),
		RESOURCE_PATH("shaders/" + filePath + ".glsl"),
		RESOURCE_PATH("res/shaders/" + filePath),
		RESOURCE_PATH("res/shaders/" + filePath + ".glsl"),
	};

	if (!FileUtils::loadFileAttemptPaths(paths, dest)) {
		return false;
	}

	return true;
}

int Shader::parseIncludes(std::string source, std::stringstream& parsedSource, std::vector<std::string>& includes) {
	std::istringstream ss(source);
	int count = 0;

	for (std::string line; std::getline(ss, line); ) {
		std::string include = StringUtils::trim(line);
		if (include.rfind("#include ", 0) == 0) { // line is an include
			include = include.substr(9); // remove "#include "
			include = StringUtils::trim(include); // remove white space
			include = include.substr(1, include.size() - 2); // remove quotation marks

			if (std::find(includes.begin(), includes.end(), include) == includes.end()) { // not already included
				includes.push_back(include);

				std::string includedSource;
				if (!Shader::loadShaderFile(include, includedSource)) {
					error("Could not include shader source \"%s\"\n", include.c_str());
					return -1;
				}

				int c = Shader::parseIncludes(includedSource, parsedSource, includes);
				if (c < 0) {
					return -1;
				}

				count += c;

				parsedSource << "#line 2\n"; // reset line number for correct error logs. Set to 2 to account for this "#include" line
				count++;
			}
		} else {
			parsedSource << line << "\n";
			count++;
		}
	}

	return count;
}
