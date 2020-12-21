pragma experimental SMTChecker;
contract A {
	int x = 0;

	modifier m virtual {
		assert(x == 0); // should hold
		assert(x == 42); // should fail
		_;
	}
}
contract C is A {

	modifier m override {
		assert(x == 1); // This assert is not reachable, should NOT be reported
		_;
	}

	function f() public A.m returns (uint) {
	}
}
// ----
// Warning 6328: (115-130): CHC: Assertion violation happens here.\nCounterexample:\nx = 0\n\n = 0\n\nTransaction trace:\nconstructor()\nState: x = 0\nf()
