contract C {
    function f(int256 a, int256 b) public returns (int256) {
        return a >> b;
    }
    function g(int256 a, int256 b) public returns (int256) {
        return a >> (256 - b);
    }
}
// ----
// TypeError 2271: (89-95): Built-in binary operator >> cannot be applied to types int256 and int256.
// TypeError 2271: (179-193): Built-in binary operator >> cannot be applied to types int256 and int256.
