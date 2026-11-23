#include "CanvasRenderer.h"

namespace paintcore {

void CanvasRenderer::draw(
	PaintEngine& engine,
	int previewLayerId,
	const DrawLayer& drawLayer,
    const LayerPlacement& placement) {
	const auto & layers = engine.getLayers();

	for (auto it = layers.rbegin(); it != layers.rend(); it++) {
		// itが各ループで参照している配列要素のイテレータというものであり、*itが実際の要素
		// layersはスマートポインタの配列だから、各要素をaとすると*aで実際のレイヤーが得られる
		// すなわち**itが各ループで参照しているレイヤー
		const Layer & layer = **it;
        if (!layer.visible || layer.opacity <= 0.0f) continue;

		// 描画中のレイヤーならpreFbo2Dを、そうじゃないなら各レイヤーの内容を描画
		ofFbo & source = (layer.id == previewLayerId)
			? engine.getStrokePreview()
			: (*it)->canvas;

        const auto position = placement ? placement(layer) : layer.position;
        ofPushMatrix();
        ofTranslate(position.x, position.y);
		drawLayer(layer, source);
        ofPopMatrix();
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
