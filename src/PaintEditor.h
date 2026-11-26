#pragma once

// Optional native UI. PaintSession has no dependency on this class or React.
#include "BrushControls.h"
#include "ColorPicker.h"
#include "PaintSession.h"
#include "ofxGui.h"
#include "LayerList.h"
#include <algorithm>

namespace paintcoreui {

struct PaintEditorLayout {
	glm::vec2 colorPickerPosition { 0.0f, 0.0f };
	glm::vec2 brushPanelPosition { 0.0f, 0.0f };
	glm::vec2 backgroundButtonPosition { 0.0f, 0.0f };

	// レイヤー欄全体の左上。
	// この位置に追加ボタン、その下に削除ボタンと一覧を配置する。
	glm::vec2 layerAreaPosition { 0.0f, 0.0f };
	int visibleLayerRows = 5;
	float backgroundWidth = 100.0f;
};

class PaintEditor {
public:
	paintcore::PaintSession paint;
	explicit PaintEditor(const paintcore::PaintSettings & settings = {})
		: paint(settings) { }
	PaintEditor(const PaintEditor &) = delete;
	PaintEditor & operator=(const PaintEditor &) = delete;

	// setup()の後に呼ぶ。
	// sizeの単位は従来どおりで、直径はsize * 50px。
	void setBrushSizeRange(
		float initialSize,
		float minimumSize,
		float maximumSize) {

		brushControls.penSize.setMin(minimumSize);
		brushControls.penSize.setMax(maximumSize);
		brushControls.penSize = ofClamp(initialSize, minimumSize, maximumSize);
	}

	void setup() {
		const float x = paint.canvasWidth + 8.0f;

		PaintEditorLayout defaultLayout;
		defaultLayout.colorPickerPosition = { x, 20.0f };
		defaultLayout.brushPanelPosition = { x, 484.0f };
		defaultLayout.backgroundButtonPosition = { x, 312.0f };
		defaultLayout.layerAreaPosition = {
			x + controlWidth + 16.0f,
			20.0f
		};

		setup(defaultLayout);
	}

	void setup(const PaintEditorLayout & initialLayout) {
		// setupは初期化時に一度だけ行う。
		if (ready) {
			return;
		}

		layout = initialLayout;
		visibleRows = std::max(1, layout.visibleLayerRows);
		backgroundControlWidth = std::max(1.0f, layout.backgroundWidth);
		paint.setup();

		const auto & initialBrush = paint.getBrushSettings();

		// ブラシ色
		picker.setup(
			layout.colorPickerPosition.x,
			layout.colorPickerPosition.y,
			controlWidth);
		picker.setColor(initialBrush.color);

		// 背景色
		backgroundButton.setup("Background");
		backgroundButton.setSize(backgroundControlWidth, 30.0f);
		backgroundButton.setPosition(
			layout.backgroundButtonPosition.x,
			layout.backgroundButtonPosition.y);

		backgroundPicker.setup(
			layout.backgroundButtonPosition.x,
			layout.backgroundButtonPosition.y + 34.0f,
			backgroundControlWidth);
		backgroundPicker.setColor(paint.getBackground());

		backgroundPickerVisible = false;
		backgroundPicker.setEnabled(false);

		// Brushパネル内の部品
		panel.setup(
			"Brush", "",
			layout.brushPanelPosition.x,
			layout.brushPanelPosition.y);

		pen.setup("Pen (off = eraser)", initialBrush.isPen);

		brushControls.setup(
			layout.brushPanelPosition.x,
			layout.brushPanelPosition.y,
			controlWidth,
			initialBrush,
			0.1f,
			8.0f,
			0.0f,
			1.0f);

		undo.setup("Undo");
		redo.setup("Redo");
		clear.setup("Clear selected layer");

		rebuildBrushPanel();

		// レイヤー操作ボタンはBrushパネルへ追加しない。
		const float layerX = layout.layerAreaPosition.x;
		const float layerY = layout.layerAreaPosition.y;

		add.setup("Add layer");
		add.setSize(controlWidth, 30.0f);
		add.setPosition(layerX, layerY);

		remove.setup("Delete selected layer");
		remove.setSize(controlWidth, 30.0f);
		remove.setPosition(layerX, layerY + 34.0f);

		layerList.setup(
			layerX,
			layerY + 68.0f,
			controlWidth,
			24.0f,
			visibleRows);

		// イベント登録
		clear.addListener(this, &PaintEditor::onClear);
		undo.addListener(this, &PaintEditor::onUndo);
		redo.addListener(this, &PaintEditor::onRedo);
		add.addListener(this, &PaintEditor::onAdd);
		remove.addListener(this, &PaintEditor::onRemove);
		backgroundButton.addListener(
			this, &PaintEditor::toggleBackgroundPicker);

		ready = true;
	}
	~PaintEditor() {
		if (!ready) return;
		clear.removeListener(this, &PaintEditor::onClear);
		add.removeListener(this, &PaintEditor::onAdd);
		remove.removeListener(this, &PaintEditor::onRemove);
		backgroundButton.removeListener(this, &PaintEditor::toggleBackgroundPicker);
		undo.removeListener(this, &PaintEditor::onUndo);
		redo.removeListener(this, &PaintEditor::onRedo);
	}
	void draw() {
		if (panelShowsPen != static_cast<bool>(pen)) {
			rebuildBrushPanel();
		}
		picker.update();
		backgroundPicker.update();

		paint.setBackground(backgroundPicker.getColor());

		paint.draw();

		ofPushStyle();
		ofEnableAlphaBlending();
		ofSetColor(255);

		ofDrawBitmapString(
			"Brush color",
			layout.colorPickerPosition.x,
			layout.colorPickerPosition.y - 6.0f);

		picker.drawPicker();
		picker.drawSelection();
		picker.drawHueSlider();

		backgroundButton.draw();
		if (backgroundPickerVisible) {
			backgroundPicker.drawPicker();
			backgroundPicker.drawSelection();
			backgroundPicker.drawHueSlider();
		}

		panel.draw();
		add.draw();
		remove.draw();

		ofPopStyle();

		const auto & layers = paint.engine.getLayers();
		const int layerCount = static_cast<int>(layers.size());

		// 削除やUndo／Redoによって枚数が変わっても、表示位置を有効範囲に収める。
		layerList.clampScroll(layerCount);

		ofPushStyle();
		ofFill();

		// 行がない部分も含めた一覧の背景。
		ofSetColor(40);
		ofDrawRectangle(layerList.bounds());

		for (int i = 0; i < visibleRows; ++i) {
			const int index = layerList.firstVisibleIndex() + i;
			if (index >= layerCount) {
				break;
			}

			const auto & layer = *layers[index];
			const auto rect = layerList.rowRect(i);
			const bool selected = layer.id == paint.getSelectedLayerId();

			ofSetColor(
				selected ? ofColor(70, 120, 180) : ofColor(60));
			ofDrawRectangle(rect);

			ofSetColor(255);
			ofDrawBitmapString(
				layer.name,
				rect.x + 6.0f,
				rect.y + 17.0f);
		}

		ofPopStyle();
	}
	void mousePressed(int x, int y, int button) {
		if (button != OF_MOUSE_BUTTON_LEFT || paint.isDrawing()) return;

		picking = picker.contains(x, y);
		pickingBackground = backgroundPickerVisible && backgroundPicker.contains(x, y);

		if (picking) {
			picker.select(x, y);
			return;
		}

		if (pickingBackground) {
			backgroundPicker.select(x, y);
			return;
		}
		const auto & layers = paint.engine.getLayers();
		const int layerCount = static_cast<int>(layers.size());

		layerList.clampScroll(layerCount);
		layerList.cancelDrag();
		draggingLayer = false;

		if (layerList.bounds().inside(x, y)) {
			for (int i = 0; i < visibleRows; ++i) {
				const int index = layerList.firstVisibleIndex() + i;
				if (index >= layerCount) {
					break;
				}

				if (!layerList.rowRect(i).inside(x, y)) {
					continue;
				}

				const int layerId = layers[index]->id;

				if (paint.selectLayer(layerId)) {
					layerList.beginDrag(layerId);
					draggingLayer = true;
				}

				break;
			}

			// 一覧内の空白を押した場合も、描画処理には渡さない。
			return;
		}
		if (containsUi(
				static_cast<float>(x),
				static_cast<float>(y))) {
			return;
		}
		auto settings = paint.getBrushSettings();

		settings.color = picker.getColor();
		settings.isPen = pen;

		brushControls.applyTo(settings);

		paint.setBrushSettings(settings);
		paint.pointerPressed(
			glm::vec2(static_cast<float>(x), static_cast<float>(y)));
	}
	void mouseDragged(int x, int y, int button) {
		if (button != OF_MOUSE_BUTTON_LEFT) {
			return;
		}

		if (draggingLayer) {
			return;
		}

		if (picking) {
			picker.select(x, y);
		} else if (pickingBackground) {
			backgroundPicker.select(x, y);
		} else {
			paint.pointerDragged(
				glm::vec2(static_cast<float>(x), static_cast<float>(y)));
		}
	}
	void mouseReleased(int x, int y, int button) {
		if (button != OF_MOUSE_BUTTON_LEFT) {
			return;
		}

		picking = false;
		pickingBackground = false;

		if (draggingLayer) {
			const int layerCount = static_cast<int>(paint.engine.getLayers().size());

			const auto request = layerList.endDrag(x, y, layerCount);

			draggingLayer = false;

			if (request.layerId != 0
				&& request.destinationIndex >= 0) {
				paint.moveLayer(
					request.layerId,
					request.destinationIndex);
			}

			return;
		}

		paint.pointerReleased();
	}
	void mouseScrolled(float amount) {
		if (paint.isDrawing() || draggingLayer) {
			return;
		}

		if (!layerList.bounds().inside(
				ofGetMouseX(), ofGetMouseY())) {
			return;
		}

		layerList.scroll(
			amount,
			static_cast<int>(paint.engine.getLayers().size()));
	}
	void keyPressed(int key) {
		if (!ofGetKeyPressed(OF_KEY_CONTROL)) {
			return;
		}

		const bool isZ = key == 'z' || key == 'Z' || key == 26;

		if (!isZ) {
			return;
		}

		if (ofGetKeyPressed(OF_KEY_SHIFT)) {
			onRedo();
		} else {
			onUndo();
		}
	}

private:
	bool ready = false;
	PaintEditorLayout layout;

	bool panelShowsPen = true;
	static constexpr float controlWidth = 256.0f;

	bool picking = false;
	bool pickingBackground = false;
	LayerList layerList;
	bool draggingLayer = false;

	int visibleRows = 5;
	bool backgroundPickerVisible = false;
	float backgroundControlWidth = 100.0f;
	ofxButton backgroundButton;
	ColorPicker picker;
	ColorPicker backgroundPicker;
	ofxPanel panel;
	ofxToggle pen;
	BrushControls brushControls;
	ofxButton undo, redo, clear, add, remove;

	bool containsUi(float x, float y) const {
		if (picker.bounds().inside(x, y)) {
			return true;
		}

		if (panel.getShape().inside(x, y)
			|| backgroundButton.getShape().inside(x, y)
			|| add.getShape().inside(x, y)
			|| remove.getShape().inside(x, y)
			|| layerList.bounds().inside(x, y)) {
			return true;
		}

		if (backgroundPickerVisible
			&& backgroundPicker.bounds().inside(x, y)) {
			return true;
		}

		return false;
	}

	void rebuildBrushPanel() {
		// 部品の値を保持したまま、パネルへの登録だけを組み直す。
		panel.clear();
		panel.setName("Brush");

		// 非表示のスライダーが単独でマウス入力を受けないようにする。
		brushControls.penSize.unregisterMouseEvents();
		brushControls.penAlpha.unregisterMouseEvents();
		brushControls.eraserSize.unregisterMouseEvents();
		brushControls.eraserAlpha.unregisterMouseEvents();

		panel.add(&pen);

		panelShowsPen = static_cast<bool>(pen);

		if (panelShowsPen) {
			panel.add(&brushControls.penSize);
			panel.add(&brushControls.penAlpha);
		} else {
			panel.add(&brushControls.eraserSize);
			panel.add(&brushControls.eraserAlpha);
		}

		panel.add(&undo);
		panel.add(&redo);
		panel.add(&clear);

		panel.setWidthElements(controlWidth);
	}

	void onUndo() {
		if (draggingLayer || !paint.canUndo()) {
			return;
		}

		paint.undo();

		layerList.clampScroll(
			static_cast<int>(paint.engine.getLayers().size()));
	}

	void onRedo() {
		if (draggingLayer || !paint.canRedo()) {
			return;
		}

		paint.redo();

		layerList.clampScroll(
			static_cast<int>(paint.engine.getLayers().size()));
	}
	void onClear() { paint.deleteAll(); }
	void onAdd() {
		if (paint.isDrawing()) {
			return;
		}

		layerList.cancelDrag();
		draggingLayer = false;

		paint.addLayer();

		layerList.clampScroll(
			static_cast<int>(paint.engine.getLayers().size()));
	}

	void onRemove() {
		if (paint.isDrawing()) {
			return;
		}

		layerList.cancelDrag();
		draggingLayer = false;

		paint.deleteLayer(paint.getSelectedLayerId());

		layerList.clampScroll(
			static_cast<int>(paint.engine.getLayers().size()));
	}

	void toggleBackgroundPicker() {
		backgroundPickerVisible = !backgroundPickerVisible;
		backgroundPicker.setEnabled(backgroundPickerVisible);

		if (!backgroundPickerVisible) {
			pickingBackground = false;
		}
	}
};
}
