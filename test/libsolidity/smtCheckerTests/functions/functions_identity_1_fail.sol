pragma experimental SMTChecker;
contract C
{
	function h(uint x) public pure returns (uint) {
		return x;
	}
	function g() public pure {
		uint x;
		x = h(0);
		assert(x > 0);
	}
}

// ----
// Warning 4661: (161-174): Assertion violation happens here
