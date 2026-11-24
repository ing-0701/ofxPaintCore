#pragma once

#include "ofMain.h"

namespace paintcore {

// ソフト全体の設定
// 縦横比、色形式、履歴キャパ、ストロークの途中保存の間隔
struct PaintSettings {
	int width = 800;
	int height = 600;
	int internalFormat = GL_RGBA32F;
	int historyCapacity = 20;
	std::size_t strokeChunkSize = 10;

	ofFbo::Settings makeFboSettings() const;
};

}
