contract A {
    function f() public {
        super;
    }
}

// ====
// compileViaYul: also
// compileToEwasm: also
// ----
// f() ->
