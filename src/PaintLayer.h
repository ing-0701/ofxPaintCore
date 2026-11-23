#pragma once

#include "ofMain.h"
#include "PaintSettings.h"

#include <string>

namespace paintcore {

class Layer {
public:
	const int id;
	std::string name;
	ofFbo canvas;
    bool visible = true;
    bool locked = false;
    float opacity = 1.0f;
    glm::vec2 position { 0.0f, 0.0f };

	Layer(
		int layerId,
		const std::string & layerName,
		const PaintSettings & settings);

	Layer(
		int layerId,
		const std::string& layerName,
		int width,
		int height
	);
};

}
