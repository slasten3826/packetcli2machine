// ProcessLang :: FLOW
// Transform, map, pipe data through a process

const std = @import("std");

pub fn Flow(comptime T: type) type {
    return struct {
        const Self = @This();

        pub fn pipe(value: T, f: *const fn (T) T) T {
            return f(value);
        }

        pub fn chain(value: T, fns: []const *const fn (T) T) T {
            var result = value;
            for (fns) |f| {
                result = f(result);
            }
            return result;
        }

        pub fn map(in: []const T, out: []T, f: *const fn (T) T) void {
            for (in, 0..) |v, i| {
                out[i] = f(v);
            }
        }

        pub fn filter(in: []const T, out: []T, predicate: *const fn (T) bool) usize {
            var count: usize = 0;
            for (in) |v| {
                if (predicate(v)) {
                    out[count] = v;
                    count += 1;
                }
            }
            return count;
        }

        pub const Source = struct {
            value: ?T,

            pub fn init(value: T) Source {
                return .{ .value = value };
            }

            pub fn through(self: *Source, f: *const fn (T) T) *Source {
                if (self.value) |v| {
                    self.value = f(v);
                }
                return self;
            }

            pub fn manifest(self: *Source) ?T {
                const out = self.value;
                self.value = null;
                return out;
            }

            pub fn alive(self: Source) bool {
                return self.value != null;
            }
        };

        pub fn source(value: T) Source {
            return Source.init(value);
        }
    };
}
