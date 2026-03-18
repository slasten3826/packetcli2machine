// ProcessLang :: DISSOLVE
// Decompose, analyze, break structures apart

pub fn Dissolve(comptime T: type) type {
    return struct {
        pub const SplitResult = struct { yes_count: usize, no_count: usize };

        pub fn split(in: []const T, yes: []T, no: []T, predicate: *const fn (T) bool) SplitResult {
            var yc: usize = 0;
            var nc: usize = 0;
            for (in) |v| {
                if (predicate(v)) { yes[yc] = v; yc += 1; } else { no[nc] = v; nc += 1; }
            }
            return .{ .yes_count = yc, .no_count = nc };
        }

        pub fn reduce(in: []const T, init: T, f: *const fn (T, T) T) T {
            var acc = init;
            for (in) |v| { acc = f(acc, v); }
            return acc;
        }

        pub fn count(in: []const T, predicate: *const fn (T) bool) usize {
            var c: usize = 0;
            for (in) |v| { if (predicate(v)) c += 1; }
            return c;
        }

        pub fn take(in: []const T, out: []T, n: usize) usize {
            const len = @min(n, in.len);
            for (0..len) |i| { out[i] = in[i]; }
            return len;
        }

        pub fn drop(in: []const T, out: []T, n: usize) usize {
            if (n >= in.len) return 0;
            const len = in.len - n;
            for (0..len) |i| { out[i] = in[n + i]; }
            return len;
        }
    };
}
