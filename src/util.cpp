#include "sys.h"


void util::loadFile(std::string path, std::function<void(std::string)> cb) {
	std::ifstream in(path);

	std::string temp;

	while (std::getline(in, temp)) {
		cb(temp);
	}

	in.close();
}

void util::strSplit(std::string str, char delim, std::function<void(std::string)> cb) {
	std::stringstream ss(str);
	std::string temp;

	while (std::getline(ss, temp, delim)) {
		cb(temp);
	}
}

void util::loadBlob(std::string path, std::vector<char>& data) {
	data.clear();
	std::ifstream in(path, std::ios::binary);
	in.seekg(0, std::ios::end);
	data.resize(in.tellg());
	in.seekg(0, std::ios::beg);
	in.read(data.data(), data.size());
	in.close();
}

glm::vec3 util::toVec3(std::string str) {
	std::vector<std::string> temp;

	strSplit(str, ':', [&](std::string str) {
		temp.push_back(str);
	});

	return glm::vec3(
		std::stof(temp[0]),
		std::stof(temp[1]),
		std::stof(temp[2])
	);
}

glm::vec2 util::toVec2(std::string str) {
	std::vector<std::string> temp;

	strSplit(str, ':', [&](std::string str) {
		temp.push_back(str);
	});

	return glm::vec2(
		std::stof(temp[0]),
		std::stof(temp[1])
	);
}

// Rect
void util::Rect::init(glm::vec2 p, glm::vec2 s) {
	this->position = p;
	this->size = s;
}

float util::Rect::left() {
	return this->position.x;
}

float util::Rect::right() {
	return this->position.x + this->size.x;
}

float util::Rect::top() {
	return this->position.y;
}

float util::Rect::bottom() {
	return this->position.y + this->size.y;
}

bool util::Rect::isCollide(Rect& r) {
	return
		this->left() < r.right() &&
		this->right() > r.left() &&
		this->top() < r.bottom() &&
		this->bottom() > r.top();
}