pragma experimental SMTChecker;
contract C {
	int[][] array2d;
	function l() public {
		array2d.push().push();
		assert(array2d.length > 2);
		assert(array2d[array2d.length - 1].length > 3);
	}
}
// ----
// Warning 4661: (113-139): Assertion violation happens here
// Warning 4661: (143-189): Assertion violation happens here
