contract Test {
    function() internal x;

    function f() public returns (uint256 r) {
        x();
        return 2;
    }
}

// ====
// compileViaYul: also
// ----
// f() -> FAILURE
