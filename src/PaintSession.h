#pragma once


#include "ofMain.h"
#include "LayerExtension.h"
#include "PaintLayer.h"
#include "PaintEngine.h"
#include "HistoryAction.h"
#include "HistoryManager.h"
#include "PaintSettings.h"

#include <memory>
#include <vector>
#include <unordered_map>

namespace paintcore {

enum class SessionHistoryType {
	CanvasEdit,
	LayerAdded,
	LayerDeleted,
    Custom
};

class PaintSession;

struct SessionHistoryEntry : public paintcore::HistoryAction<PaintSession> {
	void undo(PaintSession & tool) override;
	void redo(PaintSession & tool) override;
	void reset();

	SessionHistoryType type = SessionHistoryType::CanvasEdit;
	int layerId = 0;
	ofFbo image;

	// UNDOで取り除いたレイヤーを預かる
	std::unique_ptr<paintcore::Layer> storedLayer;
	std::unique_ptr<LayerExtension> storedExtension;

	// 配列内の位置
	int layerIndex = 0;

	// 操作前後の選択状態
	int selectedBefore = 0;
	int selectedAfter = 0;

	// 操作前後の、次に発行するレイヤー番号
	int nextIdBefore = 0;
	int nextIdAfter = 0;
    std::unique_ptr<paintcore::HistoryAction<PaintSession>> custom;
};

class PaintSession {
public:


	static const int UNDO_CAPACITY = 20;

	explicit PaintSession(
		const paintcore::PaintSettings & settings = paintcore::PaintSettings {});
	PaintSession(int mainWidth, int mainHeight);

	void setup(std::unique_ptr<LayerExtension> initialExtension = {});
	virtual ~PaintSession() = default;

	void drawCanvas(bool drawingFlag);
	void undo();
	void redo();
	void deleteAll();


	int canvasWidth = 0;
	int canvasHeight = 0;
    int getSelectedLayerId() const { return selectedLayerId; }
    bool selectLayer(int id);
    bool isDrawing() const { return strokeLayerId != 0; }
    bool canUndo() const { return !isDrawing() && historyManager.canUndo(); }
    bool canRedo() const { return !isDrawing() && historyManager.canRedo(); }
    const paintcore::PaintSettings& getSettings() const { return paintSettings; }

    // High-level API: set a brush, press, drag, release. No separate preview call required.
    void setBrushSettings(const paintcore::BrushSettings& settings) { brushSettings = settings; }
    const paintcore::BrushSettings& getBrushSettings() const { return brushSettings; }
    bool pointerPressed(const glm::vec2& canvasPosition);
    void pointerDragged(const glm::vec2& canvasPosition);
    void pointerReleased();
    void cancelStroke();
    void draw();
    // A presentation-only placement (e.g. head-tracked parallax) does not enter history.
    void setLayerPlacement(CanvasRenderer::LayerPlacement value) { layerPlacement = std::move(value); }
    void setBackground(const ofColor& value) { if (background != value) { background = value; modified = true; } }
    const ofColor& getBackground() const { return background; }
    void newDocument(std::unique_ptr<LayerExtension> initialExtension = {});
    // Record an app-specific action AFTER the app successfully applied it.
    bool recordAction(std::unique_ptr<paintcore::HistoryAction<PaintSession>> action);
    using ExtensionFactory = std::function<std::unique_ptr<LayerExtension>(const ofJson&)>;
    bool saveProject(const std::string& path);
    bool loadProject(const std::string& path, const ExtensionFactory& factory = {});
    bool exportPng(const std::string& path, bool transparent = true);
    bool hasUnsavedChanges() const { return modified; }
    void markDocumentChanged() { modified = true; }

	// レイヤーIDから現在の配列位置を返す。見つからなければ-1。
	int findLayerIndex(int layerId) const;

	// IDからレイヤー本体やフィルターを取得する
	paintcore::Layer & getLayer(int layerId);
	LayerExtension & getExtension(int layerId);
	// 描画途中の画像を、対象レイヤーへ確定する
	void commitStroke();

	// レイヤーを追加し、IDを返す
	int addLayer(std::unique_ptr<LayerExtension> extension = {}, bool recordHistory = true);

	// 指定したレイヤーを、移動後の配列位置へ移す
	void moveLayer(int layerId, int destinationIndex, bool recordHistory = true);
    void renameLayer(int layerId, const std::string& name);
    void setLayerVisible(int layerId, bool visible);
    void setLayerLocked(int layerId, bool locked);
    void setLayerOpacity(int layerId, float opacity);
    void setLayerPosition(int layerId, const glm::vec2& position);

	void deleteLayer(int layerId);

	SessionHistoryEntry & prepareHistory();
	void finishHistory();
	void clearHistory();



	void beginStroke(const glm::vec2 & position);
	void addStrokePoint(const glm::vec2 & position);

	paintcore::PaintEngine engine;


private:
    ofFbo historySwapFbo;
    int selectedLayerId = 0;
    int nextLayerId = 1;
    int strokeLayerId = 0;
    bool initialized = false;
    bool modified = false;
    bool previewDirty = false;
    bool previewReady = false;
    paintcore::BrushSettings brushSettings { 0.1f, ofColor::black, true, 255.0f };
    paintcore::BrushSettings activeBrush = brushSettings;
    ofColor background = ofColor::white;
    ofFbo opacityBuffer;
    CanvasRenderer::LayerPlacement layerPlacement;
	void storeLayerHistory(SessionHistoryEntry & entry, int layerIndex);
	void restoreLayerFromHistory(SessionHistoryEntry & entry);
	// idというキーを使ってフィルターを検索するための連想配列
	std::unordered_map<int, std::unique_ptr<LayerExtension>> extensions;

	// SessionHistoryEntryに対して、このクラスのプライベートメンバ変数にアクセスを許可
	friend struct SessionHistoryEntry;

	void swapCanvasHistory(SessionHistoryEntry & entry);
	void takeHistoryLayer(SessionHistoryEntry & entry);
	paintcore::HistoryManager<SessionHistoryEntry, UNDO_CAPACITY> historyManager;

	paintcore::PaintSettings paintSettings;
};

} // namespace paintcore
