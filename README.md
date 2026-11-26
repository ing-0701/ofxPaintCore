# ofxPaintCore

openFrameworksで複数のペイントアプリを作るための共通基盤です。通常の利用では `PaintSession` がレイヤー、描画途中の状態、Undo/Redoをまとめて扱います。標準のWindows UIも必要なら `paintcoreui::PaintEditor` を使用できます。

## 最小の接続

```cpp
#include "PaintSession.h"

paintcore::PaintSession paint;

void ofApp::setup() {
    ofLogToConsole();
    paint.setup();
}
void ofApp::draw() { paint.draw(); }
void ofApp::mousePressed(int x, int y, int button) {
    if (button == OF_MOUSE_BUTTON_LEFT) paint.pointerPressed({x, y});
}
void ofApp::mouseDragged(int x, int y, int button) {
    if (button == OF_MOUSE_BUTTON_LEFT) paint.pointerDragged({x, y});
}
void ofApp::mouseReleased(int, int, int button) {
    if (button == OF_MOUSE_BUTTON_LEFT) paint.pointerReleased();
}
```

`paint` はofAppのメンバにできます。GPU上の画像はコンストラクタでは確保せず、OpenGLコンテキストの作成後に呼ばれる `setup()` で準備します。標準UI付きの接続例は `example-basic/src/ofApp.h` にあります。

既定値は800×600、RGBA32F、履歴20件、スタンプ間隔3px、途中保存10点です。変更する項目だけ設定できます。

```cpp
paintcore::PaintSettings settings;
settings.width = 1024;
settings.height = 768;
settings.historyCapacity = 10;
paintcore::PaintSession paint(settings);
// OpenGL初期化後に paint.setup();
```

履歴件数は画像メモリ使用量にも影響します。RGBA32Fの800×600画像1枚は約7.68MBです。履歴20枚のほかに作業中の履歴1枚、交換用1枚、ストローク用2枚、各レイヤーの画像があります。削除履歴が保持するレイヤーやアプリ独自の画像は別途必要です。GPUメモリの総量を自動制限する機能はありません。

## 主な窓口

| 操作 | API |
|---|---|
| ブラシ設定 | `setBrushSettings(settings)` |
| 描画開始・継続・確定 | `pointerPressed(p)` / `pointerDragged(p)` / `pointerReleased()` |
| レイヤー追加・選択・削除 | `addLayer()` / `selectLayer(id)` / `deleteLayer(id)` |
| 並べ替え | `moveLayer(id, index)`。indexは移動後の位置 |
| 履歴 | `undo()` / `redo()` / `canUndo()` / `canRedo()` |
| 選択中レイヤーを空にする | `deleteAll()` |
| 背景と表示 | `setBackground(color)` / `draw()` |

初期レイヤーは1枚です。追加位置は選択中レイヤーの直前で、新規レイヤーが選択されます。配列の先頭が画面の手前です。最後の1枚は削除しません。描画中のUndo、レイヤー切替、追加、削除は受け付けません。

`pointerReleased()` はリリース位置を受け取りません。最後のドラッグまでに届いた点でプレビューを更新して確定します。描画が始まってから `draw()` が一度も呼ばれていなくても確定できます。ブラシ設定は描画開始時に固定されます。

`PaintEngine`、`Brush`、`StrokeSampler`、`CanvasRenderer` は個別にも利用できます。ただし、`PaintSession::engine` を直接変更すると、選択ID・履歴・拡張オブジェクトの管理を迂回します。通常の編集は上表のAPIを使ってください。

## アプリ独自の描画

`Layer` 自身はフィルターを所有しません。`PaintSession` がレイヤーIDと `LayerExtension` を対応させます。アプリは必要なら `LayerExtension` を継承し、画像の変化を受け取る `markDirty()` と表示を行う `draw(source, time)` を実装します。

```cpp
int id = paint.addLayer(std::make_unique<MyLayerExtension>());
```

レイヤーを削除すると、レイヤーと拡張オブジェクトの両方を履歴が所有します。Undoでは同じオブジェクトを戻します。独自パラメータやキャッシュの所有権をアプリ側で再構成する必要はありません。

レイヤー位置や不透明度はPaintCoreのレイヤー属性としては管理しません。`PaintEditor` はキャンバスを画面左上の原点に等倍で表示し、マウス座標をそのままキャンバス座標として扱います。ズーム・パンの機能は提供しません。

## ブラシの差し替え

通常の円ブラシは従来と同じ経路です。`size` の単位は半径そのものではなく、半径 = `size * 25` pxです。

`paint.engine.brush.setStamp(callback)` は中心と半径を受け取る従来の差し替えです。`setDetailedStamp(callback)` は `StampInfo` を受け取ります。`strokeId` と `index` の組で、同じストローク内で同じ座標に重なったスタンプも区別できます。`position`、`radius`、`angleDegrees` も渡されます。

角度はストロークIDとスタンプ番号から決まるため、プレビューの再描画や途中保存で変わりません。コールバックは同じスタンプに何度も呼ばれる場合があります。外部に配置情報を蓄積するならIDの組で重複を除いてください。

画像ブラシは `setImageStamp(std::shared_ptr<ofImage>)` で設定します。画像は透過PNGなどの通常の非乗算済みアルファを想定します。縦横比を保持し、幅をブラシの直径に合わせて描画します。高さは元画像の縦横比から計算します。画像の寿命はshared_ptrで保持します。FBOから取得した乗算済み画像をそのまま同じ画像ブラシへ渡す用途とは区別してください。

## 標準UIと導入

`PaintEditor` はofxGuiを使う任意のUIです。ブラシ色、背景色、ペン／消しゴムの切り替え、サイズ・不透明度、Undo/Redo、選択レイヤーのクリア、レイヤーの追加・削除・選択・並べ替えを提供します。

Brushパネルには選択中のモードのサイズ・不透明度スライダーだけを表示し、ペンと消しゴムの設定値は独立して保持します。非表示側のスライダーにはマウス入力を渡しません。レイヤー追加・削除ボタンはBrushパネルから独立し、レイヤー一覧の上に配置します。

レイヤー一覧はスクロールとドラッグ並べ替えに対応します。Ctrl+ZでUndo、Ctrl+Shift+ZでRedoを実行します。並べ替え・背景色・ブラシ設定の変更は、標準では履歴に記録しません。

引数なしの `setup()` は標準配置で初期化します。アプリから配置を指定する場合は、4つの位置を設定した `PaintEditorLayout` を `setup(layout)` に渡します。次は800×600キャンバスで、右側のレイヤー一覧を8行表示し、その下にBackgroundを配置する例です。

```cpp
paintcoreui::PaintEditorLayout layout;
layout.colorPickerPosition = { 808.0f, 20.0f };
layout.brushPanelPosition = { 808.0f, 324.0f };
layout.layerAreaPosition = { 1080.0f, 20.0f };
layout.visibleLayerRows = 8;
layout.backgroundWidth = 256.0f;
layout.backgroundButtonPosition = {
    layout.layerAreaPosition.x,
    layout.layerAreaPosition.y
        + 68.0f + 24.0f * layout.visibleLayerRows + 16.0f
};
editor.setup(layout);
```

各座標は画面上の左上位置です。`layerAreaPosition` はレイヤー欄全体の左上で、追加ボタン・削除ボタン・一覧を上から順に配置します。一覧は指定位置から下に68px、行の高さは24px、幅は256pxです。`visibleLayerRows` の既定値は5で、1未満の指定は1に補正します。表示行数はレイヤーの総数を制限しません。

`backgroundWidth` の既定値は100pxで、1px未満の指定は1pxに補正します。この値はBackgroundボタンの幅と、展開時の色選択部分の一辺の長さに使います。背景色ピッカーはボタンの上端から下に34pxの位置に開き、その下にHueスライダーを表示します。閉じている間は背景色ピッカーの入力を無効にします。

配置指定は初期化時に行います。`setup()` を再度呼んでも再配置されません。UI同士やキャンバスと重ならず、ウィンドウ内に収まる座標をアプリ側で設定してください。Brushパネルを移動・折りたたみしても、他のUIは自動追従しません。

マウスとキーのイベントは `PaintEditor` へ転送します。スクロールもアプリ側で右側領域などに制限せず `editor.mouseScrolled(scrollY)` へ転送し、一覧領域の判定は `PaintEditor` に任せます。保存ボタンや独自フィルターの設定パネルは付属しません。

共通基盤はoFに依存します。React、GlowingDrawingのGlowFilter、FastAPI、録画・QRコードには依存しません。`addon_config.mk` のofxGui依存は付属ColorPickerと標準UIのためです。

通常のoFプロジェクトでは、oFの `addons/ofxPaintCore` にこのリポジトリを置き、Project GeneratorでofxPaintCoreを追加します。`example-basic` はその配置で生成するためのソース例です。GlowingDrawingではアプリ内のサブモジュールを使うため、`addons.make` に `addons/ofxPaintCore` と書き、VSのプロジェクトにもこのローカルパスを登録しています。

WebのRGBA32F描画には対応したWebGL2コンテキストと拡張の有効化が必要です。GlowingDrawingでは既存の `main.cpp` が確認しています。新しいWebアプリでも、コンテキスト作成後・`setup()` 前に同じ対応が必要です。

アドオンの改善を他アプリへ反映する場合は、アドオン側をコミットして配布し、各アプリのサブモジュール参照を更新して再ビルドします。すでにビルド済みの別アプリが自動で更新される仕組みではありません。
