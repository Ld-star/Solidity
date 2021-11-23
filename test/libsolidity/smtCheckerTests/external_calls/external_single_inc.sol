abstract contract D {
	function d() external virtual;
}

contract C {
	uint x;
	uint y;
	D d;

	function inc() public {
		if (y == 1)
			x = 1;
		if (x == 0)
			y = 1;
	}

	function f() public {
		uint oldX = x;
		d.d();
		assert(oldX == x);
	}
}
// ====
// SMTEngine: all
// ----
// Warning 6328: (223-240): CHC: Assertion violation happens here.\nCounterexample:\nx = 1, y = 1, d = 0\noldX = 0\n\nTransaction trace:\nC.constructor()\nState: x = 0, y = 0, d = 0\nC.inc()\nState: x = 0, y = 1, d = 0\nC.f()\n    d.d() -- untrusted external call, synthesized as:\n        C.inc() -- reentrant call
