contract test {
    function f0() public returns(int, bool) {
        int a;
        bool b;
        ((a, b)) = (2, true);
        return (a, b);
    }
    function f1() public returns(int) {
        int a;
        (((a, ), )) = ((1, 2) ,3);
        return a;
    }
    function f2() public returns(int) {
        int a;
        (((, a),)) = ((1, 2), 3);
        return a;
    }
}
// ====
// compileViaYul: also
// ----
// f0() -> 2, true
// f1() -> 1
// f2() -> 2
