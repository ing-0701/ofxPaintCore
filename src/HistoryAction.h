#pragma once

namespace paintcore {

template <class Context>
class HistoryAction {
public:
	virtual ~HistoryAction() = default;

	virtual void undo(Context & context) = 0;
	virtual void redo(Context & context) = 0;
};
}
