// ProcessLang :: OBSERVE
// Measure, verify, inspect without changing value

pub fn Observe(comptime T: type) type {
    return struct {
        pub const Metrics = struct { count: usize, sum: i64, min: T, max: T };

        pub fn watch(value: T, observer: *const fn (T) void) T {
            observer(value);
            return value;
        }

        pub fn assert_(value: T, predicate: *const fn (T) bool) !T {
            if (!predicate(value)) return error.AssertionFailed;
            return value;
        }

        pub fn check(value: T, predicate: *const fn (T) bool) struct { result: bool, value: T } {
            return .{ .result = predicate(value), .value = value };
        }

        pub fn measure(in: []const T) Metrics {
            if (in.len == 0) return .{ .count = 0, .sum = 0, .min = 0, .max = 0 };
            var m = Metrics{ .count = in.len, .sum = 0, .min = in[0], .max = in[0] };
            for (in) |v| {
                m.sum += v;
                if (v < m.min) m.min = v;
                if (v > m.max) m.max = v;
            }
            return m;
        }

        pub fn snapshot(in: []const T, out: []T) void {
            for (in, 0..) |v, i| { out[i] = v; }
        }

        pub fn diff_count(a: []const T, b: []const T) usize {
            const len = @min(a.len, b.len);
            var diffs: usize = 0;
            for (0..len) |i| { if (a[i] != b[i]) diffs += 1; }
            if (a.len > len) diffs += a.len - len;
            if (b.len > len) diffs += b.len - len;
            return diffs;
        }
    };
}
