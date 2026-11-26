#include "CanvasRenderer.h"

namespace paintcore {

void CanvasRenderer::draw(
	PaintEngine & engine,
	int previewLayerId,
	const DrawLayer & drawLayer) {

	const auto & layers = engine.getLayers();

	// 配列の後ろから描き、一覧の上のレイヤーほど手前に表示する
	for (auto it = layers.rbegin(); it != layers.rend(); ++it) {
		const Layer & layer = **it;

		ofFbo & source = (layer.id == previewLayerId)
			? engine.getStrokePreview()
			: (*it)->canvas;

		drawLayer(layer, source);
	}
}

void CanvasRenderer::drawNormalLayer(const Layer&, ofFbo& source) {
	drawNormalImage(source);
}

void CanvasRenderer::drawNormalImage(ofFbo& source) {
	ofEnableAlphaBlending();
	ofSetColor(255, 255, 255, 255);

	glBlendEquation(GL_FUNC_ADD);
	glBlendFuncSeparate(
		GL_ONE, GL_ONE_MINUS_SRC_ALPHA,
		GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

	source.draw(0, 0);
}

}
