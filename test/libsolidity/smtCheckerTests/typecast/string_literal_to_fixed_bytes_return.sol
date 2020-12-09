pragma experimental SMTChecker;
contract C {
    function g() public pure returns (bytes32 val) { return "abc"; }
    function f1() public pure returns (bytes32 val) { return g(); }

	function a() public pure {
		assert(f1() == "abc");
		assert(f1() == "cde");
	}
}
// ----
// Warning 6328: (238-259): CHC: Assertion violation happens here.\nCounterexample:\n\n\n\n\nTransaction trace:\nconstructor()\na()
