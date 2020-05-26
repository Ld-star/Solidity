contract C {
    function f(bytes calldata b, uint i) internal pure returns (byte) {
        return b[i];
    }
    function f(uint, bytes calldata b, uint) external pure returns (byte) {
        return f(b, 2);
    }
}
// ====
// compileViaYul: also
// ----
// f(uint256,bytes,uint256): 7, 0x60, 7, 4, "abcd" -> "c"
