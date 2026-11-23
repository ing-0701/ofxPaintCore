#include "ofApp.h"
int main() {
    ofSetupOpenGL(1072, 850, OF_WINDOW);
    ofRunApp(std::make_shared<BasicPaintApp>());
}
