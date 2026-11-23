#include "PaintSettings.h"

#include <stdexcept>

namespace paintcore {

ofFbo::Settings PaintSettings::makeFboSettings() const {
	if (width <= 0 || height <= 0) {
		throw std::invalid_argument(
			"Canvas width and height must be positive."
		);
	}

    ofFbo::Settings settings;
	settings.width = width;
	settings.height = height;
	settings.internalformat = internalFormat;

#ifdef __EMSCRIPTEN__
	settings.textureTarget = GL_TEXTURE_2D;
#endif

	return settings;
}

}
