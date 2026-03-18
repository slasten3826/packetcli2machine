// ProcessLang :: CHOOSE
// Branch, decide, collapse possibilities into one path

pub fn Choose(comptime T: type) type {
    return struct {
        pub const Branch = struct {
            predicate: ?*const fn (T) bool,
            transform: *const fn (T) T,
        };

        pub fn branch(value: T, conditions: []const Branch) ?T {
            for (conditions) |c| {
                if (c.predicate) |pred| {
                    if (pred(value)) return c.transform(value);
                } else {
                    return c.transform(value);
                }
            }
            return null;
        }

        pub fn best(in: []const T, score_fn: *const fn (T) i32) ?T {
            if (in.len == 0) return null;
            var best_val = in[0];
            var best_score = score_fn(in[0]);
            for (in[1..]) |v| {
                const s = score_fn(v);
                if (s > best_score) { best_score = s; best_val = v; }
            }
            return best_val;
        }

        pub fn gate(value: T, predicate: *const fn (T) bool, fallback: T) T {
            return if (predicate(value)) value else fallback;
        }

        pub fn either(condition: bool, if_true: T, if_false: T) T {
            return if (condition) if_true else if_false;
        }

        pub fn first(in: []const T, sentinel: T) ?T {
            for (in) |v| { if (v != sentinel) return v; }
            return null;
        }
    };
}
