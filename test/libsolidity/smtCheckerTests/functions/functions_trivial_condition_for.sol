pragma experimental SMTChecker;

contract C
{
	function f(bool x) public pure { require(x); for (;x;) {} }
}
// ----
// Warning 6838: (98-99): BMC: Condition is always true.
