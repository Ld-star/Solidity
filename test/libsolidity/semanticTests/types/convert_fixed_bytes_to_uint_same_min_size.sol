contract Test {
    function bytesToUint(bytes1 s) public returns (uint8 h) {
        return uint8(s);
    }
}
// ====
// compileViaYul: also
// compileToEwasm: also
// ----
// bytesToUint(bytes1): "a" -> 0x61
