contract C {
    function f() public returns (address payable) {
        return block.coinbase;
    }
}
// ====
// compileViaYul: also
// compileToEwasm: also
// ----
// f() -> 0x7878787878787878787878787878787878787878
// f() -> 0x7878787878787878787878787878787878787878
// f() -> 0x7878787878787878787878787878787878787878
