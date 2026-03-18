// ProcessLang :: LOGIC
// Reason, validate, apply rules

pub fn Logic(comptime T: type) type {
    return struct {
        pub fn all(value: T, predicates: []const *const fn (T) bool) bool {
            for (predicates) |p| { if (!p(value)) return false; }
            return true;
        }

        pub fn any(value: T, predicates: []const *const fn (T) bool) bool {
            for (predicates) |p| { if (p(value)) return true; }
            return false;
        }

        pub fn none(value: T, predicates: []const *const fn (T) bool) bool {
            for (predicates) |p| { if (p(value)) return false; }
            return true;
        }

        pub fn implies(a: bool, b: bool) bool {
            return !a or b;
        }

        pub fn validate(in: []const T, rule: *const fn (T) bool) struct { passed: usize, failed: usize } {
            var passed: usize = 0;
            var failed: usize = 0;
            for (in) |v| { if (rule(v)) passed += 1 else failed += 1; }
            return .{ .passed = passed, .failed = failed };
        }

        pub fn enforce(in: []const T, out: []T, rules: []const *const fn (T) bool) usize {
            var c: usize = 0;
            outer: for (in) |v| {
                for (rules) |rule| { if (!rule(v)) continue :outer; }
                out[c] = v;
                c += 1;
            }
            return c;
        }

        pub fn classify(in: []const T, out_indices: []i32, rules: []const *const fn (T) bool) void {
            for (in, 0..) |v, i| {
                out_indices[i] = -1;
                for (rules, 0..) |rule, ri| {
                    if (rule(v)) { out_indices[i] = @intCast(ri); break; }
                }
            }
        }
    };
}
