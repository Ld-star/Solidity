pragma experimental SMTChecker;
contract C {
	function abiencodePackedHash(uint a, uint b) public pure {
		require(a == b);
		bytes memory b1 = abi.encodePacked(a, a, a, a);
		bytes memory b2 = abi.encodePacked(b, a, b, a);
		assert(keccak256(b1) == keccak256(b2));

		bytes memory b3 = abi.encode(a, a, a, a);
		assert(keccak256(b1) == keccak256(b3)); // should fail
	}
}
// ----
// Warning 1218: (313-351): CHC: Error trying to invoke SMT solver.
// Warning 6328: (313-351): CHC: Assertion violation might happen here.
// Warning 4661: (313-351): BMC: Assertion violation happens here.
