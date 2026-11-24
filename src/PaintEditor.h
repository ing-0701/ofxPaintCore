#pragma once

// Optional native UI. PaintSession has no dependency on this class or React.
#include "CanvasView.h"
#include "ColorPicker.h"
#include "PaintSession.h"
#include "ofxGui.h"

namespace paintcoreui {
class PaintEditor {
public:
	paintcore::PaintSession paint;
	paintcore::CanvasView view;
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

		size.setMin(minimumSize);
		size.setMax(maximumSize);
		size = ofClamp(initialSize, minimumSize, maximumSize);
	}

	void setup() {
		paint.setup();
		const float x = paint.canvasWidth + 8.0f;
		picker.setup(x, 0);
		panel.setup("Brush", "", x, 290);

		const auto & initialBrush = paint.getBrushSettings();

		picker.setColor(initialBrush.color);
		panel.add(pen.setup("Pen (off = eraser)", initialBrush.isPen));
		panel.add(size.setup("Size", initialBrush.size, 0.05f, 1.0f));
		panel.add(alpha.setup("Opacity", initialBrush.color.a / 255.0f, 0.01f, 1.0f));
		panel.add(angle.setup("Angle (deg)", initialBrush.angleDegrees, -180.0f, 180.0f));
		panel.add(randomAngle.setup("Random angle (+/- deg)", initialBrush.randomAngleDegrees, 0.0f, 180.0f));
		panel.add(spacing.setup("Spacing (px)", initialBrush.spacingPx, 1.0f, 400.0f));
		panel.add(background.setup("Background", ofColor::white, ofColor(0, 0), ofColor(255, 255)));
		panel.add(undo.setup("Undo"));
		panel.add(redo.setup("Redo"));
		panel.add(clear.setup("Clear selected layer"));
		panel.add(add.setup("Add layer"));
		panel.add(remove.setup("Delete selected layer"));
		panel.add(up.setup("Layer up"));
		panel.add(down.setup("Layer down"));
		undo.addListener(this, &PaintEditor::onUndo);
		redo.addListener(this, &PaintEditor::onRedo);
		clear.addListener(this, &PaintEditor::onClear);
		add.addListener(this, &PaintEditor::onAdd);
		remove.addListener(this, &PaintEditor::onRemove);
		up.addListener(this, &PaintEditor::onUp);
		down.addListener(this, &PaintEditor::onDown);
		ready = true;
	}
	~PaintEditor() {
		if (!ready) return;
		undo.removeListener(this, &PaintEditor::onUndo);
		redo.removeListener(this, &PaintEditor::onRedo);
		clear.removeListener(this, &PaintEditor::onClear);
		add.removeListener(this, &PaintEditor::onAdd);
		remove.removeListener(this, &PaintEditor::onRemove);
		up.removeListener(this, &PaintEditor::onUp);
		down.removeListener(this, &PaintEditor::onDown);
	}
	void draw() {
		picker.update();
		paint.setBackground(background);
		view.begin();
		paint.draw();
		view.end();
		picker.drawPicker();
		picker.drawSelection();
		picker.drawHueSlider();
		panel.draw();
		ofPushStyle();
		const auto & layers = paint.engine.getLayers();
		for (int i = 0; i < static_cast<int>(layers.size()) && i < visibleRows; ++i) {
			const int index = firstRow + i;
			if (index >= static_cast<int>(layers.size())) break;
			const auto & layer = *layers[index];
			const auto rect = row(i);
			ofFill();
			ofSetColor(layer.id == paint.getSelectedLayerId() ? ofColor(70, 120, 180) : ofColor(60));
			ofDrawRectangle(rect);
			ofSetColor(255);
			ofDrawBitmapString(layer.name, rect.x + 6, rect.y + 17);
		}
		ofPopStyle();
	}
	void mousePressed(int x, int y, int button) {
		if (button != OF_MOUSE_BUTTON_LEFT || paint.isDrawing()) return;
		picking = picker.contains(x, y);
		if (picking) {
			picker.select(x, y);
			return;
		}
		const auto & layers = paint.engine.getLayers();
		for (int i = 0; i < visibleRows && firstRow + i < static_cast<int>(layers.size()); ++i)
			if (row(i).inside(x, y)) {
				paint.selectLayer(layers[firstRow + i]->id);
				return;
			}
		if (x >= paint.canvasWidth) return; // UI owns this region.
		auto settings = paint.getBrushSettings();

		settings.size = size;

		settings.color = picker.getColor();
		settings.color.a = static_cast<unsigned char>(
			alpha * 255.0f);

		settings.isPen = pen;
		settings.eraserAlpha = alpha * 255.0f;

		settings.angleDegrees = angle;
		settings.randomAngleDegrees = randomAngle;
		settings.spacingPx = spacing;

		paint.setBrushSettings(settings);
		paint.pointerPressed(view.toCanvas({ x, y }));
	}
	void mouseDragged(int x, int y, int button) {
		if (button != OF_MOUSE_BUTTON_LEFT) return;
		if (picking)
			picker.select(x, y);
		else
			paint.pointerDragged(view.toCanvas({ x, y }));
	}
	void mouseReleased(int, int, int button) {
		if (button != OF_MOUSE_BUTTON_LEFT) return;
		picking = false;
		paint.pointerReleased();
	}
	void mouseScrolled(float amount) {
		const int maxRow = std::max(0, static_cast<int>(paint.engine.getLayers().size()) - visibleRows);
		firstRow = std::max(0, std::min(maxRow, firstRow - static_cast<int>(amount)));
	}
	void keyPressed(int key) {
		if (!ofGetKeyPressed(OF_KEY_CONTROL)) return;
		if (key == 'z' || key == 26) paint.undo();
		if (key == 'y' || key == 25) paint.redo();
	}

private:
	bool ready = false, picking = false;
	int firstRow = 0;
	static constexpr int visibleRows = 5;
	ColorPicker picker;
	ofxPanel panel;
	ofxToggle pen;
	ofxFloatSlider size, alpha;
	ofxFloatSlider angle;
	ofxFloatSlider randomAngle;
	ofxFloatSlider spacing;
	ofxColorSlider background;
	ofxButton undo, redo, clear, add, remove, up, down;
	ofRectangle row(int i) const { return { paint.canvasWidth + 8.0f, panel.getPosition().y + panel.getHeight() + 4 + i * 24.0f, 256, 24 }; }
	void onUndo() { paint.undo(); }
	void onRedo() { paint.redo(); }
	void onClear() { paint.deleteAll(); }
	void onAdd() {
		paint.addLayer();
		firstRow = 0;
	}
	void onRemove() {
		paint.deleteLayer(paint.getSelectedLayerId());
		mouseScrolled(0);
	}
	void onUp() { paint.moveLayer(paint.getSelectedLayerId(), paint.findLayerIndex(paint.getSelectedLayerId()) - 1); }
	void onDown() { paint.moveLayer(paint.getSelectedLayerId(), paint.findLayerIndex(paint.getSelectedLayerId()) + 1); }
};
}
