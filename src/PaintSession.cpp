#include <cmath>
#include <utility>
#include <stdexcept>

#include "PaintSession.h"
#include "CanvasRenderer.h"

namespace paintcore {

PaintSession::PaintSession(const paintcore::PaintSettings & settings)
	: historyManager(settings.historyCapacity), paintSettings(settings) {
	canvasWidth = paintSettings.width;
	canvasHeight = paintSettings.height;

}

PaintSession::PaintSession(int mainWidth, int mainHeight)
	: PaintSession(paintcore::PaintSettings { mainWidth, mainHeight }) {
}

void PaintSession::setup(std::unique_ptr<LayerExtension> filter) {
    if (initialized) throw std::logic_error("PaintSession is already initialized.");
	const int initialLayerId = engine.setup(paintSettings);
	const ofFbo::Settings fboSettings = paintSettings.makeFboSettings();

	historySwapFbo.allocate(fboSettings);

	historyManager.initializeEntries(
		[&fboSettings](SessionHistoryEntry & entry) {
			entry.image.allocate(fboSettings);
		});

	if (!filter) filter = std::make_unique<LayerExtension>();
	filter->setup(canvasWidth, canvasHeight);
	filter->markDirty();

	extensions.emplace(initialLayerId, std::move(filter));

	selectedLayerId = initialLayerId;
	nextLayerId = initialLayerId + 1;
	strokeLayerId = 0;
    initialized = true;
}

void PaintSession::beginStroke(const glm::vec2 & position) {
    if (!std::isfinite(position.x) || !std::isfinite(position.y)) return;
    if (!initialized || isDrawing() || getLayer(selectedLayerId).locked || !getLayer(selectedLayerId).visible) return;
    activeBrush = brushSettings;
    previewDirty = true;
    previewReady = false;
	strokeLayerId = selectedLayerId;
	paintcore::Layer & layer = getLayer(strokeLayerId);
	engine.beginStroke(position - layer.position, layer.canvas, paintSettings.spacingPx);
}

void PaintSession::addStrokePoint(const glm::vec2 & position) {
    if (!isDrawing() || !std::isfinite(position.x) || !std::isfinite(position.y)) return;
	engine.addStrokePoint(position - getLayer(strokeLayerId).position);
    previewDirty = true;
}

void PaintSession::drawCanvas(bool drawingFlag) {
    if (isDrawing() && (!previewReady || previewDirty)) {
        engine.drawStroke(activeBrush);
        getExtension(strokeLayerId).markDirty();
        previewReady = true;
        previewDirty = false;
    }
	const float time = ofGetElapsedTimef();
	const int previewLayerId = drawingFlag ? strokeLayerId : 0;

	// 第三引数はラムダ式
	// 目的のレイヤーに対応するフィルターに応じて描画関数を呼ぶ
	// thisは今回使いたいgetExtension()やdraw()を持つオブジェクト
	// timeはこのdrawCanvas内で定義されたローカル変数なので明示が必要
	paintcore::CanvasRenderer::draw(
		engine,
		previewLayerId,
		[this, time](const paintcore::Layer & layer, ofFbo & source) {
			if (!layer.visible || layer.opacity <= 0.0f) return;
            if (layer.opacity == 1.0f) {
                getExtension(layer.id).draw(source, time);
            } else {
                if (!opacityBuffer.isAllocated()) opacityBuffer.allocate(paintSettings.makeFboSettings());
                opacityBuffer.begin();
                ofClear(0, 0, 0, 0);
                getExtension(layer.id).draw(source, time);
                opacityBuffer.end();
                ofEnableAlphaBlending();
                glBlendEquation(GL_FUNC_ADD);
                glBlendFuncSeparate(GL_ONE, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
                // All four channels are premultiplied by opacity.
                ofSetColor(ofFloatColor(layer.opacity, layer.opacity, layer.opacity, layer.opacity));
                opacityBuffer.draw(0, 0);
            }
		}, layerPlacement);
}

void PaintSession::storeLayerHistory(SessionHistoryEntry& entry, int layerIndex) {
	entry.layerIndex = layerIndex;
	entry.storedLayer = engine.takeLayerAt(layerIndex);

	
	const int id = entry.storedLayer->id;
	entry.storedExtension = std::move(extensions.at(id));
	extensions.erase(id);
}

void PaintSession::restoreLayerFromHistory(SessionHistoryEntry& entry) {
	const int id = entry.storedLayer->id;

	entry.storedExtension->markDirty();
	extensions.emplace(id, std::move(entry.storedExtension));

	engine.insertLayerAt(entry.layerIndex, std::move(entry.storedLayer));
}

void PaintSession::swapCanvasHistory(SessionHistoryEntry& entry) {
	paintcore::Layer & layer = getLayer(entry.layerId);
	ofFbo & target = layer.canvas;

	paintcore::copyFbo(target, historySwapFbo);
	paintcore::copyFbo(entry.image, target);
	paintcore::copyFbo(historySwapFbo, entry.image);

	getExtension(layer.id).markDirty();
}

// レイヤーの位置を調べてから履歴を保存する関数
// レイヤー削除時はすでに位置がわかってるから直接storeLayerHistoryを呼ぶ
void PaintSession::takeHistoryLayer(SessionHistoryEntry& entry) {
	const int layerIndex = findLayerIndex(entry.layerId);

	if (layerIndex == -1) {
		throw std::logic_error("History layer was not found.");
	}

	storeLayerHistory(entry, layerIndex);
}

void SessionHistoryEntry::undo(PaintSession& tool) {
	switch (type) {
	case SessionHistoryType::Custom:
        if (custom) custom->undo(tool);
        break;
	case SessionHistoryType::CanvasEdit:
		tool.swapCanvasHistory(*this);
		break;

	case SessionHistoryType::LayerAdded:
		tool.takeHistoryLayer(*this);
		tool.selectedLayerId = selectedBefore;
		tool.nextLayerId = nextIdBefore;
		break;

	case SessionHistoryType::LayerDeleted:
		tool.restoreLayerFromHistory(*this);
		tool.selectedLayerId = selectedBefore;
		break;
	}
}

void SessionHistoryEntry::redo(PaintSession & tool) {
	switch (type) {
	case SessionHistoryType::Custom:
        if (custom) custom->redo(tool);
        break;
	case SessionHistoryType::CanvasEdit:
		tool.swapCanvasHistory(*this);
		break;

	case SessionHistoryType::LayerAdded:
		tool.restoreLayerFromHistory(*this);
		tool.selectedLayerId = selectedAfter;
		tool.nextLayerId = nextIdAfter;
		break;

	case SessionHistoryType::LayerDeleted:
		tool.takeHistoryLayer(*this);
		tool.selectedLayerId = selectedAfter;
		break;
	}
}

void SessionHistoryEntry::reset() {
    custom.reset();
	storedLayer.reset();
	storedExtension.reset();

	type = SessionHistoryType::CanvasEdit;
	layerId = 0;
	layerIndex = 0;
	selectedBefore = 0;
	selectedAfter = 0;
	nextIdBefore = 0;
	nextIdAfter = 0;
}

void PaintSession::undo() {
	if (strokeLayerId != 0) {
		return;
	}

    if (historyManager.canUndo()) modified = true;
	historyManager.undo(*this);
}

void PaintSession::redo() {
	if (strokeLayerId != 0) {
		return;
	}

    if (historyManager.canRedo()) modified = true;
	historyManager.redo(*this);
}

void PaintSession::deleteAll() {
	if (strokeLayerId != 0) {
		return;
	}

	paintcore::Layer & layer = getLayer(selectedLayerId);
    if (layer.locked) return;

    auto& entry = prepareHistory();
    entry.type = SessionHistoryType::CanvasEdit;
    entry.layerId = layer.id;
    copyFbo(layer.canvas, entry.image);

	layer.canvas.begin();
	ofClear(0, 0, 0, 0);
	layer.canvas.end();
    finishHistory();

	getExtension(layer.id).markDirty();
}

int PaintSession::addLayer(std::unique_ptr<LayerExtension> filter, bool recordHistory) {
    if (strokeLayerId != 0) return 0;
    if (!filter) filter = std::make_unique<LayerExtension>();

	// IDと表示名を決める
	const int id = nextLayerId;
	const std::string name = "Layer" + std::to_string(id);

	// レイヤー本体を作る
	auto layer = std::make_unique<paintcore::Layer>(
		id,
		name,
		paintSettings);

	filter->setup(canvasWidth, canvasHeight);
	filter->markDirty();

	// 現在選択中のレイヤーの上に新規レイヤーを作る
	// 選択中のレイヤーがないときは基本起動時につき最初のレイヤーを生成
	const int selectedIndex = findLayerIndex(selectedLayerId);
	const int insertIndex = selectedIndex >= 0 ? selectedIndex : 0;

	if (recordHistory) {
		SessionHistoryEntry & entry = prepareHistory();

		entry.type = SessionHistoryType::LayerAdded;
		entry.layerId = id;
		entry.layerIndex = insertIndex;
		entry.selectedBefore = selectedLayerId;
		entry.selectedAfter = id;
		entry.nextIdBefore = nextLayerId;
		entry.nextIdAfter = nextLayerId + 1;
	}

	extensions.emplace(id, std::move(filter));

    engine.insertLayerAt(insertIndex, std::move(layer));

	++nextLayerId;
	selectedLayerId = id;

	if (recordHistory) {
		finishHistory();
	} else {
        // An unrecorded structural edit must not leave old actions targeting reused IDs.
        clearHistory();
        modified = true;
    }

	return id;
}

int PaintSession::findLayerIndex(int layerId) const {
	return engine.findLayerIndex(layerId);
}

paintcore::Layer & PaintSession::getLayer(int layerId) {
	return engine.getLayer(layerId);
}

LayerExtension& PaintSession::getExtension(int layerId) {
	return *extensions.at(layerId);
}

void PaintSession::commitStroke() {
    if (!isDrawing()) return;
    if (!previewReady || previewDirty) engine.drawStroke(activeBrush);
	paintcore::Layer & layer = getLayer(strokeLayerId);

    auto& entry = prepareHistory();
    entry.type = SessionHistoryType::CanvasEdit;
    entry.layerId = layer.id;
    copyFbo(layer.canvas, entry.image);
	engine.commitStroke(layer.canvas);
    finishHistory();
	getExtension(layer.id).markDirty();

	strokeLayerId = 0;
}

namespace {
class LambdaAction : public HistoryAction<PaintSession> {
public:
    std::function<void(PaintSession&)> backward, forward;
    void undo(PaintSession& s) override { backward(s); }
    void redo(PaintSession& s) override { forward(s); }
};
template<class T> void changeProperty(PaintSession& s, int id, T Layer::* member, const T& value) {
    if (s.isDrawing() || s.findLayerIndex(id) < 0) return;
    auto& layer = s.getLayer(id);
    const T old = layer.*member;
    if (old == value) return;
    auto action = std::make_unique<LambdaAction>();
    action->backward = [id, member, old](PaintSession& tool) { tool.getLayer(id).*member = old; };
    action->forward = [id, member, value](PaintSession& tool) { tool.getLayer(id).*member = value; };
    layer.*member = value;
    s.recordAction(std::move(action));
}
}
void PaintSession::moveLayer(int id, int index, bool recordHistory) {
    if (isDrawing()) return;
    const int old = findLayerIndex(id);
    if (old < 0 || index < 0 || index >= static_cast<int>(engine.getLayers().size()) || old == index) return;
    auto action = std::make_unique<LambdaAction>();
    action->backward = [id, old](PaintSession& s) { s.engine.moveLayer(id, old); };
    action->forward = [id, index](PaintSession& s) { s.engine.moveLayer(id, index); };
    engine.moveLayer(id, index);
    modified = true;
    if (recordHistory) recordAction(std::move(action));
}
void PaintSession::renameLayer(int id, const std::string& name) { changeProperty(*this, id, &Layer::name, name); }
void PaintSession::setLayerVisible(int id, bool value) { changeProperty(*this, id, &Layer::visible, value); }
void PaintSession::setLayerLocked(int id, bool value) { changeProperty(*this, id, &Layer::locked, value); }
void PaintSession::setLayerOpacity(int id, float value) {
    if (std::isfinite(value)) changeProperty(*this, id, &Layer::opacity, ofClamp(value, 0.0f, 1.0f));
}
void PaintSession::setLayerPosition(int id, const glm::vec2& p) {
    if (std::isfinite(p.x) && std::isfinite(p.y)) changeProperty(*this, id, &Layer::position, p);
}

void PaintSession::deleteLayer(int layerId) {
	const auto & layers = engine.getLayers();

	// 描画中と、最後の1枚になった場合は削除しない
	if (strokeLayerId != 0 || layers.size() <= 1) {
		return;
	}

	const int layerCount = static_cast<int>(layers.size());
	const int deleteIndex = findLayerIndex(layerId);

	if (deleteIndex == -1) {
		return;
	}

	// 削除後に、どのレイヤーを選択するか決める
	int selectedAfter = selectedLayerId;

	if (selectedLayerId == layerId) {
		if (deleteIndex + 1 < layerCount) {
			selectedAfter = layers[deleteIndex + 1]->id;
		} else {
			selectedAfter = layers[deleteIndex - 1]->id;
		}
	}

	// 削除操作を履歴に記録する
	SessionHistoryEntry & entry = prepareHistory();

	entry.type = SessionHistoryType::LayerDeleted;
	entry.layerId = layerId;
	entry.selectedBefore = selectedLayerId;
	entry.selectedAfter = selectedAfter;

    // 削除時の位置を記録し、レイヤー本体を履歴へ預ける
	storeLayerHistory(entry, deleteIndex);

    selectedLayerId = selectedAfter;

	finishHistory();
}

// 作業用の履歴を準備する。Redoの破棄はfinishHistoryで確定した時だけ行う。
SessionHistoryEntry& PaintSession::prepareHistory() {
	return historyManager.prepare();
}

void PaintSession::finishHistory() {
	historyManager.finish();
    modified = true;
}

void PaintSession::clearHistory() {
	historyManager.clear();
}


bool PaintSession::selectLayer(int id) {
    if (isDrawing() || findLayerIndex(id) < 0) return false;
    selectedLayerId = id;
    return true;
}

bool PaintSession::pointerPressed(const glm::vec2& p) {
    if (!initialized || isDrawing() || p.x < 0 || p.y < 0 || p.x >= canvasWidth || p.y >= canvasHeight)
        return false;
    beginStroke(p);
    return isDrawing();
}
void PaintSession::pointerDragged(const glm::vec2& p) { addStrokePoint(p); }
void PaintSession::pointerReleased() { commitStroke(); }
void PaintSession::cancelStroke() {
    if (!isDrawing()) return;
    getExtension(strokeLayerId).markDirty();
    strokeLayerId = 0;
    previewReady = previewDirty = false;
}
void PaintSession::draw() {
    ofPushStyle();
    ofFill();
    ofEnableAlphaBlending();
    glBlendEquation(GL_FUNC_ADD);
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    ofSetColor(background);
    ofDrawRectangle(0, 0, canvasWidth, canvasHeight);
    drawCanvas(isDrawing());
    ofPopStyle();
}
void PaintSession::newDocument(std::unique_ptr<LayerExtension> extension) {
    if (!initialized) { setup(std::move(extension)); return; }
    // Allocate replacement before discarding the current document.
    auto layer = std::make_unique<Layer>(1, "Layer1", paintSettings);
    if (!extension) extension = std::make_unique<LayerExtension>();
    extension->setup(canvasWidth, canvasHeight);
    extension->markDirty();
    cancelStroke();
    clearHistory();
    while (!engine.getLayers().empty()) engine.takeLayerAt(0);
    extensions.clear();
    engine.insertLayerAt(0, std::move(layer));
    extensions.emplace(1, std::move(extension));
    selectedLayerId = 1;
    nextLayerId = 2;
    modified = true;
}
bool PaintSession::recordAction(std::unique_ptr<HistoryAction<PaintSession>> action) {
    if (!action || isDrawing()) return false;
    auto& entry = prepareHistory();
    entry.type = SessionHistoryType::Custom;
    entry.custom = std::move(action);
    finishHistory();
    return true;
}

} // namespace paintcore
