interface I {
	function f() external payable;
}

contract C {
	function g(I i) public {
		require(address(this).balance == 100);
		i.f{value: 0}();
		assert(address(this).balance == 100); // should hold
		assert(address(this).balance == 90); // should fail
	}
}
// ====
// SMTEngine: all
// ----
// Warning 6328: (150-186): CHC: Assertion violation might happen here.
// Warning 6328: (205-240): CHC: Assertion violation happens here.\nCounterexample:\n\ni = 0\n\nTransaction trace:\nC.constructor()\nC.g(0)\n    i.f{value: 0}() -- untrusted external call
// Warning 4661: (150-186): BMC: Assertion violation happens here.
