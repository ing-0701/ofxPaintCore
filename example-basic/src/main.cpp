#include "ofApp.h"
int main() {
    ofSetupOpenGL(1360, 850, OF_WINDOW);
    ofRunApp(std::make_shared<BasicPaintApp>());
}
