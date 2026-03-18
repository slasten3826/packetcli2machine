// ProcessLang :: ENCODE
// Compress, synthesize, accumulate into structure

pub fn Encode(comptime T: type) type {
    return struct {
        pub const Group = struct { start: usize, len: usize, key: T };

        pub fn group(in: []const T, groups: []Group, key_fn: *const fn (T) T, eql: *const fn (T, T) bool) usize {
            if (in.len == 0) return 0;
            var gc: usize = 0;
            var start: usize = 0;
            var current_key = key_fn(in[0]);
            for (1..in.len) |i| {
                const k = key_fn(in[i]);
                if (!eql(k, current_key)) {
                    groups[gc] = .{ .start = start, .len = i - start, .key = current_key };
                    gc += 1;
                    start = i;
                    current_key = k;
                }
            }
            groups[gc] = .{ .start = start, .len = in.len - start, .key = current_key };
            return gc + 1;
        }

        pub fn compress(in: []const T, out: []T, f: *const fn (T) T) void {
            for (in, 0..) |v, i| { out[i] = f(v); }
        }

        pub fn freeze(in: []const T, out: []T) void {
            for (in, 0..) |v, i| { out[i] = v; }
        }

        pub fn hash(in: []const T, seed: T, mix: *const fn (T, T) T) T {
            var h = seed;
            for (in) |v| { h = mix(h, v); }
            return h;
        }
    };
}
