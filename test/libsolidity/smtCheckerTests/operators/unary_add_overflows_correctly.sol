pragma experimental SMTChecker;

contract C {
	uint8 x = 254;

	function inc_pre() public {
		++x;
	}

	function check() view public {
		uint y = x;
		assert(y < 256);
	}
}
// ----
// Warning 4984: (94-97): CHC: Overflow (resulting value larger than 255) happens here.\nCounterexample:\nx = 255\n\n\n\nTransaction trace:\nC.constructor()\nState: x = 254\nC.inc_pre()\nState: x = 255\nC.inc_pre()
