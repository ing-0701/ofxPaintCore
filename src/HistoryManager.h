#pragma once

#include <vector>
#include <stdexcept>
#include <utility>

namespace paintcore {
// Capacity is a default, not a requirement for the caller to use a fixed history size.
template<class Entry, int Capacity = 20>
class HistoryManager {
public:
    explicit HistoryManager(int capacity = Capacity) : entries(checkedCapacity(capacity)) {}
    template<class Initialize> void initializeEntries(Initialize initialize) {
        for (auto& entry : entries) initialize(entry);
        initialize(pending);
    }
    Entry& prepare() {
        pending.reset();
        preparing = true;
        return pending;
    }
    void cancel() { pending.reset(); preparing = false; }
    void finish() {
        if (!preparing) throw std::logic_error("No pending history entry.");
        // Only a successful operation invalidates Redo.
        for (int i = 0; i < redoCount; ++i)
            entries[(writeIndex + i) % capacity()].reset();
        entries[writeIndex].reset();
        using std::swap;
        swap(entries[writeIndex], pending);
        pending.reset();
        preparing = false;
        redoCount = 0;
        writeIndex = (writeIndex + 1) % capacity();
        if (undoCount < capacity()) ++undoCount;
    }
    template<class Context> void undo(Context& context) {
        if (!canUndo() || preparing) return;
        const int index = (writeIndex + capacity() - 1) % capacity();
        entries[index].undo(context);
        writeIndex = index;
        --undoCount;
        ++redoCount;
    }
    template<class Context> void redo(Context& context) {
        if (!canRedo() || preparing) return;
        entries[writeIndex].redo(context);
        writeIndex = (writeIndex + 1) % capacity();
        ++undoCount;
        --redoCount;
    }
    void clear() {
        for (auto& entry : entries) entry.reset();
        cancel();
        writeIndex = undoCount = redoCount = 0;
    }
    bool canUndo() const { return undoCount > 0; }
    bool canRedo() const { return redoCount > 0; }
    int capacity() const { return static_cast<int>(entries.size()); }
private:
    static int checkedCapacity(int value) {
        if (value <= 0) throw std::invalid_argument("History capacity must be positive.");
        return value;
    }
    std::vector<Entry> entries;
    Entry pending;
    bool preparing = false;
    int writeIndex = 0, undoCount = 0, redoCount = 0;
};
}
