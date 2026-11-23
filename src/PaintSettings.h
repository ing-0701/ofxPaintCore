#pragma once

#include "ofMain.h"

namespace paintcore {

struct PaintSettings {
	int width = 800;
	int height = 600;
	int internalFormat = GL_RGBA32F;
	int historyCapacity = 20;
	std::size_t strokeChunkSize = 10;
	float spacingPx = 3.0f;

	ofFbo::Settings makeFboSettings() const;
};

}
