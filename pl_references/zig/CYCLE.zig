// ProcessLang :: CYCLE
// Iterate, recurse, repeat until stable

pub fn Cycle(comptime T: type) type {
    return struct {
        pub fn times(n: usize, init: T, f: *const fn (T, usize) T) T {
            var result = init;
            for (0..n) |i| { result = f(result, i); }
            return result;
        }

        pub fn until_stable(init: T, f: *const fn (T) T, stable: *const fn (T, T) bool, max_iter: usize) struct { value: T, iters: usize } {
            var current = init;
            for (0..max_iter) |i| {
                const next = f(current);
                if (stable(next, current)) return .{ .value = next, .iters = i + 1 };
                current = next;
            }
            return .{ .value = current, .iters = max_iter };
        }

        pub fn generate(out: []T, f: *const fn (usize) T) void {
            for (0..out.len) |i| { out[i] = f(i); }
        }

        pub fn scan(in: []const T, out: []T, init: T, f: *const fn (T, T) T) void {
            var acc = init;
            for (in, 0..) |v, i| { acc = f(acc, v); out[i] = acc; }
        }
    };
}
