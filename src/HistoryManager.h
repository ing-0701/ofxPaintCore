#pragma once

#include <array>

namespace paintcore {

template <class Entry, int Capacity>
class HistoryManager {
	static_assert(Capacity > 0, "History capacity must be positive.");

public:
	// 起動時に、各履歴枠のFBOなどを準備するために使う。
	// initializeはラムダ式の関数
	// 受け取った関数をforループで回しながら実行することで初期化を行う関数
	// entriesは個々のペイントソフトの内容に応じて作られるundo履歴の配列であり、このファイルの一番下で定義されてる
	template <class Initialize>
	void initializeEntries(Initialize initialize) {
		for (Entry & entry : entries) {
			initialize(entry);
		}
	}

	// 操作による履歴制作の前準備
	// REDOができなくなるのでその分のレイヤーを破棄
	Entry & prepare() {
		// Undoした後に新しい操作を行うと、それまでのRedoは使えなくなる。
		for (int i = 0; i < redoCount; ++i) {
			const int index = (writeIndex + i) % Capacity;
			entries[index].reset();
		}

		// Redoがあれば、書き込み先は上のループの先頭ですでに初期化済み。
		if (redoCount == 0) {
			entries[writeIndex].reset();
		}

		redoCount = 0;
		return entries[writeIndex];
	}

	// 呼び出し元が履歴を書き終えたら、その1件を確定する。
	void finish() {
		writeIndex = (writeIndex + 1) % Capacity;

		if (undoCount < Capacity) {
			++undoCount;
		}
	}

	template <class Context>
	void undo(Context & context) {
		if (undoCount == 0) {
			return;
		}

		const int index = (writeIndex + Capacity - 1) % Capacity;
		entries[index].undo(context);

		writeIndex = index;
		--undoCount;
		++redoCount;
	}

	template <class Context>
	void redo(Context & context) {
		if (redoCount == 0) {
			return;
		}

		entries[writeIndex].redo(context);

		writeIndex = (writeIndex + 1) % Capacity;
		++undoCount;
		--redoCount;
	}

	void clear() {
		for (Entry & entry : entries) {
			entry.reset();
		}

		writeIndex = 0;
		undoCount = 0;
		redoCount = 0;
	}

private:
	std::array<Entry, Capacity> entries;
	int writeIndex = 0;
	int undoCount = 0;
	int redoCount = 0;
};

}
