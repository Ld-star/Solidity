pragma experimental SMTChecker;

contract C
{
	function f(uint x) public pure returns (uint) {
		assembly {
			x := 2
		}
		return x;
	}
}
// ----
// Warning 7737: (97-121): Assertion checker does not support inline assembly.
// Warning 7737: (97-121): Assertion checker does not support inline assembly.
