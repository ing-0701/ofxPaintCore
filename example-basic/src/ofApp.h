#pragma once
#include "ofMain.h"
#include "PaintEditor.h"

class BasicPaintApp : public ofBaseApp {
public:
    paintcoreui::PaintEditor editor;
    void setup() override { ofLogToConsole(); editor.setup(); }
    void draw() override { ofBackground(40); editor.draw(); }
    void mousePressed(int x, int y, int b) override { editor.mousePressed(x,y,b); }
    void mouseDragged(int x, int y, int b) override { editor.mouseDragged(x,y,b); }
    void mouseReleased(int x, int y, int b) override { editor.mouseReleased(x,y,b); }
    void mouseScrolled(int x, int y, float sx, float sy) override {
        if (x >= editor.paint.canvasWidth) editor.mouseScrolled(sy);
    }
    void keyPressed(int key) override { editor.keyPressed(key); }
};
