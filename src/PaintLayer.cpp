#include "PaintLayer.h"

namespace paintcore {

Layer::Layer(
	int layerId,
	const std::string & layerName,
	const PaintSettings & settings)
	: id(layerId)
	, name(layerName) {
	canvas.allocate(settings.makeFboSettings());

	canvas.begin();
	ofClear(0, 0, 0, 0);
	canvas.end();
}

Layer::Layer(
	int layerId,
	const std::string & layerName,
	int width,
	int height)
	: Layer(layerId, layerName, PaintSettings { width, height }) {
}

}
