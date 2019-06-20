contract C {
    function f(int a, int b) public pure returns (int x) {
        x = a + b;
    }
    function g(int8 a, int8 b) public pure returns (int8 x) {
        x = a + b;
    }
}
// ====
// compileViaYul: true
// ----
// f(int256,int256): 5, 6 -> 11
// f(int256,int256): -2, 1 -> -1
// f(int256,int256): -2, 2 -> 0
// f(int256,int256): 2, -2 -> 0
// f(int256,int256): -5, -6 -> -11
// f(int256,int256): 0x7FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF0, 0x0F -> 0x7FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF
// f(int256,int256): 0x0F, 0x7FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF0 -> 0x7FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF
// f(int256,int256): 0x7FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF, 1 -> FAILURE
// f(int256,int256): 1, 0x7FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF -> FAILURE
// f(int256,int256): 0x8000000000000000000000000000000000000000000000000000000000000001, -1 -> 0x8000000000000000000000000000000000000000000000000000000000000000
// f(int256,int256): -1, 0x8000000000000000000000000000000000000000000000000000000000000001 -> 0x8000000000000000000000000000000000000000000000000000000000000000
// f(int256,int256): 0x8000000000000000000000000000000000000000000000000000000000000000, -1 -> FAILURE
// f(int256,int256): -1, 0x8000000000000000000000000000000000000000000000000000000000000000 -> FAILURE
// g(int8,int8): 5, 6 -> 11
// g(int8,int8): -2, 1 -> -1
// g(int8,int8): -2, 2 -> 0
// g(int8,int8): 2, -2 -> 0
// g(int8,int8): -5, -6 -> -11
// g(int8,int8): 126, 1 -> 127
// g(int8,int8): 1, 126 -> 127
// g(int8,int8): 127, 1 -> FAILURE
// g(int8,int8): 1, 127 -> FAILURE
// g(int8,int8): -127, -1 -> -128
// g(int8,int8): -1, -127 -> -128
// g(int8,int8): -127, -2 -> FAILURE
// g(int8,int8): -2, -127 -> FAILURE
