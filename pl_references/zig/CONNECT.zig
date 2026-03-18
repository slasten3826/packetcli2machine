// ProcessLang :: CONNECT
// Link, compose, find analogies between structures

pub fn Connect(comptime T: type) type {
    return struct {
        pub const Pair = struct { a: T, b: T };

        pub fn compose(x: T, fns: []const *const fn (T) T) T {
            var result = x;
            for (fns) |f| {
                result = f(result);
            }
            return result;
        }

        pub fn zip(a: []const T, b: []const T, out: []Pair) usize {
            const len = @min(a.len, b.len);
            for (0..len) |i| {
                out[i] = .{ .a = a[i], .b = b[i] };
            }
            return len;
        }

        pub fn bridge(a: []const T, b: []const T, out: []Pair, match: *const fn (T, T) bool) usize {
            var count: usize = 0;
            for (a) |va| {
                for (b) |vb| {
                    if (match(va, vb)) {
                        out[count] = .{ .a = va, .b = vb };
                        count += 1;
                        break;
                    }
                }
            }
            return count;
        }

        pub fn analogy(src: []const T, tgt: []const T, out: []T, mapping: *const fn (T, T) T) void {
            const len = @min(src.len, tgt.len);
            for (0..len) |i| {
                out[i] = mapping(src[i], tgt[i]);
            }
        }
    };
}
