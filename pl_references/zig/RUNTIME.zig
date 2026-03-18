// ProcessLang :: RUNTIME
// Maintain state, execute, manage context

pub fn Runtime(comptime T: type, comptime max_slots: usize, comptime max_history: usize) type {
    return struct {
        const Self = @This();

        pub const HistoryEntry = struct { slot: usize, old: T, new: T, tick: usize };

        slots: [max_slots]T,
        active: [max_slots]bool,
        history: [max_history]HistoryEntry,
        history_len: usize,
        tick: usize,

        pub fn init(default: T) Self {
            var self: Self = undefined;
            for (0..max_slots) |i| { self.slots[i] = default; self.active[i] = false; }
            self.history_len = 0;
            self.tick = 0;
            return self;
        }

        pub fn set(self: *Self, slot: usize, value: T) void {
            if (slot >= max_slots) return;
            if (self.history_len < max_history) {
                self.history[self.history_len] = .{ .slot = slot, .old = self.slots[slot], .new = value, .tick = self.tick };
                self.history_len += 1;
            }
            self.slots[slot] = value;
            self.active[slot] = true;
            self.tick += 1;
        }

        pub fn get(self: *const Self, slot: usize) ?T {
            if (slot >= max_slots or !self.active[slot]) return null;
            return self.slots[slot];
        }

        pub fn snapshot(self: *const Self, out: []T) usize {
            var c: usize = 0;
            for (0..max_slots) |i| {
                if (self.active[i] and c < out.len) { out[c] = self.slots[i]; c += 1; }
            }
            return c;
        }

        pub fn rollback(self: *Self, n: usize) void {
            var remaining = n;
            while (remaining > 0 and self.history_len > 0) {
                self.history_len -= 1;
                const entry = self.history[self.history_len];
                self.slots[entry.slot] = entry.old;
                remaining -= 1;
            }
        }

        pub fn active_count(self: *const Self) usize {
            var c: usize = 0;
            for (self.active) |a| { if (a) c += 1; }
            return c;
        }
    };
}
