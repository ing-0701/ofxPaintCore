#include "PaintLayer.h"

namespace paintcore {

Layer::Layer(
	int layerId,
	const std::string & layerName,
	int width,
	int height)
	: id(layerId)
	, name(layerName) {
	ofFbo::Settings settings;
	settings.width = width;
	settings.height = height;
	settings.internalformat = GL_RGBA32F;

#ifdef __EMSCRIPTEN__
	settings.textureTarget = GL_TEXTURE_2D;
#endif

	canvas.allocate(settings);

	canvas.begin();
	ofClear(0, 0, 0, 0);
	canvas.end();
}

}
