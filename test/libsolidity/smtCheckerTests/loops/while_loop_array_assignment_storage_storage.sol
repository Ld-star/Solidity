pragma experimental SMTChecker;

contract LoopFor2 {
	uint[] b;
	uint[] c;

	function testUnboundedForLoop(uint n) public {
		b[0] = 900;
		uint[] storage a = b;
		require(n > 0 && n < 100);
		uint i;
		while (i < n) {
			b[i] = i + 1;
			c[i] = b[i];
			++i;
		}
		// Fails as false positive.
		assert(b[0] == c[0]);
		assert(a[0] == 900);
		assert(b[0] == 900);
	}
}
// ----
// Warning 1218: (229-234): Error trying to invoke SMT solver.
// Warning 1218: (296-316): Error trying to invoke SMT solver.
// Warning 6328: (320-339): Assertion violation happens here.
// Warning 6328: (343-362): Assertion violation happens here.
// Warning 4661: (296-316): Assertion violation happens here.
