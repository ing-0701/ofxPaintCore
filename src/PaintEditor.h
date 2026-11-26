#pragma once

// Optional native UI. PaintSession has no dependency on this class or React.
#include "BrushControls.h"
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

		brushControls.penSize.setMin(minimumSize);
		brushControls.penSize.setMax(maximumSize);
		brushControls.penSize = ofClamp(initialSize, minimumSize, maximumSize);
	}

	void setup() {
		paint.setup();

		const float x = paint.canvasWidth + 8.0f;
		const float buttonY = 312.0f;
		const float pickerY = buttonY + 34.0f;

		picker.setup(x, 20);

		backgroundButton.setup("Background");
		backgroundButton.setSize(backgroundControlWidth, 30.0f);
		backgroundButton.setPosition(x, buttonY);

		panel.setup("Brush", "", x, 484);

		const auto & initialBrush = paint.getBrushSettings();

		picker.setColor(initialBrush.color);
		panel.add(pen.setup("Pen (off = eraser)", initialBrush.isPen));
		brushControls.setup(
			x,
			484.0f,
			256.0f,
			initialBrush,
			0.1f,
			8.0f,
			0.0f,
			1.0f);

		panel.add(&brushControls.penSize);
		panel.add(&brushControls.penAlpha);
		panel.add(&brushControls.eraserSize);
		panel.add(&brushControls.eraserAlpha);
		panel.add(undo.setup("Undo"));
		panel.add(redo.setup("Redo"));
		panel.add(clear.setup("Clear selected layer"));
		panel.add(add.setup("Add layer"));
		panel.add(remove.setup("Delete selected layer"));
		clear.addListener(this, &PaintEditor::onClear);
		add.addListener(this, &PaintEditor::onAdd);
		remove.addListener(this, &PaintEditor::onRemove);
		backgroundButton.addListener(this, &PaintEditor::toggleBackgroundPicker);

		backgroundPicker.setup(x, pickerY, backgroundControlWidth);
		backgroundPicker.setColor(paint.getBackground());
		backgroundPicker.setEnabled(false);

		ready = true;
	}
	~PaintEditor() {
		if (!ready) return;
		clear.removeListener(this, &PaintEditor::onClear);
		add.removeListener(this, &PaintEditor::onAdd);
		remove.removeListener(this, &PaintEditor::onRemove);
		backgroundButton.removeListener(this, &PaintEditor::toggleBackgroundPicker);
	}
	void draw() {
		picker.update();
		backgroundPicker.update();

		paint.setBackground(backgroundPicker.getColor());

		view.begin();
		paint.draw();
		view.end();

		ofPushStyle();
		ofEnableAlphaBlending();
		ofSetColor(255);

		const float pickerX = paint.canvasWidth + 8.0f;

		ofDrawBitmapString("Brush color", pickerX, 14);
		ofDrawBitmapString("Background color", pickerX + 280.0f, 14);

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

		ofPopStyle();
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
		for (int i = 0; i < visibleRows && firstRow + i < static_cast<int>(layers.size()); ++i)
			if (row(i).inside(x, y)) {
				paint.selectLayer(layers[firstRow + i]->id);
				return;
			}
		if (x >= paint.canvasWidth) return; // UI owns this region.
		auto settings = paint.getBrushSettings();

		settings.color = picker.getColor();
		settings.isPen = pen;

		brushControls.applyTo(settings);

		paint.setBrushSettings(settings);
		paint.pointerPressed(view.toCanvas({ x, y }));
	}
	void mouseDragged(int x, int y, int button) {
		if (button != OF_MOUSE_BUTTON_LEFT) {
			return;
		}

		if (picking) {
			picker.select(x, y);
		} else if (pickingBackground) {
			backgroundPicker.select(x, y);
		} else {
			paint.pointerDragged(view.toCanvas({ x, y }));
		}
	}
	void mouseReleased(int, int, int button) {
		if (button != OF_MOUSE_BUTTON_LEFT) {
			return;
		}

		picking = false;
		pickingBackground = false;

		paint.pointerReleased();
	}
	void mouseScrolled(float amount) {
		const int maxRow = std::max(0, static_cast<int>(paint.engine.getLayers().size()) - visibleRows);
		firstRow = std::max(0, std::min(maxRow, firstRow - static_cast<int>(amount)));
	}
	void keyPressed(int key) {
		if (!ofGetKeyPressed(OF_KEY_CONTROL)) return;

		const bool isZ = key == 'z' || key == 'Z' || key == 26;
		if (!isZ) return;

		if (ofGetKeyPressed(OF_KEY_SHIFT)) {
			paint.redo();
		} else {
			paint.undo();
		}
	}

private:
	bool ready = false;
	bool picking = false;
	bool pickingBackground = false;
	int firstRow = 0;
	static constexpr int visibleRows = 5;
	bool backgroundPickerVisible = false;
	float backgroundControlWidth = 100.0f;
	ofxButton backgroundButton;
	ColorPicker picker;
	ColorPicker backgroundPicker;
	ofxPanel panel;
	ofxToggle pen;
	BrushControls brushControls;
	ofxButton undo, redo, clear, add, remove;
	ofRectangle row(int i) const { return { paint.canvasWidth + 8.0f, panel.getPosition().y + panel.getHeight() + 4 + i * 24.0f, 256, 24 }; }

	void onClear() { paint.deleteAll(); }
	void onAdd() {
		paint.addLayer();
		firstRow = 0;
	}
	void onRemove() {
		paint.deleteLayer(paint.getSelectedLayerId());
		mouseScrolled(0);
	}

	void toggleBackgroundPicker() {
		backgroundPickerVisible = !backgroundPickerVisible;
		backgroundPicker.setEnabled(backgroundPickerVisible);
	}
};
}
