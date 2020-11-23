pragma experimental ABIEncoderV2;

contract C {
	function f(bool b) public pure returns (bool) { return b; }
}
// ====
// compileViaYul: also
// compileToEwasm: also
// ----
// f(bool): true -> true
// f(bool): false -> false
// f(bool): 0x000000 -> false
// f(bool): 0xffffff -> FAILURE
