contract C {
    function f() public returns (string memory) {
        return string.concat();
    }
}
// ====
// compileToEwasm: also
// compileViaYul: also
// ----
// f() -> 0x20, 0
