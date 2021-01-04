pragma experimental SMTChecker;

contract C  {
	function f(uint x, uint y) public pure returns (uint) {
		return x + y;
	}
}
// ----
// Warning 4984: (113-118): CHC: Overflow (resulting value larger than 2**256 - 1) happens here.\nCounterexample:\n\nx = 1\ny = 115792089237316195423570985008687907853269984665640564039457584007913129639935\n = 0\n\nTransaction trace:\nC.constructor()\nC.f(1, 115792089237316195423570985008687907853269984665640564039457584007913129639935)
