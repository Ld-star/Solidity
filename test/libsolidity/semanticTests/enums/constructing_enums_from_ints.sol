contract c {
    enum Truth {False, True}

    function test() public returns (uint256) {
        return uint256(Truth(uint8(0x1)));
    }
}

// ====
// compileViaYul: also
// compileToEwasm: also
// ----
// test() -> 1
