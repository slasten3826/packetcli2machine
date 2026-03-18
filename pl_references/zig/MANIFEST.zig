// ProcessLang :: MANIFEST
// Crystallize output, make real, render result

pub fn Manifest(comptime T: type) type {
    return struct {
        pub const Output = struct { value: T, tick: usize, sealed: bool };

        pub fn emit(value: T, tick: usize) Output {
            return .{ .value = value, .tick = tick, .sealed = false };
        }

        pub fn seal(out: *Output) void {
            out.sealed = true;
        }

        pub fn required(value: T, sentinel: T) !T {
            if (value == sentinel) return error.ManifestNil;
            return value;
        }

        pub fn render_to(in: []const T, out: []T) usize {
            const len = @min(in.len, out.len);
            for (0..len) |i| { out[i] = in[i]; }
            return len;
        }

        pub fn render_map(in: []const T, out: []T, f: *const fn (T) T) usize {
            const len = @min(in.len, out.len);
            for (0..len) |i| { out[i] = f(in[i]); }
            return len;
        }

        pub fn ready(in: []const T, predicate: *const fn (T) bool) bool {
            for (in) |v| { if (!predicate(v)) return false; }
            return true;
        }
    };
}
