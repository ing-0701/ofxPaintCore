#include "PaintSettings.h"

#include <stdexcept>

namespace paintcore {

// ソフト全体の設定
// 縦横比、色形式、履歴キャパ、ストロークの途中保存の間隔、ブラシの配置間隔
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
