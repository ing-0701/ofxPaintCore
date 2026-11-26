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
