pragma experimental SMTChecker;
contract C {
	function abiEncodeSlice(bytes4 sel, bytes calldata data) external pure {
		bytes memory b1 = abi.encodeWithSelector(sel, data);
		bytes memory b2 = abi.encodeWithSelector(sel, data[0:]);
		// should hold but the engine cannot infer that data is fully equals data[0:] because each index is assigned separately
		assert(b1.length == b2.length); // fails for now

		bytes memory b3 = abi.encodeWithSelector(sel, data[:data.length]);
		// should hold but the engine cannot infer that data is fully equals data[:data.length] because each index is assigned separately
		assert(b1.length == b3.length); // fails for now

		bytes memory b4 = abi.encodeWithSelector(sel, data[5:10]);
		assert(b1.length == b4.length); // should fail

		uint x = 5;
		uint y = 10;
		bytes memory b5 = abi.encodeWithSelector(sel, data[x:y]);
		// should hold but the engine cannot infer that data[5:10] is fully equals data[x:y] because each index is assigned separately
		assert(b1.length == b5.length); // fails for now

		bytes memory b6 = abi.encodeWithSelector(0xcafecafe, data[5:10]);
		assert(b4.length == b6.length); // should fail
	}
}
// ----
// Warning 6328: (357-387): CHC: Assertion violation happens here.
// Warning 6328: (610-640): CHC: Assertion violation happens here.
// Warning 1218: (723-753): CHC: Error trying to invoke SMT solver.
// Warning 6328: (723-753): CHC: Assertion violation might happen here.
// Warning 1218: (991-1021): CHC: Error trying to invoke SMT solver.
// Warning 6328: (991-1021): CHC: Assertion violation might happen here.
// Warning 1218: (1111-1141): CHC: Error trying to invoke SMT solver.
// Warning 6328: (1111-1141): CHC: Assertion violation might happen here.
// Warning 4661: (723-753): BMC: Assertion violation happens here.
// Warning 4661: (991-1021): BMC: Assertion violation happens here.
// Warning 4661: (1111-1141): BMC: Assertion violation happens here.
