contract base {
    enum Choice {A, B, C}
}


contract test is base {
    function answer() public returns (base.Choice _ret) {
        _ret = base.Choice.B;
    }
}
// ====
// compileViaYul: also
// compileToEwasm: also
// ----
// answer() -> 1
